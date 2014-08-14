#ifndef FFDL_UTILS_UTILS_H_
#define FFDL_UTILS_UTILS_H_

#include <network.h>
#include <sstream>
#include "common/common.h"



namespace ff {

const std::string globalDirStr = "globalFiles";
std::string endpoint_to_string(ffnet::EndpointPtr_t pEP);
std::string local_ip_v4();
bool recordDurationTime(std::vector<std::pair<int,int> >& recordVec,const std::string& outFileName);

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
