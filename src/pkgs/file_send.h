#pragma once

#include <iostream>
#include <fstream>
#include <curl/curl.h>

namespace ff{
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

bool file_send(std::string input_file, std::string ip, std::string pwd, std::string output_file = "");

}//end namespace ff