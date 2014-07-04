#include <network.h>
#include <map>
#include <sstream>
#include "pkgs/pkgs.h"
#include "utils/utils.h"
#include "pkgs/file_send.h"
#include <dlfcn.h>//dymanic library
#include <dirent.h>
#include <boost/any.hpp>

#define GLOBALFILENAME "globalFiles"
#define ORIGININPUT "/home/sherry/ffsae/data/mnist_uint8.mat"

class DLMaster {

public:
    DLMaster(ffnet::NetNervureFromFile & nnff, std::string UDLStr)
        : m_oNNFF(nnff), UDL(UDLStr) {}

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
                }
            }
        }
    }

    bool divideInputData(void * handle,std::string input_file,std::string output_dir)
    {
        // load the symbol
        std::cout << "Loading symbol divide_into_files..." << std::endl;
        typedef void (*divide_into_files_t)(int parts,std::string,std::string);

        // reset errors
        dlerror();
        divide_into_files_t divide_into_files = (divide_into_files_t) dlsym(handle, "divide_into_files");
        const char *dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Cannot load symbol 'divide_into_files': " << dlsym_error << std::endl;
            dlclose(handle);
            return false;
        }

        std::cout << "Calling divide_into_files..." << std::endl;
        divide_into_files(m_oSlaves.size(),input_file,output_dir);

        return true;
    }

    boost::any initParameterServer(void * handle)
    {
        // load the symbol
        std::cout << "Loading symbol SAE_create..." << std::endl;
        typedef boost::any (*SAE_create_t)();

        // reset errors
        dlerror();
        SAE_create_t SAE_create = (SAE_create_t) dlsym(handle, "SAE_create");
        const char *dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Cannot load symbol 'SAE_create': " << dlsym_error << std::endl;
            dlclose(handle);
            return nullptr;
        }

        std::cout << "Calling SAE_create..." << std::endl;
        boost::any psae = SAE_create();

        return psae;
    }

    bool trainSAE(void * handle,boost::any sae_pointer,std::string data_dir)
    {
        // load the symbol
        std::cout << "Loading symbol SAE_run..." << std::endl;
        typedef bool (*SAE_run_t)(boost::any sae_pointer,std::string data_dir);

        // reset errors
        dlerror();
        SAE_run_t SAE_run = (SAE_run_t) dlsym(handle, "SAE_run");
        const char *dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Cannot load symbol 'SAE_run': " << dlsym_error << std::endl;
            dlclose(handle);
            return nullptr;
        }

        std::cout << "Calling SAE_run..." << std::endl;
        return SAE_run(sae_pointer,data_dir);
    }

    bool trainNN(void * handle,boost::any sae_pointer,std::string input_file)
    {
        // load the symbol
        std::cout << "Loading symbol train_NN..." << std::endl;
        typedef void (*train_NN_t)(boost::any sae_pointer,std::string input_file);

        // reset errors
        dlerror();
        train_NN_t train_NN = (train_NN_t) dlsym(handle, "train_NN");
        const char *dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Cannot load symbol 'train_NN': " << dlsym_error << std::endl;
            dlclose(handle);
            return false;
        }

        std::cout << "Calling train_NN..." << std::endl;
        train_NN(sae_pointer,input_file);
        return true;
    }


    inline void * openLibrary(void)
    {
        // open the library
        std::cout << "Opening " << UDL << "..." << std::endl;
        void * handle = dlopen(UDL.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Cannot open library: " << dlerror() << std::endl;
            return nullptr;
        }
        return handle;
    }

    inline void closeLibrary(void * handle)
    {
        // close the library
        std::cout << "Closing library..." << std::endl;
        dlclose(handle);
    }

    inline std::string newDirAtCWD(std::string newFileName)
    {
        std::string output_dir;
        if((output_dir = getcwd(NULL,0)) == "") {
            std::cout << "Error when getcwd!" << std::endl;
            return output_dir;
        }
        output_dir += static_cast<std::string>("/") + newFileName;
        if(access(output_dir.c_str(),F_OK) == -1) {
            mkdir(output_dir.c_str(),S_IRWXU);
        }
        return output_dir;
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
        //Depart data into pieces based on slave number.
        //make the output dir
        std::string output_dir;
        if((output_dir = newDirAtCWD(GLOBALFILENAME)) == "")
        {
            std::cout << "Error when make output dir!" << std::endl;
            return;
        }
        std::cout << "output DIR = " << output_dir << std::endl;
        void * handle = openLibrary();

        divideInputData(handle,ORIGININPUT,output_dir.c_str());
        sae_ptr = initParameterServer(handle);
        trainSAE(handle,sae_ptr,output_dir);
	trainNN(handle,sae_ptr,ORIGININPUT);

        closeLibrary(handle);
    }

    bool send_data_from_dir(std::string input_dir,std::string ip,std::string path)//only read one .part file
    {
        std::string file_name;
        bool retVal = false;
        DIR* dirp;
        struct dirent* direntp;
        dirp = opendir(input_dir.c_str());
        if(dirp != NULL) {
            while((direntp = readdir(dirp)) != NULL) {
                file_name = direntp->d_name;
                std::cout << "file_name = " << file_name << std::endl;
                int dotIndex = file_name.find_last_of('.');
                if(dotIndex != std::string::npos && file_name.substr(dotIndex,file_name.length() - dotIndex) == ".part")
                {
                    std::cout << "find input_file " << file_name << std::endl;
                    file_name = input_dir + "/" + file_name;
                    retVal = true;
                    break;
                }
            }
            closedir(dirp);
            if(file_send(file_name,ip,path))//send file and delete the local version
            {
                remove(file_name.c_str());
                std::cout << "Remove file '" << file_name << "'" << std::endl;
            }
        }
        return retVal;
    }

    void onRecvSendFileDirAck(boost::shared_ptr<FileSendDirAck> pMsg, ffnet::EndpointPtr_t pEP)
    {
        std::string & slave_path = pMsg->dir();
        //So here, slave_path shows the path on slave point, and pEP show the address of slave point.
        //Send file here!
        file_send(UDL,pEP->address().to_string(),slave_path);
        std::cout<<"path on slave "<<slave_path<<std::endl;
        //TODO(sherryshare) using dlopen to open user defined library (UDL), and dlsym to init paramater server!
        send_data_from_dir(static_cast<std::string>("./") + GLOBALFILENAME,pEP->address().to_string(),slave_path);


        //TODO(sherryshare) when the file is sent over, send a CmdStartReq msg!
    }

