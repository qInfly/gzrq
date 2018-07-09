
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
                        cerr << "=====>co_did:" << r.value(2) << endl;
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
                u8 w_buf[BUFSIZE] = {0};
                int len = 0;
                const string& ops_id = r.value(2);
                const string& sid = r.value(3);
                string text = r.value(4);
                string send_msg = "111," + ops_id + ",EXECUTING\n"
                        + "113," + sid + "," + to_string(EXECUTING) + "\n";
                agent.send(send_msg);

                if (text == "collect single data") { // 单表抄表
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
                                                srNotice("管理机 " + mg_did + " 不在线");
                                                // string msg = "112," + ops_id
                                                //         + ",管理机" + mg_did
                                                //         + "不在线\n"
                                                //         + "113," + me_sid + ","
                                                //         + to_string(FAILED) + "\n";
                                                // agent.send(msg);
                                                return;
                                        }
                                        cs->setOpsId(co_did, ops_id);
                                        cs->setCoDid(co_did);
//                                        cs->setCoSid(sid);
                                        printf("co_did:%x\n", co_did);
                                        printf("me_did:%x\n", me_did);
                                        printf("client_id:%x\n", client_id);
                                        printf("meter_type:%x\n", meter_type);

                                }
                        }
                        len = sa.getBuf(w_buf, co_did, me_did,
                                        client_id, meter_type);
                } else if (text == "get time"){ // 查询时间
                        SendGettmAck sga;
                        len = sga.getBuf(w_buf);
                } else if (text == "collect all data") { // 数据集中器抄收其管理的所有表的数据
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
                                // cs->setCoSid(sid);

                                SendAllAck saa(_http);
                                len = saa.getBuf(w_buf, co_did);
                        }
                } else if (text == "download dinfo") { // 管理中心下传全部表具信息到指定数据集中器
                        SendDinfoAck sda;
                        len = sda.getAllBuf(w_buf, cs);
                } else if (text == "check time") { // 校时命令--设置小区管理机时钟
                        SendCheckTmAck sct;
                        len = sct.getBuf(w_buf);
                }else if (text == "download mg dinfo") { // 小区的基础资料下传到小区管理机
                        SendMgDinfoAck smda;
                        len = smda.getBuf(w_buf, cs);
                }

                cs->sendToClient(w_buf, len);
        }
}
