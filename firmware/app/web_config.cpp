#include "user_config.h"
#include <SmingCore/SmingCore.h>
#include "AppSettings.h"
#include "ota_update.h"
#include "web_ipconfig.h"
#include "mavbridge.h"

HttpServer server;
FTPServer ftp;


void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("status.html");
	auto &vars = tmpl->variables();

    vars["sw_ver"] = SW_VER;
    vars["hw_ver"] = HW_VER;

    vars["baud_rate"] = AppSettings.baud_rate;
    vars["mav_port_in"] = AppSettings.mav_port_in;
    vars["mav_port_out"] = AppSettings.mav_port_out;

    uint32_t net_rcvd, uart_rcvd;
    mavbridge_get_status(uart_rcvd, net_rcvd);
    
    vars["uart_pkts_rcvd"] = uart_rcvd;
    vars["net_pkts_rcvd"] = net_rcvd;

    if (WifiAccessPoint.isEnabled()) {
        vars["ap_ssid"] = "TODO";
        vars["ap_ip"] = WifiAccessPoint.getIP().toString();
    } else {
        vars["ap_ssid"] = "N/A";
        vars["ap_ip"] = "0.0.0.0";
    }

    if (WifiStation.isEnabled()) {
        vars["client_ssid"] = WifiStation.getSSID();
        vars["client_ip"] = WifiStation.getIP().toString();
        vars["client_gw_ip"] = WifiStation.getNetworkGateway().toString();
        if (WifiStation.isConnected())
            vars["client_status"] = "Connected";
        else
            vars["client_status"] = "Not connected";

    } else {
        vars["client_ssid"] = "N/A";
        vars["client_ip"] = "0.0.0.0";
        vars["client_status"] = "Not connected";
    }


	response.sendTemplate(tmpl); // will be automatically deleted
}

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);

	server.addPath("/networks", onIpConfig);
	server.addPath("/settings", onSettings);

	server.addPath("/ajax/get-networks", onAjaxNetworkList);
	server.addPath("/ajax/connect", onAjaxConnect);
	server.setDefaultHandler(onFile);
}

void startFTP()
{
	if (!fileExist("status.html"))
		fileSetContent("status.html", "<h3>Please connect to FTP and upload files from folder 'web/build' (details in code)</h3>");

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


