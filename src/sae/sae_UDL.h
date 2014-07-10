#pragma once

#include "sae/sae.h"
#include "dsource/divide.h"
#include <time.h>

namespace ff{
  SAE_ptr SAE_create(void);
  bool SAE_run(SAE_ptr psae,std::string data_dir);
  void train_NN(SAE_ptr psae,std::string input_file = "");
  
};//end namespace ff