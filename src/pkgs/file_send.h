#ifndef FFDL_PKGS_FILE_SEND_H_
#define FFDL_PKGS_FILE_SEND_H_

#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <dirent.h>

namespace ff {
// static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

bool file_send(const std::string& input_file,
               const std::string& ip,
               const std::string& path,
               std::string output_file = "");

const std::string send_data_from_dir(const std::string& input_dir,
                        const std::string& ip,
                        const std::string& path);

inline std::string getFileNameFromPath(const std::string& path)
{
    int32_t start = path.find_last_of('/') + 1;
    return path.substr(start,path.length() - start);
}

}//end namespace ff

#endif
