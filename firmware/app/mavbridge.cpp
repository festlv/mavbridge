#include <user_config.h>
#include <SmingCore/SmingCore.h>


#include "mavlink/ardupilotmega/mavlink.h"

#define MAX_CLIENTS 5

#define LED_PIN 13

IPAddress client_list[MAX_CLIENTS];
static uint8_t num_clients=0;


void udp_receive_callback(UdpConnection& connection, char *data, int size, 
        IPAddress remoteIP, uint16_t remotePort);
// UDP server
const uint16_t listen_port = 14550;
UdpConnection udp(udp_receive_callback);

void timer_interrupt();
Timer uart_recv_timer;


// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
	debugf("READY!");

	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());

    uart_recv_timer.initializeMs(10, timer_interrupt).start();

    udp.listen(listen_port);

}

static mavlink_message_t udp_in_msg;
static mavlink_status_t  udp_in_status;
static uint8_t udp_buffer[512];


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
    //try to decode incoming message 
    for (int i = 0; i < size; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, (uint8_t)data[i], &udp_in_msg, &udp_in_status)) {
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
            digitalWrite(LED_PIN, 0);
            //mavlink packet received from UART
            //debugf("UART > : %d\n", msg.msgid, msg.len);

            uint16_t len = mavlink_msg_to_send_buffer(out_buffer, &msg);
            if (len > 0) {
                if (msg.msgid == 0) {
                    //heartbeat messages are also broadcast
                    udp.sendTo(IPAddress(192, 168, 13, 255), 14550, (const char*)out_buffer, len);
                } 
                //forward other packets to connected clients
                for (uint8_t i=0; i < num_clients; i++)
                    udp.sendTo(client_list[i], 14550, (const char*)out_buffer, len);
            }
            digitalWrite(LED_PIN, 1);
        }
    }
}

void mavbridge_init() 
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, 0);
    delay(1000);
    digitalWrite(LED_PIN, 1);

	// Set system ready callback method
	System.onReady(ready);
}
