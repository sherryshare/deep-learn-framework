#include <network.h>
#include <map>
#include <sstream>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "pkgs/file_send.h"
#include "pkgs/sae_handles.h"
#include "dsource/divide.h"
#include "sae/sae.h"
#include "sae/sae_from_config.h"

using namespace ff;
class DLMaster {

public:
    DLMaster(ffnet::NetNervureFromFile & nnff, std::string sae_config_file, std::string fbnn_config_file)
        : m_oNNFF(nnff), SdAEConfigFileStr(sae_config_file), FBNNConfigFileStr(fbnn_config_file) {}

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
            std::cout << "add TCP Client: " << points[i]->ip_addr <<" : "<<points[i]->tcp_port << std::endl;
        }
        //Depart data into pieces based on slave number.
        //make the output dir
        std::string output_dir;
        if((output_dir = newDirAtCWD(GLOBALFILENAME)) == "")
        {
            std::cout << "Error when make output dir!" << std::endl;
            return;
        }
        std::cout << "output DIR = " << output_dir << std::endl;

        pSAE_nc = std::make_shared<ffnet::NervureConfigure>(ffnet::NervureConfigure("../confs/apps/SdAE_train.ini"));
        divide_into_files(m_oSlaves.size(),pSAE_nc->get<std::string>("path.input-file"),output_dir.c_str());
        psae = SAE_create(pSAE_nc);
        SAE_run(psae,output_dir,pSAE_nc);
        pFFNN_nc = std::make_shared<ffnet::NervureConfigure>(ffnet::NervureConfigure("../confs/apps/FFNN_train.ini"));
        train_NN(psae,pFFNN_nc);
    }



    void onRecvSendFileDirAck(boost::shared_ptr<FileSendDirAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        std::string & slave_path = pMsg->dir();
        //So here, slave_path shows the path on slave point, and pEP show the address of slave point.
        //Send file here!
	file_send(SdAEConfigFileStr,pEP->address().to_string(),slave_path);
        std::cout<<"path on slave "<<slave_path<<std::endl;
        //TODO(sherryshare) using dlopen to open user defined library (UDL), and dlsym to init paramater server!
        send_data_from_dir(static_cast<std::string>("./") + GLOBALFILENAME,pEP->address().to_string(),slave_path);


        //TODO(sherryshare) when the file is sent over, send a CmdStartReq msg!
    }

protected:
    ffnet::NetNervureFromFile &     m_oNNFF;
    std::vector<ffnet::EndpointPtr_t> m_oSlaves;
    std::string SdAEConfigFileStr;
    std::string FBNNConfigFileStr;
    ff::SAE_ptr psae;
    NervureConfigurePtr pSAE_nc;
    NervureConfigurePtr pFFNN_nc;
    
//     void * UDL_handle;
};

int main(int argc, char *argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "dl_master.log");
    //TODO(sherryshare) pass user defined library (UDL) as an argument!
    std::string sae_config_file = "../confs/apps/SdAE_train.ini";
    std::string fbnn_config_file = "../confs/apps/FFNN_train.ini";
    if(argc > 1) {//UDL argc
        std::stringstream ss_argv;
        ss_argv << argv[1];
        ss_argv >> sae_config_file;
    }
    std::cout << "sae config file = " << sae_config_file << std::endl;

    if(argc > 2) {//data path argc
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

    nnff.run();
    return 0;
}
