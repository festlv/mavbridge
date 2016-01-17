#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "AppSettings.h"

#include "mavlink/ardupilotmega/mavlink.h"
#include "mavbridge.h"

#define MAX_CLIENTS 5
#define MAX_INTERFACES 2


IPAddress client_list[MAX_CLIENTS];

IPAddress interface_ips[MAX_INTERFACES];
static uint8_t num_interfaces=0;
static uint8_t num_clients=0;

static uint32_t uart_pkts_rcvd = 0;
static uint32_t net_pkts_rcvd = 0;

void udp_receive_callback(UdpConnection& connection, char *data, int size, 
        IPAddress remoteIP, uint16_t remotePort);
// UDP server
UdpConnection udp(udp_receive_callback);

void timer_interrupt();
Timer uart_recv_timer;

void interface_update_interrupt();
Timer interface_update_timer;


// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
	debugf("READY!");

	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());

    uart_recv_timer.initializeMs(1, timer_interrupt).start();

    interface_update_timer.initializeMs(1000, interface_update_interrupt).start();

    udp.listen(AppSettings.mav_port_in);
}


void interface_update_interrupt(void) {

    //update interface_ips with IP addresses of currently active interfaces
    if (WifiAccessPoint.isEnabled()) {
        for (uint8_t i=0; i < MAX_INTERFACES; i++) {
            if (interface_ips[i] == WifiAccessPoint.getIP())
                goto update_station;
        }
        interface_ips[num_interfaces] = WifiAccessPoint.getIP();
        num_interfaces++;
    }
    //update station IP address if connected
update_station:
    if (WifiStation.isEnabled() && WifiStation.isConnected()) {
        for (uint8_t i=0; i < MAX_INTERFACES; i++) {
            if (interface_ips[i] == WifiStation.getIP())
                goto update_clients;
        }
        interface_ips[num_interfaces] = WifiStation.getIP();
        num_interfaces++;
    }
update_clients:
	//@TODO: Clean the client_list from clients which have not
    //sent a heartbeat during last XX seconds

//    debugf("num_interfaces: %d\n, num_clients: %d\n", num_interfaces, num_clients);
    return;
}

static mavlink_message_t udp_in_msg;
static mavlink_status_t  udp_in_status;
static uint8_t udp_buffer[512];



void udp_receive_callback(UdpConnection& connection, char *data, int size, 
        IPAddress remoteIP, uint16_t remotePort) {

    digitalWrite(NET_LED_PIN, 1);    
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
    //try to decode incoming message 
    for (int i = 0; i < size; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, (uint8_t)data[i], &udp_in_msg, &udp_in_status)) {
            net_pkts_rcvd++;
            debugf("UDP > : %d\n", udp_in_msg.msgid);
            //message decoded, send via serial
            uint16_t len = mavlink_msg_to_send_buffer(udp_buffer, &udp_in_msg);
            if (len > 0) {
                //really inefficient but should do the trick
                for (uint16_t j=0; j < len; j++) 
                    Serial.write(udp_buffer[j]);
            }
        }
    }

    digitalWrite(NET_LED_PIN, 0);    
    return;
}

//executed frequently, reads available data from UART
void timer_interrupt() {

    static mavlink_message_t msg;
    static mavlink_status_t status;
    static uint8_t out_buffer[300];
    static uint8_t in_buffer[RX_BUFF_SIZE];
    int read_len = Serial.readMemoryBlock((char*)in_buffer, RX_BUFF_SIZE);
    for (int idx=0; idx < read_len; idx++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, in_buffer[idx], &msg, &status)) {
            digitalWrite(UART_LED_PIN, 1);
            //mavlink packet received from UART
            //debugf("UART > : %d\n", msg.msgid, msg.len);

            uint16_t len = mavlink_msg_to_send_buffer(out_buffer, &msg);
            if (len > 0) {
                uart_pkts_rcvd++;
                if (msg.msgid == 0) {
                    //heartbeat messages are also broadcast
                    for (uint8_t interface = 0; interface < num_interfaces; interface++) {

                        udp.sendTo(
                                IPAddress(
                                    interface_ips[interface][0], 
                                    interface_ips[interface][1], 
                                    interface_ips[interface][2], 
                                    255
                                    ), AppSettings.mav_port_out, (const char*)out_buffer, len);
                    }
                } 
                //forward other packets to connected clients
                for (uint8_t i=0; i < num_clients; i++)
                    udp.sendTo(client_list[i], AppSettings.mav_port_out, (const char*)out_buffer, len);
            }
            digitalWrite(UART_LED_PIN, 0);
        }
    }
}

void mavbridge_get_status(uint32_t &uart_packets_received, uint32_t &net_packets_received) {
    uart_packets_received = uart_pkts_rcvd;
    net_packets_received = net_pkts_rcvd;
}

void mavbridge_init() 
{
    pinMode(UART_LED_PIN, OUTPUT);
    pinMode(NET_LED_PIN, OUTPUT);

    digitalWrite(UART_LED_PIN, 1);
    digitalWrite(NET_LED_PIN, 1);

    delay(200);
    digitalWrite(UART_LED_PIN, 0);
    digitalWrite(NET_LED_PIN, 0);

	// Set system ready callback method
	System.onReady(ready);
}
