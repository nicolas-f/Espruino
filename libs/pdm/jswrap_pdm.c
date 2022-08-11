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

// Double buffering for not missing samples
JsVar* jswrap_pdm_bufferA = NULL;
JsVar* jswrap_pdm_bufferB = NULL;
// buffer length
uint16_t jswrap_pdm_buffer_length = 0;
// JS Function to call when the samples are available. With samples in argument
JsVar* jswrap_pdm_samples_callback = NULL;


void jswrap_pdm_log_error( nrfx_err_t err ) {
  switch (err) {
  case NRFX_ERROR_INTERNAL:
    jsiConsolePrint("PDM Internal error.\r\n");
    break;
  case NRFX_ERROR_NO_MEM:
    jsiConsolePrint("PDM No memory for operation.\r\n");
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


void jswrap_pdm_handler( nrfx_pdm_evt_t const * const pdm_evt) {
  // NRFX request a location to save the samples
  // We will use 2 buffers in order to not loose samples
	if (pdm_evt->buffer_requested) {
		if (jswrap_pdm_useBufferA && jswrap_pdm_bufferA) {
			nrfx_pdm_buffer_set(jswrap_pdm_bufferA, jswrap_pdm_buffer_length);
		} else if(jswrap_pdm_bufferB){
			nrfx_pdm_buffer_set(jswrap_pdm_bufferB, jswrap_pdm_buffer_length);
		}
		jswrap_pdm_useBufferA = !jswrap_pdm_useBufferA;
	}
  if (pdm_evt->buffer_released) {
    int16_t *samples = (int16_t *)pdm_evt->buffer_released;
    if(jswrap_pdm_samples_callback) {
      jspExecuteFunction(jswrap_pdm_samples_callback, NULL, 1, );
    }
  }
  jswrap_pdm_log_error(pdm_evt->error);
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
    ["pin_din","pin","Data pin of PDM microphone"],
    ["callback","JsVar","The function callback when samples are available"],
    ["buffer_a","JsVar","Adress of the first samples buffer"],
    ["buffer_b","JsVar","Adress of the second samples buffer (double buffering)"],
    ["buffer_length","JsVarInt","Length of the samples buffer (same length for the two)"]
  ]
}*/
void jswrap_pdm_setup(Pin pin_clock, Pin pin_din, JsVar* callback, JsVar* buffer_a, JsVar* buffer_b) {
  if (!jshIsPinValid(pin_din)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jshIsPinValid(pin_clock)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jsvIsFunction(callback)) {
    jsExceptionHere(JSET_ERROR, "Function not supplied!");
    return 0;
  }
  if (!jsvIsArrayBuffer(buffer_a)) {
    jsExceptionHere(JSET_ERROR, "Buffer A is not an ArrayBuffer! call new Int16Array(arr, byteOffset, length)");
    return 0;
  }
  if (!jsvIsArrayBuffer(buffer_b)) {
    jsExceptionHere(JSET_ERROR, "Buffer B is not an ArrayBuffer! call new Int16Array(arr, byteOffset, length)");
    return 0;
  }
  if(jsvGetLength(buffer_a) != jsvGetLength(buffer_b)) {
    jsExceptionHere(JSET_ERROR, "The two buffers must be of the same length");
    return 0;
  }
  JsVarDataArrayBufferViewType arrayBufferTypeA = buffer_a->varData.arraybuffer.type;
  JsVarDataArrayBufferViewType arrayBufferTypeB = buffer_a->varData.arraybuffer.type;
  if(!(arrayBufferTypeA == ARRAYBUFFERVIEW_INT16 && arrayBufferTypeA == arrayBufferTypeB )) {    
    jsExceptionHere(JSET_ERROR, "The two buffers must be of the same type (Int16Array)");
    return 0;
  }
  int buffer_length = (int)jsvGetLength(buffer_a);

  jshPinSetState(pin_din, JSHPINSTATE_GPIO_IN);
  jshPinSetState(pin_clock, JSHPINSTATE_GPIO_OUT);

  jswrap_pdm_useBufferA = true;
  jswrap_pdm_bufferA = buffer_a;
  jswrap_pdm_bufferB = buffer_b;
  jswrap_pdm_buffer_length = buffer_length;
  jswrap_pdm_samples_callback = callback;

	// Load PDM default values
	nrfx_pdm_config_t config = NRFX_PDM_DEFAULT_CONFIG(pin_clock, pin_din);

	nrfx_err_t err = nrfx_pdm_init(&config, jswrap_pdm_handler);
  jswrap_pdm_log_error(err); // log error if there is one

  jsiConsolePrint("Driver init ok!\r\n");
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

