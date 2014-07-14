#include <network.h>
#include <map>
#include "pkgs/pkgs.h"
#include "utils/utils.h"

using namespace ff;
//! This is global thing!
class RMMaster{
public:
    RMMaster(ffnet::NetNervureFromFile& nnff)
        : m_oNNFF(nnff){}
    typedef std::map<std::string, slave_point_spt>    slave_points_t;
    
    void stop(){
      if(!m_oSlavePoints.empty())
	m_oSlavePoints.clear();
    }

    void onLostTCPConnection(ffnet::EndpointPtr_t pEP)
    {
        auto p = pEP;
        std::string key = endpoint_to_string(p);
        auto it = m_oSlavePoints.find(key);
        if(it != m_oSlavePoints.end())
        {
            m_oSlavePoints.erase(it);
            std::cout<<"remove slave point: "<<p->address().to_string()<<":"<<p->port()<<std::endl;
        }

    }

    void onRecvHeartBeat(boost::shared_ptr<HeartBeatMsg> pPing, ffnet::EndpointPtr_t pEP)
    {
        std::string key = endpoint_to_string(pEP);
        if(m_oSlavePoints.find(key) == m_oSlavePoints.end())
        {
            slave_point_spt p(new slave_point_t(pPing->ip_addr(), pPing->tcp_port()));
            m_oSlavePoints.insert(std::make_pair(key, p));
            std::cout<<"new slave point from: "<< pEP->address().to_string()<<":"<<pEP->port()<<std::endl;
            std::cout<<"\t-- the slave point is : "<<p->ip_addr<<":"<<p->tcp_port<<std::endl;
        }
    }

    void onRecvReqNode(boost::shared_ptr<ReqNodeMsg> pMsg, ffnet::EndpointPtr_t pEP)
    {
        boost::shared_ptr<AckNodeMsg> reply(new AckNodeMsg());
        for(slave_points_t::iterator it = m_oSlavePoints.begin(); it != m_oSlavePoints.end(); ++it)
        {
            reply->all_slave_points().push_back(it->second);
        }
        m_oNNFF.send(reply, pEP);
    }

protected:
    slave_points_t      m_oSlavePoints;
    ffnet::NetNervureFromFile&     m_oNNFF;
};

void  press_and_stop(ffnet::NetNervureFromFile& nnff, RMMaster& master)
{
    std::cout<<"Press any key to quit..."<<std::endl;
    getc(stdin);
    master.stop();
    nnff.stop();
    std::cout<<"Stopping, please wait..."<<std::endl;
}

int main(int argc, char* argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "rm_master.log");

    ffnet::NetNervureFromFile nnff("../confs/rm_master_net_conf.ini");
    RMMaster master(nnff);

    nnff.addNeedToRecvPkg<HeartBeatMsg>(boost::bind(&RMMaster::onRecvHeartBeat, &master, _1, _2));
    nnff.addNeedToRecvPkg<ReqNodeMsg>(boost::bind(&RMMaster::onRecvReqNode, &master, _1, _2));
    ffnet::event::Event<ffnet::event::tcp_lost_connection>::listen(&nnff, boost::bind(&RMMaster::onLostTCPConnection, &master, _1));

    boost::thread monitor_thrd(boost::bind(press_and_stop, boost::ref(nnff), boost::ref(master)));
    
    nnff.run();
    monitor_thrd.join();
    return 0;
}
