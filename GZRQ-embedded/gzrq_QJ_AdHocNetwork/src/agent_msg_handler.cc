
#include <unordered_map>
#include "srlogger.h"

#include "agent_msg_handler.h"

#include "send_single_ack.h"
#include "send_gettm_ack.h"
#include "send_all_ack.h"
#include "send_dinfo_ack.h"
#include "send_error_ack.h"
#include "send_checktm_ack.h"
#include "send_mgdinfo_ack.h"

extern unordered_map<string, ClientSession*> manager_sid_to_ptr;
//------------------------小区管理机-----------------
extern map<string, string> manager_sid_to_did;
extern map<string, string> manager_did_to_sid;

/*
  用于单抄信息，获取表具设备所有的信息
*/
static bool getMeterInfo(const SrAgent& agent, const string& sid, SrNetHttp* http,
                         u32& co_did, u32& me_did,
                         u8& client_id, u8& meter_type, string& mg_did)
{
        bool ret = false;
        SmartRest sr(http->response());
        SrRecord r;
        srDebug("get manager and collector info by device");

        http->clear();
        if (http->post("136," + sid) > 0) {
                sr.reset(http->response());
                r = sr.next();

                if (r.size() == 3 && r[0].second == "601") {
                        me_did = (u32)atoi(r.value(2).c_str());
                        srDebug("=====>meter_id: " + r.value(2));
                }
        }

        http->clear();
        if (http->post("135," + sid) > 0) {
                sr.reset(http->response());
                r = sr.next();
                while (r.size() && r.value(0) != "600") {
                        r = sr.next();
                }

                // 600,1,col_id,meter_id,client_id,meter_type
                if (r.size() == 7 && r[0].second == "600") {
                        ret = true;
                        cerr << "=====>co_did:" << r.value(2) << endl;
                        cerr << "=====>client_id:" << r.value(4) << endl;
                        cerr << "=====>meter_type:" << r.value(5) << endl;

                        co_did = (u32)atoi(r.value(2).c_str());
                        client_id = (u8)atoi(r.value(4).c_str());
                        meter_type = (u8)atoi(r.value(5).c_str());

                        mg_did = r.value(6);

                        cerr << "=====>mg_did" << mg_did << endl;

                        http->clear();
                }
        }
        return ret;
}

static bool getManagerId(const SrAgent& agent, const string& mg_did,
                         SrNetHttp* http, string& mg_sid)
{
        bool ret = false;

        http->clear();
        if (http->post("100," + mg_did) > 0) {
                SmartRest sr(http->response());
                SrRecord r = sr.next();

                if (r.size() == 3 && r.value(0) == "500") {
                        mg_sid = r.value(2);
                        ret = true;
                        http->clear();
                }
        }

        return ret;
}

static bool getCoDid(SrNetHttp* http, const string& sid,
                     u32& co_did, string& mg_did)
{
        bool ret = false;
        SmartRest sr(http->response());
        SrRecord r;
        srDebug("get manager and collector info by device");

        http->clear();
        if (http->post("136," + sid) > 0) {
                sr.reset(http->response());
                r = sr.next();

                if (r.size() == 3 && r[0].second == "601") {
                        co_did = (u32)atoi(r.value(2).c_str());
                        cerr << "=====>co_did:" << r.value(2) << endl;;
                }
        }

        http->clear();
        if (http->post("135," + sid) > 0) {
                sr.reset(http->response());
                r = sr.next();

                while (r.size() && r.value(0) != "602") {
                        r = sr.next();
                }

                if (r.size() == 3 && r[0].second == "602") {
                        ret = true;
                        mg_did = r.value(2);
                        cerr << "=====>602 mg_did: " << mg_did << endl;
                }
                http->clear();
        }

        return ret;
}

static bool getColChildMeters(SrNetHttp* http, const string& sid,
                              u32& co_did, string& mg_did,
                              vector<struct MeterInfo>& me_info)
{
        bool ret = false;
        SmartRest sr(http->response());
        SrRecord r;
        srDebug("get manager and collector info by device");

        http->clear();
        if (http->post("136," + sid) > 0) {
                sr.reset(http->response());
                r = sr.next();

                if (r.size() == 3 && r[0].second == "601") {
                        co_did = (u32)atoi(r.value(2).c_str());
                        cerr << "=====>co_did:" << r.value(2) << endl;;
                }
        }

        http->clear();
        if (http->post("135," + sid) > 0) {
                sr.reset(http->response());
                r = sr.next();

                while (r.size() && r.value(0) != "602") {
                        r = sr.next();
                }

                if (r.size() == 3 && r[0].second == "602") {
                        mg_did = r.value(2);
                        cerr << "=====>602 mg_did: " << mg_did << endl;
                }
        }

        http->clear();
        if (http->post("450," + sid) > 0) {
                // 700,1,225628,16,0
                sr.reset(http->response());
                r = sr.next();
                while (r.size() == 5 && r.value(0) == "700") {
                        string me_sid = r.value(2);
                        u8 me_type = (u8)atoi(r.value(3).c_str());
                        u8 me_client_id = (u8)atoi(r.value(4).c_str());

                        http->clear();
                        if (http->post("136," + me_sid) > 0) {
                                SmartRest sr_(http->response());
                                SrRecord r_ = sr_.next();

                                if (r_.size() == 3 && r_.value(0) == "601") {
                                        u32 me_did = (u32)atoi(r_.value(2).c_str());
                                        cerr << ">meter_id:" << r_.value(2) << endl;
                                        struct MeterInfo mi;
                                        memset(&mi, 0, sizeof(mi));
                                        mi.meter_id = me_did;
                                        mi.type = me_type;
                                        mi.client_id = me_client_id;
                                        me_info.push_back(mi);
                                }
                        }

                        r = sr.next();
                }
                ret = true;
                http->clear();
        }

        return ret;
}

