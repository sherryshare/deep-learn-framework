#include "dsource/divide.h"

using namespace ff;
int main(void) {
    divide_into_files(3);
    FMatrix_ptr res = read_matrix_from_dir(".");
    if(res)
        std::cout << res->rows() << "," << res->columns() << std::endl;
    return 0;
}
