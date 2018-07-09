#include <iostream>
#include <fstream>
#include <string.h>
#include <srlogger.h>
#include <srutils.h>
#include "util.h"
#include <boost/thread/mutex.hpp>
#include <mutex>

extern map<u32, vector<u32>> manager_to_collector; // 从小区管理机到集中器映射
extern map<u32, vector<u32>> collector_to_meter;   // 从集中器到表具映射
extern map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射

extern map<int, map<string, vector<string>>> reg_to_dnum; // 最后修改要改成这种形式
extern map<string, vector<u8>> dnum_to_dinfo;

void readConfig(const char* path, const string& key, string& out_ptr)
{
        fstream cfgFile;
        cfgFile.open(path);//Open cfg file

        if (!cfgFile.is_open()) {
                return;
        }

        while (!cfgFile.eof()) { //Read file by line in loop
                char tmp[200] = {0};
                cfgFile.getline(tmp, 200);
                string line(tmp);
                size_t pos = line.find('=');

                if (pos == string::npos) {
                        cfgFile.close();
                        return;
                }

                string tmpKey = line.substr(0, pos);
                if (key == tmpKey) {
                        string tmpValue = line.substr(pos + 1);
                        cfgFile.close();
                        out_ptr = tmpValue;
                }
        }

        cfgFile.close();
        return;
}

void initType()
{
        vector<u32> collectors;
        collectors.push_back(18035);
        manager_to_collector.insert(pair<u32, vector<u32>>(16631, collectors));

        vector<u32> meters;
        meters.push_back(700365619);
        meters.push_back(700434963);
        meters.push_back(900641138);
        collector_to_meter.insert(pair<u32, vector<u32>>(18035, meters));

        struct MeterInfo mi;
        memset(&mi, 0, sizeof(mi));
        mi.type = 0x10;
        mi.client_id = 0x00;
        meter_to_info.insert(pair<u32, struct MeterInfo>(900641138, mi));
        mi.client_id = 0x01;
        meter_to_info.insert(pair<u32, struct MeterInfo>(700365619, mi));
        mi.client_id = 0x02;
        meter_to_info.insert(pair<u32, struct MeterInfo>(700434963, mi));
}

string getSid(const SrAgent& agent, const string& did, SrNetHttp* http)
{
        srDebug("get system id");
        string sid = "";
        string did_ = "";
        SmartRest sr(http->response());
        SrRecord r;

        http->clear();
        if (http->post("100," + did) > 0) {
                sr.reset(http->response());
                r = sr.next();

                while (r.size()) {
//                        if (r.size() == 3 && r.value(0) == "500") {
                                sid = r.value(2);
                                http->clear();
                                if (http->post("136," + sid) > 0) {
                                        if (r.size() == 3 && r.value(0) == "601") {
                                                did_ = r.value(2).c_str();
                                                if (did_ == did) {
                                                        sid = r.value(2);
                                                        break;
                                                }
                                        }
                                }
//                        }
                        r = sr.next();
                }
                http->clear();
        }

        return sid;
}

Bool getSid(const SrAgent& agent, const string& did, SrNetHttp* http,
            string& me_sid, string& me_spid, string& me_badge_nbr, u32& co_did)
{
        srDebug("get system id");
        Bool ret_1 = FALSE, ret_2 = FALSE;
        string did_ = "";
        SmartRest sr(http->response());
        SrRecord r;

        http->clear();
        if (http->post("100," + did) > 0) {
                sr.reset(http->response());
                r = sr.next();

                while (r.size()) {
                        if (r.size() == 3 && r.value(0) == "500") {
                                me_sid = r.value(2);
                                http->clear();
                                if (http->post("136," + me_sid) > 0) {
                                        if (r.size() == 3 && r.value(0) == "601") {
                                                did_ = r.value(2).c_str();
                                                if (did_ == did) {
                                                        me_sid = r.value(2);
                                                        ret_1 = TRUE;
                                                        break;
                                                }
                                        }
                                }
                        }
                        r = sr.next();
                }
                http->clear();
        }

        http->clear();

        if (http->post("135," + me_sid) > 0) {
                sr.reset(http->response());
                r = sr.next();

                while (r.size()) {
                        if (r.size() == 5 && r.value(0) == "701") {
                                me_spid = r.value(2);
                                me_badge_nbr = r.value(3);
                                co_did = (u32)atoi(r.value(4).c_str());
                                ret_2 = TRUE;
                                break;
                        }
                        r = sr.next();
                }
                http->clear();
        }

        return (ret_1 && ret_2);
}

u8 addCheck(u8* p, const u32& len)
{
        u8 sum = 0x00;
        for (u32 i = 0; i < len; i++) {
                sum += p[i];
        }
        return sum;
}

u8& charToHex(u8& b_hex)
{
        if ((b_hex >= 0) && (b_hex <= 9)) {
                b_hex += 0x30;
        } else {
                char tmp[2] = {0};
                snprintf(tmp, 2, "%c", b_hex);
                b_hex = tmp[0];
        }

        return b_hex;
}

string hexToAsc(uint8_t& val)
{
        char p[8] = {0};

        p[0] = (val >> 4) & 0x0F;
        p[1] = val & 0x0F;
        snprintf(p, 3, "%x%x", p[0], p[1]);

        if (p[0] >= 'a' && p[0] <= 'f')
                p[0] -= 0x20;
        if (p[1] >= 'a' && p[1] <= 'f')
                p[1] -= 0x20;

        string s(p);
        return s;
}

u32 findRN(u8* p, const u32& len)
{
        u32 i = 0;
        for (i = 0; i < len; i++) {
                if ((p[i] == 0x0A) && (p[i-1] == 0x0D))
                        break;
        }

        return (i-1);
}

u8 hexToAscii(u8* p)
{
        char t[4] = {0};

        snprintf(t, sizeof(t), "%c%c", p[0], p[1]);
//        sprintf(t, "%d", atoi(t));
        int v = 0;
        sscanf(t, "%x", &v);
        snprintf((char*)t, sizeof(t), "%c", v);

        return t[0];
}

u32 rotationWhl(const u32& value, const string& me_sid)
{
        u32 ret = 0x00;


        ret = value;
        return ret;
}