AgentMsgHandler::AgentMsgHandler(SrAgent& agent, SrNetHttp* http)
        : _agent(agent), _http(http)
{

}

/* 析构函数 */
AgentMsgHandler::~AgentMsgHandler()
{

}

void AgentMsgHandler::operator()(SrRecord &r, SrAgent &agent)
{
        if (r.value(0) == "505") {
                ClientSession* cs = NULL;
                u8 w_buf[1024] = {0};
                int len = 0;
                const string& ops_id = r.value(2);
                const string& sid = r.value(3);
                string text = r.value(4);
                string send_msg = "111," + ops_id + ",EXECUTING\n"
                        + "113," + sid + "," + to_string(EXECUTING) + "\n";
                agent.send(send_msg);

                // 自组网设备只支持：单抄、群抄和下载表具信息到集中器
                // 505,1,75022522,75020600,collect single data
                if (text == "collect single data") { // 单表抄表
                        cerr << "collect single data func" << endl;
                        SendSingleAck sa(_http);

                        u32 co_did = 0x00;
                        u32 me_did = 0x00;
                        u8 client_id = 0x00;
                        u8 meter_type = 0x00;

                        string mg_did = "";

                        if (getMeterInfo(_agent, sid, _http, co_did,
                                         me_did, client_id, meter_type, mg_did)) {
                                string mg_sid = "";
                                if (getManagerId(agent, mg_did, _http, mg_sid)) {
                                        cs = manager_sid_to_ptr[mg_sid];
                                        if (!cs) {
                                                srInfo("send single data, cs NULL");
                                                string me_sid = getSid(
                                                        agent, to_string(me_did),
                                                        _http
                                                        );
                                                string msg = "112," + ops_id
                                                        + ",管理机" + mg_did
                                                        + "不在线\n"
                                                        + "113," + me_sid + ","
                                                        + to_string(FAILED) + "\n";
                                                agent.send(msg);
                                                return;
                                        }

                                        cs->setOpsId(co_did, ops_id);
                                        cs->setCoDid(co_did);
                                        cs->setCoSid(sid);
                                        printf("co_did:%x\n", co_did);
                                        printf("me_did:%x\n", me_did);
                                        printf("client_id:%x\n", client_id);
                                        printf("meter_type:%x\n", meter_type);

                                        len = sa.getBuf(w_buf, co_did, me_did,
                                                        client_id, meter_type);
                                }
                        }
                } else if (text == "collect all data") { // 群抄
                        u32 co_did = 0x00;
                        string mg_did = "";
                        if (getCoDid(_http, sid, co_did, mg_did)) {
                                const string& mg_sid = manager_did_to_sid[mg_did];
                                cs = manager_sid_to_ptr[mg_sid];
                                if (!cs) {
                                        srInfo("send all data, cs is NULL");
                                        string msg = "112," + ops_id + ",管理机"
                                                + mg_did + "不在线\n"
                                                + "113," + sid + ","
                                                + to_string(FAILED) + "\n";
                                        agent.send(msg);
                                        return;
                                }
                                cs->setOpsId(co_did, ops_id);
                                cs->setAllFrameNum(co_did, 0);
                                cs->setCoDid(co_did);
                                cs->setCoSid(sid);

                                SendAllAck saa(_http);
                                len = saa.getBuf(w_buf, co_did);
                        }
                } else if (text == "download dinfo") { // 下载表具信息到集中器
                        u32 co_did = 0x00;
                        string mg_did = "";
                        vector<struct MeterInfo> me_info;
                        if (getColChildMeters(_http, sid, co_did,
                                              mg_did, me_info)) {
                                const string& mg_sid = manager_did_to_sid[mg_did];

                                cs = manager_sid_to_ptr[mg_sid];
                                if (!cs) {
                                        srInfo("download dinfo, cs is NULL");
                                        string msg = "112," + ops_id + ",管理机"
                                                + mg_did + "不在线\n"
                                                + "113," + sid + ","
                                                + to_string(FAILED) + "\n";
                                        agent.send(msg);
                                        return;
                                }
                                cs->setOpsId(co_did, ops_id);
                                cs->setCoDid(co_did);
                                cs->setCoSid(sid);
                                SendDinfoAck sda(_http);

                                u8 times = me_info.size() / 10;
                                if (me_info.size() % 10 > 0) times++;

                                for (u8 t = 1; t <= times; t++) {
                                        len = sda.getAllBuf(w_buf, cs,
                                                            me_info, t, co_did);
                                        cs->sendToClient(w_buf, len);
                                }
                                cs = NULL;
                        }
                }

                if (cs) cs->sendToClient(w_buf, len);
        } else {
                srNotice("not 505 operation");
        }
}
