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

bool jswrap_pdm_useBufferA = true;

int16_t* jswrap_pdm_bufferA = NULL;
int16_t* jswrap_pdm_bufferB = NULL;
uint16_t jswrap_pdm_buffer_length = 0;

void jswrap_pdm_handler( nrfx_pdm_evt_t const * const pEvent) {

	if (pEvent->buffer_requested) {
		if (jswrap_pdm_useBufferA) {
			nrfx_pdm_buffer_set(jswrap_pdm_bufferA, jswrap_pdm_buffer_length);
		} else {
			nrfx_pdm_buffer_set(jswrap_pdm_bufferB, jswrap_pdm_buffer_length);
		}
		jswrap_pdm_useBufferA = !jswrap_pdm_useBufferA;
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
    ["pin","pin","Clock pin of PDM microphone"],
    ["pin","pin","Data pin of PDM microphone"],
    ["function","JsVar","The function callback when samples are available"]
  ]
}*/
void jswrap_pdm_setup(Pin pin_clock, Pin pin_din, JsVar *func) {
  if (!jshIsPinValid(pin_din)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jshIsPinValid(pin_clock)) {
    jsError("Invalid pin supplied as an argument to Trig.setup");
    return;
  }
  jshPinSetState(pin_din, JSHPINSTATE_GPIO_IN);
  jshPinSetState(pin_clock, JSHPINSTATE_GPIO_OUT);

	// Load PDM default values
	nrfx_pdm_config_t config = NRFX_PDM_DEFAULT_CONFIG(pin_clock, pin_din);

	nrfx_err_t err = nrfx_pdm_init(&config, jswrap_pdm_handler);
  jswrap_pdm_log_error(err); // log error if there is one

  jsiConsolePrint("Driver init ok!\r\n");
}

void jswrap_pdm_log_error( nrfx_err_t err ) {

  switch (err)
  {
  case NRFX_ERROR_INTERNAL:
    jsiConsolePrint("PDM Internal error.\r\n");
    break;
  case NRFX_ERROR_NO_MEM:
    jsiConsolePrint("No memory for operation.\r\n");
    break;
  case NRFX_ERROR_NOT_SUPPORTED:
    jsiConsolePrint("PDM Not supported.\r\n");
    break;
  case NRFX_ERROR_INVALID_PARAM:
    jsiConsolePrint("PDM Invalid parameter.\r\n");
    break;
  case NRFX_ERROR_INVALID_STATE:
    jsiConsolePrint("PDM Invalid state, operation disallowed in this state.\r\n");
    break;
  case NRFX_ERROR_INVALID_LENGTH:
    jsiConsolePrint("PDM Invalid length.\r\n");
    break;
  case NRFX_ERROR_FORBIDDEN:
    jsiConsolePrint("PDM Operation is forbidden.\r\n");
    break;
  case NRFX_ERROR_NULL:
    jsiConsolePrint("PDM Null pointer.\r\n");
    break;
  case NRFX_ERROR_INVALID_ADDR:
    jsiConsolePrint("PDM Bad memory address.\r\n");
    break;
  case NRFX_ERROR_BUSY:
    jsiConsolePrint("PDM Busy.\r\n");
    break;
  case NRFX_ERROR_ALREADY_INITIALIZED:
    jsiConsolePrint("PDM Module already initialized.\r\n");
    break;
  case NRFX_ERROR_DRV_TWI_ERR_OVERRUN:
    jsiConsolePrint("PDM TWI error: Overrun.\r\n");
    break;
  case NRFX_ERROR_DRV_TWI_ERR_ANACK:
    jsiConsolePrint("PDM TWI error: Address not acknowledged.\r\n");
    break;
  case NRFX_ERROR_DRV_TWI_ERR_DNACK:
    jsiConsolePrint("PDM TWI error: Data not acknowledged.\r\n");
    break;
  default:
    break;
  }

}

/*JSON{
  "type" : "function",
  "class" : "Pdm",
  "name" : "start",
  "generate" : "jswrap_pdm_start"
} */
void jswrap_pdm_start( ) {
	nrfx_err_t err = nrfx_pdm_start();
  jswrap_pdm_log_error(err); // log error if there is one
}


/*JSON{
  "type" : "function",
  "class" : "Pdm",
  "name" : "stop",
  "generate" : "jswrap_pdm_stop"
} */
void jswrap_pdm_stop( ) {

}

