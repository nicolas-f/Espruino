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
nrfx_pdm_config_t jswrap_pdm_config = NRFX_PDM_DEFAULT_CONFIG(22, 21);
uint8_t           jswrap_pdm_pin_clk;  // user defined clock pin
uint8_t           jswrap_pdm_pin_din;  // user defined data in pin


void jswrap_pdm_log_error( nrfx_err_t err ) {
  switch (err) {
  case NRFX_ERROR_INTERNAL:
    jsError("PDM Internal error.\r\n");
    break;
  case NRFX_ERROR_NO_MEM:
    jsError("PDM No memory for operation.\r\n");
    break;
  case NRFX_ERROR_NOT_SUPPORTED:
    jsError("PDM Not supported.\r\n");
    break;
  case NRFX_ERROR_INVALID_PARAM:
    jsError("PDM Invalid parameter.\r\n");
    break;
  case NRFX_ERROR_INVALID_STATE:
    jsError("PDM Invalid state, operation disallowed in this state.\r\n");
    break;
  case NRFX_ERROR_INVALID_LENGTH:
    jsError("PDM Invalid length.\r\n");
    break;
  case NRFX_ERROR_FORBIDDEN:
    jsError("PDM Operation is forbidden.\r\n");
    break;
  case NRFX_ERROR_NULL:
    jsError("PDM Null pointer.\r\n");
    break;
  case NRFX_ERROR_INVALID_ADDR:
    jsError("PDM Bad memory address.\r\n");
    break;
  case NRFX_ERROR_BUSY:
    jsError("PDM Busy.\r\n");
    break;
  case NRFX_ERROR_ALREADY_INITIALIZED:
    jsError("PDM Module already initialized.\r\n");
    break;
  case NRFX_ERROR_DRV_TWI_ERR_OVERRUN:
    jsError("PDM TWI error: Overrun.\r\n");
    break;
  case NRFX_ERROR_DRV_TWI_ERR_ANACK:
    jsError("PDM TWI error: Address not acknowledged.\r\n");
    break;
  case NRFX_ERROR_DRV_TWI_ERR_DNACK:
    jsError("PDM TWI error: Data not acknowledged.\r\n");
    break;
  default:
    break;
  }
}

int16_t bufA[128];
int16_t bufB[128];

