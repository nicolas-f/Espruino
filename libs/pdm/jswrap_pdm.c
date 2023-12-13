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
#include "nrf_drv_pdm.h"
#include "nrf_gpio.h"
#include <math.h>

// Double buffering for not missing samples
JsVar* jswrap_pdm_bufferA = NULL;
JsVar* jswrap_pdm_bufferB = NULL;
int16_t* jswrap_pdm_bufferA_data = NULL;
int16_t* jswrap_pdm_bufferB_data = NULL;
uint16_t jswrap_pdm_buffer_length = 0;                                  ///< Length of a single buffer (in 16-bit words).
double jswrap_pdm_rms_value = 0.0;
nrf_pdm_mode_t jswrap_pdm_mode = (nrf_pdm_mode_t)1;       ///< Interface operation mode. Default to mono
nrf_pdm_edge_t jswrap_pdm_edge = (nrf_pdm_edge_t)PDM_CONFIG_EDGE;       ///< Sampling mode.
uint8_t           jswrap_pdm_pin_clk;                                   // user defined clock pin
uint8_t           jswrap_pdm_pin_din;                                   // user defined data in pin
nrf_pdm_freq_t    jswrap_pdm_frequency = PDM_PDMCLKCTRL_FREQ_Default;   // sampling frequency Fs= 16125 Hz 
nrf_pdm_gain_t jswrap_pdm_gain_l= NRF_PDM_GAIN_DEFAULT;                 ///< Left channel gain.
nrf_pdm_gain_t jswrap_pdm_gain_r = NRF_PDM_GAIN_DEFAULT;                ///< Right channel gain.
uint8_t        jswrap_pdm_interrupt_priority = PDM_CONFIG_IRQ_PRIORITY; ///< Interrupt priority.

void jswrap_pdm_log_error( ret_code_t err ) {
  switch (err) {
  case NRF_ERROR_INTERNAL:
    jsError("PDM Internal error.\r\n");
    break;
  case NRF_ERROR_NO_MEM:
    jsError("PDM No memory for operation.\r\n");
    break;
  case NRF_ERROR_NOT_SUPPORTED:
    jsError("PDM Not supported.\r\n");
    break;
  case NRF_ERROR_INVALID_PARAM:
    jsError("PDM Invalid parameter.\r\n");
    break;
  case NRF_ERROR_INVALID_STATE:
    jsError("PDM Module already initialized.\r\n");
    break;
  case NRF_ERROR_INVALID_LENGTH:
    jsError("PDM Invalid length.\r\n");
    break;
  case NRF_ERROR_FORBIDDEN:
    jsError("PDM Operation is forbidden.\r\n");
    break;
  case NRF_ERROR_NULL:
    jsError("PDM Null pointer.\r\n");
    break;
  case NRF_ERROR_INVALID_ADDR:
    jsError("PDM Bad memory address.\r\n");
    break;
  case NRF_ERROR_BUSY:
    jsError("PDM Busy.\r\n");
    break;
  case NRF_ERROR_DRV_TWI_ERR_OVERRUN:
    jsError("PDM TWI error: Overrun.\r\n");
    break;
  case NRF_ERROR_DRV_TWI_ERR_ANACK:
    jsError("PDM TWI error: Address not acknowledged.\r\n");
    break;
  case NRF_ERROR_DRV_TWI_ERR_DNACK:
    jsError("PDM TWI error: Data not acknowledged.\r\n");
    break;
  default:
    break;
  }
}

