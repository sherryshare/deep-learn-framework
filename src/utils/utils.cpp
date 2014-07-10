#include "utils/utils.h"
#include <boost/asio.hpp>
#include <cassert>

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
            boost::asio::ip::address addr=(it++)->endpoint().address();
            if(addr.is_v6())
            {
//                std::cout<<"ipv6 address: ";
            }
            else
                return addr.to_string();
        }
    }
    catch(std::exception &e)
    {
        std::cout<<e.what()<<std::endl;
    }
    std::cout<<"cannot get local ip addr !"<<std::endl;
    exit(-1);
    return "";
}

namespace ff{
int count_elapse_microsecond(const std::function<void ()> & f)
{
    using namespace std::chrono;

    time_point<system_clock> start, end;

    start = system_clock::now();
    f();
    end = system_clock::now();
    return duration_cast<microseconds>(end-start).count();
}

int count_elapse_second(const std::function<void ()> & f)
{
    using namespace std::chrono;

    time_point<system_clock> start, end;

    start = system_clock::now();
    f();
    end = system_clock::now();
    return duration_cast<seconds>(end-start).count();
}
};//end namespace ff
