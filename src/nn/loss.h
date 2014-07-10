#pragma once
#include "common/common.h"

namespace ff
{
  typedef struct
  {
    std::vector<double> train_error;
    std::vector<double> train_error_fraction;
    std::vector<double> valid_error;
    std::vector<double> valid_error_fraction;
  } Loss;//define struct type Loss
}//end namespace ff
