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
#include "nrfx_pdm.h"

void jswrap_pdm_handler( nrfx_pdm_evt_t const * const pEvent) {

}

/*JSON{
"type" : "class",
"class" : "Pdm"
}*/

/*JSON{
"type" : "staticmethod",
"class" : "Pdm",
"name" : "setup",
"generate" : "jswrap_pdm_setup"
}*/
void jswrap_pdm_setup(Pin pin_clock, Pin pin_din, JsVar *func) {
  if (!jshIsPinValid(PIN_DIN)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jshIsPinValid(PIN_CLK)) {
    jsError("Invalid pin supplied as an argument to Trig.setup");
    return;
  }

	// Start with default vales
	nrfx_pdm_config_t config = NRFX_PDM_DEFAULT_CONFIG(PIN_CLK, PIN_DIN);

	nrfx_err_t err = nrfx_pdm_init(&config, jswrap_pdm_handler);

  jsiConsolePrint("Driver init ok!\r\n");
}

/*JSON{
  "type" : "function",
  "class" : "Pdm",
  "name" : "start",
  "generate" : "jswrap_pdm_start",
  "params" : [
    ["function","JsVar","A Function or String to be executed"]
  ]
} */
void jswrap_pdm_start( ) {


}


/*JSON{
  "type" : "function",
  "class" : "Pdm",
  "name" : "stop",
  "generate" : "jswrap_pdm_stop"
} */
void jswrap_pdm_stop( ) {

}

