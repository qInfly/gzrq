
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <future>
#include <unistd.h>
#include <sys/types.h>
#include "srutils.h"
#include "integrate.h"
#include "srreporter.h"
#include "srdevicepush.h"
#include <srnethttp.h>
#include "util.h"
#include "cpuinfo.h"
#include "client_session.h"
#include "io_service_thread.h"
#include "agent_msg_handler.h"

#include <boost/thread/thread_pool.hpp>
//ing namespace boost::threadpool;

//#define USE_MQTT

#ifdef CATCH_SIGNAL

void sig_handler(int sig)
{
        if (sig == SIGINT) {
                exit(0);
        }
}

#endif

int main(void)
{
#ifdef CATCH_SIGNAL
        signal(SIGINT, sig_handler);
#endif

        string device_id, server, credential_path,
                log_dir, template_dir, port,interval;
        readConfig("./conf/agentdtu.conf", "device_id", device_id);
        readConfig("./conf/agentdtu.conf", "server", server);
        readConfig("./conf/agentdtu.conf", "credential_path", credential_path);
        readConfig("./conf/agentdtu.conf", "log_dir", log_dir);
        readConfig("./conf/agentdtu.conf", "template_dir", template_dir);
        readConfig("./conf/agentdtu.conf", "port", port);
        readConfig("./conf/agentdtu.conf", "interval", interval);

        string srversion, srtemplate;
        if (0 != readSrTemplate(template_dir, srversion, srtemplate)) {
                srError("read template failed");
                return 0;
        }

        Integrate igt;
        SrAgent agent(server, device_id, &igt);
        srLogSetLevel(SRLOG_DEBUG);
        srLogSetDest(log_dir);

        if (0 != agent.bootstrap(credential_path)) {
                srError("create agent credential_path failed");
                return 0;
        }

        if (0 != agent.integrate(srversion, srtemplate)) {
                srError("parent device integrate failed");
                return 0;
        }

        agent.send("400," + agent.ID() + "," + interval);

        CpuInfo ci;
        SrTimer cit(60*1000, &ci);
        agent.addTimer(cit);
        cit.start();

        agent.send("110," + agent.ID() +
                   ",\"\"\"c8y_Command\"\",\"\"c8y_DTUCommand\"\"\"");

#ifdef USE_MQTT
        SrReporter reporter(string(server) + ":1883", device_id, agent.XID(),
                            agent.tenant() + '/' + agent.username(),
                            agent.password(), agent.egress, agent.ingress);

        reporter.mqttSetOpt(SR_MQTTOPT_KEEPALIVE, 180);
        if (reporter.start()) {
                return 0;
        }
#else
        SrReporter reporter(server, agent.XID(),
                            agent.auth(), agent.egress, agent.ingress);
        SrDevicePush push(server, agent.XID(),
                          agent.auth(), agent.ID(), agent.ingress);

        if (reporter.start() || push.start())
        {
                return 0;
        }
#endif

//        initType();             // 后续修改

        SrNetHttp http(agent.server() + "/s", srversion, agent.auth());
        IOServiceThread thrd(agent, atoi(port.c_str()), &http);
        thrd.Start();

        AgentMsgHandler amh(agent, &http);
        agent.addMsgHandler(505, &amh);
        agent.addMsgHandler(600, &amh);

        agent.loop();

        return 0;
}
