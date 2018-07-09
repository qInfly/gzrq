#include <iostream>
#include <fstream>
#include <string.h>
#include <srlogger.h>
#include <srutils.h>
#include "util.h"
#include <boost/thread/mutex.hpp>
#include <mutex>

// extern map<u32, vector<u32>> manager_to_collector; // 从小区管理机到集中器映射
// extern map<u32, vector<u32>> collector_to_meter;   // 从集中器到表具映射
// extern map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射

// extern map<int, map<string, vector<string>>> reg_to_dnum; // 最后修改要改成这种形式
// extern map<string, vector<u8>> dnum_to_dinfo;

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

/*
void initType()
{
        vector<u32> collectors;
        collectors.push_back(90732);
        manager_to_collector.insert(pair<u32, vector<u32>>(17480, collectors));

        vector<u32> meters;
        meters.push_back(700365619);
        meters.push_back(700434963);
        collector_to_meter.insert(pair<u32, vector<u32>>(90732, meters));

        struct MeterInfo mi;
        memset(&mi, 0, sizeof(mi));
        mi.type = 0x10;
        mi.client_id = 0x07;
        meter_to_info.insert(pair<u32, struct MeterInfo>(700365619, mi));
        mi.client_id = 0x06;
        meter_to_info.insert(pair<u32, struct MeterInfo>(700434963, mi));
}
*/

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
                        if (r.size() == 3 && r.value(0) == "500") {
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
                        }
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
                                string me_sid_tmp = r.value(2);
                                http->clear();
                                if (http->post("136," + me_sid_tmp) > 0) {
                                        sr.reset(http->response());
                                        r = sr.next();
                                        if (r.size() == 3 && r.value(0) == "601") {
                                                did_ = r.value(2).c_str();
                                                if (did_ == did) {
                                                        me_sid = me_sid_tmp;
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

u8 addCheck(u8* p, const int& len)
{
        char sum = 0x00;
        for (int i = 0; i < len; i++) {
                sum += p[i];
        }
        return sum;
}
