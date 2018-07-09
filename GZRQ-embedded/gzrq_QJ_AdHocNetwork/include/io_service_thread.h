
#ifndef __IO_SERVICE_THREAD_H__
#define __IO_SERVICE_THREAD_H__

#include "client_session.h"
#include "sragent.h"
#include <srnethttp.h>
#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <stdexcept>
#include <boost/lexical_cast.hpp>


using boost::asio::ip::tcp;

class ClientSession;
class TransferData;

#if 0
/*********************************************/
class io_service_pool
: public boost::noncopyable
{
public:

explicit io_service_pool(std::size_t pool_size)
: next_io_service_(0)
{ 
for (std::size_t i = 0; i < pool_size; ++ i)
{
io_service_sptr io_service(new boost::asio::io_service);
work_sptr work(new boost::asio::io_service::work(*io_service));
io_services_.push_back(io_service);
work_.push_back(work);
}
}

void start()
{ 
for (std::size_t i = 0; i < io_services_.size(); ++ i)
{
boost::shared_ptr<boost::thread> thread(new boost::thread(
boost::bind(&boost::asio::io_service::run, io_services_[i])));
thread->detach();
threads_.push_back(thread);
}
}

void join()
{
for (std::size_t i = 0; i < threads_.size(); ++ i)
{
threads_[i]->join();
} 
}

void stop()
{ 
for (std::size_t i = 0; i < io_services_.size(); ++ i)
{
io_services_[i]->stop();
}
}

boost::asio::io_service& get_io_service()
{
//boost::mutex::scoped_lock lock(mtx);
boost::asio::io_service& io_service = *io_services_[next_io_service_];
++ next_io_service_;
if (next_io_service_ == io_services_.size())
{
next_io_service_ = 0;
}
return io_service;
}

private:
typedef boost::shared_ptr<boost::asio::io_service> io_service_sptr;
typedef boost::shared_ptr<boost::asio::io_service::work> work_sptr;
typedef boost::shared_ptr<boost::thread> thread_sptr;

boost::mutex mtx;

std::vector<io_service_sptr> io_services_;
std::vector<work_sptr> work_;
std::vector<thread_sptr> threads_; 
std::size_t next_io_service_;
};
/*********************************************/
#endif
class ThreadPool {
public:
  explicit ThreadPool(size_t size) : work_(io_service_) {
    for (size_t i = 0; i < size; ++i) {
      workers_.create_thread(
          boost::bind(&boost::asio::io_service::run, &io_service_));
    }
  }

  ~ThreadPool() {
    io_service_.stop();
    workers_.join_all();
  }

   //Add new work item to the pool.
     template<class F>
       void Enqueue(F f) {
           io_service_.post(f);
             }
	
	boost::asio::io_service& getIoService() {
		return io_service_;
	}	
  
            private:
               boost::thread_group workers_;
                 boost::asio::io_service io_service_;
                   boost::asio::io_service::work work_;
                   };
class IOServiceThread
{
        typedef std::shared_ptr<ClientSession> session_ptr;	//智能指针
public:
        /* IOServiceThread构造函数 */
        IOServiceThread(SrAgent& agent, const int& port, SrNetHttp* http);

        /* IOServiceThread析构函数 */
        ~IOServiceThread();

        /* 启动io_service */
	void StartHandleAccept();
        //void StartIOService();

        /* 接收设备的连接 */
        void ProcessAccept(const boost::system::error_code& error,
                           const session_ptr& session);
	
	void Start();

private:
//	io_service_pool _io_service_pool;
	ThreadPool _tp;
        SrAgent& _agent;
        //boost::asio::io_service _io_service; 	//创建boost::asio::io_service
        tcp::endpoint _endpoint;
        tcp::acceptor _acceptor;
        std::thread _t;
	//const int& _nThreads;
	std::vector<boost::shared_ptr<boost::thread>> m_listThread;
	SrNetHttp* _http;
};

#endif //__IO_SERVICE_THREAD_H__
