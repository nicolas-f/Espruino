#include "jswrap_pdm.h"
#include "jsvar.h"
#include "jsinteractive.h"
#include "nrfx_pdm.h"


/*JSON{
"type" : "class",
"class" : "Hello"
}*/

/*JSON{
"type" : "staticmethod",
"class" : "Hello",
"name" : "world",
"generate" : "jswrap_hello_world"
}*/
void jswrap_hello_world() {
  jsiConsolePrint("Hello World!\r\n");
}


