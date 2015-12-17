#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "mavbridge.h"
#include "web_config.h"
#include "AppSettings.h"


void init()
{
    //disable debug output while we're mounting spiffs
    //this has to be done because we can't access settings before mounting
    system_set_os_print(0);
    Serial.systemDebugOutput(false);
	int slot = rboot_get_current_rom();

	if (slot == 0) {
		spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
	} else {
		spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
	}

    


    if (AppSettings.exist() && AppSettings.baud_rate) {
        Serial.begin(AppSettings.baud_rate);
    } else {
        Serial.begin(DEFAULT_MAVLINK_BAUDRATE);
    }
	AppSettings.load();
    Serial.systemDebugOutput(AppSettings.debug_output);

    if (!AppSettings.debug_output) 
        system_set_os_print(0);
    else
        system_set_os_print(1);

    webserver_init();

    mavbridge_init();
}
