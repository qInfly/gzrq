#include "srlogger.h"
#include "client_session.h"
#include <iostream>
#include <string.h>
#include <boost/thread/mutex.hpp>
#include <mutex>
#include <srlogger.h>
#include <unordered_map>
#include <srutils.h>

#include "send_single_ack.h"
#include "send_gettm_ack.h"
#include "send_all_ack.h"
#include "send_dinfo_ack.h"
#include "send_error_ack.h"
#include "send_checktm_ack.h"
#include "send_mgdinfo_ack.h"

//#include "redisSession.h"

using namespace std;

/******************************************************************/
unordered_map<ClientSession*, bool> ptr; // 连接session指针
// 小区管理机sid与session指针对应关系
unordered_map<string, ClientSession*> manager_sid_to_ptr;
//------------------------小区管理机-----------------
map<string, string> manager_sid_to_did;
map<string, string> manager_did_to_sid;
//--------------------------------------------------
//------------------------集中器-----------------
map<string, string> collector_sid_to_did;
map<string, string> collector_did_to_sid;
//--------------------------------------------------
//------------------------表具-----------------
map<string, string> meter_sid_to_did;
map<string, string> meter_did_to_sid;
//--------------------------------------------------
/******************************************************************/
/******************************************************************/
// 以下信息在小区管理机断线重连的时候不清除
map<u32, vector<u32>> manager_to_collector; // 从小区管理机到集中器映射
map<u32, vector<u32>> collector_to_meter;   // 从集中器到表具映射
map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射(表类型、通道号)
/******************************************************************/

vector<ClientSession*> v_ptr;
//vector<string> device_id;

boost::mutex io_mutex;

static string getSid(const SrAgent& agent, const string& did,
                     SrNetHttp* http, const string& system_id)
{
        srDebug("get system id");
        string sid = "";

        http->clear();
        if (http->post("100," + did) <= 0) {
                return sid;
        }

        SmartRest sr(http->response());
        SrRecord r = sr.next();

        if (r.size() && r[0].second == "50") {
                string s1 = did;
                http->clear();
                http->post("133," + s1 + ",\"c8y_Manager\"");
                sr.reset(http->response());
                r = sr.next();
                if (r.size() == 3 && r[0].second == "501") {
                        sid = r[2].second;
                        string s = "102," + sid + "," + did;
                        if (http->post(s) <= 0) {
                                return sid;
                        }
                        http->clear();
                }
        } else if (r.size() == 3 && r[0].second == "500") {
                sid = r[2].second;
                http->clear();
        }

        http->post("132," + system_id + "," + sid);
        srNotice("sys: " + sid);
        return sid;
}

static bool checkHeartBeat(u8* _data, const size_t& bytes_transferred)
{
        bool ret = false;
        if (bytes_transferred == 1 && _data[0] == 0xfe) {
                ret = true;
        }

        return ret;
}

static void clearCollectors(const string& co_sid, const string& co_did)
{
        if (!co_sid.empty()) {
                srNotice(collector_sid_to_did[co_sid] + ": remove collector_sid_to_did");
                collector_sid_to_did.erase(co_sid);
        }
        if (!co_did.empty()) {
                srNotice(collector_did_to_sid[co_did] + ": remove collector_did_to_sid");
                collector_did_to_sid.erase(co_did);
        }
}

static void clearMeters(const string& me_sid, const string& me_did)
{
        if (!me_sid.empty()) {
                srNotice(meter_sid_to_did[me_sid] + ": remove meter_sid_to_did");
                meter_sid_to_did.erase(me_sid);
        }
        if (!me_did.empty()) {
                srNotice(meter_did_to_sid[me_did] + ": remove meter_sid_to_did");
                meter_did_to_sid.erase(me_did);
        }
}

