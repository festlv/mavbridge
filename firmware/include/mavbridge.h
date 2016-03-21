#pragma once
#include <stdint.h>
#include <queue>
#include <Wiring/WVector.h>
#include "mavlink/ardupilotmega/mavlink.h"

#define NET_LED_PIN     12
#define UART_LED_PIN    13

#define MAX_CLIENTS 5
#define MAX_INTERFACES 2



void mavbridge_init(void);

void mavbridge_get_status(uint32_t &uart_packets_received, uint32_t &net_packets_received);

typedef enum {
    PROTO_UDP,
    PROTO_TCP
} mavlink_proto_type_t;

typedef struct {
    uint8_t data[MAVLINK_MAX_PACKET_LEN];
    uint16_t length;
} mavlink_packet_t;

class MavlinkServer {

    public:
        static void initialize(uint16_t port, mavlink_proto_type_t protocol);

        static void transmit_packet(mavlink_message_t msg);
        static bool pop_received_packet(mavlink_packet_t* packet);

        static void udp_receive_callback(UdpConnection &conn, char* data, int size,
                IPAddress remoteIP, uint16_t remotePort);
        static bool tcp_client_receive(TcpClient &client, char* data, int size);
        static void tcp_client_complete(TcpClient& client, bool successful);
        static void tcp_client_connected(TcpClient* client);

        static void interface_update_interrupt();

        static MavlinkServer& get_instance() {
            static MavlinkServer inst;
            return inst;
        }

        /**
         * Various performance/statistics counters.
         *
         */
        static uint32_t ct_tcp_in;
        static uint32_t ct_tcp_out;
        static uint32_t ct_tcp_dropped;

        static uint32_t ct_udp_in;
        static uint32_t ct_udp_out;
        static uint32_t ct_udp_dropped;

        static uint32_t ct_uart_in;
        static uint32_t ct_uart_out;

    private:
        MavlinkServer() 
        {
        }

        static UdpConnection* udp_conn;
        static TcpServer*     tcp_server;

        static Vector<IPAddress> udp_clients;
        static Vector<IPAddress> interfaces;
        
        static Vector<TcpClient*> tcp_clients;

        static Vector<mavlink_packet_t> incoming_message_queue;

        static bool client_exists(IPAddress ip, mavlink_proto_type_t proto);
        static bool interface_exists(IPAddress ip);

        static Timer interface_update_timer;
};
