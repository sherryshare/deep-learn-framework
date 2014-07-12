#ifndef FFDL_SAE_SAE_FROM_CONFIG_H_
#define FFDL_SAE_SAE_FROM_CONFIG_H_

#include "sae/sae.h"
#include "dsource/divide.h"
#include <time.h>
#include "framework/nervure_config.h"//get ini config
#include <sstream>

namespace ff {
typedef std::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
SAE_ptr SAE_create(const NervureConfigurePtr& pnc);
bool SAE_run(const SAE_ptr& psae,const std::string& data_dir, const NervureConfigurePtr& pnc);
void train_NN(const SAE_ptr& psae, const NervureConfigurePtr& pnc);

}//end namespace ff

#endif
