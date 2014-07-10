/*
 * This shall be header file for mathmatic
 */
#pragma once

#include "matlib.h"

namespace ff{
    double sigm(const double & x);
    
    FMatrix sigm(const FMatrix & m);
    
    double sigmrnd(const double & x);
    
    FMatrix sigmrnd(const FMatrix & m);
    
    void softmax(double * x, int n_out);
    
    FMatrix softmax(const FMatrix & m);
    
    double tanh_opt(const double & x);
    
    FMatrix tanh_opt(const FMatrix & m);
}//end namespace ff


