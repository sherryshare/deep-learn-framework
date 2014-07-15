#ifndef FFDL_COMMON_TYPES_H_
#define FFDL_COMMON_TYPES_H_

#include <string>
#include <memory>
#include <boost/shared_ptr.hpp>

namespace ff{

typedef std::string string_t;

struct slave_point_t{
    slave_point_t(const string_t& str, uint16_t p)
        : ip_addr(str)
        , tcp_port(p){}

    string_t    ip_addr;
    uint16_t    tcp_port;
};

typedef boost::shared_ptr<slave_point_t> slave_point_spt;
typedef slave_point_t* slave_point_pt;

typedef  boost::uint16_t uint16_t;

}//end namespace ff

#endif