static void clrSession(const string& sid, const string& did, ClientSession* cs)
{
        if (!did.empty()) {
                u32 ma_did = (u32)atoi(did.c_str());
                vector<u32>& collectors = manager_to_collector[ma_did];
                for (auto& collector : collectors) { // 遍历删除小区管理机下所有的集中器
                        vector<u32>& meters = collector_to_meter[collector];
                        for (auto& meter : meters) {
                                string me_did = to_string(meter);
                                string me_sid = meter_did_to_sid[me_did];
                                clearMeters(me_sid, me_did);
                        }
                        string co_did = to_string(collector);
                        string co_sid = collector_did_to_sid[co_did];
                        clearCollectors(co_sid, co_did);
                }
                srNotice(manager_did_to_sid[did] + ": remove manager_did_to_sid");
                manager_did_to_sid.erase(did);
        }

        if (!sid.empty()) {
                srNotice(manager_sid_to_did[sid] + ": remove manager_sid_to_did");
                manager_sid_to_did.erase(sid);
                manager_sid_to_ptr.erase(sid);
                srDebug("=====>ptr last size: " + to_string(ptr.size()));
                ptr.erase(cs);
                srDebug("=====>ptr last size: " + to_string(ptr.size()));
        }
}

ClientSession::ClientSession(SrAgent& agent,
                             boost::asio::io_service& ioservice, SrNetHttp* http)
        : _agent(agent), _sock(ioservice),
          _io_service(ioservice), _ops_id(""), _reg_len(0), _data_len(0),
          _all_frame_number(0), _dinfo_frame_number(0), _mg_dinfo_frame_number(0),
          _co_did(0), _n_flag(0), _http(http), _stopped(false),
          _deadline(ioservice), _heartbeat_timer(ioservice)
{
        _data = new u8[1024];
        _data_pos = _data;
        _hdr = nullptr;

        _deadline.async_wait(boost::bind(&ClientSession::check_deadline, this));

}

ClientSession::~ClientSession()
{
        if (_data) {
                delete[] _data;
                _data = nullptr;
                _data_pos = nullptr;
                _hdr = nullptr;
        }
}

void ClientSession::check_deadline()
{
        if (_stopped)// check 是存已标识并等待程序退出
                return;

        if (_deadline.expires_at() <= deadline_timer::traits_type::now())
        {// 真正的等待超时

                srInfo(_did + " check_deadline and socket close");
                _sock.close();// 关闭对应的socket 包括连接超时/recv超时

                _deadline.expires_at(boost::posix_time::pos_infin);// 定时器 设定为永不超时/不可用状态
        } else {

        // 如果不是真正的超时，定是其操作已成功/用户重新设置了定时器
        // 重新启动定时器
        	_deadline.async_wait(boost::bind(&ClientSession::check_deadline, this));
	}
}

