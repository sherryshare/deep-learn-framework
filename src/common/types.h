#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <boost/shared_ptr.hpp>


typedef std::string string_t;

struct slave_point_t{
    slave_point_t(const string_t & str, uint16_t p)
        : ip_addr(str)
        , tcp_port(p){}

    string_t    ip_addr;
    uint16_t    tcp_port;
};

typedef boost::shared_ptr<slave_point_t> slave_point_spt;
typedef slave_point_t * slave_point_pt;

typedef  std::uint16_t uint16_t;
