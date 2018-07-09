
#ifndef __SEND_ALL_ACK_H__
#define __SEND_ALL_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

class SendAllAck {
public:
        SendAllAck(SrNetHttp* http);
        ~SendAllAck();
        int getBuf(u8* w_buf, const u32& co_did);

        int getPollBuf(u8* w_buf, const u32& co_did);
        int getAllBuf(u8* w_buf, ClientSession* cs, const u32& co_did);

        void sendPollAck(u8* data, SrAgent& agent, ClientSession* cs);
        void sendAllAck(u8* data, SrAgent& agent,
                        ClientSession* cs, const u32& origin_ptr_len);

        string getValveStatus(const u8& type);

        void sendMeData(u8* data, SrAgent& agent,
                        ClientSession* cs, const u32& origin_ptr_len);

private:
        SrNetHttp* _http;
};

#endif
