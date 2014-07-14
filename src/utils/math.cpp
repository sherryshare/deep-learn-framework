#include "math.h"

namespace ff{
    double sigm(const double& x){return 1.0/(1+exp(-x));};
    
    FMatrix sigm(const FMatrix& m){
      FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
           for(size_t j = 0; j < m.columns(); ++j)
           {
             res(i, j) = 1.0 / (1 + exp(-m(i, j)));
           }
        }
        return res;
    }
    
    double sigmrnd(const double& x){//use rand() to compare
	return double(1.0/(1+exp(-x)) > rand());
    };//used in DBN/rbmtrain.m
    
    FMatrix sigmrnd(const FMatrix& m){
      FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
           for(size_t j = 0; j < m.columns(); ++j)
           {
             res(i, j) = double(1.0 / (1 + exp(-m(i, j))) > rand());
           }
        }
        return res;
    }    
    
    void softmax(double* x, const int32_t n_out){
        //Used in double vector
        double sum = 0.0;
        //     double max = 0.0;//softmax optimization
        //     for(size_t i=0; i<n_out; ++i) if(max < x[i]) max = x[i];
        for(size_t i=0; i<n_out; ++i) {
            // x[i] = exp(x[i] - max);
            x[i] = exp(x[i] * 3);//refer to softmax.m，c=3
            sum += x[i];
        }
        for(size_t i=0; i<n_out; ++i) x[i] /= sum;
    };//not used in SAE
    
    FMatrix softmax(const FMatrix& m)//softmax according to nnff.m
    {
      FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
	   double sum = 0.0, max = res(i,0);
           for(size_t j = 1; j < m.columns(); ++j)
           {	     
	     if(max < res(i,j))
	       max = res(i,j);             
           }
           for(size_t j = 0; j < m.columns(); ++j)
	   {
	     res(i, j) = exp(m(i, j) - max);
	     sum += res(i, j);
	   }
           for(size_t j = 0; j < m.columns(); ++j)
	     res(i, j) /= sum;
        }
        return res;
    }    
    
    double tanh_opt(const double& x){return 1.7159 * tanh(2.0 / 3 * x);};
    
    FMatrix tanh_opt(const FMatrix& m){
      FMatrix res(m.rows(), m.columns());
        for(size_t i = 0; i < m.rows(); ++i)
        {
           for(size_t j = 0; j < m.columns(); ++j)
           {
             res(i, j) = 1.7159 * tanh(2.0 / 3 * m(i, j));
           }
        }
        return res;
    }
}//end namespace ff
