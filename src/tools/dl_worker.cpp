#include <network.h>
#include <map>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "sae/sae_from_config.h"

using namespace ff;
class DLWorker {

public:
    DLWorker(ffnet::NetNervureFromFile& nnff, 
             const std::string& sae_config_file, 
             const std::string& input_data_file,
             const std::string& server_ip,
             const uint16_t server_port)
        : m_oNNFF(nnff),
          m_str_sae_configfile(sae_config_file),
          m_str_inputfile(input_data_file),
          m_str_server_ip(server_ip),
          m_u_server_port(server_port) {}

    void    onConnSucc(ffnet::TCPConnectionBase*pConn)
    {
        ffnet::EndpointPtr_t it = pConn->getRemoteEndpointPtr();
        std::string master_addr = m_oNNFF.NervureConf()->get<std::string>("tcp-client.target-svr-ip-addr");
        uint16_t port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-client.target-svr-port");

        //to check if it's a connection from rm_master
        if(it->address().to_string() == master_addr &&
                it->port() == port)
        {
            ffnet::EndpointPtr_t sp(
                new ffnet::Endpoint(
                    boost::asio::ip::tcp::endpoint(
                        boost::asio::ip::address::from_string(m_str_server_ip), m_u_server_port
                    )));
            m_oDLMaster = sp;
            m_oNNFF.addTCPClient(sp);
            std::cout << "add TCP Client: " << m_str_server_ip <<" : "<< m_u_server_port << std::endl;
        }
        else if(it->address().to_string() == m_str_server_ip &&
                it->port() == m_u_server_port){
            std::string strDir = static_cast<std::string>("./") + globalDirStr + "/";
            m_p_sae_nc = NervureConfigurePtr(new ffnet::NervureConfigure(strDir + m_str_sae_configfile));
            m_p_sae = SAE_create(m_p_sae_nc);
            SAE_run(m_p_sae,strDir + m_str_inputfile,m_p_sae_nc);//run an sae
        }
    }

protected:
    typedef boost::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
    ffnet::NetNervureFromFile&     m_oNNFF;
    ffnet::EndpointPtr_t m_oDLMaster;
    const std::string& m_str_sae_configfile;
    const std::string& m_str_inputfile;
    const std::string& m_str_server_ip;
    const uint16_t m_u_server_port;
    ff::SAE_ptr m_p_sae;
    NervureConfigurePtr m_p_sae_nc;
};



void  press_and_stop(ffnet::NetNervureFromFile& nnff)
{
    std::cout<<"Press any key to quit..."<<std::endl;
    getc(stdin);
    nnff.stop();
    std::cout<<"Stopping, please wait..."<<std::endl;
}

int main(int argc, char* argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "dl_worker.log");
    std::string sae_config_file = "globalFiles/SdAE_train.ini";
    std::string input_data_file = globalDirStr;
    std::string server_ip;
    uint16_t server_port;
    if(argc > 1) {//sae_config_file argc
        std::stringstream ss_argv;
        ss_argv << argv[1];
        ss_argv >> sae_config_file;
    }
    std::cout << "sae config file = " << sae_config_file << std::endl;

    if(argc > 2) {//input_data_file argc
        std::stringstream ss_argv;
        ss_argv << argv[2];
        ss_argv >> input_data_file;
    }
    std::cout << "input data file = " << input_data_file << std::endl;
    
    if(argc > 3) {//server_ip argc
        std::stringstream ss_argv;
        ss_argv << argv[3];
        ss_argv >> server_ip;
    }
    
    if(argc > 4) {//server_port argc
        std::stringstream ss_argv;
        ss_argv << argv[4];
        ss_argv >> server_port;
    }
    std::cout << "Server " << server_ip << " : " << server_port << std::endl;

    ffnet::NetNervureFromFile nnff("../confs/dl_worker_net_conf.ini");
    DLWorker worker(nnff,sae_config_file,input_data_file,server_ip,server_port);

//     nnff.addNeedToRecvPkg<AckParaServerMsg>(boost::bind(&DLWorker::onRecvAck, &worker, _1, _2));
//     nnff.addNeedToRecvPkg<FileSendDirAck>(boost::bind(&DLWorker::onRecvSendFileDirAck, &master, _1, _2));
    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&DLWorker::onConnSucc, &worker, _1));

    boost::thread monitor_thrd(boost::bind(press_and_stop, boost::ref(nnff)));
    nnff.run();
    monitor_thrd.join();
    return 0;
}
