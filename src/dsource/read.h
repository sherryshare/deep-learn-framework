#pragma once

#include "common/common.h"
#include "dtype/type.h"
#include "common/scope_guard.h"
#include <matio.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace ff
{
  struct TData
  {
        FMatrix_ptr train_x;
        FMatrix_ptr train_y;
        FMatrix_ptr test_x;
        FMatrix_ptr test_y;
  };//end struct TData
  FMatrix_ptr read_matrix_in_mat(mat_t * mat, const char *varname);
  TData read_data(std::string input_file = "../data/mnist_uint8.mat");
}//end namespace ff;
