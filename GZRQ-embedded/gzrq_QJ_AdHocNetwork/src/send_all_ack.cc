
#include "srlogger.h"
#include "send_all_ack.h"

extern map<string, string> meter_did_to_sid;

u32 _co_did;

SendAllAck::SendAllAck(SrNetHttp* http)
        : _http(http)
{

}

SendAllAck::~SendAllAck()
{

}

int SendAllAck::getBuf(u8* w_buf, const u32& co_did)
{
//        const u32& co_did = cs->coDid();
        int i = 0;
        int j = 0;

        w_buf[i++] = 0x43;
        w_buf[i++] = 0x4D;
        w_buf[i++] = 0x44;

        w_buf[i++] = 0x20;

        string col_did = to_string(co_did);
        for (u32 m = 0; m < col_did.size(); m++) {
                uint8_t t = col_did[m];
                w_buf[i++] = charToHex(t);
        }

        w_buf[i++] = 0x20;
        w_buf[i++] = 0x30;      // 应答字节数(固定0x00,转换到ascii为0x30)
        w_buf[i++] = 0x20;

        uint8_t p_buf[1024] = {0};
        p_buf[j++] = 0x25;
        p_buf[j++] = 0x0A;
        p_buf[j++] = 0x55;

        p_buf[j++] = 0x30;

        p_buf[j++] = co_did & 0xFF;
        p_buf[j++] = (co_did >> 8) & 0xFF;
        p_buf[j++] = (co_did >> 16) & 0xFF;
        p_buf[j++] = (co_did >> 24) & 0xFF;

        p_buf[j++] = addCheck(p_buf, j);
        p_buf[j++] = 0x0d;

        for (int k = 0; k < j; k++) {
                string s = hexToAsc(p_buf[k]);
                w_buf[i++] = s[0];
                w_buf[i++] = s[1];
                w_buf[i++] = 0x20;
        }

        w_buf[i-1] = 0x0D;
        w_buf[i++] = 0x0A;

        return i;
}

/*
  轮询集中器群命令状态
*/
int SendAllAck::getPollBuf(u8* w_buf, const u32& co_did)
{
//        const u32& co_did = cs->coDid();
        int i = 0;
        int j = 0;

        w_buf[i++] = 0x43;
        w_buf[i++] = 0x4D;
        w_buf[i++] = 0x44;

        w_buf[i++] = 0x20;

        string col_did = to_string(co_did);
        for (u32 m = 0; m < col_did.size(); m++) {
                uint8_t t = col_did[m];
                w_buf[i++] = charToHex(t);
        }

        w_buf[i++] = 0x20;
        w_buf[i++] = 0x30;      // 应答字节数(固定0x00,转换到ascii为0x30)
        w_buf[i++] = 0x20;

        uint8_t p_buf[1024] = {0};
        p_buf[j++] = 0x25;
        p_buf[j++] = 0x0A;
        p_buf[j++] = 0x55;

        p_buf[j++] = 0x36;

        p_buf[j++] = co_did & 0xFF;
        p_buf[j++] = (co_did >> 8) & 0xFF;
        p_buf[j++] = (co_did >> 16) & 0xFF;
        p_buf[j++] = (co_did >> 24) & 0xFF;

        p_buf[j++] = addCheck(p_buf, j);
        p_buf[j++] = 0x0d;

        for (int k = 0; k < j; k++) {
                string s = hexToAsc(p_buf[k]);
                w_buf[i++] = s[0];
                w_buf[i++] = s[1];
                w_buf[i++] = 0x20;
        }

        w_buf[i-1] = 0x0D;
        w_buf[i++] = 0x0A;

        return i;
}

/*
  提取集中器群抄数据
*/
int SendAllAck::getAllBuf(u8* w_buf, ClientSession* cs, const u32& co_did)
{
//        const u32& co_did = cs->coDid();
        int i = 0;
        int j = 0;

        w_buf[i++] = 0x43;
        w_buf[i++] = 0x4D;
        w_buf[i++] = 0x44;

        w_buf[i++] = 0x20;

        string col_did = to_string(co_did);
        for (u32 m = 0; m < col_did.size(); m++) {
                uint8_t t = col_did[m];
                w_buf[i++] = charToHex(t);
        }

        w_buf[i++] = 0x20;
        w_buf[i++] = 0x30;      // 应答字节数(固定0x00,转换到ascii为0x30)
        w_buf[i++] = 0x20;

        uint8_t p_buf[1024] = {0};
        p_buf[j++] = 0x25;
        p_buf[j++] = 0x0B;
        p_buf[j++] = 0x55;

        p_buf[j++] = 0x31;

        p_buf[j++] = co_did & 0xFF;
        p_buf[j++] = (co_did >> 8) & 0xFF;
        p_buf[j++] = (co_did >> 16) & 0xFF;
        p_buf[j++] = (co_did >> 24) & 0xFF;

        p_buf[j++] = cs->allFrameNum(co_did);

        p_buf[j++] = addCheck(p_buf, j);
        p_buf[j++] = 0x0d;

        for (int k = 0; k < j; k++) {
                string s = hexToAsc(p_buf[k]);
                w_buf[i++] = s[0];
                w_buf[i++] = s[1];
                w_buf[i++] = 0x20;
        }

        w_buf[i-1] = 0x0D;
        w_buf[i++] = 0x0A;

        return i;
}

