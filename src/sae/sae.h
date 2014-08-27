/*
 * This shall be header file for SAE
 */
#ifndef FFDL_SAE_SAE_H_
#define FFDL_SAE_SAE_H_

#include "common/common.h"
#include "nn/fbnn.h"

namespace ff
{
class SAE;
typedef boost::shared_ptr<SAE> SAE_ptr;
class SAE
{
public:
    SAE(const Arch_t& arch,
        const std::string& activationFunction = "sigm",
        const double learningRate = 1,
        const double inputZeroMaskedFraction = 0.5,
        const int32_t maxSynchronicStep = 20
       );
    void SAETrain(const FMatrix& train_x,
                  const Opts& opts);
    void SAETrain(const FMatrix& train_x,
                  const Opts& opts,
                  ffnet::NetNervureFromFile& ref_NNFF,
                  const ffnet::EndpointPtr_t& pEP,
                  TimePoint& startTime,
                  int32_t defaultSynchronicStep = -1
                 );

    bool train_after_end_AE(ffnet::NetNervureFromFile& ref_NNFF,
                            const ffnet::EndpointPtr_t& pEP,
                            TimePoint& startTime
                           );
    void SAETest(const FMatrix& train_x, const Opts& opts);
    const std::vector<FBNN_ptr>& get_m_oAEs(void) const {
        return m_oAEs;
    };

protected:
    std::vector<FBNN_ptr>        m_oAEs;
    const std::string    m_strActivationFunction;
    double          m_fLearningRate;
    const double          m_fInputZeroMaskedFraction;

    //members used in network version
    int32_t m_iAEIndex;
    FMatrix_ptr m_pTrain_x;
    Opts m_sOpts;
    int32_t m_iDefaultSynchronicStep;


};//end class SAE

}//end namespace ff

#endif
