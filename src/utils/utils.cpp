#include "utils/utils.h"
#include <boost/asio.hpp>
#include <cassert>

namespace ff {
std::string endpoint_to_string(ffnet::EndpointPtr_t pEP)
{
    std::stringstream ss;
    ss<<pEP->address().to_string()<<pEP->port();
    return ss.str();
}

using boost::asio::ip::tcp;
std::string local_ip_v4()
{
    try
    {
        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(boost::asio::ip::host_name(),"");
        tcp::resolver::iterator it=resolver.resolve(query);

        while(it!=tcp::resolver::iterator())
        {
            boost::asio::ip::address addr=(++it)->endpoint().address();
            if(addr.is_v6())
            {
//                std::cout<<"ipv6 address: ";
            }
            else
                return addr.to_string();
        }
    }
    catch(std::exception& e)
    {
        std::cout<<e.what()<<std::endl;
    }
    std::cout<<"cannot get local ip addr !"<<std::endl;
    exit(-1);
    return "";
}

bool recordDurationTime(std::vector<std::pair<int,int> >& recordVec,const std::string& outFileName)
{
    std::ofstream output_file(outFileName.c_str());
    if(!output_file.is_open()) {
        std::cout<<"Failed to open file: "<< outFileName << std::endl;
        return false;
    }
    else {
        std::cout << "Write file: " << outFileName << std::endl;
        for(std::vector<std::pair<int,int> >::iterator iter = recordVec.begin(); iter!= recordVec.end(); ++iter)
        {
            output_file << iter->first << "\t" << iter->second << std::endl;
        }
    }
    output_file.close();
    return true;
}

}//end namespace ff
