/*
 * Copyright (c) 2017, 基本立子(北京)科技发展有限公司
 * All rights reserved
 *
 * 文件名称: client_session.cc
 * 摘要: 设备处理类，读取每个设备发来的数据
 * 当前版本: 1.0
 * 作者: 范文瀚
 * 完成日期: 2017年1月13日
 */

#ifndef __CLIENT_SESSION_H__
#define __CLIENT_SESSION_H__

//#pragma once

#include "util.h"
#include <srnethttp.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::tcp;
using boost::asio::ip::address;

#define REG_LENGTH        21

class TransferData;

typedef map<int, map<int, string>> _F;

class ClientSession : public std::enable_shared_from_this<ClientSession>
{
public:
        /* 构造函数：创建与设备的通讯 */

        ClientSession(SrAgent& agent,
                      boost::asio::io_service& ioservice, SrNetHttp* http);

        /* 析构函数 */
        ~ClientSession();

        /* 获得连接客户端的socket */
        tcp::socket& socket() {
                return _sock;
        }

        string& sid() {
                return _sid;
        }

        void ReadReg(const unsigned long& len);

        /* 异步读取数据 */
        void Read(const unsigned int& len);

        void ReadSpec(const unsigned int& len);

        void setOptId(const string& opt_id) {
                _opt_id = opt_id;
        }

        // 群抄帧号
        inline u8& allFrameNum(const u32& co_did) {
                u8& frame_no = _codid_to_framenum[co_did];
                printf("----->frame_no:%x\n ", frame_no);
                return (frame_no = ((frame_no) >= 255 ? 1 : ++frame_no));
        }

        // 重置帧号
        void setAllFrameNum(const u32& co_did, const u8 afn) {
                _codid_to_framenum[co_did] = afn;
        }

        // 下载表具信息到集中器第一步帧号
        inline u8& dinfoFrameNum() {
                return ++_dinfo_frame_number;
        }
        void setDinfoFrameNum(const u8 sdfn) {
                _dinfo_frame_number = sdfn;
        }

        // 下载表具信息到小区管理机帧号
        inline u8& mgDinfoFrameNum() {
                return ++_mg_dinfo_frame_number;
        }
        void setMgDinfoFrameNum(const u8 smdfn) {
                _mg_dinfo_frame_number = smdfn;
        }


        void setOpsId(const u32& co_did, const string& ops_id) {
                _codid_to_opsid[co_did] = ops_id;
        }

        const string& getOpsId(const u32& co_did) {
                return _codid_to_opsid[co_did];
        }





        const string& getOptId() {
                return _opt_id;
        }

/****************标志着当前小区管理机处理的集中器*********************/
        const u32& coDid() {
                return _co_did;
        }

        void setCoDid(const u32& co_did) {
                _co_did = co_did;
        }
/***************************************************************/
        void setFlag() {
                _n_flag = 1;
        }

public:
//        virtual void operator()(SrRecord &r, SrAgent &agent);

private:
        void OnReadReg(const boost::system::error_code& error,
                       const size_t& bytes_transferred);

        /* 读完数据后，决定做什么操作
         * bytes_transferred:实际读到的字节数 */
        void OnRead(const boost::system::error_code& error,
                    const size_t& bytes_transferred);

        void OnReadSpec(const boost::system::error_code& error,
                    const size_t& bytes_transferred);


        void UpdateAllInfo(const u32& ptr_len);

        void DataProcessing();

        void regDev();

        //void sendCmd(SrRecord& r);

public:
        void sendToClient(u8* w_buf, const int& len);

        void onWrite(const boost::system::error_code& error,
                     const size_t& bytes_transferred, const string& sid);

private:
        SrAgent& _agent;
        tcp::socket _sock;
        boost::asio::io_service &_io_service;
        u8* _data;
        u8* _data_pos;
        u8* _body_pos;

        struct Header* _hdr;	//消息头结构指针
        string _did;
        string _sid;
        map<int, string> _d_type;
        map<string, string> _d_unit;

        map<u32, string> _codid_to_opsid;

        string _opt_id;

        size_t _reg_len;
        size_t _data_len;

        u8 _all_frame_number;
        map<u32, u8> _codid_to_framenum;
        u8 _dinfo_frame_number;
        u8 _mg_dinfo_frame_number;

        u32 _co_did;

        int _n_flag;

        _F _flag_type;
        SrNetHttp* _http;
};

#endif //__CLIENT_SESSION_H__
