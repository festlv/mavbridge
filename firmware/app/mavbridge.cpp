#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "AppSettings.h"

#include "mavbridge.h"

#define MAX_CLIENTS 5
#define MAX_INTERFACES 2

//@TODO: this is wrong and depends on netmask
#define BROADCAST_ADDRESS(ip) (IPAddress(ip[0],ip[1],ip[2],255))

void check_serial_buffer();
void check_network_buffer();
Timer uart_recv_timer, net_recv_timer;


// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());

    uart_recv_timer.initializeMs(1, check_serial_buffer).start();
    net_recv_timer.initializeMs(1, check_network_buffer).start();

    MavlinkServer::get_instance().initialize(AppSettings.mav_port_in, PROTO_UDP);
    MavlinkServer::get_instance().initialize(AppSettings.tcp_mav_port_in, PROTO_TCP);
}

void check_network_buffer() {
    static mavlink_packet_t in_packet;

    if (MavlinkServer::get_instance().pop_received_packet(&in_packet)) {
        digitalWrite(NET_LED_PIN, 1);
        for (int i = 0; i < in_packet.length; i++) {
            //really inefficient but should do the trick
            Serial.write(in_packet.data[i]);
        }

        MavlinkServer::get_instance().ct_uart_out++;
        digitalWrite(NET_LED_PIN, 0);    
    }

    return;
}

