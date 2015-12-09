#ifndef INCLUDE_APPSETTINGS_H_
#define INCLUDE_APPSETTINGS_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>


#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)

#define DEFAULT_MAVLINK_PORT 14550
#define DEFAULT_OTA_LINK "http://imprimus.lv/mavbridge/fw/rev1/"
#define DEFAULT_MAVLINK_BAUDRATE 57600

struct ApplicationSettingsStorage
{
	String ssid;
	String password;

	bool dhcp = true;

	IPAddress ip;
	IPAddress netmask;
	IPAddress gateway;

    /** Custom options
     */
    int baud_rate;
    int mav_port_in;
    int mav_port_out;
    String ota_link;

	void load()
	{
		DynamicJsonBuffer jsonBuffer;
        baud_rate = DEFAULT_MAVLINK_BAUDRATE;
        mav_port_in = DEFAULT_MAVLINK_PORT;
        mav_port_out = DEFAULT_MAVLINK_PORT;
        ota_link = DEFAULT_OTA_LINK;
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

            JsonObject& mavbridge = root["mavbridge"];
            baud_rate = mavbridge["baud_rate"];
            mav_port_in = mavbridge["mav_port_in"];
            mav_port_out = mavbridge["mav_port_out"];
            ota_link = mavbridge["ota_link"].asString();
        
			delete[] jsonString;
		}
	}

	void save()
	{
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();

		JsonObject& network = jsonBuffer.createObject();
		root["network"] = network;
		network["ssid"] = ssid.c_str();
		network["password"] = password.c_str();

		network["dhcp"] = dhcp;

		// Make copy by value for temporary string objects
		network["ip"] = ip.toString();
		network["netmask"] = netmask.toString();
		network["gateway"] = gateway.toString();


        JsonObject& mavbridge = jsonBuffer.createObject();
        root["mavbridge"] = mavbridge;
        mavbridge["baud_rate"] = baud_rate;
        mavbridge["ota_link"] = ota_link.c_str();
        mavbridge["mav_port_in"] = mav_port_in;
        mavbridge["mav_port_out"] = mav_port_out;
                                                 
		//TODO: add direct file stream writing
		String rootString;
		root.printTo(rootString);
		fileSetContent(APP_SETTINGS_FILE, rootString);
	}

	bool exist() { return fileExist(APP_SETTINGS_FILE); }
};

static ApplicationSettingsStorage AppSettings;

#endif /* INCLUDE_APPSETTINGS_H_ */
