#include "dsource/divide.h"

namespace ff {

FMatrix_ptr read_train_x(const std::string& input_file)
{
    FMatrix_ptr res = FMatrix_ptr((FMatrix*)NULL);
    mat_t *mat = NULL;
    mat = Mat_Open(input_file.c_str(), MAT_ACC_RDONLY);

    if(!mat) {
        std::cout <<"cannot open "<<input_file<<std::endl;
        return res;
    }
    matvar_t *var = 0;
    while((var = Mat_VarReadNextInfo(mat)) != NULL) {
        if(strcmp(var->name, "train_x") == 0)
        {
            res = read_matrix_in_mat(mat, var->name);
            Mat_VarFree(var);
            break;
        }
        Mat_VarFree(var);
    }
    Mat_Close(mat);
    return res;
}

static bool write_to_file(const std::string& output_file,const FMatrix_ptr& train_x)
{
    std::ofstream out_file(output_file.c_str());
    if(!out_file.is_open()) {
        std::cout<<"failed to open file: "<< output_file << std::endl;
        return false;
    }
    else {
        std::cout << "write file: " << output_file << std::endl;
// 	out_file << train_x->rows() << " " << train_x->columns() << std::endl;//real command
        out_file << "1000" << " " << train_x->columns() << std::endl;//for quick test
//         for(size_t r = 0; r < train_x->rows(); ++r)//real command
        for(size_t r = 0; r < train_x->rows() && r < 1000; ++r)//for quick test
        {
            out_file << row(*train_x,r);
        }
    }
    out_file.close();
}

FMatrix_ptr read_matrix_from_file(const std::string& file_name)
{
    FMatrix_ptr res = FMatrix_ptr((FMatrix*)NULL);
    std::ifstream read_file(file_name.c_str());
    if(!read_file.is_open())
    {
        std::cout<<"failed to open file: "<< file_name << std::endl;
        return res;
    }
    std::cout << "read file:" << file_name << std::endl;
    std::string line;
    size_t rows,columns;
    getline(read_file,line);
    std::stringstream ss(line);
    ss >> rows;
    ss >> columns;
    std::cout << "read matrix: " << rows << "," << columns << std::endl;
    res = FMatrix_ptr(new FMatrix(rows,columns));
    for(size_t i = 0; i < rows && read_file.good(); ++i) {
        getline(read_file,line);
        std::stringstream ss(line);
        for(size_t j = 0; j < columns; ++j)
        {
            ss >> res->operator()(i, j);
        }
    }
    return res;
}

FMatrix_ptr read_matrix_from_dir(const std::string& dir)//only read one .part file
{
    FMatrix_ptr res = FMatrix_ptr((FMatrix*)NULL);
    if(!is_dir(dir) && (res = read_matrix_from_file(dir))){
        return res;
    }    
    std::string file_name;
    DIR* dirp;
    struct dirent* direntp;
    dirp = opendir(dir.c_str());
    if(dirp != NULL) {
        while((direntp = readdir(dirp)) != NULL) {
            file_name = direntp->d_name;
            std::cout << "file_name = " << file_name << std::endl;
            int dotIndex = file_name.find_last_of('.');
            if(dotIndex != std::string::npos && file_name.substr(dotIndex,file_name.length() - dotIndex) == ".part")
            {
                std::cout << "find input_file " << file_name << std::endl;
                file_name = dir + "/" + file_name;
                break;
            }
        }
        closedir(dirp);
        res = read_matrix_from_file(file_name);
    }
    return res;
}

bool divide_into_files(const int parts, const std::string& input_file, const std::string& output_dir)
{
    FMatrix_ptr train_x = read_train_x(input_file);
    if(train_x == NULL)
        return false;
    bool retVal = true;
    // Divide inputs into para groups.
    std::vector<int> iRandVec;
    randperm(train_x->rows(),iRandVec);
    int baseSize = train_x->rows() / parts;
    std::vector<FMatrix_ptr> train_x_pvec;
    std::cout << "Divide inputs:";
    for(int i = 0; i < parts; ++i)
    {
        std::cout << " " << i;
        int curSize = baseSize;
        if(i == parts - 1)//add the remains to the last group
            curSize += train_x->rows() % parts;
        FMatrix para_x(curSize,train_x->columns());
        for(int r = 0; r < curSize; ++r)//randperm()
            row(para_x,r) = row(*train_x,iRandVec[i * baseSize + r]);
        train_x_pvec.push_back(FMatrix_ptr(new FMatrix(para_x)));
    }
    std::cout << std::endl;
    for(int i = 0; i < parts; ++i)
    {
        std::stringstream ss;
        ss << output_dir << "/train_x_" << i << ".part";
        std::cout << "file:" << ss.str() << std::endl;
        retVal &= write_to_file(ss.str(),train_x_pvec[i]);
    }
    return retVal;
}
}//end namespace ff