protected:
    ffnet::NetNervureFromFile &     m_oNNFF;
    std::vector<ffnet::EndpointPtr_t> m_oSlaves;
    std::string UDL;
    boost::any sae_ptr;
};

int main(int argc, char *argv[])
{
    ffnet::Log::init(ffnet::Log::TRACE, "dl_master.log");
    //TODO(sherryshare) pass user defined library (UDL) as an argument!
    std::string UDLStr = "/home/sherry/ffsae/lib/libffsae.so";
    if(argc > 1) {
        std::stringstream ss_argv;
        ss_argv << argv[1];
        ss_argv >> UDLStr;
    }
    std::cout << "User defined library (UDL) = " << UDLStr << std::endl;

    ffnet::NetNervureFromFile nnff("../confs/dl_master_net_conf.ini");
    DLMaster master(nnff,UDLStr);

    nnff.addNeedToRecvPkg<AckNodeMsg>(boost::bind(&DLMaster::onRecvAck, &master, _1, _2));
    nnff.addNeedToRecvPkg<FileSendDirAck>(boost::bind(&DLMaster::onRecvSendFileDirAck, &master, _1, _2));
    ffnet::event::Event<ffnet::event::tcp_get_connection>::listen(&nnff, boost::bind(&DLMaster::onConnSucc, &master, _1));

    nnff.run();
    return 0;
}
