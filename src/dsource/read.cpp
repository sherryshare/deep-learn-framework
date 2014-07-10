#include "dsource/read.h"

namespace ff
{

  FMatrix_ptr read_matrix_in_mat(mat_t * mat, const char *varname)
  {
    matvar_t  * tvar = Mat_VarRead(mat, varname);
    int rows = tvar->dims[0];
    int columns = tvar->dims[1];
    std::cout<<rows<<"   "<<columns<<std::endl;
    uint8_t * pData = static_cast<uint8_t *>(tvar->data);
    FMatrix_ptr res = std::make_shared<FMatrix>(rows, columns);
    for(int k = 0; k < rows; ++k)
    {
      for(int i = 0; i < columns; ++i)
      {
        res->operator()(k, i) = static_cast<double>(pData[rows * i + k]);
      }
    }
    Mat_VarFree(tvar);
    return res;
  }
  TData         read_data(std::string input_file)
  {
    const char * fileName = input_file.c_str();
    TData res;
    mat_t *mat = nullptr;
    scope_guard _l([&mat, fileName](){
      mat = Mat_Open(fileName, MAT_ACC_RDONLY);
      },
    [&mat](){Mat_Close(mat);});
    if(!mat){
      std::cout <<"cannot open "<<fileName<<std::endl;
      return res;
    }  
    matvar_t *var = 0;
    int read_items = 0;
    while((var = Mat_VarReadNextInfo(mat)) != nullptr){
      if(strcmp(var->name, "train_x") == 0)
      {
        res.train_x = read_matrix_in_mat(mat, var->name);
        read_items ++ ;
      }
      else if(strcmp(var->name, "train_y") == 0)
      {
        res.train_y = read_matrix_in_mat(mat, var->name);
        read_items ++;
      }
      else if(strcmp(var->name, "test_x") == 0)
      {
        res.test_x = read_matrix_in_mat(mat, var->name);
        read_items ++;
      }
      else if(strcmp(var->name, "test_y" ) == 0)
      {
        res.test_y = read_matrix_in_mat(mat, var->name);
        read_items ++;
      }
      else{
        //nothing here, just for fun...
      }
      Mat_VarFree(var);
    }
    if(read_items != 4)
    {
      std::cout<<"ooooooops, cannot read "<< fileName<<std::endl;
    }
    return res;
  }
};//end namespace ff
