#include <network.h>
#include <map>
#include <sstream>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "pkgs/file_send.h"
#include "dsource/divide.h"
#include "sae/sae_from_config.h"

DEF_LOG_MODULE(dl_master)
ENABLE_LOG_MODULE(dl_master)

namespace ff {
class DLMaster {

public:
    DLMaster(ffnet::NetNervureFromFile& nnff, const std::string& sae_config_file, const std::string& fbnn_config_file)
        : m_oNNFF(nnff),
          m_str_sae_configfile(sae_config_file),
          m_str_fbnn_ConfigFile(fbnn_config_file),
          m_iEndPretrain(0),
          m_str_pushhandlefile("push_handle_time.txt"),
          m_str_pullhandlefile("pull_handle_time.txt"),
          m_b_is_occupied(false)
    {}

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
        m_p_sae_nc = NervureConfigurePtr(new ffnet::NervureConfigure("../confs/apps/SdAE_train.ini"));
        divide_into_files(m_oSlaves.size(),getInputFileNameFromNervureConfigure(m_p_sae_nc),".");
        m_p_sae = SAE_create(m_p_sae_nc);
        /*
               //test SAE default error rate
               LOG_TRACE(dl_master) << "Test SAE default error rate.";
               test_SAE(m_p_sae,m_p_sae_nc);
               //train NN with random init value
               LOG_TRACE(dl_master) << "Test FFNN default error rate.";
               m_p_fbnn_nc = NervureConfigurePtr(new ffnet::NervureConfigure("../confs/apps/FFNN_train.ini"));
               train_NN(SAE_ptr((SAE*)NULL),m_p_fbnn_nc);//train a final fbnn after pretraining
               LOG_TRACE(dl_master) << "End test FFNN default error rate.";
               */
    }

    void onRecvSendFileDirAck(boost::shared_ptr<FileSendDirAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        LOG_TRACE(dl_master) << "Receive FileSendDirAck from " << pEP->address().to_string() << ":" << pEP->port();
        std::string& slave_path = pMsg->dir();
        //So here, slave_path shows the path on slave point, and pEP show the address of slave point.
        file_send(m_str_sae_configfile,pEP->address().to_string(),slave_path);//Send sae config file
        std::cout<<"path on slave "<<slave_path<<std::endl;
        //Send divided inputs
        std::string data_dir = send_data_from_dir(".",pEP->address().to_string(),slave_path);
        std::cout << "data dir = " << data_dir << std::endl;
        //Get dl_master server port & ip
        std::string server_addr = m_oNNFF.NervureConf()->get<string_t>("tcp-server.ip");
        uint16_t server_port = m_oNNFF.NervureConf()->get<uint16_t>("tcp-server.port");
        //send a CmdStartReq msg!
        boost::shared_ptr<CmdStartReq> startMsg(new CmdStartReq());
        int startIndex = m_str_sae_configfile.find_last_of('/') + 1;
        std::stringstream ss;
        ss << "./dl_worker " << m_str_sae_configfile.substr(startIndex,m_str_sae_configfile.length() - startIndex);
        ss << " " << data_dir;
        ss << " " << server_addr << " " << server_port;
        //One slave, set local serial train or para train.
        if(m_oSlaves.size() == 1)
        {
//             ss << " " <<false;// Serial
            ss << " " << true;// Parallel
        }
        startMsg->cmd() = ss.str();
        std::cout << "send start Cmd Message!" << std::endl;
        LOG_TRACE(dl_master) << "Send start Cmd message to " << pEP->address().to_string() << ":" << pEP->port();
        m_oNNFF.send(startMsg, pEP);
    }

