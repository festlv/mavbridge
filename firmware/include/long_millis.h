#pragma once

#include <user_config.h>
#include <SmingCore/SmingCore.h>


static uint32_t millis_offset = 0;
static uint32_t micros_extra = 0;
static uint32_t last_us = 0;


uint32_t long_millis() {
   uint32_t us = micros();
   if (us < last_us) {
      millis_offset += 4294967;
      micros_extra += 296;
   }
   last_us = us;
   return millis_offset + us / 1000 + ((us % 1000) + micros_extra) / 1000;
}
