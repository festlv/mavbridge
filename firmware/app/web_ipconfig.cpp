#include "user_config.h"
#include <SmingCore/SmingCore.h>
#include "AppSettings.h"
#include "ota_update.h"

#include "web_ipconfig.h"

BssList networks;
String network, password;
Timer connectionTimer;

Timer restartTimer;

void settingsRestart()
{
    System.restart();
}

void onSettings(HttpRequest &request, HttpResponse &response) {
	TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
	auto &vars = tmpl->variables();

	if (request.getRequestMethod() == RequestMethod::POST)
	{
        AppSettings.mav_port_in = atoi(request.getPostParameter("mav_port_in").c_str());
        AppSettings.tcp_mav_port_in = atoi(request.getPostParameter("tcp_mav_port_in").c_str());
        AppSettings.mav_port_out = atoi(request.getPostParameter("mav_port_out").c_str());
        AppSettings.baud_rate = atoi(request.getPostParameter("baud_rate").c_str());
        AppSettings.ota_link = request.getPostParameter("ota_link");
		AppSettings.debug_output = request.getPostParameter("debug_output") == "1";

        if (request.getPostParameter("do_update").length() > 0)
        {
            ota_update();
        } 

        if (request.getPostParameter("save_reboot").length() > 0)
        {
			restartTimer.initializeMs(1000, settingsRestart).startOnce();
            vars["message"] = "MAVBridge is now being restarted...";
        }
    } 
    else
    {
        vars["message"] = "Settings will be applied after restart";
    }


    vars["baud_rate"] = AppSettings.baud_rate;
    vars["ota_link"] = AppSettings.ota_link;
    vars["mav_port_in"] = AppSettings.mav_port_in;
    vars["tcp_mav_port_in"] = AppSettings.tcp_mav_port_in;
    vars["mav_port_out"] = AppSettings.mav_port_out;

	vars["dbg_on"] = AppSettings.debug_output ? "checked='checked'" : "";
	vars["dbg_off"] = !AppSettings.debug_output ? "checked='checked'" : "";

    vars["sw_ver"] = SW_VER;
    vars["hw_ver"] = HW_VER;

	response.sendTemplate(tmpl); // will be automatically deleted

    //save settings, reboot if neccessary
    AppSettings.save();
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
		AppSettings.ip = request.getPostParameter("ip");
		AppSettings.netmask = request.getPostParameter("netmask");
		AppSettings.gateway = request.getPostParameter("gateway");

		AppSettings.ap_ssid = request.getPostParameter("ap_ssid");

        if (request.getPostParameter("ap_password").length() >= 8)
            AppSettings.ap_password = request.getPostParameter("ap_password");

		debugf("Updating IP settings: %d", AppSettings.ip.isNull());

        AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("networks.html");
	auto &vars = tmpl->variables();

	bool dhcp = WifiStation.isEnabledDHCP();



	vars["dhcpon"] = dhcp ? "checked='checked'" : "";
	vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";
    vars["ap_ssid"] = AppSettings.ap_ssid;
    vars["ap_password"] = AppSettings.ap_password;

	if (!WifiStation.getIP().isNull())
	{
		vars["ip"] = WifiStation.getIP().toString();
		vars["netmask"] = WifiStation.getNetworkMask().toString();
		vars["gateway"] = WifiStation.getNetworkGateway().toString(); 
    }
	else
	{
		vars["ip"] = "0.0.0.0";
		vars["netmask"] = "255.255.255.0";
		vars["gateway"] = "0.0.0.0";
	}
	response.sendTemplate(tmpl); // will be automatically deleted
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}


void onAjaxNetworkList(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["status"] = (bool)true;

	bool connected = WifiStation.isConnected();
	json["connected"] = connected;
	if (connected)
	{
		// Copy full string to JSON buffer memory
		json["network"]= WifiStation.getSSID();
	}

	JsonArray& netlist = json.createNestedArray("available");
	for (int i = 0; i < networks.count(); i++)
	{
		if (networks[i].hidden) continue;
		JsonObject &item = netlist.createNestedObject();
		item["id"] = (int)networks[i].getHashId();
		// Copy full string to JSON buffer memory
		item["title"] = networks[i].ssid;
		item["signal"] = networks[i].rssi;
		item["encryption"] = networks[i].getAuthorizationMethodName();
	}

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void makeConnection()
{
	WifiStation.enable(true);
	WifiStation.config(network, password);

	AppSettings.ssid = network;
	AppSettings.password = password;
	AppSettings.save();

	network = ""; // task completed
}

void onAjaxConnect(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String curNet = request.getPostParameter("network");
	String curPass = request.getPostParameter("password");

	bool updating = curNet.length() > 0 && (WifiStation.getSSID() != curNet || WifiStation.getPassword() != curPass);
	bool connectingNow = WifiStation.getConnectionStatus() == eSCS_Connecting || network.length() > 0;

	if (updating && connectingNow)
	{
		debugf("wrong action: %s %s, (updating: %d, connectingNow: %d)", network.c_str(), password.c_str(), updating, connectingNow);
		json["status"] = (bool)false;
		json["connected"] = (bool)false;
	}
	else
	{
		json["status"] = (bool)true;
		if (updating)
		{
			network = curNet;
			password = curPass;
			debugf("CONNECT TO: %s %s", network.c_str(), password.c_str());
			json["connected"] = false;
			connectionTimer.initializeMs(1200, makeConnection).startOnce();
		}
		else
		{
			json["connected"] = WifiStation.isConnected();
			debugf("Network already selected. Current status: %s", WifiStation.getConnectionStatusName());
		}
	}

	if (!updating && !connectingNow && WifiStation.isConnectionFailed())
		json["error"] = WifiStation.getConnectionStatusName();

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void networkScanCompleted(bool succeeded, BssList list)
{
	if (succeeded)
	{
		for (int i = 0; i < list.count(); i++)
			if (!list[i].hidden && list[i].ssid.length() > 0)
				networks.add(list[i]);
	}
	networks.sort([](const BssInfo& a, const BssInfo& b){ return b.rssi - a.rssi; } );
}
