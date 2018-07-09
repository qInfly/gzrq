
#ifndef __AGENT_MSG_HANDLER_H__
#define __AGENT_MSG_HANDLER_H__

#include "sragent.h"
#include "srnethttp.h"

class AgentMsgHandler : public SrMsgHandler {
public:
        AgentMsgHandler(SrAgent& agent, SrNetHttp* http);

        /* 析构函数 */
        ~AgentMsgHandler();

public:
        virtual void operator()(SrRecord &r, SrAgent &agent);

private:
        SrAgent& _agent;
        SrNetHttp* _http;
};


#endif  // __AGENT_MSG_HANDLER_H__
