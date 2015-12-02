#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "mavlink/ardupilotmega/mavlink.h"

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "Ares" // Put you SSID and Password here
	#define WIFI_PWD "w1f1p455"
#endif

#define MAX_CLIENTS 5

IPAddress client_list[MAX_CLIENTS];
static uint8_t num_clients=0;

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
	debugf("READY!");

	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
}


void udp_receive_callback(UdpConnection& connection, char *data, int size, 
        IPAddress remoteIP, uint16_t remotePort) {
    
    //test if this IP is already in client list
    if (num_clients > 0) {
        for (uint8_t i=0; i < num_clients; i++) {
            if (client_list[i] == remoteIP)
                goto existing_client;
        }
    }
    client_list[num_clients] = remoteIP;
    num_clients++;

existing_client:
    static mavlink_message_t udp_in_msg;
    static mavlink_status_t  udp_in_status;
    static uint8_t buffer[512];

    //try to decode incoming message 
    for (int i = i; i < size; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, (uint8_t)data[i], &udp_in_msg, &udp_in_status)) {
            debugf("UDP > : %d\n", udp_in_msg.msgid);
            //message decoded, send via serial
            uint16_t len = mavlink_msg_to_send_buffer(buffer, &udp_in_msg);
            if (len > 0) {
                //really inefficient but usable
                for (uint16_t j=0; j < len; j++) 
                    Serial.write(buffer[j]);
            }
        }
    }

    return;
}


// UDP server
const uint16_t listen_port = 14550;
UdpConnection udp(udp_receive_callback);


//executed frequently, reads available data from UART
void timer_interrupt() {
    static mavlink_message_t msg;
    static mavlink_status_t status;
    static uint8_t buffer[512];

    while (Serial.available() > 0) {
        if (mavlink_parse_char(MAVLINK_COMM_0, (uint8_t)Serial.read(), &msg, &status)) {
            //mavlink packet received from UART
            debugf("UART > : %d\n", msg.msgid);

            uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
            if (msg.msgid == 0) {
                //heartbeat messages are broadcast
                udp.sendTo(IPAddress(192, 168, 13, 255), 14550, (const char*)buffer, len);
            } else {
                //forward other packets to connected clients
                for (uint8_t i=0; i < num_clients; i++)
                    udp.sendTo(client_list[i], 14550, (const char*)buffer, len);
            }

        }
    }
}

Timer uart_recv_timer;

void init()
{
	Serial.begin(921600);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Sming. Let's do smart things!");
	// Set system ready callback method
	System.onReady(ready);

	// Soft access point
	WifiAccessPoint.enable(true);
	WifiAccessPoint.config("ESP8266-MAVLink-bridge", "", AUTH_OPEN);
    WifiStation.enable(false);

	// Optional: Change IP addresses (and disable DHCP)
	WifiAccessPoint.setIP(IPAddress(192, 168, 13, 1));
    uart_recv_timer.initializeMs(10, timer_interrupt).start();

    udp.listen(listen_port);
}
