#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "mavbridge.h"
#include "web_config.h"
#include "AppSettings.h"


void init()
{

	int slot = rboot_get_current_rom();

	if (slot == 0) {
		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
	} else {
		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
	}

    if (AppSettings.exist() && AppSettings.baud_rate) {
        Serial.begin(AppSettings.baud_rate);
    } else {
        Serial.begin(DEFAULT_MAVLINK_BAUDRATE);
    }

	Serial.systemDebugOutput(true); // Allow debug print to serial

	AppSettings.load();
    webserver_init();

    mavbridge_init();
}
