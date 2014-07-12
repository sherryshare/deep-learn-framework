#include "file_send.h"


namespace ff{

/* NOTE: if you want this example to work on Windows with libcurl as a
   DLL, you MUST also provide a read callback with CURLOPT_READFUNCTION.
   Failing to do so will give you a crash since a DLL may not use the
   variable's memory when passed in to it from an app like this. */
static size_t read_callback(void* ptr, const size_t size, const size_t nmemb, void* stream)
{
  curl_off_t nread;
  std::ifstream* file_ptr = static_cast<std::ifstream*>(stream);
  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  file_ptr->read(static_cast<char*>(ptr),size* nmemb);
//   size_t retcode = fread(ptr, size, nmemb, (FILE*)stream);
  size_t retcode = file_ptr->gcount();
  
  nread = (curl_off_t)retcode;
  
  std::cout << "*** We read " << nread << " bytes from file " << std::endl;

  return retcode;
}

bool file_send(const std::string& input_file, 
	       const std::string& ip, 
	       const std::string& path, 
	       std::string output_file)
{
  CURL* curl;
  CURLcode res;  
  curl_off_t fsize;  
  std::string outputPath = path;
  std::ifstream src_file(input_file.c_str(), std::ios::binary|std::ios::ate);

  /* get the file size of the local file */
  if(!src_file.is_open()){
    std::cout << "Couldn't open '" << input_file << "'" << std::endl;
    return false;
  }
  
  if(output_file == "")
    output_file = getFileNameFromPath(input_file);
  
  if(path.find_last_of("/") != path.size() - 1){
    outputPath += "/";
//     std::cout << "path = " << path << std::endl;
  }
  std::string remote_url = "scp://" + ip + outputPath + output_file;
  
  std::cout << "remote_url = " << remote_url << std::endl;
  
  bool ret = true;
  
  fsize = (curl_off_t)src_file.tellg();
  
  src_file.seekg (0, std::ios::beg);

  std::cout << "Local file size: " << fsize << " bytes." << std::endl;

  /* In windows, this will init the winsock stuff */
  curl_global_init(CURL_GLOBAL_ALL);

  /* get a curl handle */
  curl = curl_easy_init();
  if(curl) {
    /* we want to use our own read function */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    
    /* ssh-key file */
    curl_easy_setopt(curl,CURLOPT_SSH_PUBLIC_KEYFILE, "/root/.ssh/id_rsa.pub");
    curl_easy_setopt(curl,CURLOPT_SSH_PRIVATE_KEYFILE, "/root/.ssh/id_rsa");

    /* adjust user and password */ 
    curl_easy_setopt(curl, CURLOPT_USERPWD, "root:"); 

    //WARNING: this would prevent curl from detecting a 'man in the middle' attack
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 


    /* enable uploading */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* specify target */
    curl_easy_setopt(curl,CURLOPT_URL, remote_url.c_str());

    /* now specify which file to upload */
    curl_easy_setopt(curl, CURLOPT_READDATA, &src_file);

    /* Set the size of the file to upload (optional).  If you give a *_LARGE
       option you MUST make sure that the type of the passed-in argument is a
       curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
       make sure that to pass in a type 'long' argument. */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)fsize);

    /* Now run off and do what you've been told! */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK){
      std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
      ret = false;
    }
      
    /* now extract transfer info */ 
    double speed_upload,total_time;
    curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload); 
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
    std::cout << "Speed: " << speed_upload << " bytes/sec during " << total_time << " seconds" << std::endl;

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  src_file.close(); /* close the local file */

  curl_global_cleanup();
  return ret;
}

bool send_data_from_dir(const std::string& input_dir, const std::string& ip, const std::string& path)//only read one .part file
{
    std::string file_name;
    bool retVal = false;
    DIR* dirp;
    struct dirent* direntp;
    dirp = opendir(input_dir.c_str());
    if(dirp != NULL) {
        while((direntp = readdir(dirp)) != NULL) {
            file_name = direntp->d_name;
            std::cout << "file_name = " << file_name << std::endl;
            int dotIndex = file_name.find_last_of('.');
            if(dotIndex != std::string::npos && file_name.substr(dotIndex,file_name.length() - dotIndex) == ".part")
            {
                std::cout << "find input_file " << file_name << std::endl;
                file_name = input_dir + "/" + file_name;
                retVal = true;
                break;
            }
        }
        closedir(dirp);
        if(file_send(file_name,ip,path))//send file and delete the local version
        {
            remove(file_name.c_str());
            std::cout << "Remove file '" << file_name << "'" << std::endl;
        }
    }
    return retVal;
}

}//end namespace ff
