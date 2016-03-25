#pragma once
#include <stdint.h>

/**
 *
 * Dumb MAVLink v1.0 decoder. Decodes incoming messages but does not verify
 * CRC.
 *
 * Extracts core message fields (msgid, length, sequence number, etc.) but does
 * not attempt to parse fields.
 *
 */

#ifndef MAVLINK_MAX_PACKET_LEN
#define MAVLINK_MAX_PACKET_LEN 263
#endif


typedef enum
{
    WAITING_STX,
    WAITING_LENGTH,
    WAITING_SEQ,
    WAITING_SYS,
    WAITING_COMP,
    WAITING_MSG,
    WAITING_DATA,
    WAITING_CKA,
    WAITING_CKB

} mavlink_decoder_state_t;

typedef enum {
    MSG_DECODED,
    MSG_WAIT
} mavlink_parse_status_t;

typedef struct
{
    uint8_t     seq;
    uint8_t     sysid;
    uint8_t     compid;
    uint8_t     msgid; 

    uint8_t     payload_len;
    uint16_t    buf_len;
    uint8_t     buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t    checksum;

} mavlink_message_t;

class MavlinkDecoder
{
    public:
        MavlinkDecoder();
        
        mavlink_parse_status_t parse_char(char c);

        mavlink_message_t *get_message(void)
        {
            return &msg;
        }
    private:
        mavlink_decoder_state_t state;
        mavlink_message_t msg;

        inline void add_to_buf(char c)
        {
            msg.buf[msg.buf_len] = c;
            msg.buf_len++;
        }
};
