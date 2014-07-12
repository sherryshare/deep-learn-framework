/*
 * This shall be header file for mathmatic
 */
#ifndef FFDL_UTILS_MATH_H_
#define FFDL_UTILS_MATH_H_

#include "matlib.h"

namespace ff{
    double sigm(const double& x);
    
    FMatrix sigm(const FMatrix& m);
    
    double sigmrnd(const double& x);
    
    FMatrix sigmrnd(const FMatrix& m);
    
    void softmax(double* x, const int n_out);
    
    FMatrix softmax(const FMatrix& m);
    
    double tanh_opt(const double& x);
    
    FMatrix tanh_opt(const FMatrix& m);
}//end namespace ff

#endif