    void onRecvPullReq(boost::shared_ptr<PullParaReq> pMsg, ffnet::EndpointPtr_t pEP)
    {
        m_b_is_occupied = true;//directly used in serial version
        m_oStartTime = boost::chrono::system_clock::now();
        LOG_TRACE(dl_master) << "Receive pull request from " << pEP->address().to_string() << ":" << pEP->port() <<
                             ", index = " << pMsg->sae_index();
        std::cout << "From " << pEP->address() << ":" << pEP->port() << std::endl;
        std::cout << "Receive pull request index = " << pMsg->sae_index() << std::endl;
        boost::shared_ptr<PullParaAck> ackMsg(new PullParaAck());
        ackMsg->sae_index() = pMsg->sae_index();
        //get current parameters from server
        const std::vector<FMatrix_ptr>& org_Ws = (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->get_m_oWs();
//         const std::vector<FMatrix_ptr>& org_VWs = (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->get_m_oVWs();
        copy(org_Ws.begin(),org_Ws.end(),std::back_inserter(ackMsg->Ws()));
//         copy(org_VWs.begin(),org_VWs.end(),std::back_inserter(ackMsg->VWs()));
        m_oNNFF.send(ackMsg,pEP);
        LOG_TRACE(dl_master) << "Send ack message to " << pEP->address().to_string() << ":" << pEP->port() <<
                             ", index = " << pMsg->sae_index();
        m_oEndTime = boost::chrono::system_clock::now();
        int duration_time = boost::chrono::duration_cast<boost::chrono::milliseconds>(m_oEndTime-m_oStartTime).count();
        m_iPullHandleDurations.push_back(std::make_pair<int,int>(pMsg->sae_index(),duration_time));
        m_b_is_occupied = false;//directly used in serial version
    }

    void onRecvPushReq(boost::shared_ptr<PushParaReq> pMsg, ffnet::EndpointPtr_t pEP)
    {
        m_b_is_occupied = true;//directly used in serial version
        m_oStartTime = boost::chrono::system_clock::now();
        LOG_TRACE(dl_master) << "Receive push request from " << pEP->address().to_string() << ":" << pEP->port() <<
                             ", index = " << pMsg->sae_index();
        std::cout << "From " << pEP->address() << ":" << pEP->port() << std::endl;
        std::cout << "Receive push request index = " << pMsg->sae_index() << std::endl;
        //set odWs
        (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->set_m_odWs(pMsg->dWs());
        //nnapplygrads
        (m_p_sae->get_m_oAEs()[pMsg->sae_index()])->nnapplygrads();//read odWs, write oWs and oVWs
        boost::shared_ptr<PushParaAck> ackMsg(new PushParaAck());
        ackMsg->sae_index() = pMsg->sae_index();
        m_oNNFF.send(ackMsg,pEP);
        LOG_TRACE(dl_master) << "Send ack message to " << pEP->address().to_string() << ":" << pEP->port() <<
                             ", index = " << pMsg->sae_index();
        m_oEndTime = boost::chrono::system_clock::now();
        int duration_time = boost::chrono::duration_cast<boost::chrono::milliseconds>(m_oEndTime-m_oStartTime).count();
        m_iPushHandleDurations.push_back(std::make_pair<int,int>(pMsg->sae_index(),duration_time));
        bool bTestAfterPush = false;
//         bTestAfterPush = true;//Annotate if needn't test
        if(m_oSlaves.size() == 1 && bTestAfterPush)
        {
            //test SAE error rate after push
            LOG_TRACE(dl_master) << "Test SAE error rate ";
            test_SAE(m_p_sae,m_p_sae_nc);
            //train NN with random init value
            LOG_TRACE(dl_master) << "Test FFNN error rate after push.";
//         m_p_fbnn_nc = NervureConfigurePtr(new ffnet::NervureConfigure("../confs/apps/FFNN_train.ini"));
            train_NN(SAE_ptr((SAE*)NULL),m_p_fbnn_nc);//train a final fbnn after pretraining
            LOG_TRACE(dl_master) << "End test FFNN error rate after push.";
        }
        m_b_is_occupied = false;//directly used in serial version
    }

    void onRecvEndTrain(boost::shared_ptr<NodeTrainEnd> pMsg, ffnet::EndpointPtr_t pEP)
    {
        LOG_TRACE(dl_master) << "Receive end train message from " << pEP->address().to_string() << ":" << pEP->port();
        std::cout << pEP->address() << ":" << pEP->port();
        std::cout << " node SAE train ends." << std::endl;
        ++m_iEndPretrain;//Every node sends only one end message.
        if(m_iEndPretrain == m_oSlaves.size())//worker number depends on slave number
        {
            std::cout << "Ready to train a FFNN." << std::endl;
            recordDurationTime(m_iPullHandleDurations,m_str_pullhandlefile);
            recordDurationTime(m_iPushHandleDurations,m_str_pushhandlefile);
            /*
            //test SAE error rate
            test_SAE(m_p_sae,m_p_sae_nc);
            //train FFNN
            if(pMsg->startNNTrain())//1 slave run alone
            {
                m_p_fbnn_nc = NervureConfigurePtr(new ffnet::NervureConfigure("../confs/apps/FFNN_train.ini"));
                train_NN(m_p_sae,m_p_fbnn_nc);//train a final fbnn after pretraining
                LOG_TRACE(dl_master) << "End training FFNN.";
            }*/
        }
    }

    void onRecvPushResourceReq(boost::shared_ptr<PushResourceReq> pMsg, ffnet::EndpointPtr_t pEP)
    {
        LOG_TRACE(dl_master) << "Receive push resource req from " << pEP->address().to_string() << ":" << pEP->port();
        boost::shared_ptr<PushResourceAck> ackMsg(new PushResourceAck(pMsg->sae_index(),false));// default unavailable
        if(!m_b_is_occupied) {
            ackMsg->resource_available() = true;//Annotate if want to send unavailable
            m_b_is_occupied = true;
        }
        m_oNNFF.send(ackMsg,pEP);
    }

    void onRecvPullResourceReq(boost::shared_ptr<PullResourceReq> pMsg, ffnet::EndpointPtr_t pEP)
    {
        LOG_TRACE(dl_master) << "Receive pull resource req from " << pEP->address().to_string() << ":" << pEP->port();
        boost::shared_ptr<PullResourceAck> ackMsg(new PullResourceAck(pMsg->sae_index(),false));// default unavailable
        if(!m_b_is_occupied) {
            ackMsg->resource_available() = true;//Annotate if want to send unavailable
            m_b_is_occupied = true;
        }
        m_oNNFF.send(ackMsg,pEP);
    }

protected:
    typedef boost::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
    ffnet::NetNervureFromFile&     m_oNNFF;
    std::vector<ffnet::EndpointPtr_t> m_oSlaves;
    std::string m_str_sae_configfile;
    std::string m_str_fbnn_ConfigFile;
    const std::string m_str_pushhandlefile;
    const std::string m_str_pullhandlefile;
    ff::SAE_ptr m_p_sae;
    NervureConfigurePtr m_p_sae_nc;
    NervureConfigurePtr m_p_fbnn_nc;
    int m_iEndPretrain;
    TimePoint m_oStartTime;
    TimePoint m_oEndTime;
    std::vector<std::pair<int,int> > m_iPushHandleDurations;
    std::vector<std::pair<int,int> > m_iPullHandleDurations;
    bool m_b_is_occupied;
};

}//end namespace ff

using namespace ff;

bool bNetNervureIsStopped = false;

void  press_and_stop(ffnet::NetNervureFromFile& nnff)
{

    std::cout<<"Press Q to quit..."<<std::endl;
    while(getc(stdin)!='Q');
    bNetNervureIsStopped = true;
    nnff.stop();
    std::cout<<"Stopping, please wait..."<<std::endl;
}

void  trace_queue_length(ffnet::NetNervureFromFile& nnff)
{
    while(!bNetNervureIsStopped)
    {
        size_t s = nnff.getTaskQueue().size();
        LOG_TRACE(dl_master) << "Task queue length = " << s;
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
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
    nnff.addNeedToRecvPkg<PullParaReq>(boost::bind(&DLMaster::onRecvPullReq, &master, _1, _2));
    nnff.addNeedToRecvPkg<PushParaReq>(boost::bind(&DLMaster::onRecvPushReq, &master, _1, _2));
    nnff.addNeedToRecvPkg<NodeTrainEnd>(boost::bind(&DLMaster::onRecvEndTrain, &master, _1, _2));
    //add serial scheduler
    nnff.addNeedToRecvPkg<PushResourceReq>(boost::bind(&DLMaster::onRecvPushResourceReq, &master, _1, _2));
    nnff.addNeedToRecvPkg<PullResourceReq>(boost::bind(&DLMaster::onRecvPullResourceReq, &master, _1, _2));

    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&DLMaster::onConnSucc, &master, _1));

    boost::thread monitor_thrd(boost::bind(press_and_stop, boost::ref(nnff)));
    boost::thread trace_thrd(boost::bind(trace_queue_length, boost::ref(nnff)));
    nnff.run();
    monitor_thrd.join();
    trace_thrd.join();
    return 0;
}
