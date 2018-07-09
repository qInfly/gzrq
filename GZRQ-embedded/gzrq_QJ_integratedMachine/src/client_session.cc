#include "srlogger.h"
#include "client_session.h"
#include <iostream>
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

using namespace std;

unordered_map<ClientSession*, bool> ptr;
/******************************************************************/
unordered_map<string, ClientSession*> manager_sid_to_ptr;
map<u32, vector<u32>> manager_to_collector; // 从小区管理机到集中器映射
map<u32, vector<u32>> collector_to_meter;   // 从集中器到表具映射
map<u32, struct MeterInfo> meter_to_info;   // 从表号到表具信息的映射(表类型、通道号)

// {注册id:{集中器addr:[表号1,表号2,...,表号n]}
//map<string, map<string, vector<string>>> reg_to_dnum; // 最后修改要改成这种形式
//map<string, vector<u8>> dnum_to_dinfo;

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

vector<ClientSession*> v_ptr;
map<string, struct Header> sid_to_header;
vector<string> device_id;

boost::mutex io_mutex;

static string getSid(const SrAgent& agent, const string& did,
                     SrNetHttp* http, const string& system_id)
{
        srDebug("get system id");
//        SrNetHttp http(agent.server() + "/s", srv, agent.auth());
        string sid = "";

        if (http->post("100," + did) <= 0) {
                return sid;
        }

        SmartRest sr(http->response());
        SrRecord r = sr.next();

        if (r.size() && r[0].second == "50") {
                http->clear();
                string s1 = did;
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

static void clrSession(const string& sid, ClientSession* p_session
                       , const string& did)
{
        if (p_session) {
                p_session->socket().close();
                unordered_map<ClientSession*, bool>::iterator it_ptr = ptr.find(p_session);
                if (it_ptr != ptr.end()) {
                        //delete it_ptr->first;
                        //it_ptr->first = NULL;
                        ptr.erase(it_ptr++);
                }

                cerr << "=====>ptr last size: " << ptr.size() << endl;
        }

        if (!did.empty()) {
                vector<string>::iterator did_it;
                for (did_it = device_id.begin();
                     did_it != device_id.end(); ++did_it) {
                        if ((*did_it) == did) {
                                device_id.erase(did_it);
                                break;
                        }
                }
        }

        if (!sid.empty()) {
                srNotice(manager_sid_to_did[sid] + ": remove client session");
                manager_sid_to_did.erase(sid);
                sid_to_header.erase(sid);
                unordered_map<string, ClientSession*>::iterator it_ptr = manager_sid_to_ptr.find(sid);
                if (it_ptr != manager_sid_to_ptr.end()) {
                        manager_sid_to_ptr.erase(it_ptr++);
                }
        }

/*
  if (!did.empty()) {
  vector<string>::iterator did_it;
  for (did_it = device_id.begin();
  did_it != device_id.end(); ++did_it) {
  if ((*did_it) == did) {
  device_id.erase(did_it);
  break;
  }
  }
  }

  if (!sid.empty()) {
  srNotice(manager_sid_to_did[sid] + ": remove client session");
  manager_sid_to_did.erase(sid);
  sid_to_header.erase(sid);
  }
*/
/*
  if (p_session) {
//	        vector<std::shared_ptr<ClientSession>>::iterator c_it;
vector<ClientSession*>::iterator c_it;
for (c_it = v_ptr.begin(); c_it != v_ptr.end(); c_it++) {
//if (&(*(*c_it)) == p_session) {
if (*c_it == p_session) {
p_session->socket().close();
v_ptr.erase(c_it);
break;
}
}
cerr << "=====>v_ptr last size: " << v_ptr.size() << endl;
}
*/
}

ClientSession::ClientSession(SrAgent& agent,
                             boost::asio::io_service& ioservice, SrNetHttp* http)
        : _agent(agent), _sock(ioservice),
          _io_service(ioservice), _reg_len(0), _data_len(0),
          _all_frame_number(0), _dinfo_frame_number(0), _mg_dinfo_frame_number(0),
          _co_did(0), _n_flag(0), _http(http)
{
        _data = new u8[BUFSIZE];
        _data_pos = _data;
        _hdr = nullptr;
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
        _sock.async_read_some(
                boost::asio::buffer(_data_pos, len),
                boost::bind(&ClientSession::OnRead,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void ClientSession::ReadSpec(const unsigned int& len)
{
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
                //_sock.close();
                clrSession(this->sid(), this, "");
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

                        // char did[16] = {0};
                        // snprintf(did, sizeof(did), "%x%x%x%x",
                        //          r.id[3], r.id[2], r.id[1], r.id[0]);

                        int did = r.id[0] + (r.id[1] << 8) + (r.id[2] << 16)
                                + (r.id[3] << 24);
                        _did = to_string(did);

//                        _did = string(regdid_to_did[did]);
                        cerr << "======>did: " << _did << endl;

//                        device_id.push_back(_did);
                        regDev();

                        memset(_data, 0, BUFSIZE);
                        _data_pos = _data;
                        _reg_len = 0;
                        Read(1); // 设备主动发送的数据只有一字节的心跳
                }
        }
}

void ClientSession::OnRead(const boost::system::error_code& error,
                           const size_t& bytes_transferred)
{
        if (error) {
                srNotice(_did + ": read error and socket close");
                //_sock.close();
                clrSession(this->sid(), this, "");
        } else {
                int rd_len = 0;
                if (checkHeartBeat(_data, bytes_transferred)) {
                        srDebug("心跳包");
                        _agent.send("107," + _sid);
                        memset(_data, 0, BUFSIZE);
                        _data_pos = _data;
                        rd_len = 1;
                } else {
                        _data_len += bytes_transferred;
                        _data_pos += bytes_transferred;
                        size_t hdr_len = sizeof(struct Header);
                        if (_data_len < hdr_len) { // 保证消息头读完整
                                rd_len = hdr_len - _data_len;
                        } else {
                                struct Header hdr;
                                memset(&hdr, 0, sizeof(hdr));
                                memcpy(&hdr, _data, sizeof(hdr));
                                if (_data_len < hdr.len) { // 保证消息体读完整
                                        rd_len = hdr.len - _data_len;
                                } else { // 先将包收完整了再处理(分类、上传等)
                                        if (_n_flag == 1) {
                                                cerr << "_n_flag == 1" << endl;
                                                rd_len = 9;
                                                ReadSpec(rd_len);
                                                return;
                                        } else {
                                                cerr << "_n_flag = 0" << endl;
                                                rd_len = 1;
                                                DataProcessing();
                                                memset(_data, 0, BUFSIZE);
                                                _data_pos = _data;
                                                _data_len = 0;
                                        }

/*
                                        DataProcessing();
                                        memset(_data, 0, BUFSIZE);
                                        _data_pos = _data;
                                        _data_len = 0;
                                        if (_n_flag == 1) {
                                                cerr << "_n_flag == 1" << endl;
                                                rd_len = 9;
                                                ReadSpec(rd_len);
                                                return;
                                        } else {
                                                cerr << "_n_flag = 0" << endl;
                                                rd_len = 1;
                                        }
*/
                              }
                        }
                }
                Read(rd_len);
        }
}

void ClientSession::OnReadSpec(const boost::system::error_code& error,
                           const size_t& bytes_transferred)
{
        if (error) {
                srNotice(_did + ": read error and socket close");
                //_sock.close();
                clrSession(this->sid(), this, "");
        } else {
                _data_len += bytes_transferred;
                _data_pos += bytes_transferred;
                if (_data_len < 9) {
                        ReadSpec(9 - _data_len);
                } else {
                        u8 len = _data[1];
                        u32 total_len = 9 + len*19 + 1 + 1;
                        printf("---------len: %02x\n", len);
                        printf("---------total_len: %02x\n", total_len);
                        if (_data_len < total_len)
                                ReadSpec(total_len - _data_len);
                        else {
                                cerr << "=======>: " << endl;
                                for (u32 i = 0; i < total_len; i++) {
                                        printf("%02x ", _data[i]);
                                }
                                printf("\n");
                                cerr << "=======" << endl;

                                UpdateAllInfo(total_len);
                                _n_flag = 0;
                                memset(_data, 0, BUFSIZE);
                                _data_pos = _data;
                                _data_len = 0;
                                Read(1);
                        }
                }
        }
}

void ClientSession::UpdateAllInfo(const u32& ptr_len)
{
        srDebug(__func__);
        SendAllAck saa(_http);
        saa.sendAllAck(_data, _agent, this, ptr_len);
}

void ClientSession::DataProcessing()
{
        srDebug("====>read data:");
        for (size_t i = 0; i < _data_len; i++) {
                printf("%02x ", _data[i]);
        }
        printf("\n");

        switch (_data[1]) {     // 根据长度判断消息类型
        case 0x0a: {             // 4.5.6.3/4.5.10.C
		        SendErrorAck sea;
        		sea.sendErrorAck(_data, _agent, this);
	    }
                break;
        case 0x0b: {             // 4.5.2/4.5.3/4.5.4/4.5.6.2/4.5.6.4/4.5.8/
                const u8& frame_cmd = _data[4];
                if (frame_cmd == 0x33 || frame_cmd == 0x6F) { // 下载表具信息
                        SendDinfoAck sda;
                        sda.sendDinfoAck(_data, _agent, this);
                } else if (frame_cmd == 0x30 || frame_cmd == 0x36) { // 群抄
                        SendAllAck saa(_http);
                        saa.sendPollAck(_data, _agent, this);
                } else if (frame_cmd == 0x04) { // 设置小区管理机时间
                        SendCheckTmAck sct;
                        sct.sendCheckTmAck(_data, _agent, this);
                } else {        // 小区管理机应答管理中心下传资料结束
                        SendMgDinfoAck smda;
                        smda.sendMgStopAck(_data, _agent, this);
                }
        }
                break;
        case 0x0c: {
                const u8& frame_cmd = _data[3];
                if (frame_cmd == 0xFD || frame_cmd == 0xFC) {
                        SendErrorAck sea;
                        sea.sendErrorAck(_data, _agent, this);
                } else if (frame_cmd == 0x10 || frame_cmd == 0x74) {
                        SendMgDinfoAck smda;
                        smda.sendMgDinfoAck(_data, _agent, this);
                }
	    }
                break;
        case 0x12: {             // 查询时间(4.5.7)
                SendGettmAck sga;
                sga.sendGettmAck(_data, _agent, this);
        }
                break;
        case 0x15: {             // 单表抄表
                SendSingleAck ssa(_http);
                ssa.sendSingleAck(_data, _agent, this);
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
//                _agent.addMsgHandler(505, this);
        }
}

void ClientSession::sendToClient(u8* w_buf, const int& len)
{
        int j = 0;
        srInfo("set to client");
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
                clrSession(_sid, this, "");
        } else {
                srInfo("write to " + did + " sucessful|"
                       + to_string(bytes_transferred) + " bytes");
        }
}


// void ClientSession::sendCmd(SrRecord& r)
// {

// }
