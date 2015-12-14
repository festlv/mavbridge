#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>
#include "ota_update.h"


rBootHttpUpdate* otaUpdater = 0;

void ota_update_callback(bool result) {
    if (result == true) {
        // success
        uint8 slot;
        slot = rboot_get_current_rom();
        if (slot == 0) slot = 1; else slot = 0;
        // set to boot new rom and then reboot
        debugf("Firmware updated, rebooting to rom %d...\r\n", slot);
        rboot_set_current_rom(slot);
        System.restart();
    } else {
        // fail
        debugf("Firmware update failed!");
    }
}

void ota_update() {

    uint8 slot;
    rboot_config bootconf;

    // need a clean object, otherwise if run before and failed will not run again
    if (otaUpdater) delete otaUpdater;
    otaUpdater = new rBootHttpUpdate();

    // select rom slot to flash
    bootconf = rboot_get_config();
    slot = bootconf.current_rom;
    if (slot == 0) slot = 1; else slot = 0;

	char update_rom0[256];
	char update_spiffs[256];

	sprintf(update_rom0, "%s%s", AppSettings.ota_link.c_str(), "rom0.bin");
	sprintf(update_spiffs, "%s%s", AppSettings.ota_link.c_str(), "spiff_rom.bin");

    debugf("rom0 -> %s\n", update_rom0);
    debugf("spiffs -> %s\n", update_spiffs);

    // flash rom to position indicated in the rBoot config rom table
    otaUpdater->addItem(bootconf.roms[slot], update_rom0);

    // use user supplied values (defaults for 4mb flash in makefile)
    if (slot == 0) {
        otaUpdater->addItem(RBOOT_SPIFFS_0, update_spiffs);
    } else {
        otaUpdater->addItem(RBOOT_SPIFFS_1, update_spiffs);
    }

    // set a callback
    otaUpdater->setCallback(ota_update_callback);
    debugf("Starting update\n");
    // start update
    otaUpdater->start();
}