void check_serial_buffer() {
    static mavlink_message_t* msg;
    static uint8_t in_buffer[RX_BUFF_SIZE];
    int read_len = Serial.readMemoryBlock((char*)in_buffer, RX_BUFF_SIZE);


    if (read_len > 0) {
        digitalWrite(UART_LED_PIN, 1);
    }

    for (int idx=0; idx < read_len; idx++) {
        if (MavlinkServer::decoder.parse_char(in_buffer[idx]) == MSG_DECODED) {
            msg = MavlinkServer::decoder.get_message();
            MavlinkServer::get_instance().transmit_packet(*msg);
            MavlinkServer::get_instance().ct_uart_in++;
        }
    }
    digitalWrite(UART_LED_PIN, 0);
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



Timer MavlinkServer::interface_update_timer;

Vector<IPAddress> MavlinkServer::udp_clients;
Vector<IPAddress> MavlinkServer::interfaces;
Vector<TcpClient*> MavlinkServer::tcp_clients;
Vector<mavlink_packet_t> MavlinkServer::incoming_message_queue;

bool MavlinkServer::pop_received_packet(mavlink_packet_t* packet) {
    if (incoming_message_queue.size() > 0) {
        *packet = incoming_message_queue[0];
        incoming_message_queue.removeElementAt(0);
        return true;
    }
    return false;
}

bool MavlinkServer::client_exists(IPAddress ip, mavlink_proto_type_t proto) {

    bool found = false;
    if (proto == PROTO_UDP) {
        for (int i=0; i < udp_clients.size(); i++) {
            if (udp_clients[i] == ip) {
                found = true;
                break;
            }
        }
    }
    return found;
}

bool MavlinkServer::interface_exists(IPAddress ip) {

    bool found = false;
    for (int i=0; i < interfaces.size(); i++) {
        if (interfaces[i] == ip) {
            found = true;
            break;
        }
    }
    return found;
}

void MavlinkServer::interface_update_interrupt() {
    //update interface_ips with IP addresses of currently active interfaces
    if (WifiAccessPoint.isEnabled()) {
        if (!interface_exists(BROADCAST_ADDRESS(WifiAccessPoint.getIP()))) {
            interfaces.add(BROADCAST_ADDRESS(WifiAccessPoint.getIP()));
        }
    }

    //update station IP address if connected
    if (WifiStation.isEnabled() && WifiStation.isConnected()) {
        if (!interface_exists(BROADCAST_ADDRESS(WifiStation.getIP()))) {
            interfaces.add(BROADCAST_ADDRESS(WifiStation.getIP()));
        }
    }
}

bool MavlinkServer::tcp_client_receive(TcpClient &client, char* data, int size) {
    //reject too large packets
    if (size > MAVLINK_MAX_PACKET_LEN) {
        return false;
    }

    debugf("TCP < %d bytes\n", size);
    static mavlink_packet_t packet; 
    memcpy(&packet.data, data, size); 
    packet.length = size;
    incoming_message_queue.add(packet);
    ct_tcp_in++;
    return true;
}

void MavlinkServer::tcp_client_connected(TcpClient* client) {
    //adds to the list of connections
    
    for (int i=0; i < tcp_clients.size(); i++) {
        if (tcp_clients[i] == client)
            return;
    }
    //not found
    tcp_clients.add(client);  
    debugf("Adding TCP client\n");
}

void MavlinkServer::tcp_client_complete(TcpClient& client, bool successful) {
    //find the client in list of connections and remove it
    for (int i=0; i < tcp_clients.size(); i++) {
        if (tcp_clients[i] == &client) {
            //client found, remove it
            tcp_clients.removeElementAt(i);
            debugf("Removing TCP client: %d\n");
            break;
        }
    }
}

void MavlinkServer::udp_receive_callback(UdpConnection &conn, char* data, int size,
                IPAddress remoteIP, uint16_t remotePort) {
    if (!client_exists(remoteIP, PROTO_UDP)) {
        udp_clients.add(remoteIP);
    }
    //reject too large packets
    if (size > MAVLINK_MAX_PACKET_LEN) {
        return;
    }

    debugf("UDP < %d bytes\n", size);

    static mavlink_packet_t packet; 
    memcpy(&packet.data, data, size); 
    packet.length = size;
    incoming_message_queue.add(packet);
    ct_udp_in++;
}

UdpConnection* MavlinkServer::udp_conn = NULL;
TcpServer* MavlinkServer::tcp_server = NULL;

void MavlinkServer::initialize(uint16_t port, mavlink_proto_type_t protocol)
{

    bool start_timer = false;

    if (protocol == PROTO_UDP && port != 0) {
        if (udp_conn != NULL) {
            delete udp_conn;
        }
        udp_conn = new UdpConnection(MavlinkServer::udp_receive_callback); 
        udp_conn->listen(port);
        debugf("MavlinkServer listening to UDP at %d\n", port); 
        start_timer = true;

    } else if (protocol == PROTO_TCP && port != 0) {
        if (tcp_server != NULL) {
            delete tcp_server;
        }
        tcp_server = new TcpServer(MavlinkServer::tcp_client_connected, 
                MavlinkServer::tcp_client_receive,
                MavlinkServer::tcp_client_complete);
        tcp_server->listen(port);

        debugf("MavlinkServer listening to TCP at %d\n", port); 
        start_timer = true;
    }

    if (start_timer)
        interface_update_timer.initializeMs(1000, MavlinkServer::interface_update_interrupt).start();
}

void MavlinkServer::transmit_packet(mavlink_message_t& msg) {

    if (msg.buf_len == 0) {
        return;
    }

    int i;

    if (msg.msgid == 0) {
        for (i=0; i < interfaces.size(); i++) {
            udp_conn->sendTo(interfaces[i], AppSettings.mav_port_out, 
                    (const char*)msg.buf, msg.buf_len);
            ct_udp_out++;
        }
    }

    for (i=0; i < udp_clients.size(); i++) {
        udp_conn->sendTo(udp_clients[i], AppSettings.mav_port_out, 
                (const char*)msg.buf, msg.buf_len);
        ct_udp_out++;
    }

    for (i=0; i < tcp_clients.size(); i++) {
        if (tcp_clients[i]->send((const char*)msg.buf, msg.buf_len))
        {
            ct_tcp_out++;
        }
        else
        {
            ct_tcp_dropped++;
        }
    }
}

uint32_t MavlinkServer::ct_tcp_in = 0;
uint32_t MavlinkServer::ct_tcp_out = 0;
uint32_t MavlinkServer::ct_tcp_dropped = 0;

uint32_t MavlinkServer::ct_udp_in = 0;
uint32_t MavlinkServer::ct_udp_out = 0;
uint32_t MavlinkServer::ct_udp_dropped = 0;

uint32_t MavlinkServer::ct_uart_in = 0;
uint32_t MavlinkServer::ct_uart_out = 0;

MavlinkDecoder MavlinkServer::decoder;
