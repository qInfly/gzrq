

#ifndef __SEND_SINGLE_ACK_H__
#define __SEND_SINGLE_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

using namespace std;

class SendSingleAck {
public:
        SendSingleAck(SrNetHttp* http);
        ~SendSingleAck();
        int getBuf(u8* w_buf, const u32& co_did, const u32& me_did,
                   const u8& client_id, const u8& meter_type);

        void sendSingleAck(u8* data, SrAgent& agent, ClientSession* cs);
        string getValveStatus(const u8& valve_status);

private:
        SrNetHttp* _http;
};

#endif
