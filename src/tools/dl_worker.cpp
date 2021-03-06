#include <network.h>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "sae/sae_from_config.h"

DEF_LOG_MODULE(dl_worker)
ENABLE_LOG_MODULE(dl_worker)

namespace ff {
class DLWorker {

public:
    DLWorker(ffnet::NetNervureFromFile& nnff,
             const std::string& sae_config_file,
             const std::string& input_data_file,
             const std::string& server_ip,
             const uint16_t server_port,
             const bool bIsPara
            )
        : m_oNNFF(nnff),
          m_str_sae_configfile(sae_config_file),
          m_str_inputfile(input_data_file),
          m_str_server_ip(server_ip),
          m_u_server_port(server_port),
          m_b_isPara(bIsPara),
          m_str_pushtimefile("push_time.txt"),
          m_str_pullfimefile("pull_time.txt")
    {}

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
                it->port() == m_u_server_port) {
            m_p_sae_nc = NervureConfigurePtr(new ffnet::NervureConfigure(m_str_sae_configfile));
            m_p_sae = SAE_create(m_p_sae_nc);
            SAE_run(it);
        }
    }

    void FBNN_run(const ffnet::EndpointPtr_t& pEP)
    {
        /* training FFNN local version*/
        m_oStartTime = boost::chrono::system_clock::now();
        NervureConfigurePtr m_p_fbnn_nc = NervureConfigurePtr(new ffnet::NervureConfigure("../confs/apps/FFNN_train.ini"));
        train_NN(m_p_sae,m_p_fbnn_nc);//train a final fbnn after pretraining
        m_oEndTime = boost::chrono::system_clock::now();
        int duration_time = boost::chrono::duration_cast<boost::chrono::minutes>(m_oEndTime-m_oStartTime).count();
        std::cout << "Duration time = " << duration_time << "min" << std::endl;
        boost::shared_ptr<NodeTrainEnd> endMsg(new NodeTrainEnd());
        endMsg->startNNTrain() = false;//No need to train NN on master
        m_oNNFF.send(endMsg,pEP);
        m_oNNFF.stop();
        std::cout<<"Leaving dl_worker..."<<std::endl;
    }

    bool SAE_run(const ffnet::EndpointPtr_t& pEP)
    {
        FMatrix_ptr train_x = read_matrix_from_dir(m_str_inputfile);
        if(train_x == NULL)
            return false;
        *train_x = (*train_x) / 255;
        Opts opts;
        getOptsFromNervureConfigure(m_p_sae_nc,opts);
        if(m_b_isPara)
        {
            /* para version */
            const int32_t maxSynchronicStep = getMaxSynchronicStepFromNervureConfigure(m_p_sae_nc);
            int32_t stepControl = getStepControlFromNervureConfigure(m_p_sae_nc);
            if(stepControl > maxSynchronicStep)
                stepControl = maxSynchronicStep;
            std::cout << "Accepted step control = " << stepControl << std::endl;
            bool bResourceControl = false;
            if(stepControl == 0)//random control
            {
                bResourceControl = getResourceControlFromNervureConfigure(m_p_sae_nc);
            }
            m_p_sae->SAETrain(*train_x,opts,m_oNNFF,m_oDLMaster,m_oStartTime,stepControl-1,bResourceControl);
            /* end para version */
        }
        else
        {
            /* serial version */
            m_oStartTime = boost::chrono::system_clock::now();
            m_p_sae->SAETrain(*train_x,opts);//serial train
            m_oEndTime = boost::chrono::system_clock::now();
            int duration_time = boost::chrono::duration_cast<boost::chrono::minutes>(m_oEndTime-m_oStartTime).count();
            std::cout << "Duration time = " << duration_time << "min" << std::endl;
            test_SAE(m_p_sae,m_p_sae_nc);
            FBNN_run(pEP);
            /* end serial version */
        }
        return true;
    }

    void onRecvPullAck(boost::shared_ptr<PullParaAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        m_oEndTime = boost::chrono::system_clock::now();
        LOG_TRACE(dl_worker) << "Receive pull ack from " << pEP->address().to_string() << ":" << pEP->port() << 
                            ", index = " << pMsg->sae_index();
        std::cout << "Receive pull ack! Index = " << pMsg->sae_index() << std::endl;
        int duration_time = boost::chrono::duration_cast<boost::chrono::milliseconds>(m_oEndTime-m_oStartTime).count();
        m_iPullDurations.push_back(std::make_pair<int,int>(pMsg->sae_index(),duration_time));
        std::cout << "Duration time = " << duration_time << "ms" << std::endl;
//         for(int i = 0; i < pMsg->Ws().size(); ++i)
//             std::cout << pMsg->Ws()[i]->operator()(0,0) << std::endl;
        (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->set_m_oWs(pMsg->Ws());
//         (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->set_m_oVWs(pMsg->VWs());
        (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->train_after_pull(pMsg->sae_index(),m_oNNFF,m_oDLMaster,m_oStartTime);
    }

    void onRecvPushAck(boost::shared_ptr<PushParaAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        m_oEndTime = boost::chrono::system_clock::now();
        LOG_TRACE(dl_worker) << "Receive push ack from " << pEP->address().to_string() << ":" << pEP->port() << 
                            ", index = " << pMsg->sae_index();
        std::cout << "Receive push ack! " << pMsg->sae_index() << std::endl;
        int duration_time = boost::chrono::duration_cast<boost::chrono::milliseconds>(m_oEndTime-m_oStartTime).count();
        m_iPushDurations.push_back(std::make_pair<int,int>(pMsg->sae_index(),duration_time));
        std::cout << "Duration time = " << duration_time << "ms" << std::endl;
        bool b_AEIsEnd = (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->train_after_push(pMsg->sae_index(),m_oNNFF,m_oDLMaster,m_oStartTime);
        bool b_SAEIsEnd;
        if(b_AEIsEnd)
            b_SAEIsEnd = m_p_sae->train_after_end_AE(m_oNNFF,m_oDLMaster,m_oStartTime);
        if(b_SAEIsEnd) {
            boost::shared_ptr<NodeTrainEnd> endMsg(new NodeTrainEnd());
//             endMsg->startNNTrain() = false;// add when no need to train NN
            m_oNNFF.send(endMsg,pEP);
            m_oNNFF.stop();
            std::cout<<"Leaving dl_worker..."<<std::endl;
            recordDurationTime(m_iPullDurations,m_str_pullfimefile);
            recordDurationTime(m_iPushDurations,m_str_pushtimefile);
        }
    }

protected:
    typedef boost::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
//     typedef boost::chrono::time_point<boost::chrono::system_clock> TimePoint;
    ffnet::NetNervureFromFile&     m_oNNFF;
    ffnet::EndpointPtr_t m_oDLMaster;
    const std::string& m_str_sae_configfile;
    const std::string& m_str_inputfile;
    const std::string& m_str_server_ip;
    const std::string m_str_pushtimefile;
    const std::string m_str_pullfimefile;
    const uint16_t m_u_server_port;
    ff::SAE_ptr m_p_sae;
    NervureConfigurePtr m_p_sae_nc;
    TimePoint m_oStartTime;
    TimePoint m_oEndTime;
    std::vector<std::pair<int,int> > m_iPushDurations;
    std::vector<std::pair<int,int> > m_iPullDurations;
    bool m_b_isPara;
};

}//end namespace ff

using namespace ff;
// void  press_and_stop(ffnet::NetNervureFromFile& nnff)
// {
//     std::cout<<"Press any key to quit..."<<std::endl;
//     getc(stdin);
//     nnff.stop();
//     std::cout<<"Stopping, please wait..."<<std::endl;
// }

int main(int argc, char* argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "dl_worker.log");
    std::string sae_config_file = "globalFiles/SdAE_train.ini";
    std::string input_data_file = globalDirStr;
    std::string server_ip;
    uint16_t server_port;
    bool bIsPara = true;
    if(argc > 1) {//sae_config_file argc
        std::stringstream ss_argv;
        ss_argv << argv[1];
        ss_argv >> sae_config_file;
        sae_config_file = static_cast<std::string>("./") + globalDirStr + "/" + sae_config_file;
    }
    std::cout << "sae config file = " << sae_config_file << std::endl;

    if(argc > 2) {//input_data_file argc
        std::stringstream ss_argv;
        ss_argv << argv[2];
        ss_argv >> input_data_file;
        input_data_file = static_cast<std::string>("./") + globalDirStr + "/" + input_data_file;
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
    if(argc > 5) {//bIsPara argc
        std::stringstream ss_argv;
        ss_argv << argv[5];
        ss_argv >> bIsPara;
    }

    ffnet::NetNervureFromFile nnff("../confs/dl_worker_net_conf.ini");
    DLWorker worker(nnff,sae_config_file,input_data_file,server_ip,server_port,bIsPara);

    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&DLWorker::onConnSucc, &worker, _1));
    nnff.addNeedToRecvPkg<PullParaAck>(boost::bind(&DLWorker::onRecvPullAck, &worker, _1, _2));
    nnff.addNeedToRecvPkg<PushParaAck>(boost::bind(&DLWorker::onRecvPushAck, &worker, _1, _2));

//     boost::thread monitor_thrd(boost::bind(press_and_stop, boost::ref(nnff)));
    nnff.run();
//     monitor_thrd.join();
    return 0;
}
