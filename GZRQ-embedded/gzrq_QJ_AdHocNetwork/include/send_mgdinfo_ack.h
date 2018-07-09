
#ifndef __SEND_MGDINFO_ACK_H__
#define __SEND_MGDINFO_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

class SendMgDinfoAck {
public:
        SendMgDinfoAck();
        ~SendMgDinfoAck();
        int getBuf(u8* w_buf, ClientSession* cs);

        int getStopBuf(u8* w_buf);

        void sendMgDinfoAck(const u8* data, SrAgent& agent, ClientSession* cs);
        void sendMgStopAck(const u8* data, SrAgent& agent, ClientSession* cs);
};

#endif
