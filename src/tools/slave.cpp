#include <network.h>
#include <map>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <stdio.h>

using namespace ff;
//! This is global thing!
class Slave{
public:
    Slave(ffnet::NetNervureFromFile& nnff)
        : m_oNNFF(nnff)
        , m_b_svr_connected(false)
        , m_b_is_stopped(false){}

    void        stop(){
        m_b_is_stopped.store( true );
    }
    
    void        onTimerSendHearBeat(const boost::system::error_code& /*e*/,
                                    boost::asio::deadline_timer* t)
    {
        if(m_b_svr_connected)
        {
            boost::shared_ptr<HeartBeatMsg> msg (new HeartBeatMsg()); //std::make_shared<HeartBeatMsg>();
            msg->ip_addr() = m_oNNFF.NervureConf()->get<string_t>("tcp-server.ip");
            msg->tcp_port() = m_oNNFF.NervureConf()->get<uint16_t>("tcp-server.port");
            m_oNNFF.send(msg, m_p_svr);
        }
        if(!m_b_is_stopped.load()){
            t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
            t->async_wait(boost::bind(&Slave::onTimerSendHearBeat, this,
                  boost::asio::placeholders::error, t));
        }
    }

    void    onConnSucc(ffnet::TCPConnectionBase* pConn)
    {
        auto it = pConn->getRemoteEndpointPtr();
        std::string master_addr = m_oNNFF.NervureConf()->get<std::string>("tcp-client.target-svr-ip-addr");
        uint16_t port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-client.target-svr-port");

        if(it->address().to_string() == master_addr &&
                it->port() == port)
        {
            m_p_svr = it;
            m_b_svr_connected = true;
        }
    }

    void    onLostTCPConnection(ffnet::EndpointPtr_t pEP)
    {
        auto it = pEP;
        std::string master_addr = m_oNNFF.NervureConf()->get<std::string>("tcp-client.target-svr-ip-addr");
        uint16_t port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-client.target-svr-port");

        if(it->address().to_string() == master_addr &&
                it->port() == port)
        {
            m_b_svr_connected = false;
        }
    }
    
    void onRecvSendFileDirReq(boost::shared_ptr<FileSendDirReq> pMsg, ffnet::EndpointPtr_t pEP)
    {
        boost::shared_ptr<FileSendDirAck> pReply(new FileSendDirAck());
        //Specify the dire path here
	if((pReply->dir() = newDirAtCWD(globalDirStr,"/home/sherry")) == "")
        {
            std::cout << "Error when make output dir!" << std::endl;
            return;
        }
        std::cout << "DIR = " << pReply->dir() << std::endl;
        m_oNNFF.send(pReply, pEP);
    }
    
    void onRecvCmdStartReq(boost::shared_ptr<CmdStartReq> pMsg, ffnet::EndpointPtr_t pEP)
    {
        //TODO(sherryshare)Start the program here!
    }

protected:
    ffnet::NetNervureFromFile& m_oNNFF;
    bool    m_b_svr_connected;
    ffnet::EndpointPtr_t m_p_svr;
    boost::atomic<bool>    m_b_is_stopped;
};

void  press_and_stop(ffnet::NetNervureFromFile& nnff, Slave& s)
{
    std::cout<<"Press any key to quit..."<<std::endl;
    getc(stdin);
    s.stop();
    nnff.stop();
    std::cout<<"Stopping, please wait..."<<std::endl;
}
int main(int argc, char* argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "slave.log");

    ffnet::NetNervureFromFile nnff("../confs/slave_net_conf.ini");
    Slave s(nnff);

    nnff.addNeedToRecvPkg<FileSendDirReq>(boost::bind(&Slave::onRecvSendFileDirReq, &s, _1, _2));
    nnff.addNeedToRecvPkg<CmdStartReq>(boost::bind(&Slave::onRecvCmdStartReq, &s, _1, _2));
    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&Slave::onConnSucc, &s, _1));
    ffnet::event::Event<ffnet::event::tcp_lost_connection>::listen(&nnff, boost::bind(&Slave::onLostTCPConnection, &s, _1));

    boost::asio::deadline_timer t(nnff.getIOService(), boost::posix_time::seconds(1));

    t.async_wait(boost::bind(&Slave::onTimerSendHearBeat, &s,
            boost::asio::placeholders::error, &t));
    boost::thread monitor_thrd(boost::bind(&press_and_stop, boost::ref(nnff), boost::ref(s)));
    
    nnff.run();
    monitor_thrd.join();
    return 0;
}
