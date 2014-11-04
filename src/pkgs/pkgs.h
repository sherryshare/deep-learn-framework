#ifndef FFDL_PKGS_PKGS_H_
#define FFDL_PKGS_PKGS_H_


#include <network.h>
#include "common/common.h"
#include "common/types.h"
#include <dtype/type.h>

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
    msg_node_train_end,
    msg_pull_resource_req,
    msg_pull_resource_ack,
    msg_push_resource_req,
    msg_push_resource_ack,
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
            int32_t len = m_oPoints.size();
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                ar.archive(m_oPoints[i]->ip_addr);
                ar.archive(m_oPoints[i]->tcp_port);
            }
        }
        if(ar.is_deserializer())
        {
            int32_t len;
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
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
        , m_u_sae_index(0)
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

void serialize_FMatrix(ffnet::Archive& ar,const FMatrix_ptr& pMat);

FMatrix deserialize_FMatrix(ffnet::Archive& ar);

class PullParaAck: public ffnet::Package
{
public:
    PullParaAck()
        : Package(msg_pull_para_ack)
        , m_u_sae_index(0)
    {}

    virtual void archive(ffnet::Archive& ar)
    {
        if(ar.is_serializer() || ar.is_lengther())
        {
            ar.archive(m_u_sae_index);
            int32_t len = m_oWs.size();
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                serialize_FMatrix(ar,m_oWs[i]);
            }
            /*len = m_oVWs.size();
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                serialize_FMatrix(ar,m_oVWs[i]);
            }       */
        }
        if(ar.is_deserializer())
        {
            ar.archive(m_u_sae_index);
            int32_t len;
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                m_oWs.push_back(FMatrix_ptr(new FMatrix(deserialize_FMatrix(ar))));
            }
            /*ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                m_oVWs.push_back(FMatrix_ptr(new FMatrix(deserialize_FMatrix(ar))));
            }      */
        }
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }
    const std::vector<FMatrix_ptr>& Ws() const {
        return m_oWs;
    };
    std::vector<FMatrix_ptr>& Ws() {
        return m_oWs;
    };
//     const std::vector<FMatrix_ptr>& VWs() const {
//         return m_oVWs;
//     };
//     std::vector<FMatrix_ptr>& VWs() {
//         return m_oVWs;
//     };

protected:
    int32_t m_u_sae_index;
    std::vector<FMatrix_ptr>  m_oWs;
//     std::vector<FMatrix_ptr>  m_oVWs;
};

class PushParaReq: public ffnet::Package
{
public:
    PushParaReq()
        : Package(msg_push_para_req)
        , m_u_sae_index(0)
    {}

    virtual void archive(ffnet::Archive& ar)
    {
        if(ar.is_serializer() || ar.is_lengther())
        {
            ar.archive(m_u_sae_index);
            int32_t len = m_odWs.size();
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                serialize_FMatrix(ar,m_odWs[i]);
            }
        }
        if(ar.is_deserializer())
        {
            ar.archive(m_u_sae_index);
            int32_t len;
            ar.archive(len);
            for(int32_t i = 0; i < len; ++i)
            {
                m_odWs.push_back(FMatrix_ptr(new FMatrix(deserialize_FMatrix(ar))));
            }
        }
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }
    const std::vector<FMatrix_ptr>& dWs() const {
        return m_odWs;
    };
    std::vector<FMatrix_ptr>& dWs() {
        return m_odWs;
    };

protected:
    int32_t m_u_sae_index;
    std::vector<FMatrix_ptr>  m_odWs;
};

class PushParaAck : public ffnet::Package
{
public:
    PushParaAck()
        :Package(msg_push_para_ack)
        , m_u_sae_index(0)
    {}
    virtual void archive(ffnet::Archive& ar) {
        ar.archive(m_u_sae_index);
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }
protected:
    int32_t m_u_sae_index;
};

class NodeTrainEnd : public ffnet::Package
{
public:
    NodeTrainEnd()
        :Package(msg_node_train_end)
        , m_b_start_NNTrain(true)
    {}
    virtual void archive(ffnet::Archive& ar) {
        ar.archive(m_b_start_NNTrain);
    }

    bool& startNNTrain() {
        return m_b_start_NNTrain;
    }

    const bool& startNNTrain() const {
        return m_b_start_NNTrain;
    }

protected:
    bool m_b_start_NNTrain;
};

class PullResourceReq : public ffnet::Package
{
public:
    PullResourceReq()
        :Package(msg_pull_resource_req)
    {}
    PullResourceReq(int32_t saeIndex)
        :Package(msg_pull_resource_req)
        ,m_u_sae_index(saeIndex)
    {}
    virtual void archive(ffnet::Archive& ar) {
        ar.archive(m_u_sae_index);
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }
protected:
    int32_t m_u_sae_index;
};

class PullResourceAck : public ffnet::Package
{
public:
    PullResourceAck()
        :Package(msg_pull_resource_ack)
    {}
    PullResourceAck(int32_t saeIndex, bool bIsAvailable)
        :Package(msg_pull_resource_ack)
        ,m_u_sae_index(saeIndex)
        , m_b_resource_available(bIsAvailable)

    {}
    virtual void archive(ffnet::Archive& ar) {
        ar.archive(m_u_sae_index);
        ar.archive(m_b_resource_available);
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }

    bool & resource_available() {
        return m_b_resource_available;
    }
    const bool & resource_available() const {
        return m_b_resource_available;
    }
protected:
    int32_t m_u_sae_index;
    bool m_b_resource_available;
};

class PushResourceReq : public ffnet::Package
{
public:
    PushResourceReq()
        :Package(msg_push_resource_req)
    {}
    PushResourceReq(int32_t saeIndex)
        :Package(msg_push_resource_req)
        ,m_u_sae_index(saeIndex)
    {}
    virtual void archive(ffnet::Archive& ar) {
        ar.archive(m_u_sae_index);
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }
protected:
    int32_t m_u_sae_index;
};

class PushResourceAck : public ffnet::Package
{
public:
    PushResourceAck()
        :Package(msg_push_resource_ack)
    {}
    PushResourceAck(int32_t saeIndex, bool bIsAvailable)
        :Package(msg_push_resource_ack)
        ,m_u_sae_index(saeIndex)
        , m_b_resource_available(bIsAvailable)
    {}
    virtual void archive(ffnet::Archive& ar) {
        ar.archive(m_u_sae_index);
        ar.archive(m_b_resource_available);
    }

    int32_t& sae_index() {
        return m_u_sae_index;
    }
    const int32_t& sae_index() const {
        return m_u_sae_index;
    }

    bool & resource_available() {
        return m_b_resource_available;
    }
    const bool & resource_available() const {
        return m_b_resource_available;
    }
protected:
    int32_t m_u_sae_index;
    bool m_b_resource_available;
};


}//end namespace ff
#endif
