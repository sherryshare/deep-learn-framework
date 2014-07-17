#include <network.h>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "sae/sae_from_config.h"

namespace ff {
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
                it->port() == m_u_server_port) {
            m_p_sae_nc = NervureConfigurePtr(new ffnet::NervureConfigure(m_str_sae_configfile));
            m_p_sae = SAE_create(m_p_sae_nc);
            SAE_run(m_p_sae,m_str_inputfile,m_p_sae_nc,it);//run an sae
        }
    }

    bool SAE_run(void)
    {
        FMatrix_ptr train_x = read_matrix_from_dir(m_str_inputfile);
        if(train_x == NULL)
            return false;
        *train_x = (*train_x) / 255;
        Opts opts;
        getOptsFromNervureConfigure(m_p_sae_nc,opts);
        m_p_sae->SAETrain(*train_x,opts,this);
        return true;
    }

    friend void FBNN::train(const FMatrix& train_x,
//                     const FMatrix& train_y ,
                            const Opts& opts,
                            const DLWorker* pDLWorker,
                            const int32_t sae_index);

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

//trains a neural net
void FBNN::train(const FMatrix& train_x,
//                     const FMatrix& train_y ,
                 const Opts& opts,
                 const DLWorker* pDLWorker,
                 const int32_t sae_index)
{
    int32_t ibatchNum = train_x.rows() / opts.batchsize + (train_x.rows() % opts.batchsize != 0);
    FMatrix L = zeros(opts.numpochs * ibatchNum, 1);
    m_oLp = FMatrix_ptr(new FMatrix(L));
    Loss loss;
//       std::cout << "numpochs = " << opts.numpochs << std::endl;
    for(int32_t i = 0; i < opts.numpochs; ++i)
    {
        std::cout << "start numpochs " << i << std::endl;
        //int32_t elapsedTime = count_elapse_second([&train_x,&train_y,&L,&opts,i,pFBNN,ibatchNum,this] {
        std::vector<int32_t> iRandVec;
        randperm(train_x.rows(),iRandVec);
        std::cout << "start batch: ";
        for(int32_t j = 0; j < ibatchNum; ++j)
        {
            std::cout << " " << j;
            //pull under network conditions
            std::cout << "Need to pull weights!" << std::endl;
            boost::shared_ptr<PullParaReq> pullReqMsg(new PullParaReq());
            pullReqMsg->sae_index() = sae_index;
//                 boost::shared_lock<RWMutex> rlock(pFBNN->W_RWMutex);
//                 set_m_oWs(pFBNN->get_m_oWs());
//                 if(double_larger_than_zero(m_fMomentum))
//                     set_m_oVWs(pFBNN->get_m_oVWs());
//                 rlock.unlock();


            int32_t curBatchSize = opts.batchsize;
            if(j == ibatchNum - 1 && train_x.rows() % opts.batchsize != 0)
                curBatchSize = train_x.rows() % opts.batchsize;
            FMatrix batch_x(curBatchSize,train_x.columns());
            for(int32_t r = 0; r < curBatchSize; ++r)//randperm()
                row(batch_x,r) = row(train_x,iRandVec[j * opts.batchsize + r]);

            //Add noise to input (for use in denoising autoencoder)
            if(m_fInputZeroMaskedFraction != 0)
                batch_x = bitWiseMul(batch_x,(rand(curBatchSize,train_x.columns())>m_fInputZeroMaskedFraction));

            FMatrix batch_y(curBatchSize,train_y.columns());
            for(int32_t r = 0; r < curBatchSize; ++r)//randperm()
                row(batch_y,r) = row(train_y,iRandVec[j * opts.batchsize + r]);

            L(i*ibatchNum+j,0) = nnff(batch_x,batch_y);
            nnbp();
            nnapplygrads();
            //push under network conditions
            std::cout << "Need to push weights!" << std::endl;
//                 boost::unique_lock<RWMutex> wlock(pFBNN->W_RWMutex);
//                 pFBNN->set_m_odWs(m_odWs);
//                 pFBNN->nnapplygrads();
//                 wlock.unlock();
//            std::cout << "end batch " << j << std::endl;
        }
        std::cout << std::endl;
        //});
        //std::cout << "elapsed time: " << elapsedTime << "s" << std::endl;
        //loss calculate use nneval
        if(valid_x.rows() == 0 || valid_y.rows() == 0) {
            nneval(loss, train_x, train_y);
            std::cout << "Full-batch train mse = " << loss.train_error.back() << std::endl;
        }
        else {
            nneval(loss, train_x, train_y, valid_x, valid_y);
            std::cout << "Full-batch train mse = " << loss.train_error.back() << " , val mse = " << loss.valid_error.back() << std::endl;
        }
        //std::cout << "epoch " << i+1 << " / " <<  opts.numpochs << " took " << elapsedTime << " seconds." << std::endl;
        std::cout << "Mini-batch mean squared error on training set is " << columnMean(submatrix(L,i*ibatchNum,0UL,ibatchNum,L.columns())) << std::endl;
        m_fLearningRate *= m_fScalingLearningRate;

//        std::cout << "end numpochs " << i << std::endl;
    }

}
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
    ffnet::Log::init(ffnet::Log::TRACE, "dl_worker.log");
    std::string sae_config_file = "globalFiles/SdAE_train.ini";
    std::string input_data_file = globalDirStr;
    std::string server_ip;
    uint16_t server_port;
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
