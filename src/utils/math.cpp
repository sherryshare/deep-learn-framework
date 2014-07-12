#include "math.h"

namespace ff{
    double sigm(const double& x){return 1.0/(1+exp(-x));};
    
    FMatrix sigm(const FMatrix& m){
      FMatrix res(m.rows(), m.columns());
        for(int i = 0; i < m.rows(); ++i)
        {
           for(int j = 0; j < m.columns(); ++j)
           {
             res(i, j) = 1.0 / (1 + exp(-m(i, j)));
           }
        }
        return res;
    }
    
    double sigmrnd(const double& x){//并用rand函数生成0-1间数的随机矩阵并进行比较
	return double(1.0/(1+exp(-x)) > rand());
    };//仅在DBN/rbmtrain.m中用到，SAE中未使用    
    
    FMatrix sigmrnd(const FMatrix& m){
      FMatrix res(m.rows(), m.columns());
        for(int i = 0; i < m.rows(); ++i)
        {
           for(int j = 0; j < m.columns(); ++j)
           {
             res(i, j) = double(1.0 / (1 + exp(-m(i, j))) > rand());
           }
        }
        return res;
    }    
    
    void softmax(double* x, const int n_out){
        //直接对一维数组计算,n_out表示数组长度，要求用户输入数组长度必须正确。结果直接修改原数组内容。
        double sum = 0.0;
        //     double max = 0.0;//注释掉的是可以优化softmax效果的代码，官方代码中未提供
        //     for(int i=0; i<n_out; ++i) if(max < x[i]) max = x[i];
        for(int i=0; i<n_out; ++i) {
            // x[i] = exp(x[i] - max);
            x[i] = exp(x[i] * 3);//参照softmax.m，c=3，直接取值3未保留c变量。
            sum += x[i];
        }
        for(int i=0; i<n_out; ++i) x[i] /= sum;
    };//SAE示例代码中未使用
    
    FMatrix softmax(const FMatrix& m)//softmax according to nnff.m
    {
      FMatrix res(m.rows(), m.columns());
        for(int i = 0; i < m.rows(); ++i)
        {
	   double sum = 0.0, max = res(i,0);
           for(int j = 1; j < m.columns(); ++j)
           {	     
	     if(max < res(i,j))
	       max = res(i,j);             
           }
           for(int j = 0; j < m.columns(); ++j)
	   {
	     res(i, j) = exp(m(i, j) - max);
	     sum += res(i, j);
	   }
           for(int j = 0; j < m.columns(); ++j)
	     res(i, j) /= sum;
        }
        return res;
    }    
    
    double tanh_opt(const double& x){return 1.7159 * tanh(2.0 / 3 * x);};
    
    FMatrix tanh_opt(const FMatrix& m){
      FMatrix res(m.rows(), m.columns());
        for(int i = 0; i < m.rows(); ++i)
        {
           for(int j = 0; j < m.columns(); ++j)
           {
             res(i, j) = 1.7159 * tanh(2.0 / 3 * m(i, j));
           }
        }
        return res;
    }
}//end namespace ff
