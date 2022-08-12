/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * Contains JavaScript interface for trigger wheel functionality
 * ----------------------------------------------------------------------------
 */

/* DO_NOT_INCLUDE_IN_DOCS - this is a special token for common.py */

#include "jswrap_pdm.h"
#include "jsvar.h"
#include "jsinteractive.h"

uint8_t           jswrap_pdm_pin_clk;  // user defined clock pin
uint8_t           jswrap_pdm_pin_din;  // user defined data in pin

#include <hal/nrf_pdm.h>

#define DEFAULT_PDM_GAIN     20
#define PDM_IRQ_PRIORITY     7

#define NRF_PDM_FREQ_1280K  (nrf_pdm_freq_t)(0x0A000000UL)               ///< PDM_CLK= 1.280 MHz (32 MHz / 25) => Fs= 20000 Hz
#define NRF_PDM_FREQ_2000K  (nrf_pdm_freq_t)(0x10000000UL)               ///< PDM_CLK= 2.000 MHz (32 MHz / 16) => Fs= 31250 Hz
#define NRF_PDM_FREQ_2667K  (nrf_pdm_freq_t)(0x15000000UL)               ///< PDM_CLK= 2.667 MHz (32 MHz / 12) => Fs= 41667 Hz
#define NRF_PDM_FREQ_3200K  (nrf_pdm_freq_t)(0x19000000UL)               ///< PDM_CLK= 3.200 MHz (32 MHz / 10) => Fs= 50000 Hz
#define NRF_PDM_FREQ_4000K  (nrf_pdm_freq_t)(0x20000000UL)               ///< PDM_CLK= 4.000 MHz (32 MHz /  8) => Fs= 62500 Hz



  int _dinPin;
  int _clkPin;
  int _pwrPin;

  int _channels;
  int _samplerate;

  int _gain = -1;
  int _init = -1;

  int _cutSamples = 0;

int jswrap_pdm_begin(int channels, int sampleRate)
{
  _channels = channels;
  _samplerate = sampleRate;

  // Enable high frequency oscillator if not already enabled
  if (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) { }
  }

  // configure the sample rate and channels
  switch (sampleRate) {
    case 16000:
      NRF_PDM->RATIO = ((PDM_RATIO_RATIO_Ratio80 << PDM_RATIO_RATIO_Pos) & PDM_RATIO_RATIO_Msk);
      nrf_pdm_clock_set(NRF_PDM_FREQ_1280K);
      break;
    case 41667:
      nrf_pdm_clock_set(NRF_PDM_FREQ_2667K);
      break;
    default:
      return 0; // unsupported
  }

  switch (channels) {
    case 2:
      nrf_pdm_mode_set(NRF_PDM_MODE_STEREO, NRF_PDM_EDGE_LEFTFALLING);
      break;

    case 1:
      nrf_pdm_mode_set(NRF_PDM_MODE_MONO, NRF_PDM_EDGE_LEFTFALLING);
      break;

    default:
      return 0; // unsupported
  }

  if(_gain == -1) {
    _gain = DEFAULT_PDM_GAIN;
  }
  nrf_pdm_gain_set(_gain, _gain);

  // configure the I/O and mux
  pinMode(_clkPin, OUTPUT);
  digitalWrite(_clkPin, LOW);

  pinMode(_dinPin, INPUT);

  nrf_pdm_psel_connect(digitalPinToPinName(_clkPin), digitalPinToPinName(_dinPin));

  // clear events and enable PDM interrupts
  nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
  nrf_pdm_event_clear(NRF_PDM_EVENT_END);
  nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
  nrf_pdm_int_enable(NRF_PDM_INT_STARTED | NRF_PDM_INT_STOPPED);

  if (_pwrPin > -1) {
    // power the mic on
    pinMode(_pwrPin, OUTPUT);
    digitalWrite(_pwrPin, HIGH);
  }

  // clear the buffer
  _doubleBuffer.reset();

  // set the PDM IRQ priority and enable
  NVIC_SetPriority(PDM_IRQn, PDM_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(PDM_IRQn);
  NVIC_EnableIRQ(PDM_IRQn);

  // set the buffer for transfer
  // nrf_pdm_buffer_set((uint32_t*)_doubleBuffer.data(), _doubleBuffer.availableForWrite() / (sizeof(int16_t) * _channels));
  // _doubleBuffer.swap();
  
  // enable and trigger start task
  nrf_pdm_enable();
  nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
  nrf_pdm_task_trigger(NRF_PDM_TASK_START);

  return 1;
}

