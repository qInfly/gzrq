
#ifndef __SEND_DINFO_ACK_H__
#define __SEND_DINFO_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

class SendDinfoAck {
public:
        SendDinfoAck();
        ~SendDinfoAck();
        int getAllBuf(u8* w_buf, ClientSession* cs);
        int getStopBuf(u8* w_buf, ClientSession* cs);

        void sendDinfoAck(u8* data, SrAgent& agent, ClientSession* cs);
};

#endif
