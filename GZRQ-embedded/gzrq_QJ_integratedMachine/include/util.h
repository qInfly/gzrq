
#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include "sragent.h"
#include "srnethttp.h"
#include "def_type.h"
using namespace std;

#pragma pack(push, 1)

struct MeterInfo {
        u8 type;
        u8 client_id;
};

struct Header {
        u8 start_flag;          // 起始标志
        u8 len;                 // 长度
        u8 frame_direction;     // 帧方向
};

struct Regist {
        u8 id[4];
        u8 phone[11];
        u8 split;
        u8 addr[4];
        u8 end;
};

struct SingleAck {              // 单表抄表数据
        struct Header hdr;
        u8 status;              // 抄表状态
        u32 number;             // 表号
        u32 rd_num;             // 表读数
        u8 chr_status[6];       // 字轮状态
        u8 valve_status;        // 阀门状态
        u8 check;               // 校验和
        u8 end;                 // 结束符
};

struct GettmAck {               // 4.5.7.提取指定小区管理机的时钟命令(查询时间)
        struct Header hdr;
        u8 corr_cmd;            // 应答正确命令
        u8 frame_cmd;           // 帧命令
        u32 concentrator_addr;   // 集中器地址
        u8 tm[7];                // 时间-->HEX码(SS/MM/HH/DD/MO/2位年)
        u8 check;                // 校验和
        u8 end;                  // 结束符
};

struct SettmAck {
        struct Header hdr;
        u8 corr_cmd;            // 应答正确命令
        u8 frame_cmd;           // 帧命令
        u32 manager_addr;   // 集中器地址
        u8 check;                // 校验和
        u8 end;                  // 结束符
};

/*********************************/
struct ExtractInfo {            // 群抄中的表具信息

};
/*********************************/

#pragma pack(pop) // 恢复先前的pack设置

/* 读取配置文件 */
void readConfig(const char* path, const string& key, string& out_ptr);

//void initType();

string getSid(const SrAgent& agent, const string& did, SrNetHttp* http);

Bool getSid(const SrAgent& agent, const string& did, SrNetHttp* http,
            string& me_sid, string& me_spid, string& me_badge_nbr, u32& co_did);

u8 addCheck(u8* p, const int& len);

#endif //__UTIL_H__
