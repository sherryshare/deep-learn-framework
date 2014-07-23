#include "pkgs/pkgs.h"

namespace ff {
void serialize_FMatrix(ffnet::Archive& ar,const FMatrix_ptr& pMat)
{
    int32_t row = pMat->rows();
    int32_t column = pMat->columns();
    ar.archive(row);
    ar.archive(column);
    double * data = new double[row*column];
    for(int32_t r = 0; r < row; ++r) {
        for(int32_t c = 0; c < column; ++c)
            data[r*column + c] = pMat->operator()(r,c);
    }
    ar.archive(data, row*column);
    delete data;
}

FMatrix_ptr deserialize_FMatrix(ffnet::Archive& ar)
{
    int32_t row,column;
    ar.archive(row);
    ar.archive(column);
    double * data = new double[row*column];
    ar.archive(data, row*column);
    FMatrix m(row,column);
    for(int32_t r = 0; r < row; ++r) {
        for(int32_t c = 0; c < column; ++c)
            m(r,c) = data[r*column + c];
    }
    delete data;
    return FMatrix_ptr(new FMatrix(m));
}

}//end namespace ff
