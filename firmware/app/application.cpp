#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "mavbridge.h"
#include "web_config.h"
#include "AppSettings.h"

void init()
{
    spiffs_mount(); // Mount file system, in order to work with files

    if (AppSettings.exist() && AppSettings.baud_rate) {
        Serial.begin(AppSettings.baud_rate);
    } else {
        Serial.begin(115200);
    }

	Serial.systemDebugOutput(true); // Allow debug print to serial

	AppSettings.load();
    webserver_init();
    mavbridge_init();
}
