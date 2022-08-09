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

/** @brief Enable PDM driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define PDM_ENABLED 1
#define PDM_CONFIG_MODE 
#define PDM_CONFIG_EDGE 
#define PDM_CONFIG_CLOCK_FREQ 
#define PDM_CONFIG_IRQ_PRIORITY 
#include "jswrap_pdm.h"
#include "jsvar.h"
#include "jsinteractive.h"
#include "pdm_espruino.h"


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
void setup(Pin PIN_CLK, Pin PIN_DIN, int16_t * BUFF_A, int16_t * BUFF_B, uint16_t BUFF_LEN) {
  if (!jshIsPinValid(PIN_DIN)) {
    jsError("Invalid pin supplied as an argument to Pdm.setup");
    return;
  }
  if (!jshIsPinValid(PIN_CLK)) {
    jsError("Invalid pin supplied as an argument to Trig.setup");
    return;
  }
  nrf_drv_pdm_config_t* pdm_config = NRF_DRV_PDM_DEFAULT_CONFIG(PIN_CLK, PIN_DIN, BUFF_A, BUFF_B, BUFF_LEN);
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
void start(JsVar *func) {


}


/*JSON{
  "type" : "function",
  "class" : "Pdm",
  "name" : "stop",
  "generate" : "jswrap_pdm_stop"
} */
void stop( ) {

}

