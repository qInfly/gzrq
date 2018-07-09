
#include "send_error_ack.h"

SendErrorAck::SendErrorAck()
{

}

SendErrorAck::~SendErrorAck()
{

}

void SendErrorAck::sendErrorAck(u8* data, SrAgent& agent, ClientSession* cs)
{
        string failure_reason = getFailureReason(data);
        string msg = "112," + cs->getOptId() + "," + failure_reason;
        agent.send(msg);
}

string SendErrorAck::getFailureReason(const u8* d)
{
        const u8& frame_cmd = d[3];
        string failure_reason;
        switch (frame_cmd) {
        case 0x0F:              // 通道短路
                failure_reason = "通道短路";
                break;
        case 0xFC:              // 小区管理机忙
                failure_reason = "小区管理机忙";
                break;
        case 0xFD:              // 集中器无应答（小区管理机回复的）
                failure_reason = "集中器无应答";
                break;
        case 0xFE:              // 集中器接收错误
                failure_reason = "集中器接收错误";
                break;
        case 0x1B:              // 参数（基础资料）不全
                failure_reason = "参数（基础资料）不全";
                break;
        case 0x73: {             // 小区管理机出错
                if (d[8] == 0x01 && d[9] == 0x00) { // 无线模块无法设置链路代码
                        failure_reason = "无线模块无法设置链路代码";
                } else if (d[8] == 0x02 && d[9] == 0x00) { // 无线模块无法设置版本号
                        failure_reason = "无线模块无法设置版本号";
                } else if (d[8] == 0x03 && d[9] == 0x00) { // 小区管理机断电
                        failure_reason = "小区管理机断电";
                }
        }
                break;
        }

        return failure_reason;
}
