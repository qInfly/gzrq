
#include "srlogger.h"
#include "send_dinfo_ack.h"

extern map<u32, vector<u32>> collector_to_meter;
extern map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射

SendDinfoAck::SendDinfoAck()
{

}

SendDinfoAck::~SendDinfoAck()
{

}

/*
  管理中心下传全部表具信息到指定数据集中器
 */
int SendDinfoAck::getAllBuf(u8* w_buf, ClientSession* cs)
{
        int i = 0;
        const u32& co_did = cs->coDid();
        vector<u32>& meters = collector_to_meter[co_did];

        u8 quantity = (u8)meters.size();

        w_buf[i++] = 0x25;
        w_buf[i++] = quantity;      // 一次性下载10个表具
        w_buf[i++] = 0x55;

        w_buf[i++] = 0x33;

        w_buf[i++] = co_did & 0xFF;
        w_buf[i++] = (co_did >> 8) & 0xFF;
        w_buf[i++] = (co_did >> 16) & 0xFF;
        w_buf[i++] = (co_did >> 24) & 0xFF;

        w_buf[i++] = cs->dinfoFrameNum();

        /* 表具信息 */
        // 表号后续修改为根据集中器地址来找到
        for (auto& meter : meters) {
                w_buf[i++] = meter & 0xFF;
                w_buf[i++] = (meter >> 8) & 0xFF;
                w_buf[i++] = (meter >> 16) & 0xFF;
                w_buf[i++] = (meter >> 24) & 0xFF;
                struct MeterInfo& mi = meter_to_info[meter];
                w_buf[i++] = mi.type;
                w_buf[i++] = mi.client_id;
        }

        w_buf[i++] = addCheck(w_buf, i);
        w_buf[i++] = 0x0d;

        return i;
}

/*
  管理中心向数据集中器发送数传结束命令
 */
int SendDinfoAck::getStopBuf(u8* w_buf, ClientSession* cs)
{
        int i = 0;
        const u32& co_did = cs->coDid();

        w_buf[i++] = 0x25;
        w_buf[i++] = 0x0A;
        w_buf[i++] = 0x55;

        w_buf[i++] = 0x6F;

        w_buf[i++] = co_did & 0xFF;
        w_buf[i++] = (co_did >> 8) & 0xFF;
        w_buf[i++] = (co_did >> 16) & 0xFF;
        w_buf[i++] = (co_did >> 24) & 0xFF;

        w_buf[i++] = addCheck(w_buf, i);
        w_buf[i++] = 0x0d;

        return i;
}

void SendDinfoAck::sendDinfoAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        u8 w_buf[BUFSIZE] = {0};
        int len = 0;

        const u8& corr_cmd = data[3];
        const u8& frame_cmd = data[4];

        switch (frame_cmd) {
        case 0x33: {
                if (corr_cmd == 0x4F) {
                        len = getStopBuf(w_buf, cs);
                        cs->sendToClient(w_buf, len);
                }
        }
                break;
        case 0x6F: {
                if (corr_cmd == 0x4F) {
                        srInfo(to_string(cs->coDid())
                               + " download msg to collector is OK!");
                        string msg = "111," + cs->getOptId() + ",SUCCESSFUL";
                        agent.send(msg);
                        cs->setDinfoFrameNum(0);
                }
        }
                break;
        }
}
