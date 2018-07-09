
#ifndef __SEND_GETTM_ACK_H__
#define __SEND_GETTM_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

using namespace std;

class SendGettmAck : public Interface {
public:
        SendGettmAck();
        virtual ~SendGettmAck();
        virtual int getBuf(u8* w_buf);
        virtual int getBuf(u8* w_buf, const int& did) { return 0; }

        void sendGettmAck(u8* data, SrAgent& agent, ClientSession* cs);
};

#endif
