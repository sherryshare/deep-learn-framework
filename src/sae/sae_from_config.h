#ifndef FFDL_SAE_SAE_FROM_CONFIG_H_
#define FFDL_SAE_SAE_FROM_CONFIG_H_

#include "sae/sae.h"
#include "dsource/divide.h"
#include <time.h>
#include "framework/nervure_config.h"//get ini config
#include <sstream>

namespace ff {
typedef boost::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
SAE_ptr SAE_create(const NervureConfigurePtr& pnc);
bool SAE_run(const SAE_ptr& psae,
             const std::string& data_dir, 
             const NervureConfigurePtr& pnc);
bool test_SAE(const SAE_ptr& psae, const NervureConfigurePtr& pnc);
void train_NN(const SAE_ptr& psae, const NervureConfigurePtr& pnc);

inline const double getLearningRateFromNervureConfigure(const NervureConfigurePtr& pnc) {
    return pnc->get<double>("init.learning-rate");
}

inline const double getInputZeroMaskedFractionFromNervureConfigure(const NervureConfigurePtr& pnc) {
    return pnc->get<double>("init.input-zero-masked-fraction");
}

inline const std::string getActivationFunctionFromNervureConfigure(const NervureConfigurePtr& pnc) {
    return pnc->get<std::string>("init.activation-function");
}

inline void getOptsFromNervureConfigure(const NervureConfigurePtr& pnc, Opts& opts) {
    opts.numpochs = pnc->get<int32_t>("opt.num-epochs");
    std::cout << "numpochs = " << opts.numpochs << std::endl;
    opts.batchsize = pnc->get<int32_t>("opt.batch-size");
}

inline const std::string getInputFileNameFromNervureConfigure(const NervureConfigurePtr& pnc) {
    return pnc->get<std::string>("path.input-file");
}

inline const int32_t getMaxSynchronicStepFromNervureConfigure(const NervureConfigurePtr& pnc) {
    return pnc->get<int32_t>("step.max-synchronic-step");    
}

inline const int32_t getStepControlFromNervureConfigure(const NervureConfigurePtr& pnc) {
    return pnc->get<int32_t>("step.step-control");    
}

void getArchFromNervureConfigure(const NervureConfigurePtr& pnc,
                                 const std::string& structure_name,
                                 Arch_t & arch);
}//end namespace ff

#endif
