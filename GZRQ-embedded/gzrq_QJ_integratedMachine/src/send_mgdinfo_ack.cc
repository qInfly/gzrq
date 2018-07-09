
#include "send_mgdinfo_ack.h"

extern map<string, string> manager_sid_to_did;
extern map<u32, vector<u32>> manager_to_collector; // 从小区管理机到集中器映射
extern map<u32, vector<u32>> collector_to_meter;   // 从集中器到表具映射
extern map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射

SendMgDinfoAck::SendMgDinfoAck()
{

}

SendMgDinfoAck::~SendMgDinfoAck()
{

}

int SendMgDinfoAck::getBuf(u8* w_buf, ClientSession* cs)
{
        int i = 0;
        u8 group_count = 0;
        u32 ma_did = (u32)atoi(manager_sid_to_did[cs->sid()].c_str());


        w_buf[i++] = 0x25;
        w_buf[i++] = 0x01;      // 一次性下载10个表具
        w_buf[i++] = 0x55;

        w_buf[i++] = 0x10;      // 0x10 or 0x74

        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;

        w_buf[i++] = cs->mgDinfoFrameNum();

        vector<u32>& collectors = manager_to_collector[ma_did];
        for (auto& collector : collectors) {
                u32& co_did = collector;
                vector<u32>& meters = collector_to_meter[co_did];
                for (auto& meter : meters) {
                        w_buf[i++] = co_did & 0xFF;
                        w_buf[i++] = (co_did >> 8) & 0xFF;
                        w_buf[i++] = (co_did >> 16) & 0xFF;
                        w_buf[i++] = (co_did >> 24) & 0xFF;

                        // 集中器号
                        // 抄表类型: (0:有线；1:电力载波；2:点对点无线；3:无线路由)
                        w_buf[i++] = 0x00;

                        w_buf[i++] = 0x00;      // 所辖用户类型：(预留)

                        w_buf[i++] = 0xD4;      // 集中器代码：2Byte
                        w_buf[i++] = 0x55;

                        /* 表具信息 */
                        w_buf[i++] = meter & 0xFF;
                        w_buf[i++] = (meter >> 8) & 0xFF;
                        w_buf[i++] = (meter >> 16) & 0xFF;
                        w_buf[i++] = (meter >> 24) & 0xFF;

                        w_buf[i++] = 0x00; // 表具代码

                        struct MeterInfo& mi = meter_to_info[meter];
                        w_buf[i++] = mi.type; // 表类型
                        w_buf[i++] = mi.client_id; // 通道号
                	group_count++;
                }
        }

        w_buf[1] = group_count;

        w_buf[i++] = addCheck(w_buf, i);
        w_buf[i++] = 0x0d;

        return i;
}

int SendMgDinfoAck::getStopBuf(u8* w_buf)
{
        int i = 0;

        w_buf[i++] = 0x25;
        w_buf[i++] = 0x0A;
        w_buf[i++] = 0x55;

        w_buf[i++] = 0x11;      // 下载基础资料命令字

        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;

        w_buf[i++] = addCheck(w_buf, i);
        w_buf[i++] = 0x0d;

        return i;
}

void SendMgDinfoAck::sendMgDinfoAck(const u8* data, SrAgent& agent,
                                    ClientSession* cs)
{
        const u8& ack_code = data[4];
        u8 w_buf[BUFSIZE] = {0};
        int len = 0;

        switch (ack_code) {
        case 0x4F: {             // 正确
                len = getStopBuf(w_buf);
                cs->sendToClient(w_buf, len);
        }
                break;
        case 0x4E: {             // 错误
                string failure_reason = "错误";
                string msg = "112," + cs->getOptId() + "," + failure_reason;
        }
                break;
        }
}

void SendMgDinfoAck::sendMgStopAck(const u8* data, SrAgent& agent,
                                    ClientSession* cs)
{
        const u8& ack_code = data[4];
        u8 w_buf[BUFSIZE] = {0};
        int len = 0;
        string msg;

        switch (ack_code) {
        case 0x4F: {             // 正确
                len = getStopBuf(w_buf);
                msg = "111," + cs->getOptId() + ",SUCCESSFUL";
        }
                break;
        case 0x4E: {             // 错误
                string failure_reason = "错误";
                msg = "112," + cs->getOptId() + "," + failure_reason;
        }
                break;
        }

        if (!msg.empty()) agent.send(msg);
}
