


#include "srlogger.h"

#include "send_single_ack.h"
#include "util.h"

extern map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射
extern map<string, string> meter_did_to_sid;

SendSingleAck::SendSingleAck(SrNetHttp* http)
        : _http(http)
{

}

SendSingleAck::~SendSingleAck()
{

}

/*
  单表抄表
*/
int SendSingleAck::getBuf(u8* w_buf, const u32& co_did, const u32& me_did,
                          const u8& client_id, const u8& meter_type)
{
        int i = 0;
        int j = 0;

        w_buf[i++] = 0x43;
        w_buf[i++] = 0x4D;
        w_buf[i++] = 0x44;

        w_buf[i++] = 0x20;

        string col_did = to_string(co_did);
        for (int m = 0; m < col_did.size(); m++) {
                uint8_t t = col_did[m];
                w_buf[i++] = charToHex(t);
        }

        w_buf[i++] = 0x20;
        w_buf[i++] = 0x30;      // 应答字节数(固定0x00,转换到ascii为0x30)
        w_buf[i++] = 0x20;

        uint8_t p_buf[1024] = {0};
        p_buf[j++] = 0x25;
        p_buf[j++] = 0x10;
        p_buf[j++] = 0x55;
        p_buf[j++] = 0x21;

        p_buf[j++] = co_did & 0xFF;
        p_buf[j++] = (co_did >> 8) & 0xFF;
        p_buf[j++] = (co_did >> 16) & 0xFF;
        p_buf[j++] = (co_did >> 24) & 0xFF;

        p_buf[j++] = me_did & 0xFF;
        p_buf[j++] = (me_did >> 8) & 0xFF;
        p_buf[j++] = (me_did >> 16) & 0xFF;
        p_buf[j++] = (me_did >> 24) & 0xFF;

        p_buf[j++] = client_id;
        p_buf[j++] = meter_type;
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

int SendSingleAck::getPickUpBuf(uint8_t* w_buf, ClientSession* cs)
{
        int i = 0;
        int j = 0;

        w_buf[i++] = 0x43;
        w_buf[i++] = 0x4D;
        w_buf[i++] = 0x44;

        w_buf[i++] = 0x20;

        string col_did = to_string(cs->coDid());
        for (int m = 0; m < col_did.size(); m++) {
                uint8_t t = col_did[m];
                w_buf[i++] = charToHex(t);
        }

        w_buf[i++] = 0x20;
        w_buf[i++] = 0x30;      // 应答字节数(固定0x00,转换到ascii为0x30)
        w_buf[i++] = 0x20;

        u8 p_buf[128] = {0};
        // PICK-UP
        p_buf[j++] = 0x50;
        p_buf[j++] = 0x49;
        p_buf[j++] = 0x43;
        p_buf[j++] = 0x4B;
        p_buf[j++] = 0x2D;
        p_buf[j++] = 0x55;
        p_buf[j++] = 0x50;
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

void SendSingleAck::sendSingleAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        struct SingleAck sa;
        memset(&sa, 0, sizeof(sa));
        memcpy(&sa, data, sizeof(sa));
        string failure_reason = "";
        string& sid = cs->sid();
        string msg = "";

        string me_sid, me_spid, me_badge_nbr;
        string me_thrd_id = "QJ";
        string me_resource = "rs";
        string me_did = to_string(sa.number);

        u32 co_did = 0x00;

        bool ret = getSid(agent, me_did, _http, me_sid,
                          me_spid, me_badge_nbr, co_did);

        if (sa.status == 0x22) { // 通讯正常,正常解析后面的数据
                u32 value = rotationWhl(sa.rd_num, me_sid); // 处理字轮翻转
                msg += "111," + cs->getOpsId(co_did) + ",SUCCESSFUL\n";
                msg += "113," + me_sid + "," + to_string(SUCCESSFUL) + "\n";
                msg += "200," + me_sid + ","
                        + "c8y_MeterValue,c8y_MeterValue,Value,"
                        + to_string(value) + ",m^3,"
                        + me_spid + "," + me_badge_nbr + ","
                        + me_thrd_id + "," + me_resource + "\n";
                msg += "204," + me_sid + "," + to_string(value) + "\n";
                msg += "105," + me_sid + ",c8y_ValveStatus,"
                        + getValveStatus(sa.valve_status);
        } else {                // 异常状况，不解析后面的数据，只发送报警
                switch (sa.status) {
                case 0x23:              // 表具无应答
                        failure_reason = "表具无应答";
                        break;
                case 0x24:              // 字轮异常
                        failure_reason = "字轮异常";
                        break;
                case 0x25:              // 集中器无应答
                        failure_reason = "集中器无应答";
                        break;
                case 0x26:              // 未启用
                        failure_reason = "未启用";
                        break;
                case 0x27:              // 倒转
                        failure_reason = "倒转";
                        break;
                }

                msg = "112," + cs->getOpsId(co_did) + "," + failure_reason + "\n";
                + "113," + me_sid + "," + to_string(FAILED) + "\n";
        }
        agent.send(msg);
}

string SendSingleAck::getValveStatus(const u8& valve_status)
{
        string ret_str = "";
        switch (valve_status) {
        case 0xf0:
                ret_str = "阀门开";
                break;
        case 0xf1:
                ret_str = "阀门关";
                break;
        case 0xf2:
                ret_str = "阀门异常";
                break;
        case 0xf3:
                ret_str = "阀门无状态";
                break;
        }

        return ret_str;
}
