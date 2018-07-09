
#include <time.h>
#include "util.h"
#include "send_checktm_ack.h"

static void getTime(struct tm& stm)
{
        time_t tm = time(NULL);
        memcpy(&stm, localtime(&tm), sizeof(stm));
}


SendCheckTmAck::SendCheckTmAck()
{

}

SendCheckTmAck::~SendCheckTmAck()
{

}

int SendCheckTmAck::getBuf(u8* w_buf)
{
        int i = 0;

        w_buf[i++] = 0x25;
        w_buf[i++] = 0x11;
        w_buf[i++] = 0x55;

        w_buf[i++] = 0x04;

        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;
        w_buf[i++] = 0x00;

        struct tm stm;
        memset(&stm, 0, sizeof(stm));
        getTime(stm);

        w_buf[i++] = (u8)stm.tm_sec;
        w_buf[i++] = (u8)stm.tm_min;
        w_buf[i++] = (u8)stm.tm_hour;
        w_buf[i++] = (u8)stm.tm_mday;
        w_buf[i++] = (u8)stm.tm_mon + 1;
        u16 year = (u16)stm.tm_year + 1900;
        w_buf[i++] = year & 0xFF;
        w_buf[i++] = (year >> 8) & 0xFF;

        w_buf[i++] = addCheck(w_buf, i);
        w_buf[i++] = 0x0d;

        return i;
}

void SendCheckTmAck::sendCheckTmAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        struct SettmAck sta;
        memset(&sta, 0, sizeof(sta));
        memcpy(&sta, data, sizeof(sta));

        if (sta.corr_cmd == 0x4F) {
                string msg = "111," + cs->getOptId() + ",SUCCESSFUL\n";
                agent.send(msg);
        }
}