void jswrap_pdm_handler( nrfx_pdm_evt_t const * const pdm_evt) {
  jsiConsolePrint("Call to jswrap_pdm_handler\r\n");

	if (pdm_evt->buffer_requested) {
		if (jswrap_pdm_useBufferA) {
      nrfx_pdm_buffer_set(bufA, 128);
		} else {
      nrfx_pdm_buffer_set(bufB, 128);
		}
		jswrap_pdm_useBufferA = !jswrap_pdm_useBufferA;
	}
  if (pdm_evt->buffer_released) {
    int16_t *samples = (int16_t *)pdm_evt->buffer_released;
    jspExecuteFunction(jswrap_pdm_samples_callback, NULL, 1, &jswrap_pdm_bufferA);
  }


  // NRFX request a location to save the samples
  // We will use 2 buffers in order to not loose samples
  // size_t buffer_length;        
	// if (pdm_evt->buffer_requested) {
  //   int16_t * buffer_ptr;
	// 	if (jswrap_pdm_useBufferA && jswrap_pdm_bufferA) {
  //     buffer_ptr = (int16_t *)jsvGetDataPointer(jswrap_pdm_bufferA, &buffer_length);
	// 	} else if(jswrap_pdm_bufferB){
  //     buffer_ptr = (int16_t *)jsvGetDataPointer(jswrap_pdm_bufferB, &buffer_length);
	// 	}
  //   nrfx_pdm_buffer_set(buffer_ptr, jswrap_pdm_buffer_length);
	// 	jswrap_pdm_useBufferA = !jswrap_pdm_useBufferA;
	// }
  // if (pdm_evt->buffer_released) {
  //   // We got samples
  //   int16_t *samples = (int16_t *)pdm_evt->buffer_released;
  //   if(jswrap_pdm_samples_callback) {
  //     int16_t * buffer_ptr = (int16_t *)jsvGetDataPointer(jswrap_pdm_bufferA, &buffer_length);
  //     // find original Js objects for this array adress
  //     if(buffer_ptr == samples) {
  //       jspExecuteFunction(jswrap_pdm_samples_callback, NULL, 1, &jswrap_pdm_bufferA);
  //     } else {
  //       jspExecuteFunction(jswrap_pdm_samples_callback, NULL, 1, &jswrap_pdm_bufferB);
  //     }
  //   }
  // }
  // jswrap_pdm_log_error(pdm_evt->error);
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
"name" : "init",
"generate" : "jswrap_pdm_init",
"params" : [
  ["callback","JsVar","The function callback when samples are available"],
  ["buffer_a","JsVar","First samples buffer of type Int16Array"],
  ["buffer_b","JsVar","Second samples buffer (double buffering) must be same size and type than buffer A"]
]
}*/
void jswrap_pdm_init(JsVar* callback, JsVar* buffer_a, JsVar* buffer_b) {

  if (!jsvIsFunction(callback)) {
    jsExceptionHere(JSET_ERROR, "Function not supplied!");
    return;
  }
  if (!jsvIsArrayBuffer(buffer_a)) {
    jsExceptionHere(JSET_ERROR, "Buffer A is not an ArrayBuffer! call new Int16Array(arr, byteOffset, length)");
    return;
  }
  if (!jsvIsArrayBuffer(buffer_b)) {
    jsExceptionHere(JSET_ERROR, "Buffer B is not an ArrayBuffer! call new Int16Array(arr, byteOffset, length)");
    return;
  }
  if(jsvGetLength(buffer_a) != jsvGetLength(buffer_b)) {
    jsExceptionHere(JSET_ERROR, "The two buffers must be of the same length");
    return;
  }
  JsVarDataArrayBufferViewType arrayBufferTypeA = buffer_a->varData.arraybuffer.type;
  JsVarDataArrayBufferViewType arrayBufferTypeB = buffer_a->varData.arraybuffer.type;
  if(!(arrayBufferTypeA == ARRAYBUFFERVIEW_INT16 && arrayBufferTypeA == arrayBufferTypeB )) {    
    jsExceptionHere(JSET_ERROR, "The two buffers must be of the same type (Int16Array)");
    return;
  }
  int buffer_length = (int)jsvGetLength(buffer_a);

  jswrap_pdm_config.pin_clk = NRF_GPIO_PIN_MAP(1, jswrap_pdm_pin_clk);
  jswrap_pdm_config.pin_din = NRF_GPIO_PIN_MAP(1, jswrap_pdm_pin_din);
  jswrap_pdm_useBufferA = true;
  jswrap_pdm_bufferA = buffer_a;
  jswrap_pdm_bufferB = buffer_b;
  jswrap_pdm_buffer_length = buffer_length;
  jswrap_pdm_samples_callback = callback;

	nrfx_err_t err = nrfx_pdm_init(&jswrap_pdm_config, jswrap_pdm_handler);
  jswrap_pdm_log_error(err); // log error if there is one
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "start",
  "generate" : "jswrap_pdm_start"
} */
void jswrap_pdm_start( ) {
	nrfx_err_t err = nrfx_pdm_start();
  jswrap_pdm_log_error(err); // log error if there is one
}


/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "stop",
  "generate" : "jswrap_pdm_stop"
} */
void jswrap_pdm_stop( ) {
	nrfx_err_t err = nrfx_pdm_stop();
  jswrap_pdm_log_error(err); // log error if there is one
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "uninit",
  "generate" : "jswrap_pdm_uninit"
} */
void jswrap_pdm_uninit( ) {
  nrfx_pdm_uninit();
  jswrap_pdm_useBufferA = true;
  jswrap_pdm_bufferA = NULL;
  jswrap_pdm_bufferB = NULL;
  jswrap_pdm_buffer_length = 0;
  jswrap_pdm_samples_callback = NULL;
}
