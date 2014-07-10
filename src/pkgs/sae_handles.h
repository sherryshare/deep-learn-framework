#pragma once


#include <common/common.h>
// #include <dlfcn.h>//dymanic library
#include <dirent.h>
// #include <boost/any.hpp>
#include "pkgs/file_send.h"

namespace ff{
// bool divideInputData(void * handle,std::string input_file,std::string output_dir, int pieces);
// boost::any initParameterServer(void * handle);
// bool trainSAE(void * handle,boost::any sae_pointer,std::string data_dir);
// bool trainNN(void * handle,boost::any sae_pointer,std::string input_file);
bool send_data_from_dir(std::string input_dir,std::string ip,std::string path);

  
}//end namespace ff;