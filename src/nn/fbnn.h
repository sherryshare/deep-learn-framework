#ifndef FFDL_NN_FBNN_H_
#define FFDL_NN_FBNN_H_

#include "common/common.h"
#include "nn/arch.h"
#include "nn/opt.h"
#include "utils/math.h"
#include "nn/loss.h"
#include "utils/utils.h"
//mutex
// #include <boost/thread/locks.hpp>
// #include <boost/thread/shared_mutex.hpp>
// #include <boost/thread/condition.hpp>

#include <boost/chrono.hpp>//time

#include "pkgs/pkgs.h"


DEF_LOG_MODULE(fbnn)//log
ENABLE_LOG_MODULE(fbnn)

namespace ff
{
/* This class represents Feedforward Backpropagate Neural Network
 */
class FBNN;
typedef boost::shared_ptr<FBNN> FBNN_ptr;

// typedef boost::shared_mutex RWMutex;
typedef boost::chrono::time_point<boost::chrono::system_clock> TimePoint;
class FBNN {
public:
    FBNN(const Arch_t& arch,
         const std::string& activeStr = "tanh_opt",
         const double learningRate = 2,
         const double zeroMaskedFraction = 0.0,
         const int32_t maxSynchronicStep = 20,
         const bool testing = false,         
         const std::string& outputStr = "sigm"         
        );
    //FBNN(const FBNN& p) = delete;
    //FBNN& operator =(const FBNN& p) = delete;

    const std::vector<FMatrix_ptr>& get_m_oWs(void) const {
        return m_oWs;
    };
    const std::vector<FMatrix_ptr>& get_m_oVWs(void) const {
        return m_oVWs;
    };
    const std::vector<FMatrix_ptr>& get_m_oPs(void) const {
        return m_oPs;
    };
    const std::vector<FMatrix_ptr>& get_m_oAs(void) const {
        return m_oAs;
    };
    const std::vector<FMatrix_ptr>& get_m_odWs(void) const {
        return m_odWs;
    };

    void set_m_oWs(const std::vector<FMatrix_ptr>& oWs) {
        for(size_t i = 0; i < oWs.size(); i++)
            *m_oWs[i] = *oWs[i];
    };

    void set_m_oWs_column(const FMatrix_ptr W_col, int32_t index) {
        *m_oWs[index] = *W_col;
    };

    void set_m_oVWs(const std::vector<FMatrix_ptr>& oVWs) {
        for(size_t i = 0; i < oVWs.size(); i++)
            *m_oVWs[i] = *oVWs[i];
    };
    void set_m_odWs(const std::vector<FMatrix_ptr>& odWs) {
        if(m_odWs.empty())
        {
            for(size_t i = 0; i < odWs.size(); i++)
            {
                FMatrix m(*odWs[i]);
                m_odWs.push_back(FMatrix_ptr(new FMatrix(m)));
            }
        }
        else
        {
            for(size_t i = 0; i < odWs.size(); i++)
            {
                *m_oWs[i] = *odWs[i];
            }
        }
    };

    void train(const FMatrix& train_x,
               const Opts& opts,
               ffnet::NetNervureFromFile& ref_NNFF,
               const ffnet::EndpointPtr_t& pEP,
               const int32_t sae_index,
               TimePoint& startTime,
               int32_t defaultSynchronicStep
              );

    void train_after_pull(const int32_t sae_index,
                          ffnet::NetNervureFromFile& ref_NNFF,
                          const ffnet::EndpointPtr_t& pEP,
                          TimePoint& startTime
                         );
    bool train_after_push(const int32_t sae_index,
                          ffnet::NetNervureFromFile& ref_NNFF,
                          const ffnet::EndpointPtr_t& pEP,
                          TimePoint& startTime
                         );

    void train(const FMatrix& train_x,
               const FMatrix& train_y,
               const Opts& opts,
               const FMatrix& valid_x,
               const FMatrix& valid_y);

    void train(const FMatrix& train_x,
               const FMatrix& train_y ,
               const Opts& opts);
    
    void AEtest(const FMatrix& train_x,
               const Opts& opts);
    double nnff(const FMatrix& x, const FMatrix& y);
    void nnbp(void);
    void nnapplygrads(void);
    void nneval(Loss& loss,
                const FMatrix& train_x,
                const FMatrix& train_y,
                const FMatrix& valid_x,
                const FMatrix& valid_y);
    void nneval(Loss& loss, const FMatrix& train_x, const FMatrix& train_y);
    double nntest(const FMatrix& x, const FMatrix& y);
    void nnpredict(const FMatrix& x, const FMatrix& y, FColumn& labels);

