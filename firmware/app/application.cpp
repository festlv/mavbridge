#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "mavbridge.h"
#include "web_config.h"

void init()
{
	Serial.begin(921600);
	Serial.systemDebugOutput(true); // Allow debug print to serial
    
    webserver_init();
    mavbridge_init();
}
