
#ifndef __SEND_CHECKTM_ACK_H__
#define __SEND_CHECKTM_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

class SendCheckTmAck {
public:
        SendCheckTmAck();
        ~SendCheckTmAck();
        int getBuf(u8* w_buf);

        void sendCheckTmAck(u8* data, SrAgent& agent, ClientSession* cs);
};

#endif
