#include "mavlink_decoder.h"
#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include <string.h>

MavlinkDecoder::MavlinkDecoder():
    state(WAITING_STX)
{
    memset(&msg, 0, sizeof(msg));
}

mavlink_parse_status_t MavlinkDecoder::parse_char(char c)
{
    static uint8_t received_payload_len = 0;
    switch (state) {
        case WAITING_STX:
            if (c == 0xFE) {
                memset(&msg, 0, sizeof(msg));
                add_to_buf(c);
                state = WAITING_LENGTH;
            }
        break;
        case WAITING_LENGTH:
            msg.payload_len = received_payload_len = (uint8_t) c; 
            add_to_buf(c);
            state = WAITING_SEQ;
        break;
        case WAITING_SEQ:
            msg.seq = (uint8_t) c;
            add_to_buf(c);
            state = WAITING_SYS;
        break;
        case WAITING_SYS:
            msg.sysid = (uint8_t) c;
            add_to_buf(c);
            state = WAITING_COMP;
        break;
        case WAITING_COMP:
            msg.compid = (uint8_t) c;
            state = WAITING_MSG;
            add_to_buf(c);
        break;
        case WAITING_MSG:
            msg.msgid = (uint8_t) c;
            state = WAITING_DATA;
            add_to_buf(c);
        break;
        case WAITING_DATA:
        case WAITING_CKA:
            //this case also handles CKA
            if (received_payload_len == 0)
            {
                //incoming byte is already CKA, skip that state
                //
                state = WAITING_CKB;
                add_to_buf(c);
            }
            else
            {
                add_to_buf(c);
                received_payload_len--;
            }
        break;
        case WAITING_CKB:
            state = WAITING_STX;
            add_to_buf(c);
            return MSG_DECODED;
    }

    if (msg.buf_len == MAVLINK_MAX_PACKET_LEN)
    {
        //buffer overflow, force transition to initial state
        state = WAITING_STX;
    }

    return MSG_WAIT;
}
