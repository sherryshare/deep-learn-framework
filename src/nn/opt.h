#ifndef FFDL_NN_OPT_H_
#define FFDL_NN_OPT_H_

#include "common/common.h"

namespace ff
{
  typedef struct 
  {
    int numpochs;// = 1;
    int batchsize;// = 100;
  } Opts;
}//end namespace ff

#endif