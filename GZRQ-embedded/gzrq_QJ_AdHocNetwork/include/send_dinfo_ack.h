
#ifndef __SEND_DINFO_ACK_H__
#define __SEND_DINFO_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

class SendDinfoAck {
public:
        SendDinfoAck(SrNetHttp* http);
        ~SendDinfoAck();
        int getAllBuf(u8* w_buf, ClientSession* cs,
                      vector<struct MeterInfo>& me_info, const u8& t,
                      const u32& co_did);
        int getStopBuf(u8* w_buf, ClientSession* cs, const u32& co_did);

        void sendDinfoAck(u8* data, SrAgent& agent, ClientSession* cs);

private:
        SrNetHttp* _http;
};

#endif
