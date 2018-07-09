
#include <stdlib.h>
#include "srlogger.h"
#include "send_dinfo_ack.h"

SendDinfoAck::SendDinfoAck(SrNetHttp* http)
        : _http(http)
{

}

SendDinfoAck::~SendDinfoAck()
{

}

/*
  管理中心下传全部表具信息到指定数据集中器
 */
int SendDinfoAck::getAllBuf(u8* w_buf, ClientSession* cs,
                            vector<struct MeterInfo>& me_info, const u8& t,
                            const u32& co_did)
{
        int i = 0;
        int j = 0;
//        const u32& co_did = cs->coDid();

        u8 quantity = 0;
        const u32& len = (u32)t*10 - me_info.size();
        if (len <= 0) quantity = 10;
        else quantity = me_info.size();

        printf("quantity: %x\n", quantity);

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
        p_buf[j++] = quantity;      // 一次性下载10个表具
        p_buf[j++] = 0x55;

        p_buf[j++] = 0x33;

        p_buf[j++] = co_did & 0xFF;
        p_buf[j++] = (co_did >> 8) & 0xFF;
        p_buf[j++] = (co_did >> 16) & 0xFF;
        p_buf[j++] = (co_did >> 24) & 0xFF;

        p_buf[j++] = cs->dinfoFrameNum();

        /* 表具信息 */
        // 表号后续修改为根据集中器地址来找到
        cerr << "quantity: " << quantity << endl;
        cerr << "t: " << t << endl;
        for (u8 times = 0; times < quantity; times++) {
                struct MeterInfo& mi = me_info[(t-1)*10+times];
                cerr << "/*******************************************/" << endl;
                p_buf[j++] = mi.meter_id & 0xFF;
                p_buf[j++] = (mi.meter_id >> 8) & 0xFF;
                p_buf[j++] = (mi.meter_id >> 16) & 0xFF;
                p_buf[j++] = (mi.meter_id >> 24) & 0xFF;
                p_buf[j++] = mi.type;
                p_buf[j++] = mi.client_id;
                cerr << "meter id: " << mi.meter_id << endl;
                printf("meter type: %x\n", mi.type);
                printf("meter client id: %x\n", mi.client_id);
                cerr << "/*******************************************/" << endl;
        }

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
  管理中心向数据集中器发送数传结束命令
 */
int SendDinfoAck::getStopBuf(u8* w_buf, ClientSession* cs, const u32& co_did)
{
        int i = 0;
        int j = 0;
//        const u32& co_did = cs->coDid();

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
        p_buf[j++] = 0x0A;
        p_buf[j++] = 0x55;

        p_buf[j++] = 0x6F;

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

void SendDinfoAck::sendDinfoAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        u8 w_buf[1024] = {0};
        int len = 0;

        const u8& corr_cmd = data[3];
        const u8& frame_cmd = data[4];
        u32 co_did = data[5] + (data[6] << 8)
                + (data[7] << 16) + (data[8] << 24);

        switch (frame_cmd) {
        case 0x33: {
                if (corr_cmd == 0x4F) {
                        len = getStopBuf(w_buf, cs, co_did);
                        cs->sendToClient(w_buf, len);
                }
        }
                break;
        case 0x6F: {
                if (corr_cmd == 0x4F) {
                        srInfo(to_string(co_did)
                               + " download msg to collector is OK!");
                        string msg = "111," + cs->getOpsId(co_did) + ",SUCCESSFUL\n";
                        string co_sid = getSid(agent, to_string(co_did), _http);
                        msg += "113," + co_sid + ","
                                + to_string(SUCCESSFUL) + "\n";
                        agent.send(msg);
                        cs->setDinfoFrameNum(0);
                }
        }
                break;
        }
}
