#include "user_config.h"
#include <SmingCore/SmingCore.h>

Timer uptime_timer;

static uint64_t uptime_counter = 0;

static void uptime_callback() {
    uptime_counter++;
}

void uptime_init() {
    uptime_timer.initializeMs(1000, uptime_callback).start();
}

uint64_t uptime_seconds() {
    return uptime_counter;
}

String uptime_string() {
    char buf[64];

    uint32_t days = uptime_counter  / (3600 * 24);
    uint32_t hours = (uptime_counter - days * 3600 * 24) / 3600;
    uint32_t minutes = ((uptime_counter - days * 3600 * 24) - hours * 3600) / 60;
    
    uint32_t seconds = uptime_counter - days * 3600 * 24 - hours * 3600 - minutes * 60;
    
    if (days > 0)
        sprintf(buf, "%dd %dh %dm %ds", days, hours, minutes, seconds);
    else if (hours > 0) 
        sprintf(buf, "%dh %dm %ds",hours, minutes, seconds);
    else if (minutes > 0)
        sprintf(buf, "%dm %ds", minutes, seconds);
    else 
        sprintf(buf, "%ds", seconds);

    return String(buf);
}
