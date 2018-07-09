#include "io_service_thread.h"
#include "srlogger.h"
#include <iostream>
#include <mutex>
#include <unordered_map>
using namespace std;

extern vector<ClientSession*> v_ptr;
extern unordered_map<ClientSession*, bool> ptr;

IOServiceThread::IOServiceThread(SrAgent& agent, const int& port, SrNetHttp* http)
        :// _io_service_pool(6) ,
        _tp(2),
        _agent(agent)
        //, _io_service()
        , _endpoint(tcp::v4(), port)
        //, _acceptor(_io_service_pool.get_io_service(), _endpoint)
        , _acceptor(_tp.getIoService(), _endpoint)
        , _http(http)
{
        StartHandleAccept();
}

IOServiceThread::~IOServiceThread()
{

}


void IOServiceThread::StartHandleAccept()
{
        //session_ptr new_session(new ClientSession(_agent, _io_service_pool.get_io_service(), _http));
        session_ptr new_session(new ClientSession(_agent, _tp.getIoService(), _http));
        _acceptor.async_accept(new_session->socket(),
                               boost::bind(
                                       &IOServiceThread::ProcessAccept, this,
                                       boost::asio::placeholders::error,
                                       new_session));
        //_io_service.run();
}

void IOServiceThread::Start()
{
//	_io_service_pool.start();
//	_io_service_pool.join();
}

void IOServiceThread::ProcessAccept(const boost::system::error_code& error,
                                    const session_ptr& session)
{
        srInfo("a client has been connected");
        if(!error) {
                ptr[&(*(session))] = true;
                cerr << "ptr.size: " << ptr.size() << "|" <<  boost::lexical_cast<std::string>(boost::this_thread::get_id()) << endl;
                session->ReadReg(REG_LENGTH);
                session_ptr new_session(new ClientSession(_agent, _tp.getIoService(), _http));
                _acceptor.async_accept(new_session->socket(),
                                       boost::bind(&IOServiceThread::ProcessAccept,
                                                   this,
                                                   boost::asio::placeholders::error,
                                                   new_session));
        }
}
