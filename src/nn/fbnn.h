#ifndef FFDL_NN_FBNN_H_
#define FFDL_NN_FBNN_H_

#include "common/common.h"
#include "nn/arch.h"
#include "nn/opt.h"
#include "utils/math.h"
#include "nn/loss.h"
#include "utils/utils.h"
//mutex
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "pkgs/pkgs.h"

namespace ff
{
/* This class represents Feedforward Backpropagate Neural Network
 */
class DLWorker;
class FBNN;
typedef boost::shared_ptr<FBNN> FBNN_ptr;

class FBNN {
public:
    FBNN(const Arch_t& arch,
         const std::string& activeStr = "tanh_opt",
         const double learningRate = 2, const double zeroMaskedFraction = 0.0,
         const bool testing = false, const std::string& outputStr = "sigm");
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

    void      train(const FMatrix& train_x,
//                     const FMatrix& train_y ,
                    const Opts& opts,
                    const DLWorker* pDLWorker,
                    const int32_t sae_index);

    void      train(const FMatrix& train_x,
                    const FMatrix& train_y,
                    const Opts& opts,
                    const FMatrix& valid_x,
                    const FMatrix& valid_y);

    void      train(const FMatrix& train_x,
                    const FMatrix& train_y ,
                    const Opts& opts);
    double    nnff(const FMatrix& x, const FMatrix& y);
    void      nnbp(void);
    void	nnapplygrads(void);
    void	nneval(Loss& loss,
                   const FMatrix& train_x,
                   const FMatrix& train_y,
                   const FMatrix& valid_x,
                   const FMatrix& valid_y);
    void	nneval(Loss& loss, const FMatrix& train_x, const FMatrix& train_y);
    double	nntest(const FMatrix& x, const FMatrix& y);
    void	nnpredict(const FMatrix& x, const FMatrix& y, FColumn& labels);

//     RWMutex W_RWMutex;//only needed in para & master -- How to avoid when unnecessary? public

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
    bool      m_fTesting;//shall be changed during nnpredict()
    const std::string m_strOutput;

    std::vector<FMatrix_ptr>  m_oWs;
    std::vector<FMatrix_ptr>  m_oVWs;
    std::vector<FMatrix_ptr>  m_oPs;
    std::vector<FMatrix_ptr>  m_oAs;
    std::vector<FMatrix_ptr>  m_odOMs;//dropOutMask
    std::vector<FMatrix_ptr>  m_odWs;


    FMatrix_ptr  m_oLp;//Loss matrix
    FMatrix_ptr  m_oEp;//Error matrix


};//end class FBNN
//   typedef std::shared_ptr<FBNN> FBNN_ptr;
}//end namespace ff

#endif