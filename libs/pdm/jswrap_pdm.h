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
 * Contains JavaScript interface for Neopixel/WS281x/APA10x devices
 * ----------------------------------------------------------------------------
 */

#include "jspin.h"
#include "jsvar.h"

void jswrap_pdm_setup(Pin pin_clock, Pin pin_din);

void jswrap_pdm_init(JsVar *callback, JsVar* buffer_a, JsVar* buffer_b);

void jswrap_pdm_start( );

void jswrap_pdm_stop( );

void jswrap_pdm_uninit( );

bool jswrap_pdm_fetch_data( );

