#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "Ares" // Put you SSID and Password here
	#define WIFI_PWD "w1f1p455"
#endif

void onReceive(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort); // Declaration

#define LED 13

// UDP server
const uint16_t EchoPort = 1234;
UdpConnection udp(onReceive);

void timer_interrupt();
Timer uart_recv_timer;

IPAddress remIP;
bool remote_initialized = false;

void onReceive(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort) { 
    if (!remote_initialized) {
        remIP = remoteIP;
        uart_recv_timer.initializeMs(1, timer_interrupt).start();
        remote_initialized = true;  
    }
}

void onConnected()
{
	udp.listen(EchoPort);

	Serial.println("\r\n=== UDP SERVER STARTED ===");
	Serial.print(WifiStation.getIP().toString()); Serial.print(":"); Serial.println(EchoPort);
	Serial.println("=============================\r\n");

	//udp.connect(IPAddress(192, 168, 1, 180), 1234);
	//udp.sendString("Hello!");
}

static char* data[128];

void timer_interrupt() {
    static uint64_t counter=0;
    if (counter == 0) {
        memset(data, 0xFF, 128);
    }

    memcpy(data, (const char*)&counter, sizeof(counter));
    udp.sendTo(remIP, (uint16_t)1234, (const char*)data, 128); 
    digitalWrite(LED, HIGH);
    counter++;
/*
    memcpy(data, (const char*)&counter, sizeof(counter));
    udp.sendTo(remIP, (uint16_t)1234, (const char*)data, 128); 
    digitalWrite(LED, HIGH);
    counter++;
*/

}
void init()
{
	Serial.begin(921600); // 115200 by default
	Serial.systemDebugOutput(true);
    pinMode(LED, OUTPUT);
/*
	WifiAccessPoint.enable(true);
    AUTH_MODE mode = AUTH_OPEN;
    String password = "";

	WifiAccessPoint.config("Udp-test-ap", password, mode);
*/
    WifiStation.enable(true);
    WifiStation.config("Ares", "w1f1p455");

	System.onReady(onConnected);
}