void ClientSession::ReadReg(const unsigned long& len)
{
        _sock.async_read_some(
                boost::asio::buffer(_data_pos, len),
                boost::bind(&ClientSession::OnReadReg,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void ClientSession::Read(const unsigned int& len)
{
        // 设置recv 超时时间，此处会触发 check_deadline 但不是真正的超时
        _deadline.expires_from_now(boost::posix_time::seconds(70));

        _sock.async_read_some(
                boost::asio::buffer(_data_pos, len),
                boost::bind(&ClientSession::OnRead,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void ClientSession::ReadSpec(const unsigned int& len)
{
        // 设置recv 超时时间，此处会触发 check_deadline 但不是真正的超时
        _deadline.expires_from_now(boost::posix_time::seconds(70));
        _sock.async_read_some(
                boost::asio::buffer(_data_pos, len),
                boost::bind(&ClientSession::OnReadSpec,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void ClientSession::OnReadReg(const boost::system::error_code& error,
                              const size_t& bytes_transferred)
{
        if (error) {
                srNotice(_did + ": read error and socket close");
                clrSession(this->sid(), "", this);
        } else {
                _reg_len += bytes_transferred;
                _data_pos += bytes_transferred;
                if (_reg_len < REG_LENGTH) {
                        ReadReg(REG_LENGTH - _reg_len);
                } else {
                        srDebug("=====>read reg:");
                        for (size_t i = 0; i < bytes_transferred; i++) {
                                printf("%02x ", _data[i]);
                        }
                        printf("\n");
                        struct Regist r;
                        memset(&r, 0, sizeof(r));
                        memcpy(&r, (char*)_data, sizeof(r));

                        int did = r.id[0] + (r.id[1] << 8) + (r.id[2] << 16)
                                + (r.id[3] << 24);
                        _did = to_string(did);

                        srDebug("======>did: " + _did);
//                        device_id.push_back(_did);

                        regDev();

                        memset(_data, 0, 1024);
                        _data_pos = _data;
                        _reg_len = 0;
                        Read(1); // 设备主动发送的数据只有一字节的心跳
                }
        }
}

void ClientSession::OnRead(const boost::system::error_code& error,
                           const size_t& bytes_transferred)
{
        if (_stopped) {
                srDebug(_did + " time out");
                return;
        }

        if (error) {
                srNotice(_did + ": read error and socket close");
                clrSession(this->sid(), this->did(), this);
        } else {
                int rd_len = 1;
                if (checkHeartBeat(_data, bytes_transferred)) {
                        srDebug(_did + " 心跳包");
                        _agent.send("107," + _sid);
                        memset(_data, 0, 1024);
                        _data_pos = _data;
                        rd_len = 1;
                } else {
                        _data_len += bytes_transferred;
                        _data_pos += bytes_transferred;

                        if (_data_len >= 3) {
                                if (_n_flag == 1) {
                                        rd_len = 8;
                                        ReadSpec(rd_len);
                                        return;
                                } else {
                                        rd_len = checkMsgType(_data, _data_len);
                                }
                        } else {
                                rd_len = 1;
                        }
                }

                Read(rd_len);
        }
}

int ClientSession::checkMsgType(u8* data, size_t& data_len)
{
        int rd_len = 0;
        if (_data[0] == 'O' && _data[1] == 'K') {
                if (_data_len < 11) {
                        rd_len = 11 - _data_len;
                } else {
                        u32 l = findRN(_data, _data_len);
                        int p_len = l+2;
                        int last_len = _data_len - p_len;
                        memcpy((char*)_data, (char*)(_data+p_len), last_len);
                        memset(_data+last_len, 0, 1024-p_len);
                        _data_pos = _data + last_len;
                        _data_len = last_len;
                        rd_len = 26;
                }
        } else if (_data[0] == 'A' && _data[1] == 'N' && _data[2] == 'S') {
                if (_data_len < 26) {
                        rd_len = 26 - _data_len;
                } else {
                        if (_data[4] == 0x35) { // 单抄
                                u8* p = _data + 4;
                                string s;
                                while (*p != 0x0D) {
                                        if (*p == 0x20) p += 1;
                                        s.push_back(
                                                hexToAscii(p));
                                        p += 2;
                                }
                                if (s == "RECEIVE") {
                                        srInfo("s: " + s);
                                        u8 w_buf[32] = {0};

                                        SendSingleAck ssa(_http);
                                        int len = ssa.getPickUpBuf(w_buf, this);
                                        sendToClient(w_buf, len);
                                }
                                memset(_data, 0, 1024);
                                _data_pos = _data;
                                _data_len = 0;
                                rd_len = 1;
                        } else if (_data[4] == 0x44) { // 数据
                                if (_data_len < 10) {
                                        rd_len = 10 - _data_len;
                                } else { // 满足能取到数据包长度
                                        u8* p = _data + 7;
                                        // 原始数据包长度
                                        u8 origin_len = hexToAscii(p);
                                        u8 total_len = (origin_len-1)*3 + 4 + 4;

                                        if (_data_len < total_len) {
                                                rd_len = total_len - _data_len;
                                        } else {
                                                for (int i = 0; i < _data_len; i++) {
                                                        printf("%02x ", _data[i]);
                                                }
                                                printf("\n");

                                                u8* p = _data + 4;
                                                u8* r_ptr = p;
                                                string s;
                                                u8 deal_len = 0; // 已经处理了的长度

                                                while (deal_len < total_len) {
                                                        if (*p == 0x20) p += 1;
                                                        uint8_t m = hexToAscii(p);
                                                        s.push_back(m);
                                                        p += 2;
                                                        deal_len = p - r_ptr;
                                                }

                                                u8* origin_ptr = const_cast<u8*>((u8*)s.c_str());

                                                for (int i = 0; i < s.size(); i++) {
                                                        printf("%02x ", origin_ptr[i]);
                                                }
                                                printf("\n");

                                                DataProcessing(origin_ptr);
                                                memset(_data, 0, 1024);
                                                _data_pos = _data;
                                                _data_len = 0;
                                                rd_len = 1;
                                        }

                                }
                        }

                }

        } else {
                memset(_data, 0, 1024);
                _data_pos = _data;
                _data_len = 0;
                rd_len = 1;
        }

        return rd_len;
}

void ClientSession::OnReadSpec(const boost::system::error_code& error,
                           const size_t& bytes_transferred)
{
        if (_stopped) {
                srNotice("read special time out");
                return;
        }

        if (error) {
                srNotice(_did + ": read special error and socket close");
                clrSession(this->sid(), this->did(), this);
        } else {
                _data_len += bytes_transferred;
                _data_pos += bytes_transferred;
                int rd_len = 1;

                if (_data[0] == 'O' && _data[1] == 'K') {
                        if (_data_len < 11) {
                                rd_len = 11 - _data_len;
                        } else {
                                u32 l = findRN(_data, _data_len);
                                int p_len = l+2;
                                int last_len = _data_len - p_len;
                                memcpy((char*)_data, (char*)(_data+p_len), last_len);
                                memset(_data+last_len, 0, 1024-p_len);
                                _data_pos = _data + last_len;
                                _data_len = last_len;
                                rd_len = 10;
                        }
                } else if (_data[0] == 'A' && _data[1] == 'N' && _data[2] == 'S') {
                        if (_data_len < 10) {
                                rd_len =10 - _data_len;
                        } else {
                                if (_data[4] == 0x44) { // 数据
                                        // 满足能取到数据包长度
                                        u8* p = _data + 7;
                                        // 原始数据包长度
                                        u8 origin_len = hexToAscii(p);
                                        u32 total_len = (origin_len*19+10)*3 + 4 + 4;

                                        if (_data_len < total_len) {
                                                rd_len = total_len - _data_len;
                                        } else {
                                                printf("------->origin_len:%x\n ", origin_len);
                                                printf("------->total_len:%d\n ", total_len);
                                                printf("------->_data_len:%ld\n ", _data_len);
                                                for (int i = 0; i < _data_len; i++) {
                                                        printf("%02x ", _data[i]);
                                                }
                                                printf("\n");

                                                u8* p = _data + 4;
                                                u8* r_ptr = p;
                                                string s;
                                                u32 deal_len = 0; // 已经处理了的长度
                                                while (deal_len < total_len) {
                                                        if (*p == 0x20) p += 1;
                                                        uint8_t m = hexToAscii(p);
                                                        s.push_back(m);
                                                        u8* d_ptr = const_cast<u8*>((u8*)s.c_str());
                                                        p += 2;
                                                        deal_len = p - r_ptr;
                                                        u8 sum = addCheck(d_ptr, deal_len);
                                                        if ((m == 0x0d) && (deal_len == total_len)) {
                                                                break;
                                                        }
                                                }

                                                u8* origin_ptr = const_cast<u8*>((u8*)s.c_str());

                                                srDebug("=================");
                                                u32 origin_ptr_len = s.size();
                                                for (u32 i = 0; i < origin_ptr_len; i++) {
                                                        printf("%02x ", origin_ptr[i]);
                                                }
                                                printf("\n");

                                                _n_flag = 0;
                                                UpdateAllInfo(origin_ptr, origin_ptr_len);
                                                memset(_data, 0, 1024);
                                                _data_pos = _data;
                                                _data_len = 0;
                                                Read(1);
                                                return;
                                        }

                                }
                        }
                }

                ReadSpec(rd_len);
        }
}

void ClientSession::UpdateAllInfo(u8* ptr, const u32& origin_ptr_len)
{
        srDebug(__func__);
        SendAllAck saa(_http);
        saa.sendAllAck(ptr, _agent, this, origin_ptr_len);
}

void ClientSession::DataProcessing(u8* ptr)
{
        srDebug(__func__);

        switch (ptr[1]) {     // 根据长度判断消息类型
        case 0x0a: {             // 4.5.6.3/4.5.10.C
		        SendErrorAck sea;
        		sea.sendErrorAck(ptr, _agent, this);
	    }
                break;
        case 0x0b: {             // 4.5.2/4.5.3/4.5.4/4.5.6.2/4.5.6.4/4.5.8/
                const u8& frame_cmd = ptr[4];
                if (frame_cmd == 0x33 || frame_cmd == 0x6F) { // 下载表具信息
                        SendDinfoAck sda(_http);
                        sda.sendDinfoAck(ptr, _agent, this);
                } else if (frame_cmd == 0x30 || frame_cmd == 0x36) { // 群抄
                        SendAllAck saa(_http);
                        saa.sendPollAck(ptr, _agent, this);
                } else if (frame_cmd == 0x04) { // 设置小区管理机时间
                        SendCheckTmAck sct;
                        sct.sendCheckTmAck(ptr, _agent, this);
                } else {        // 小区管理机应答管理中心下传资料结束
                        SendMgDinfoAck smda;
                        smda.sendMgStopAck(ptr, _agent, this);
                }
        }
                break;
        case 0x0c: {
                const u8& frame_cmd = ptr[3];
                if (frame_cmd == 0xFD || frame_cmd == 0xFC) {
                        SendErrorAck sea;
                        sea.sendErrorAck(ptr, _agent, this);
                } else if (frame_cmd == 0x10 || frame_cmd == 0x74) {
                        SendMgDinfoAck smda;
                        smda.sendMgDinfoAck(ptr, _agent, this);
                }
	    }
                break;
        case 0x12: {             // 查询时间(4.5.7)
                SendGettmAck sga;
                sga.sendGettmAck(ptr, _agent, this);
        }
                break;
        case 0x15: {             // 单表抄表
                SendSingleAck ssa(_http);
                ssa.sendSingleAck(ptr, _agent, this);
        }
                break;

        default:
                srNotice("暂不支持此类型");
                break;
        }
}

void ClientSession::regDev()
{
        if (_sid == "") {
                boost::mutex::scoped_lock lock(io_mutex);
                _sid = getSid(_agent, _did, _http, _agent.ID());
                manager_sid_to_did[_sid] = _did;
                manager_did_to_sid[_did] = _sid;
                manager_sid_to_ptr[_sid] = this;
		_agent.send("106," + _sid + ",2");
                //_agent.send("110," + _sid + ",\"\"\"c8y_Command\"\"\"");
                /**********注册当前小区管理机下的所有集中器为小区管理机的子设备**********/
        }
}

void ClientSession::sendToClient(u8* w_buf, const int& len)
{
        int j = 0;
        srInfo("set to client " + _did);
        for (; j < len; j++) {
                printf("%02x ", w_buf[j]);
        }
        printf("\n");

        boost::asio::async_write(
                _sock, boost::asio::buffer(w_buf, len),
                boost::bind(
                        &ClientSession::onWrite,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        _sid));
}

void ClientSession::onWrite(const boost::system::error_code& error,
                            const size_t& bytes_transferred,
                            const string& sid)
{
        string& did = manager_sid_to_did[sid];
        if (error) {
                srNotice(_did + ": read error and socket close");
                clrSession(this->sid(), this->did(), this);
        } else {
                srInfo("write to " + did + " sucessful|"
                       + to_string(bytes_transferred) + " bytes");
        }
}
