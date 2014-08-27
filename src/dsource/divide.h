#ifndef FFDL_DSOURCE_DIVIDE_H_
#define FFDL_DSOURCE_DIVIDE_H_

#include "dsource/read.h"
#include "utils/matlib.h"
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <matio.h>
#include <sys/stat.h>

namespace ff {
FMatrix_ptr read_train_x(const std::string& input_file);
FMatrix_ptr read_matrix_from_file(const std::string&  file_name);
FMatrix_ptr read_matrix_from_dir(const std::string&  dir);
bool divide_into_files(const int parts,
                       const std::string&  input_file = "../data/mnist_uint8.mat",
                       const std::string&  output_dir = ".");
inline bool is_dir(const std::string& file_name){
  struct stat file_stat;
  if(lstat(file_name.c_str() , &file_stat) < 0){
    return false;
  }
  return S_ISDIR(file_stat.st_mode);
}

}//end namespace ff

#endif
