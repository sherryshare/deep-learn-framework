#pragma once

#include "sae/sae.h"
#include "dsource/divide.h"
#include <time.h>
#include "framework/nervure_config.h"//get ini config
#include <sstream>

namespace ff{
  typedef std::shared_ptr<ffnet::NervureConfigure> NervureConfigurePtr;
  SAE_ptr SAE_create(NervureConfigurePtr const pnc);
  bool SAE_run(SAE_ptr const psae,std::string data_dir, NervureConfigurePtr const pnc);
  void train_NN(SAE_ptr const psae, NervureConfigurePtr const pnc);  
  
};//end namespace ff