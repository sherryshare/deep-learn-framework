#ifndef FFDL_UTILS_UTILS_H_
#define FFDL_UTILS_UTILS_H_

#include <network.h>
#include <sstream>
#include "common/common.h"
#include <chrono>
#include <functional>
// #include <dlfcn.h>//dymanic library

// #define GLOBALFILENAME "globalFiles"



namespace ff{
  
const std::string globalDirStr = "globalFiles";
std::string endpoint_to_string(ffnet::EndpointPtr_t pEP);
std::string local_ip_v4();

int32_t count_elapse_microsecond(const std::function<void ()>& f);
int32_t count_elapse_second(const std::function<void ()>& f);

// inline void* openLibrary(const std::string & libStr)
// {
//     // open the library
//     std::cout << "Opening " << libStr << "..." << std::endl;
//     void* handle = dlopen(libStr.c_str(), RTLD_LAZY);
//     if (!handle) {
//         std::cerr << "Cannot open library: " << dlerror() << std::endl;
//         return nullptr;
//     }
//     return handle;
// }
// 
// inline void closeLibrary(void* handle)
// {
//     // close the library
//     std::cout << "Closing library..." << std::endl;
//     dlclose(handle);
//     handle = nullptr;//necessary?
// }

inline std::string newDirAtCWD(const std::string & newFileName, const std::string & backUpPath = "")
{
    std::string output_dir;
    if((output_dir = getcwd(NULL,0)) == "") {
        std::cout << "Error when getcwd!" << std::endl;
	if(backUpPath == "")
	  return output_dir;
	else
	  output_dir = backUpPath;
    }
    output_dir += static_cast<std::string>("/") + newFileName;
    if(access(output_dir.c_str(),F_OK) == -1) {
        mkdir(output_dir.c_str(),S_IRWXU);
    }
    return output_dir;
}
}//end namespace ff

#endif
