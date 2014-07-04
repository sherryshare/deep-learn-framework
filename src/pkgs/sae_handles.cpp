#include "pkgs/sae_handles.h"



bool divideInputData(void * handle,std::string input_file,std::string output_dir, int pieces)
{
    // load the symbol
    std::cout << "Loading symbol divide_into_files..." << std::endl;
    typedef void (*divide_into_files_t)(int parts,std::string,std::string);

    // reset errors
    dlerror();
    divide_into_files_t divide_into_files = (divide_into_files_t) dlsym(handle, "divide_into_files");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'divide_into_files': " << dlsym_error << std::endl;
        dlclose(handle);
        return false;
    }

    std::cout << "Calling divide_into_files..." << std::endl;
    divide_into_files(pieces,input_file,output_dir);

    return true;
}

bool send_data_from_dir(std::string input_dir,std::string ip,std::string path)//only read one .part file
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

boost::any initParameterServer(void * handle)
{
    // load the symbol
    std::cout << "Loading symbol SAE_create..." << std::endl;
    typedef boost::any (*SAE_create_t)();

    // reset errors
    dlerror();
    SAE_create_t SAE_create = (SAE_create_t) dlsym(handle, "SAE_create");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'SAE_create': " << dlsym_error << std::endl;
        dlclose(handle);
        return nullptr;
    }

    std::cout << "Calling SAE_create..." << std::endl;
    boost::any psae = SAE_create();

    return psae;
}

bool trainSAE(void * handle,boost::any sae_pointer,std::string data_dir)
{
    // load the symbol
    std::cout << "Loading symbol SAE_run..." << std::endl;
    typedef bool (*SAE_run_t)(boost::any sae_pointer,std::string data_dir);

    // reset errors
    dlerror();
    SAE_run_t SAE_run = (SAE_run_t) dlsym(handle, "SAE_run");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'SAE_run': " << dlsym_error << std::endl;
        dlclose(handle);
        return nullptr;
    }

    std::cout << "Calling SAE_run..." << std::endl;
    return SAE_run(sae_pointer,data_dir);
}

bool trainNN(void * handle,boost::any sae_pointer,std::string input_file)
{
    // load the symbol
    std::cout << "Loading symbol train_NN..." << std::endl;
    typedef void (*train_NN_t)(boost::any sae_pointer,std::string input_file);

    // reset errors
    dlerror();
    train_NN_t train_NN = (train_NN_t) dlsym(handle, "train_NN");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'train_NN': " << dlsym_error << std::endl;
        dlclose(handle);
        return false;
    }

    std::cout << "Calling train_NN..." << std::endl;
    train_NN(sae_pointer,input_file);
    return true;
}

bool deleteSAE(void * handle, boost::any sae_pointer)
{
    // load the symbol
    std::cout << "Loading symbol SAE_delete..." << std::endl;
    typedef void (*SAE_delete_t)(boost::any sae_pointer);

    // reset errors
    dlerror();
    SAE_delete_t SAE_delete = (SAE_delete_t) dlsym(handle, "SAE_delete");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'SAE_delete': " << dlsym_error << std::endl;
        dlclose(handle);
        return false;
    }

    std::cout << "Calling SAE_delete..." << std::endl;
    SAE_delete(sae_pointer);
    sae_pointer = nullptr;//necessary?
    return true;
}
