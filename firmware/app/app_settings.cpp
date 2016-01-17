#include <SmingCore/SmingCore.h>

#include "AppSettings.h"

#include "mavbridge.h"

ApplicationSettingsStorage AppSettings;

void app_settings_tick_10hz() {

    //counts number of ticks when button was pressed, initiates parameter
    //restore. Blinks both three times, and then reboots after pin is released
    
    static unsigned int ticks_pressed = 0;
    static bool wait_release = false;

    if (digitalRead(RESTORE_PARAMETERS_PIN) == LOW) {
        ticks_pressed++;
        if (ticks_pressed >= 30 && !wait_release) {
            AppSettings.restore();
            AppSettings.save();
            digitalWrite(UART_LED_PIN, HIGH);
            delay(100);
            digitalWrite(NET_LED_PIN, HIGH);
            delay(100);
            digitalWrite(NET_LED_PIN, LOW);
            delay(100);
            digitalWrite(UART_LED_PIN, LOW);
            wait_release = true;
        }
    } else {
        ticks_pressed = 0;
        if (wait_release) {
            delay(1000);
            System.restart();
        }
    }

}
