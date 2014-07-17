#include <network.h>
#include <map>
#include <sstream>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "pkgs/file_send.h"
#include "dsource/divide.h"
#include "sae/sae_from_config.h"

namespace ff {
class DLMaster {

public:
    DLMaster(ffnet::NetNervureFromFile& nnff, const std::string& sae_config_file, const std::string& fbnn_config_file)
        : m_oNNFF(nnff),
          m_str_sae_configfile(sae_config_file),
          m_str_fbnn_ConfigFile(fbnn_config_file) {}

    void onConnSucc(ffnet::TCPConnectionBase*pConn)
    {
        ffnet::EndpointPtr_t it = pConn->getRemoteEndpointPtr();
        std::string master_addr = m_oNNFF.NervureConf()->get<std::string>("tcp-client.target-svr-ip-addr");
        uint16_t port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-client.target-svr-port");

        //to check if it's a connection from rm_master
        if(it->address().to_string() == master_addr &&
                it->port() == port)
        {
            boost::shared_ptr<ReqNodeMsg> msg(new ReqNodeMsg());
            m_oNNFF.send(msg, it);
        }
        else {
            //to check if it's a connection from slave
            boost::shared_ptr<FileSendDirReq> reqmsg(new FileSendDirReq());
            for(size_t i = 0; i < m_oSlaves.size(); ++i)
            {
                if(it->address() == m_oSlaves[i]->address() &&
                        it->port() == m_oSlaves[i]->port())
                {
                    m_oNNFF.send(reqmsg, it);
                    std::cout << "send request message to " << it->address() << std::endl;
//                     boost::shared_ptr<CmdStartReq> startMsg(new CmdStartReq());
// 		    startMsg->cmd() = "haha!";
//                     std::cout << "send start Cmd Message onConnSucc!" << std::endl;
//                     m_oNNFF.send(startMsg, it);
                }
            }
        }
    }

    void onRecvAck(boost::shared_ptr<AckNodeMsg> pMsg, ffnet::EndpointPtr_t pEP)
    {
        const std::vector<slave_point_spt>& points = pMsg->all_slave_points();

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
            std::cout << "add TCP Client: " << points[i]->ip_addr <<" : "<<points[i]->tcp_port << std::endl;
        }
        //Depart data into pieces based on slave number.
        //make the output dir
        std::string output_dir;
        if((output_dir = newDirAtCWD(globalDirStr)) == "")
        {
            std::cout << "Error when make output dir!" << std::endl;
            return;
        }
        std::cout << "output DIR = " << output_dir << std::endl;

        m_p_sae_nc = NervureConfigurePtr(new ffnet::NervureConfigure("../confs/apps/SdAE_train.ini"));
        divide_into_files(m_oSlaves.size(),getInputFileNameFromNervureConfigure(m_p_sae_nc),output_dir.c_str());
        m_p_sae = SAE_create(m_p_sae_nc);
//         m_p_fbnn_nc = std::make_shared<ffnet::NervureConfigure>(ffnet::NervureConfigure("../confs/apps/FFNN_train.ini"));
//         train_NN(m_p_sae,m_p_fbnn_nc);//train a final fbnn after pretraining
    }

    void onRecvSendFileDirAck(boost::shared_ptr<FileSendDirAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        std::string& slave_path = pMsg->dir();
        //So here, slave_path shows the path on slave point, and pEP show the address of slave point.
        file_send(m_str_sae_configfile,pEP->address().to_string(),slave_path);//Send sae config file
        std::cout<<"path on slave "<<slave_path<<std::endl;
        //Send divided inputs
        std::string data_dir = send_data_from_dir(static_cast<std::string>("./") +
                               globalDirStr,pEP->address().to_string(),slave_path);
        //Get dl_master server port & ip
        std::string server_addr = m_oNNFF.NervureConf()->get<string_t>("tcp-server.ip");
        uint16_t server_port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-server.port");
        //TODO(sherryshare) when the file is sent over, send a CmdStartReq msg!
        boost::shared_ptr<CmdStartReq> startMsg(new CmdStartReq());
        int startIndex = m_str_sae_configfile.find_last_of('/') + 1;
        std::stringstream ss;
        ss << "./dl_worker " << m_str_sae_configfile.substr(startIndex,m_str_sae_configfile.length() - startIndex);
        ss << " " << data_dir;
        ss << " " << server_addr << " " << server_port;
        startMsg->cmd() = ss.str();
        std::cout << "send start Cmd Message!" << std::endl;
        m_oNNFF.send(startMsg, pEP);
    }

protected:
    typedef boost::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
    ffnet::NetNervureFromFile&     m_oNNFF;
    std::vector<ffnet::EndpointPtr_t> m_oSlaves;
    std::string m_str_sae_configfile;
    std::string m_str_fbnn_ConfigFile;
    ff::SAE_ptr m_p_sae;
    NervureConfigurePtr m_p_sae_nc;
    NervureConfigurePtr m_p_fbnn_nc;
};

}//end namespace ff

using namespace ff;
void  press_and_stop(ffnet::NetNervureFromFile& nnff)
{
    
    std::cout<<"Press any key to quit..."<<std::endl;
    getc(stdin);
    nnff.stop();
    std::cout<<"Stopping, please wait..."<<std::endl;
}

int main(int argc, char* argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "dl_master.log");
    std::string sae_config_file = "../confs/apps/SdAE_train.ini";
    std::string fbnn_config_file = "../confs/apps/FFNN_train.ini";
    if(argc > 1) {//sae_config_file argc
        std::stringstream ss_argv;
        ss_argv << argv[1];
        ss_argv >> sae_config_file;
    }
    std::cout << "sae config file = " << sae_config_file << std::endl;

    if(argc > 2) {//fbnn_config_file argc
        std::stringstream ss_argv;
        ss_argv << argv[1];
        ss_argv >> fbnn_config_file;
    }
    std::cout << "fbnn config file = " << fbnn_config_file << std::endl;

    ffnet::NetNervureFromFile nnff("../confs/dl_master_net_conf.ini");
    DLMaster master(nnff,sae_config_file,fbnn_config_file);

    nnff.addNeedToRecvPkg<AckNodeMsg>(boost::bind(&DLMaster::onRecvAck, &master, _1, _2));
    nnff.addNeedToRecvPkg<FileSendDirAck>(boost::bind(&DLMaster::onRecvSendFileDirAck, &master, _1, _2));
    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&DLMaster::onConnSucc, &master, _1));

    boost::thread monitor_thrd(boost::bind(press_and_stop, boost::ref(nnff)));

    nnff.run();
    monitor_thrd.join();
    return 0;
}
