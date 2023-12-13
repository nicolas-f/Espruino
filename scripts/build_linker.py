#!/usr/bin/env python

# This file is part of Espruino, a JavaScript interpreter for Microcontrollers
#
# Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# ----------------------------------------------------------------------------------------
# Reads board information from boards/BOARDNAME.py and uses it to generate 
# a linker file
# ----------------------------------------------------------------------------------------
import subprocess;
import re;
import json;
import sys;
import os;
import importlib;
import common;

scriptdir = os.path.dirname(os.path.realpath(__file__))
basedir = scriptdir+"/../"
sys.path.append(basedir+"scripts");
sys.path.append(basedir+"boards");

import pinutils;

# -----------------------------------------------------------------------------------------
def die(err):
  sys.stderr.write("ERROR: "+err+"\n")
  sys.exit(1)
# -----------------------------------------------------------------------------------------

# Now scan AF file
print("Script location "+scriptdir)

if len(sys.argv)<3:
  print("ERROR, USAGE: build_linker.py BOARD_NAME LINKER_FILE [--bootloader_leave_space] [--bootloader]")
  print("                              --using_bootloader       -> step forwards in flash to leave room for bootloader")
  print("                              --bootloader             -> is a bootloader - place it in the correct position")
  exit(1)
boardname = sys.argv[1]
linkerFilename = sys.argv[2]

IS_BOOTLOADER = False
IS_USING_BOOTLOADER = False
for i in range(3,len(sys.argv)):
  arg = sys.argv[i];
  if arg=="--using_bootloader": IS_USING_BOOTLOADER=True
  elif arg=="--bootloader": IS_BOOTLOADER=True
  else: die("Unknown option '"+arg+"'");


print("LINKER_FILENAME "+linkerFilename)
print("BOARD "+boardname)
print("IS_BOOTLOADER "+str(IS_BOOTLOADER))
print("IS_USING_BOOTLOADER "+str(IS_USING_BOOTLOADER))
# import the board def
board = importlib.import_module(boardname)

# Check what board py says
BOARD_BOOTLOADER = "bootloader" in board.info and board.info["bootloader"]!=0
if BOARD_BOOTLOADER != (IS_BOOTLOADER or IS_USING_BOOTLOADER):
  die("Makefile ("+str(IS_BOOTLOADER or IS_USING_BOOTLOADER)+") and BOARD.py ("+str(BOARD_BOOTLOADER)+") do not agree over bootloaderiness")

# -----------------------------------------------------------------------------------------
linkerFile = open(linkerFilename, 'w')
def codeOut(s): linkerFile.write(s+"\n");
# -----------------------------------------------------------------------------------------
BOOTLOADER_SIZE = common.get_bootloader_size(board);
RAM_BASE = 0x20000000;
FLASH_BASE = 0x08000000;
RAM_SIZE = board.chip["ram"]*1024;
FLASH_SIZE = board.chip["flash"]*1024;

# Beware - on some devices (the STM32F4) the memory is divided into two non-continuous blocks
if board.chip["family"]=="STM32F4" and RAM_SIZE > 128*1024:
  RAM_SIZE = 128*1024

# on L476, the RAM is divided in 2 parts :
#   96k at 0x20000000
#   32k at 0x10000000
# Today, the 32k part won't be used. In the future, it can be
# used for example for the stack. In this case, need to : 
# _estack = 0x10008000; 
# and add the RAM in MEMORY table.
if board.chip["part"].startswith("STM32L476") and RAM_SIZE > 96*1024:
  RAM_SIZE = 96*1024

# IS_BOOTLOADER will only get used on official Espruino
# boards. The ST discovery bootloader rejects firmwares that
# aren't based at 0x08000000, however it seems to be wrong. If you use
# DFU then the built-in DFU bootloader will fail to load
# the firmware on reboot if FLASH_BASE is 0x08000000, but
# will work if it's 0x00000000
if IS_BOOTLOADER:
  FLASH_SIZE = BOOTLOADER_SIZE
  FLASH_BASE = 0x00000000

elif IS_USING_BOOTLOADER:
  FLASH_BASE = common.get_espruino_binary_address(board)
  FLASH_SIZE -= BOOTLOADER_SIZE

STACK_START = RAM_BASE + RAM_SIZE

codeOut("""
/* Automatically generated linker file for """+boardname+"""
   Generated by scripts/build_linker.py

ENTRY(Reset_Handler)

/* Highest stack address */
_estack = """+hex(STACK_START)+""";

MEMORY
{
  FLASH (rx)      : ORIGIN = """+hex(FLASH_BASE)+""", LENGTH = """+str(int(FLASH_SIZE/1024))+"""K
  RAM (xrw)       : ORIGIN = """+hex(RAM_BASE)+""", LENGTH = """+str(int(RAM_SIZE/1024))+"""K
}

SECTIONS
{
  /* FLASH --------------------------------------------- */
  /* Interrupt Vector table goes first */
  .isr_vector :
  {
    . = ALIGN(0x200); /* STM32 requires this alignment */
    _VECTOR_TABLE = .; /* We'll need this for relocating our table */
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
  } >FLASH

  /* Then code, then constants */
  .text :
  {
""")

if not IS_USING_BOOTLOADER and not IS_BOOTLOADER and "place_text_section" in board.chip:
  codeOut("""    /* In the .py file we were told to place text here (to skip out what was before) */
    . = ALIGN("""+hex(board.chip["place_text_section"] & 0x00FFFFFF)+"""); /* hacky! really want it absolute */
""");

codeOut("""
    . = ALIGN(4);
    *(.text)  
    *(.text*) 
    *(.rodata)  
    *(.rodata*) 

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
  } >RAM

  PROVIDE ( _end = _ebss );

  /* Remove stuff we don't want */
  /DISCARD/ :
  {
    *(.init) /* we don't call init and fini anyway. GCC 5.x starting added them but then optimising them out */
    *(.fini)
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}
""");