void jswrap_pdm_end()
{
  // disable PDM and IRQ
  nrf_pdm_disable();

  NVIC_DisableIRQ(PDM_IRQn);

  if (_pwrPin > -1) {
    // power the mic off
    digitalWrite(_pwrPin, LOW);
    pinMode(_pwrPin, INPUT);
  }

  // Don't disable high frequency oscillator since it could be in use by RADIO

  // unconfigure the I/O and un-mux
  nrf_pdm_psel_disconnect();

  pinMode(_clkPin, INPUT);
}

int jswrap_pdm_available()
{
  NVIC_DisableIRQ(PDM_IRQn);

  size_t avail = _doubleBuffer.available();

  NVIC_EnableIRQ(PDM_IRQn);

  return avail;
}

int jswrap_pdm_read(void* buffer, size_t size)
{
  NVIC_DisableIRQ(PDM_IRQn);

  int read = _doubleBuffer.read(buffer, size);

  NVIC_EnableIRQ(PDM_IRQn);

  return read;
}

void jswrap_pdm_onReceive(void(*function)(void))
{
  _onReceive = function;
}

void jswrap_pdm_setGain(int gain)
{
  _gain = gain;
  nrf_pdm_gain_set(_gain, _gain);
}

void jswrap_pdm_setBufferSize(int bufferSize)
{
  _doubleBuffer.setSize(bufferSize);
}

void jswrap_pdm_IrqHandler(bool halftranfer)
{
  if (nrf_pdm_event_check(NRF_PDM_EVENT_STARTED)) {
    nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);

    if (_doubleBuffer.available() == 0) {
      // switch to the next buffer
      nrf_pdm_buffer_set((uint32_t*)_doubleBuffer.data(), _doubleBuffer.availableForWrite() / (sizeof(int16_t) * _channels));

      // make the current one available for reading
      _doubleBuffer.swap(_doubleBuffer.availableForWrite());

      // call receive callback if provided
      if (_onReceive) {
        _onReceive();
      }
    } else {
      // buffer overflow, stop
      nrf_pdm_disable();
    }
  } else if (nrf_pdm_event_check(NRF_PDM_EVENT_STOPPED)) {
    nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
  } else if (nrf_pdm_event_check(NRF_PDM_EVENT_END)) {
    nrf_pdm_event_clear(NRF_PDM_EVENT_END);
  }
}


/*JSON{
"type" : "class",
"class" : "Pdm"
}*/

/*JSON{
"type" : "staticmethod",
"class" : "Pdm",
"name" : "setup",
"generate" : "jswrap_pdm_setup",
"params" : [
  ["pin_clock","pin","Clock pin of PDM microphone"],
  ["pin_din","pin","Data pin of PDM microphone"]
]
}*/
void jswrap_pdm_setup(Pin pin_clock, Pin pin_din) {
  if (!jshIsPinValid(pin_din)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jshIsPinValid(pin_clock)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }

	// Set PDM user defined values
	jswrap_pdm_pin_clk = pin_clock;
  jswrap_pdm_pin_din = pin_din;
}

/*JSON{
"type" : "staticmethod",
"class" : "Pdm",
"name" : "fetch_data",
"generate" : "jswrap_pdm_fetch_data",
"return" : ["bool", "Got fresh PDM data"]
}*/
bool jswrap_pdm_fetch_data( ) {
  
}


/*JSON{
"type" : "staticmethod",
"class" : "Pdm",
"name" : "init",
"generate" : "jswrap_pdm_init"
}*/
void jswrap_pdm_init( ) {

}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "start",
  "generate" : "jswrap_pdm_start"
} */
void jswrap_pdm_start( ) {
  
}


/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "stop",
  "generate" : "jswrap_pdm_stop"
} */
void jswrap_pdm_stop( ) {

}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "uninit",
  "generate" : "jswrap_pdm_uninit"
} */
void jswrap_pdm_uninit( ) {

}
