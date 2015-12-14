#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>

HttpServer server;
FTPServer ftp;

BssList networks;
String network, password;
Timer connectionTimer;

rBootHttpUpdate* otaUpdater = 0;

void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	response.sendTemplate(tmpl); // will be automatically deleted
}

void OtaUpdate_CallBack(bool result) {
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

void OtaUpdate() {

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
    otaUpdater->setCallback(OtaUpdate_CallBack);
    debugf("Starting update\n");
    // start update
    otaUpdater->start();
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
		AppSettings.ip = request.getPostParameter("ip");
		AppSettings.netmask = request.getPostParameter("netmask");
		AppSettings.gateway = request.getPostParameter("gateway");
		debugf("Updating IP settings: %d", AppSettings.ip.isNull());

        AppSettings.mav_port_in = atoi(request.getPostParameter("mav_port_in").c_str());
        AppSettings.mav_port_out = atoi(request.getPostParameter("mav_port_out").c_str());
        AppSettings.baud_rate = atoi(request.getPostParameter("baud_rate").c_str());
        AppSettings.ota_link = request.getPostParameter("ota_link");

        if (request.getPostParameter("do_update").length() > 0)
            OtaUpdate();    

		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
	auto &vars = tmpl->variables();

	bool dhcp = WifiStation.isEnabledDHCP();
	vars["dhcpon"] = dhcp ? "checked='checked'" : "";
	vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";

	if (!WifiStation.getIP().isNull())
	{
		vars["ip"] = WifiStation.getIP().toString();
		vars["netmask"] = WifiStation.getNetworkMask().toString();
		vars["gateway"] = WifiStation.getNetworkGateway().toString(); 
    }
	else
	{
		vars["ip"] = "192.168.1.77";
		vars["netmask"] = "255.255.255.0";
		vars["gateway"] = "192.168.1.1";
	}
    debugf("baud_rate: %d\n", AppSettings.baud_rate); 
    vars["baud_rate"] = AppSettings.baud_rate;
    vars["ota_link"] = AppSettings.ota_link;
    vars["mav_port_in"] = AppSettings.mav_port_in;
    vars["mav_port_out"] = AppSettings.mav_port_out;

    vars["sw_ver"] = SW_VER;
    vars["hw_ver"] = HW_VER;

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

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/ipconfig", onIpConfig);
	server.addPath("/ajax/get-networks", onAjaxNetworkList);
	server.addPath("/ajax/connect", onAjaxConnect);
	server.setDefaultHandler(onFile);
}

void startFTP()
{
	if (!fileExist("index.html"))
		fileSetContent("index.html", "<h3>Please connect to FTP and upload files from folder 'web/build' (details in code)</h3>");

	// Start FTP server
	ftp.listen(21);
	ftp.addUser("me", "123"); // FTP account
}

// Will be called when system initialization was completed
void startServers()
{
	startFTP();
	startWebServer();
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

void webserver_init()
{


	WifiStation.enable(true);

	if (AppSettings.exist())
	{
		WifiStation.config(AppSettings.ssid, AppSettings.password);
		if (!AppSettings.dhcp && !AppSettings.ip.isNull())
			WifiStation.setIP(AppSettings.ip, AppSettings.netmask, AppSettings.gateway);
	}

	WifiStation.startScan(networkScanCompleted);

    // Start AP for configuration
	WifiAccessPoint.enable(true);
	WifiAccessPoint.config("MAVBridge-" + WifiAccessPoint.getMAC(), "", AUTH_OPEN);

	// Run WEB server on system ready
	System.onReady(startServers);
}


