#include <network.h>
#include <map>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "pkgs/file_send.h"

class DLMaster{

public:
    DLMaster(ffnet::NetNervureFromFile & nnff)
        : m_oNNFF(nnff){}

    void    onConnSucc(ffnet::TCPConnectionBase *pConn)
    {
        auto it = pConn->getRemoteEndpointPtr();
        std::string master_addr = m_oNNFF.NervureConf()->get<std::string>("tcp-client.target-svr-ip-addr");
        uint16_t port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-client.target-svr-port");

        //to check if it's a connection from rm_master
        if(it->address().to_string() == master_addr &&
                it->port() == port)
        {
            boost::shared_ptr<ReqNodeMsg> msg(new ReqNodeMsg());
            m_oNNFF.send(msg, it);
        }
        else{
        //to check if it's a connection from slave
            boost::shared_ptr<FileSendDirReq> reqmsg(new FileSendDirReq());
            for(size_t i = 0; i < m_oSlaves.size(); ++i)
            {
                if(it->address() == m_oSlaves[i]->address() &&
                    it->port() == m_oSlaves[i]->port())
                {
                    m_oNNFF.send(reqmsg, it);
                }
            }
        }
    }

    void onRecvAck(boost::shared_ptr<AckNodeMsg> pMsg, ffnet::EndpointPtr_t pEP)
    {
        const std::vector<slave_point_spt> & points = pMsg->all_slave_points();

        for(size_t i = 0; i < points.size(); ++i)
        {
            std::cout<<"slave : "<<points[i]->ip_addr <<" : "<<points[i]->tcp_port<<std::endl;
            
            ffnet::EndpointPtr_t sp(
                new ffnet::Endpoint(
                    boost::asio::ip::tcp::endpoint(
                        boost::asio::ip::address::from_string(points[i]->ip_addr), points[i]->tcp_port
                    )));
            m_oSlaves.push_back(sp);
            m_oNNFF.addTCPClient(sp);
        }
        /*
        for(size_t i = 0; i < points.size(); ++i){
            if(points[i]->ip_addr != m_oNNFF.NervureConf()->get<string_t>("tcp-server.ip"))
            {
                std::cout << "File send to " << points[i]->ip_addr << std::endl;
                file_send("../confs/slave_net_conf.ini",points[i]->ip_addr,"/home/sherry");
            }
        }*/
    }
    
    void onRecvSendFileDirAck(boost::shared_ptr<FileSendDirAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        std::string & slave_path = pMsg->dir();
        //So here, slave_path shows the path on slave point, and pEP show the address of slave point.
        //Send file here!
        //TODO(sherryshare)
        std::cout<<"path on slave "<<slave_path<<std::endl;
    }

protected:
    ffnet::NetNervureFromFile &     m_oNNFF;
    std::vector<ffnet::EndpointPtr_t> m_oSlaves;

};

int main(int argc, char *argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "dl_master.log");

    ffnet::NetNervureFromFile nnff("../confs/dl_master_net_conf.ini");
    DLMaster master(nnff);

    nnff.addNeedToRecvPkg<AckNodeMsg>(boost::bind(&DLMaster::onRecvAck, &master, _1, _2));
    nnff.addNeedToRecvPkg<FileSendDirAck>(boost::bind(&DLMaster::onRecvSendFileDirAck, &master, _1, _2));
    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&DLMaster::onConnSucc, &master, _1));

    nnff.run();
    return 0;
    return 0;
}
