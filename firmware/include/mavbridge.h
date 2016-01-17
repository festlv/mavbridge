#pragma once
#include <stdint.h>

#define NET_LED_PIN     12
#define UART_LED_PIN    13


void mavbridge_init(void);

void mavbridge_get_status(uint32_t &uart_packets_received, uint32_t &net_packets_received);
