#include <network.h>
#include <map>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


//! This is global thing!
class Slave{
public:
    Slave(ffnet::NetNervureFromFile & nnff)
        : m_oNNFF(nnff)
        , m_b_svr_connected(false){}

    void        onTimerSendHearBeat(const boost::system::error_code& /*e*/,
                                    boost::asio::deadline_timer* t)
    {
        if(m_b_svr_connected)
        {
            boost::shared_ptr<HeartBeatMsg> msg (new HeartBeatMsg()); //std::make_shared<HeartBeatMsg>();
            msg->ip_addr() = local_ip_v4();
            msg->tcp_port() = m_oNNFF.NervureConf()->get<uint16_t>("tcp-server.port");
            m_oNNFF.send(msg, m_p_svr);
        }
        t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
        t->async_wait(boost::bind(&Slave::onTimerSendHearBeat, this,
                  boost::asio::placeholders::error, t));
    }

    void    onConnSucc(ffnet::TCPConnectionBase *pConn)
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

    void    onLostTCPConnection(ffnet::TCPConnectionBase *pConn)
    {
        auto it = pConn->getRemoteEndpointPtr();
        std::string master_addr = m_oNNFF.NervureConf()->get<std::string>("tcp-client.target-svr-ip-addr");
        uint16_t port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-server.port");

        if(it->address().to_string() == master_addr &&
                it->port() == port)
        {
            m_b_svr_connected = false;
        }
    }

protected:
    ffnet::NetNervureFromFile  & m_oNNFF;
    bool    m_b_svr_connected;
    ffnet::EndpointPtr_t m_p_svr;
};

int main(int argc, char *argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "slave.log");

    ffnet::NetNervureFromFile nnff("../confs/slave_net_conf.ini");
    Slave s(nnff);

    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&Slave::onConnSucc, &s, _1));
    ffnet::event::Event<ffnet::event::tcp_lost_connection>::listen(&nnff, boost::bind(&Slave::onLostTCPConnection, &s, _1));

    boost::asio::deadline_timer t(nnff.getIOService(), boost::posix_time::seconds(1));

    t.async_wait(boost::bind(&Slave::onTimerSendHearBeat, &s,
            boost::asio::placeholders::error, &t));
    nnff.run();
    return 0;
}
