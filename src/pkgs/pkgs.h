#ifndef FFDL_PKGS_PKGS_H_
#define FFDL_PKGS_PKGS_H_


#include <network.h>
#include "common/common.h"
#include "common/types.h"

namespace ff {
enum MsgType {
    msg_heart_beat = 74,
    msg_req_nodes,
    msg_ack_nodes,
    msg_send_file_dir_req,
    msg_send_file_dir_ack,
    msg_file_send,
    msg_cmd_start_req,
    msg_pull_para_req,
    msg_pull_para_ack,
    msg_push_para_req,
    msg_push_para_ack,
};

class HeartBeatMsg : public ffnet::Package
{
public:
    HeartBeatMsg()
        : Package(msg_heart_beat)
        , m_strIP("")
        , m_iTCPPort(0)
    {}

    string_t& ip_addr() {
        return m_strIP;
    }
    const string_t& ip_addr() const {
        return m_strIP;
    }

    uint16_t      tcp_port() const {
        return m_iTCPPort;
    }
    uint16_t&     tcp_port() {
        return m_iTCPPort;
    }
    virtual void                    archive(ffnet::Archive& ar)
    {
        ar.archive(m_strIP);
        ar.archive(m_iTCPPort);
    }

protected:
    string_t    m_strIP;
    uint16_t    m_iTCPPort;
};

class ReqNodeMsg : public ffnet::Package
{
public:
    ReqNodeMsg()
        :Package(msg_req_nodes)
    {}
    virtual void archive(ffnet::Archive& ar) {}
protected:
};

class AckNodeMsg: public ffnet::Package
{
public:
    AckNodeMsg()
        : Package(msg_ack_nodes)
    {}

    virtual void        archive(ffnet::Archive& ar)
    {
        if(ar.is_serializer() || ar.is_lengther())
        {
            size_t len = m_oPoints.size();
            ar.archive(len);
            for(size_t i = 0; i < len; ++i)
            {
                ar.archive(m_oPoints[i]->ip_addr);
                ar.archive(m_oPoints[i]->tcp_port);
            }
        }
        if(ar.is_deserializer())
        {
            size_t len;
            ar.archive(len);
            for(size_t i = 0; i < len; ++i)
            {
                std::string s;
                uint16_t p;
                ar.archive(s);
                ar.archive(p);
                m_oPoints.push_back(slave_point_spt(new slave_point_t(s, p)));
            }
        }
    }

    std::vector<slave_point_spt>&       all_slave_points() {
        return m_oPoints;
    }
    const std::vector<slave_point_spt>&       all_slave_points() const {
        return m_oPoints;
    }

protected:
    std::vector<slave_point_spt> m_oPoints;
};

class FileSendDirReq: public ffnet::Package
{
public:
    FileSendDirReq()
        : Package(msg_send_file_dir_req) {}
    virtual void archive(ffnet::Archive& ar) {}
};//end class FileSendDirReq

class FileSendDirAck : public ffnet::Package
{
public:
    FileSendDirAck()
        : Package(msg_send_file_dir_ack) {}

    std::string&  dir() {
        return m_strDir;
    }
    const std::string& dir() const {
        return m_strDir;
    }

    virtual void archive(ffnet::Archive& ar)
    {
        ar.archive(m_strDir);
    }

protected:
    std::string m_strDir;
};//end class FileSendDirAck

class CmdStartReq: public ffnet::Package
{
public:
    CmdStartReq()
        : Package(msg_cmd_start_req) {}

    std::string& cmd() {
        return m_strCmd;
    }
    const std::string& cmd() const {
        return m_strCmd;
    }

    virtual void archive(ffnet::Archive& ar)
    {
        ar.archive(m_strCmd);
    }
protected:
    std::string m_strCmd;
};

class PullParaReq: public ffnet::Package
{
public:
    PullParaReq()
        : Package(msg_pull_para_req)
    {}

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }

    virtual void archive(ffnet::Archive& ar)
    {
        ar.archive(m_u_sae_index);
    }    

protected:
    int32_t m_u_sae_index;
};

}//end namespace ff
#endif