static void jswrap_pdm_handler( uint32_t * buffer, uint16_t length) {
  uint16_t* samples = (uint16_t*)buffer;
  // We got samples
  double sum = 0;
  for(int i=0; i < length; i++) {
    sum += (double)((float)samples[i]*(float)samples[i]);
  }
  jswrap_pdm_rms_value = sqrt(sum / (double)length);
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
  ["options","JsVar","Object `{clock: integer, din : integer, frequency: integer, lgain: float, rgain: float, mono: boolean, sampling_mode: integer, interrupt_priority: integer}` Supported frequencies are 15625, 16125, 16667, 19230, 20000, 20833, 31250, 41667, 50000, 62500 Hz. Gain is expressed in dB and must be between -20;20"]
]
}*/
void jswrap_pdm_setup(JsVar *options) {

  Pin pin_clock = JSH_PIN5;
  Pin pin_din = JSH_PIN6;
  int frequency = 16125;

  if (jsvIsObject(options)) {
    JsVar *v = jsvObjectGetChild(options,"clock", 0);
    if (v) pin_clock = jsvGetIntegerAndUnLock(v);
    v = jsvObjectGetChild(options,"din", 0);
    if (v) pin_din = jsvGetIntegerAndUnLock(v);
    v = jsvObjectGetChild(options,"frequency", 0);
    if (v) frequency = jsvGetIntegerAndUnLock(v);
    // output gain adjustment, in 0.5 dB steps, around the default module gain
    v = jsvObjectGetChild(options,"lgain", 0);
    if (v) jswrap_pdm_gain_l = (uint8_t)MIN(NRF_PDM_GAIN_MAXIMUM, MAX(NRF_PDM_GAIN_MINIMUM, (int8_t)(jsvGetFloatAndUnLock(v) / 0.5) + (int8_t)(NRF_PDM_GAIN_DEFAULT)));
    v = jsvObjectGetChild(options,"rgain", 0);
    if (v) jswrap_pdm_gain_r = (uint8_t)MIN(NRF_PDM_GAIN_MAXIMUM, MAX(NRF_PDM_GAIN_MINIMUM, (int8_t)(jsvGetFloatAndUnLock(v) / 0.5) + (int8_t)(NRF_PDM_GAIN_DEFAULT)));
    v = jsvObjectGetChild(options,"mono", 0);
    if (v) jswrap_pdm_mode = jsvGetBoolAndUnLock(v);
    v = jsvObjectGetChild(options,"sampling_mode", 0);
    if (v) jswrap_pdm_edge = jsvGetIntegerAndUnLock(v);
    v = jsvObjectGetChild(options,"interrupt_priority", 0);
    if (v) jswrap_pdm_interrupt_priority = jsvGetIntegerAndUnLock(v);
  }
  
  switch (frequency)
  {
    case 15625:
      jswrap_pdm_frequency = NRF_PDM_FREQ_1000K;
      break;
    case 16125:
      jswrap_pdm_frequency = NRF_PDM_FREQ_1032K;
      break;
    case 16667:
      jswrap_pdm_frequency = NRF_PDM_FREQ_1067K;
      break;
    case 19230:
      jswrap_pdm_frequency = NRF_PDM_FREQ_1231K;
      break;
    case 20000:
      jswrap_pdm_frequency = NRF_PDM_FREQ_1280K;
      break;
    case 20833:
      jswrap_pdm_frequency = NRF_PDM_FREQ_1333K;
      break;
    case 31250:
      jswrap_pdm_frequency = NRF_PDM_FREQ_2000K;
      break;
    case 41667:
      jswrap_pdm_frequency = NRF_PDM_FREQ_2667K;
      break;
    case 50000:
      jswrap_pdm_frequency = NRF_PDM_FREQ_3200K;
      break;
    case 62500:
      jswrap_pdm_frequency = NRF_PDM_FREQ_4000K;
      break;  
    default:
      break;
  }

  if (!jshIsPinValid(pin_din)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jshIsPinValid(pin_clock)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }

	// Set PDM user defined values
	jswrap_pdm_pin_clk = pinInfo[pin_clock].pin;
  jswrap_pdm_pin_din = pinInfo[pin_din].pin;
}

/*JSON{
"type" : "staticmethod",
"class" : "Pdm",
"name" : "init",
"generate" : "jswrap_pdm_init",
"params" : [
  ["cache_size","int","Internal cache of audio samples"]
]
}*/
void jswrap_pdm_init(int cache_size) {
  if(cache_size <= 0 || cache_size > 16384) {
    jsError("Invalid cache size, must be between 1 and 16384.\r\n");
    return;
  }
  jswrap_pdm_bufferA = jsvNewArrayBufferWithPtr(cache_size*2, (char**)&jswrap_pdm_bufferA_data);
  if (!jswrap_pdm_bufferA) {
    jsError("Not enough free memory for this buffer size.\r\n");
    jsvUnLock(jswrap_pdm_bufferA);
    return;
  }
  jswrap_pdm_bufferB = jsvNewArrayBufferWithPtr(cache_size*2, (char**)&jswrap_pdm_bufferB_data);
  if (!jswrap_pdm_bufferB) {
    jsError("Not enough free memory for this buffer size.\r\n");
    jsvUnLock(jswrap_pdm_bufferB);
    return;
  }
  jswrap_pdm_buffer_length = (uint16_t)cache_size;

  nrf_drv_pdm_config_t jswrap_pdm_config = NRF_DRV_PDM_DEFAULT_CONFIG(jswrap_pdm_pin_clk, jswrap_pdm_pin_din,
   jswrap_pdm_bufferA_data, jswrap_pdm_bufferB_data, jswrap_pdm_buffer_length);

  jswrap_pdm_config.clock_freq = jswrap_pdm_frequency;
  jswrap_pdm_config.edge = jswrap_pdm_edge;
  jswrap_pdm_config.gain_l = jswrap_pdm_gain_l;
  jswrap_pdm_config.gain_r = jswrap_pdm_gain_r;
  jswrap_pdm_config.interrupt_priority = jswrap_pdm_interrupt_priority;
  jswrap_pdm_config.mode = jswrap_pdm_mode;

	ret_code_t err = nrf_drv_pdm_init(&jswrap_pdm_config, jswrap_pdm_handler);
  jswrap_pdm_log_error(err); // log error if there is one
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "start",
  "generate" : "jswrap_pdm_start"
} */
void jswrap_pdm_start( ) {
	ret_code_t err = nrf_drv_pdm_start();
  jswrap_pdm_log_error(err); // log error if there is one
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "rms",
  "generate" : "jswrap_pdm_rms",
  "return" : ["float", "RMS value" ]
} */
double jswrap_pdm_rms() {
  return jswrap_pdm_rms_value;
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "stop",
  "generate" : "jswrap_pdm_stop"
} */
void jswrap_pdm_stop( ) {
	ret_code_t err = nrf_drv_pdm_stop();
  jswrap_pdm_log_error(err); // log error if there is one
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Pdm",
  "name" : "uninit",
  "generate" : "jswrap_pdm_uninit"
} */
void jswrap_pdm_uninit( ) {
  nrf_drv_pdm_uninit();
  jsvUnRef(jswrap_pdm_bufferA);
  jsvUnRef(jswrap_pdm_bufferB);
  jswrap_pdm_bufferA = NULL;
  jswrap_pdm_bufferB = NULL;
  jswrap_pdm_bufferA_data = NULL;
  jswrap_pdm_bufferB_data = NULL;
  jswrap_pdm_buffer_length = 0;
}
