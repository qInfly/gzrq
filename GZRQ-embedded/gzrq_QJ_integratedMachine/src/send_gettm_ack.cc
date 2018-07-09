
#include "send_gettm_ack.h"

SendGettmAck::SendGettmAck()
{

}

SendGettmAck::~SendGettmAck()
{

}

int SendGettmAck::getBuf(u8* w_buf)
{
        int i = 0;

        w_buf[i++] = 0x25;
        w_buf[i++] = 0x0A;
        w_buf[i++] = 0x55;
        w_buf[i++] = 0x07;

        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;

        w_buf[i++] = addCheck(w_buf, i);
        w_buf[i++] = 0x0d;

        return i;
}

void SendGettmAck::sendGettmAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        struct GettmAck ga;
        memset(&ga, 0, sizeof(ga));
        memcpy(&ga, data, sizeof(ga));


        if (ga.corr_cmd == 0x4F) {
                string msg = "111," + cs->getOptId() + ",SUCCESSFUL\n";
                agent.send(msg);
                u8 second = ga.tm[0];
                u8 minute = ga.tm[1];
                u8 hour = ga.tm[2];
                u8 day = ga.tm[3];
                u8 month = ga.tm[4];
                u16 year = ga.tm[5] + (ga.tm[6] << 8);

                // 年月日时分秒
                string tm = to_string(year) + "年" + to_string(month) + "月"
                        + to_string(day) + "日" + to_string(hour) + "时"
                        + to_string(minute) + "分" + to_string(second) + "秒";
                cerr << "tm: " << tm << endl;

                msg += "105," + cs->sid() + ",c8y_当前时间," + tm;
                agent.send(msg);
        }

}
