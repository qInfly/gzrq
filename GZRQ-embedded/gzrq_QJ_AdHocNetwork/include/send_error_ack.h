
#ifndef __SEND_ERROR_ACK_H__
#define __SEND_ERROR_ACK_H__

#include "sragent.h"
#include "client_session.h"
#include "interface.h"

class SendErrorAck {
public:
        SendErrorAck();
        ~SendErrorAck();

        void sendErrorAck(u8* data, SrAgent& agent, ClientSession* cs);

        string getFailureReason(const u8* p);
};

#endif
