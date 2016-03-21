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

void ApplicationSettingsStorage::restore() {
        baud_rate = DEFAULT_MAVLINK_BAUDRATE;
        baud_rate = DEFAULT_MAVLINK_BAUDRATE;
        mav_port_in = DEFAULT_MAVLINK_PORT;
        mav_port_out = DEFAULT_MAVLINK_PORT;
        tcp_mav_port_in = DEFAULT_TCP_PORT;
        ota_link = DEFAULT_OTA_LINK;

        ap_password = DEFAULT_AP_PASSWORD;
        ap_ssid = DEFAULT_AP_SSID;
        ssid = "";
        password ="";
        dhcp = true;
        debug_output = false;
}

void ApplicationSettingsStorage::load()
{
    DynamicJsonBuffer jsonBuffer;
    baud_rate = DEFAULT_MAVLINK_BAUDRATE;
    mav_port_in = DEFAULT_MAVLINK_PORT;
    mav_port_out = DEFAULT_MAVLINK_PORT;
    ota_link = DEFAULT_OTA_LINK;
    tcp_mav_port_in = DEFAULT_TCP_PORT;
    ap_password = DEFAULT_AP_PASSWORD;
    ap_ssid = DEFAULT_AP_SSID;

    if (exist())
    {
        int size = fileGetSize(APP_SETTINGS_FILE);
        char* jsonString = new char[size + 1];
        fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
        JsonObject& root = jsonBuffer.parseObject(jsonString);

        JsonObject& network = root["network"];
        ssid = network["ssid"].asString();
        password = network["password"].asString();

        dhcp = network["dhcp"];
        
        ip = network["ip"].asString();
        netmask = network["netmask"].asString();
        gateway = network["gateway"].asString();

        ap_password = network["ap_password"].asString();
        ap_ssid = network["ap_ssid"].asString();

        JsonObject& mavbridge = root["mavbridge"];
        baud_rate = mavbridge["baud_rate"];
        mav_port_in = mavbridge["mav_port_in"];
        tcp_mav_port_in = mavbridge["tcp_mav_port_in"];
        mav_port_out = mavbridge["mav_port_out"];
        ota_link = mavbridge["ota_link"].asString();

        debug_output = root["debug_output"];
        
    
        delete[] jsonString;
    }
}

void ApplicationSettingsStorage::save()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    JsonObject& network = jsonBuffer.createObject();

    root["network"] = network;

    root["debug_output"] = debug_output;

    network["ssid"] = ssid.c_str();
    network["password"] = password.c_str();

    network["dhcp"] = dhcp;

    // Make copy by value for temporary string objects
    network["ip"] = ip.toString();
    network["netmask"] = netmask.toString();
    network["gateway"] = gateway.toString();

    network["ap_ssid"] = ap_ssid.c_str();
    network["ap_password"] = ap_password.c_str();


    JsonObject& mavbridge = jsonBuffer.createObject();
    root["mavbridge"] = mavbridge;
    mavbridge["baud_rate"] = baud_rate;
    mavbridge["ota_link"] = ota_link.c_str();
    mavbridge["mav_port_in"] = mav_port_in;
    mavbridge["tcp_mav_port_in"] = tcp_mav_port_in;
    mavbridge["mav_port_out"] = mav_port_out;

    //TODO: add direct file stream writing
    String rootString;
    root.printTo(rootString);
    fileSetContent(APP_SETTINGS_FILE, rootString);
}

bool ApplicationSettingsStorage::exist()
{
    return fileExist(APP_SETTINGS_FILE); 
}
