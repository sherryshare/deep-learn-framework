#pragma once

#include "dsource/read.h"
#include "utils/matlib.h"
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <matio.h>

namespace ff{
//   FMatrix_ptr read_train_x(std::string input_file);
//   bool write_to_file(std::string output_file,FMatrix_ptr train_x);
  FMatrix_ptr read_matrix_from_file(std::string file_name);
  FMatrix_ptr read_matrix_from_dir(std::string dir);  
  bool divide_into_files(int parts,std::string input_file = "../data/mnist_uint8.mat",std::string output_dir = ".");
  
};//end namespace ff