void SendAllAck::sendPollAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        u8 w_buf[1024] = {0};
        int len = 0;
        bool is_send = true;

        const u8& corr_cmd = data[3];
        const u8& frame_cmd = data[4];
        const u32& co_did =
                data[5] + (data[6] << 8) + (data[7] << 16) + (data[8] << 24);

        switch (frame_cmd) {
        case 0x30: {
                if (corr_cmd == 0x4F)
                        len = getPollBuf(w_buf, co_did);
        }
                break;
        case 0x36: {
                if (corr_cmd == 0x4F) {
                        len = getAllBuf(w_buf, cs, co_did);
                        cs->setFlag();
                        // string co_sid = getSid(agent, to_string(co_did), _http);
                        // string msg = "111," + cs->getOpsId(co_did)
                        //         + ",SUCCESSFUL\n";
                        // msg += "113," + co_sid + "," + to_string(SUCCESSFUL) + "\n";

                        // agent.send(msg);
                } else {
                        srInfo(to_string(co_did) + " 集中器忙");
                        // 不给用户显示"集中器忙"状态，只显示执行中
                        //string msg = "112," + cs->getOptId() + ",集中器忙";
                        //agent.send(msg);
                        static const timespec ts = {15,0};
                        nanosleep(&ts, NULL);
                        len = getPollBuf(w_buf, co_did);
                }
        }
                break;
        }

        if (is_send) cs->sendToClient(w_buf, len);
}

void SendAllAck::sendAllAck(u8* data, SrAgent& agent,
                            ClientSession* cs, const u32& origin_ptr_len)
{
        u8 i = 0;
        u8 len = data[1];
        const u8& corr_cmd = data[3];
        cerr << "update data: ";
        for (i = 0; i < len*19; i++) {
                printf("%02x ", data[i]);
        }
        printf("\n");

//        u32 co_did = 0x00;
        u32 co_did = data[4] + (data[5] << 8) + (data[6] << 16) + (data[7] << 24);
//        if (corr_cmd == 0x31) {
        printf("%x\n", co_did);
        switch (corr_cmd) {
        case 0x31: {              // 正常
                srInfo("=============>正常");
                sendMeData(data, agent, cs, origin_ptr_len);
                u8 w_buf[1024] = {0};
                cs->setFlag();
                len = getAllBuf(w_buf, cs, co_did);
                cs->sendToClient(w_buf, len);
        }
                break;
        case 0x6F: {
                srInfo("=============>抄完了");
                cs->setAllFrameNum(co_did, 0);
                string co_sid = getSid(agent, to_string(co_did), _http);
                string msg = "111," + cs->getOpsId(co_did) + ",SUCCESSFUL\n";
                msg += "113," + co_sid + "," + to_string(SUCCESSFUL) + "\n";

                agent.send(msg);
                sendMeData(data, agent, cs, origin_ptr_len);
        }
                break;
        }
}

string SendAllAck::getValveStatus(const u8& type)
{
        string valve_status = "";
        switch (type) {
        case 0x22:
                valve_status = "字轮及通信正常"; // 字轮及通信正常
                break;
        case 0x23:              // 表具无应答
                valve_status = "表具无应答";
                break;
        case 0x24:              // 字轮异常
                valve_status = "字轮异常";
                break;
        case 0x25:              // 集中器无应答
                valve_status = "集中器无应答";
                break;
        case 0x26:              // 未启用
                valve_status = "未启用";
                break;
        case 0x27:              // 倒转
                valve_status = "倒转";
                break;
        }

        return valve_status;
}

void SendAllAck::sendMeData(u8* data, SrAgent& agent,
                            ClientSession* cs, const u32& origin_ptr_len)
{
        const u8& len = data[1];
        u8* p = data + 9;
//        int total_len = len*19;
        u32 total_len = origin_ptr_len - 10;
        u32 last_len = total_len;
//        int dealed_len = 0;
        string failure_reason = "";
        string msg;



        u32 co_did = data[5] + (data[6] << 8)
                + (data[7] << 16) + (data[8] << 24);
        printf("=====>ret co_did: %x\n", co_did);
//        while (dealed_len < total_len) {
        while (last_len >= 19) {
                u32 meter_id = p[0] + (p[1] << 8)
                        + (p[2] << 16) + (p[3] << 24);
                msg = "";

                string me_sid, me_spid, me_badge_nbr;
                string me_thrd_id = "QJ";
                string me_resource = "rs";
                string me_did = to_string(meter_id);
                cerr << "===>meter did: " << meter_id << endl;
                cerr << "===>me_did: " << me_did << endl;

                Bool ret = getSid(agent, me_did, _http, me_sid
                                  , me_spid, me_badge_nbr, co_did);

                if (!me_sid.empty()) {
                        msg += "105," + me_sid + ",c8y_ValveStatus,"
                                + getValveStatus(p[6]) + "\n";

                        if (!failure_reason.empty()) {
                                msg = "104," + me_sid + ",c8y_" + failure_reason
                                        + ",MAJOR," + failure_reason + "\n";
                        }

                        const u32& data = p[7] + (p[8] << 8)
                                + (p[9] << 16) + (p[10] << 24);
                        msg += "200," + me_sid + ","
                                + "c8y_MeterValue,c8y_MeterValue,Value,"
                                + to_string(data) + ",m^3,"
                                + me_spid + "," + me_badge_nbr + ","
                                + me_thrd_id + "," + me_resource + "\n";

                        agent.send(msg);
                } else {
                        srNotice("device " + me_did
                                 + " is not exist on platform!");
                }

                last_len -= 19;
//                dealed_len += 19;
                p += 19;
        }
}