    void setCurrentPushSynchronicStep(int32_t step = -1); /*{//set before push      
        if(step == -1)
            m_iCurrentPushSynchronicStep = ::rand() % (m_iMaxSynchronicStep+1);
        else
            m_iCurrentPushSynchronicStep = step;
        int32_t deltaSteps = m_iMaxSynchronicStep - m_iAccumulatedPushSteps;
        if(m_iCurrentPushSynchronicStep > deltaSteps)
            m_iCurrentPushSynchronicStep = deltaSteps;
        if(m_ivEpoch == m_sOpts.numpochs - 1 && m_iCurrentPushSynchronicStep >= m_iBatchNum - m_ivBatch)// last epoch
            m_iCurrentPushSynchronicStep = m_iBatchNum - m_ivBatch - 1;
        m_iAccumulatedPushSteps += m_iCurrentPushSynchronicStep;
        m_iPushStepNum = 0;//reset
    }*/

    void setCurrentPullSynchronicStep(int32_t step = -1); /*{//set before pull
        if(step == -1)
            m_iCurrentPullSynchronicStep = ::rand() % (m_iMaxSynchronicStep+1);
        else
            m_iCurrentPullSynchronicStep = step;
        int32_t deltaSteps = m_iMaxSynchronicStep - m_iAccumulatedPullSteps;
        if(m_iCurrentPullSynchronicStep > deltaSteps)
            m_iCurrentPullSynchronicStep = deltaSteps;
        if(m_ivEpoch == m_sOpts.numpochs - 1 && m_iCurrentPullSynchronicStep >= m_iBatchNum - m_ivBatch)// last epoch
            m_iCurrentPullSynchronicStep = m_iBatchNum - m_ivBatch - 1;
        m_iAccumulatedPullSteps += m_iCurrentPullSynchronicStep;
        m_iPullStepNum = 0;//reset
    }*/

//     RWMutex m_g_odWsMutex;
//     RWMutex m_g_oWsMutex;

protected:

    const Arch_t&    m_oArch;
    int32_t         m_iN;
    const std::string       m_strActivationFunction;
    double         m_fLearningRate;//shall be changed during train()
    static const double      m_fMomentum;
    static const double      m_fScalingLearningRate;
    static const double      m_fWeithtPenaltyL2;
    static const double      m_fNonSparsityPenalty;
    static const double      m_fSparsityTarget;
    const double      m_fInputZeroMaskedFraction;
//       static constexpr double      m_fInputZeroMaskedFraction = 0;
    static const double      m_fDropoutFraction;
//       static constexpr double      m_fTesting = 0;
    bool      m_bTesting;//shall be changed during nnpredict()
    const std::string m_strOutput;

    std::vector<FMatrix_ptr>  m_oWs;
    std::vector<FMatrix_ptr>  m_oVWs;
    std::vector<FMatrix_ptr>  m_oPs;
    std::vector<FMatrix_ptr>  m_oAs;
    std::vector<FMatrix_ptr>  m_odOMs;//dropOutMask
    std::vector<FMatrix_ptr>  m_odWs;


    FMatrix_ptr  m_oLp;//Loss matrix
    FMatrix_ptr  m_oEp;//Error matrix


    //members used in network version
    int32_t m_iBatchNum;//Change only once
    int32_t m_ivEpoch;//Change with loops
    int32_t m_ivBatch;
    std::vector<int32_t> m_oRandVec;
    Loss m_oLoss;
    FMatrix_ptr m_opTrain_x;
    Opts m_sOpts;
    const int32_t m_iMaxSynchronicStep;//Synchronic operation steps couldn't be larger than this
    int32_t m_iCurrentPushSynchronicStep;//Vary from 1 to m_iMaxSynchronicStep, controled by network monitor
    int32_t m_iPushStepNum;//Count passed steps
    int32_t m_iAccumulatedPushSteps;//Must be less than m_iMaxSynchronicStep, m_iAccumulatedSteps += m_iCurrentSynchronicStep;
    int32_t m_iCurrentPullSynchronicStep;//Vary from 1 to m_iMaxSynchronicStep, controled by network monitor
    int32_t m_iPullStepNum;//Count passed steps
    int32_t m_iAccumulatedPullSteps;//Must be less than m_iMaxSynchronicStep, m_iAccumulatedSteps += m_iCurrentSynchronicStep;
//     bool m_bHasPushed;//Pull operation could only happen when m_bHasPushed == true;
    int32_t m_iDefaultStepValue;//Used to control step by dl_worker

};//end class FBNN
//   typedef std::shared_ptr<FBNN> FBNN_ptr;
}//end namespace ff

#endif
