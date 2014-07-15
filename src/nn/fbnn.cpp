#include "nn/fbnn.h"


namespace ff
{
const double      FBNN::m_fMomentum = 0.5;
const double      FBNN::m_fScalingLearningRate= 0.9;//1
const double      FBNN::m_fWeithtPenaltyL2 = 0;
const double      FBNN::m_fNonSparsityPenalty = 0;
const double      FBNN::m_fSparsityTarget = 0.05;
const double      FBNN::m_fDropoutFraction = 0;
FBNN::FBNN(const Arch_t& arch,
           const std::string& activeStr,
           const double learningRate,
           const double zeroMaskedFraction,
           const bool testing,
           const std::string& outputStr)
    : m_oArch(arch)
    , m_iN(numel(arch))
    , m_strActivationFunction(activeStr)
    , m_fLearningRate(learningRate)
    , m_fInputZeroMaskedFraction(zeroMaskedFraction)
    , m_fTesting(testing)
    , m_strOutput(outputStr)
{
    for(int32_t i = 1; i < m_iN; ++i)
    {
        FMatrix f = (rand(m_oArch[i], m_oArch[i-1] + 1) - 0.5) * (2 * 4 * sqrt(6.0/(m_oArch[i] + m_oArch[i-1])));//based on nnsetup.m
        m_oWs.push_back(FMatrix_ptr(new FMatrix(f)));
        if(double_larger_than_zero(m_fMomentum))
        {
            FMatrix z = zeros(f.rows(), f.columns());
            m_oVWs.push_back(FMatrix_ptr(new FMatrix(z)));
        }
        if(double_larger_than_zero(m_fNonSparsityPenalty))
        {
            FMatrix p = zeros(1, m_oArch[i]);
            m_oPs.push_back(FMatrix_ptr(new FMatrix(p)));
        }
    }

}


//trains a neural net
void FBNN::train(const FMatrix& train_x,
                 const FMatrix& train_y,
                 const Opts& opts,
                 const FMatrix& valid_x,
                 const FMatrix& valid_y,
                 const FBNN_ptr pFBNN)
{
    int32_t ibatchNum = train_x.rows() / opts.batchsize + (train_x.rows() % opts.batchsize != 0);
    FMatrix L = zeros(opts.numpochs * ibatchNum, 1);
    m_oLp = FMatrix_ptr(new FMatrix(L));
    Loss loss;
//       std::cout << "numpochs = " << opts.numpochs << std::endl;
    for(int32_t i = 0; i < opts.numpochs; ++i)
    {
        std::cout << "start numpochs " << i << std::endl;
        //int32_t elapsedTime = count_elapse_second([&train_x,&train_y,&L,&opts,i,pFBNN,ibatchNum,this] {
            std::vector<int32_t> iRandVec;
            randperm(train_x.rows(),iRandVec);
            std::cout << "start batch: ";
            for(int32_t j = 0; j < ibatchNum; ++j)
            {
                std::cout << " " << j;
                if(pFBNN)//pull
                {
// 		    TMutex::scoped_lock lock;
// 		    lock.acquire(pFBNN->W_RWMutex,false);
// 		    lock.release();//reader lock tbb
                    boost::shared_lock<RWMutex> rlock(pFBNN->W_RWMutex);
                    set_m_oWs(pFBNN->get_m_oWs());
                    if(double_larger_than_zero(m_fMomentum))
                        set_m_oVWs(pFBNN->get_m_oVWs());
                    rlock.unlock();
                }
                int32_t curBatchSize = opts.batchsize;
                if(j == ibatchNum - 1 && train_x.rows() % opts.batchsize != 0)
                    curBatchSize = train_x.rows() % opts.batchsize;
                FMatrix batch_x(curBatchSize,train_x.columns());
                for(int32_t r = 0; r < curBatchSize; ++r)//randperm()
                    row(batch_x,r) = row(train_x,iRandVec[j * opts.batchsize + r]);

                //Add noise to input (for use in denoising autoencoder)
                if(m_fInputZeroMaskedFraction != 0)
                    batch_x = bitWiseMul(batch_x,(rand(curBatchSize,train_x.columns())>m_fInputZeroMaskedFraction));

                FMatrix batch_y(curBatchSize,train_y.columns());
                for(int32_t r = 0; r < curBatchSize; ++r)//randperm()
                    row(batch_y,r) = row(train_y,iRandVec[j * opts.batchsize + r]);

                L(i*ibatchNum+j,0) = nnff(batch_x,batch_y);
                nnbp();
                nnapplygrads();
                if(pFBNN)//push
                {
// 		    TMutex::scoped_lock lock;
// 		    lock.acquire(W_RWMutex);
// 		    lock.release();//writer lock tbb
                    boost::unique_lock<RWMutex> wlock(pFBNN->W_RWMutex);
                    pFBNN->set_m_odWs(m_odWs);
                    pFBNN->nnapplygrads();
                    wlock.unlock();
                }
// 	      std::cout << "end batch " << j << std::endl;
            }
            std::cout << std::endl;
        //});
        //std::cout << "elapsed time: " << elapsedTime << "s" << std::endl;
        //loss calculate use nneval
        if(valid_x.rows() == 0 || valid_y.rows() == 0) {
            nneval(loss, train_x, train_y);
            std::cout << "Full-batch train mse = " << loss.train_error.back() << std::endl;
        }
        else {
            nneval(loss, train_x, train_y, valid_x, valid_y);
            std::cout << "Full-batch train mse = " << loss.train_error.back() << " , val mse = " << loss.valid_error.back() << std::endl;
        }
        //std::cout << "epoch " << i+1 << " / " <<  opts.numpochs << " took " << elapsedTime << " seconds." << std::endl;
        std::cout << "Mini-batch mean squared error on training set is " << columnMean(submatrix(L,i*ibatchNum,0UL,ibatchNum,L.columns())) << std::endl;
        m_fLearningRate *= m_fScalingLearningRate;

// 	  std::cout << "end numpochs " << i << std::endl;
    }

}
void FBNN::train(const FMatrix& train_x,
                 const FMatrix& train_y,
                 const Opts& opts,
                 const FBNN_ptr pFBNN)
{
    FMatrix emptyM;
    train(train_x,train_y,opts,emptyM,emptyM,pFBNN);
}

//NNFF performs a feedforward pass
double FBNN::nnff(const FMatrix& x, const FMatrix& y)
{
//     std::cout << "start nnff x = (" << x.rows() << "," << x.columns() << ")" << std::endl;
    double L = 0;
    if(m_oAs.empty())
    {
        for(int32_t i = 0; i < m_iN; ++i)
            m_oAs.push_back(FMatrix_ptr(new FMatrix()));
    }
    *m_oAs[0] = addPreColumn(x,1);
    if(double_larger_than_zero(m_fDropoutFraction) && !m_fTesting)
    {
        if(m_odOMs.empty())//clear dropOutMask
        {
            for(int32_t i = 0; i < m_iN - 1; ++i)
                m_odOMs.push_back(FMatrix_ptr(new FMatrix()));
        }
    }

//     std::cout << "start feedforward" << std::endl;
    //feedforward pass
    for(int32_t i = 1; i < m_iN - 1; ++i)
    {
//       std::cout << "activation function" << std::endl;
        //activation function
        if(m_strActivationFunction == "sigm")
        {
            //Calculate the unit's outputs (including the bias term)
            *m_oAs[i] = sigm((*m_oAs[i-1]) * blaze::trans(*m_oWs[i-1]));
        }
        else if(m_strActivationFunction == "tanh_opt")
        {
            *m_oAs[i] = tanh_opt((*m_oAs[i-1]) * blaze::trans(*m_oWs[i-1]));
        }

//       std::cout << "dropout" << std::endl;
        //dropout
        if(double_larger_than_zero(m_fDropoutFraction))
        {
            if(m_fTesting)
                *m_oAs[i] = (*m_oAs[i]) * (1 - m_fDropoutFraction);
            else
            {
                *m_odOMs[i] = rand(m_oAs[i]->rows(),m_oAs[i]->columns()) > m_fDropoutFraction;
                *m_oAs[i] = bitWiseMul(*m_oAs[i],*m_odOMs[i]);
            }
        }

//       std::cout << "sparsity" << std::endl;
        //calculate running exponential activations for use with sparsity
        if(double_larger_than_zero(m_fNonSparsityPenalty))
            *m_oPs[i] =  (*m_oPs[i]) * 0.99 + columnMean(*m_oAs[i]);

//       std::cout << "Add the bias term" << std::endl;
        //Add the bias term
        *m_oAs[i] = addPreColumn(*m_oAs[i],1);
    }

//     std::cout << "start calculate output" << std::endl;
    if(m_strOutput == "sigm")
    {
        *m_oAs[m_iN -1] = sigm((*m_oAs[m_iN-2]) * blaze::trans(*m_oWs[m_iN-2]));
    }
    else if(m_strOutput == "linear")
    {
        *m_oAs[m_iN -1] = (*m_oAs[m_iN-2]) * blaze::trans(*m_oWs[m_iN-2]);
    }
    else if(m_strOutput == "softmax")
    {
        *m_oAs[m_iN -1] = softmax((*m_oAs[m_iN-2]) * blaze::trans(*m_oWs[m_iN-2]));
    }

//     std::cout << "start error and loss" << std::endl;
    //error and loss
    m_oEp = FMatrix_ptr(new FMatrix(y - (*m_oAs[m_iN-1])));

    if(m_strOutput == "sigm" || m_strOutput == "linear")
    {
        L = 0.5 * matrixSum(bitWiseSquare(*m_oEp)) / x.rows();
    }
    else if(m_strOutput == "softmax")
    {
        L = -matrixSum(bitWiseMul(y,bitWiseLog(*m_oAs[m_iN-1]))) / x.rows();
    }
//     std::cout << "end nnff" << std::endl;
    return L;
}

//NNBP performs backpropagation
void FBNN::nnbp(void)
{
//     std::cout << "start nnbp" << std::endl;
    std::vector<FMatrix_ptr> oDs;
    //initialize oDs
    for(int32_t i = 0; i < m_iN; ++i)
        oDs.push_back(FMatrix_ptr(new FMatrix()));
    if(m_strOutput == "sigm")
    {
        *oDs[m_iN -1] = bitWiseMul(*m_oEp,bitWiseMul(*m_oAs[m_iN -1],*m_oAs[m_iN -1] - 1));
    }
    else if(m_strOutput == "softmax" || m_strOutput == "linear")
    {
        *oDs[m_iN -1] = - (*m_oEp);
    }
    for(int32_t i = m_iN - 2; i > 0; --i)
    {
        FMatrix d_act;
        if(m_strActivationFunction == "sigm")
        {
            d_act = bitWiseMul(*m_oAs[i],1 - (*m_oAs[i]));
        }
        else if(m_strActivationFunction == "tanh_opt")
        {
            d_act = 1.7159 * 2/3 - 2/(3 * 1.7159) * bitWiseSquare(*m_oAs[i]);
        }

        if(double_larger_than_zero(m_fNonSparsityPenalty))
        {
            FMatrix pi = repmat(*m_oPs[i],m_oAs[i]->rows(),1);
            FMatrix sparsityError = addPreColumn(m_fNonSparsityPenalty * (1 - m_fSparsityTarget) / (1 - pi) - m_fNonSparsityPenalty * m_fSparsityTarget / pi,0);
            //Backpropagate first derivatives
            if(i == m_iN - 2)//in this case in oDs there is not the bias term to be removed
            {
                *oDs[i] = bitWiseMul(*oDs[i+1] * (*m_oWs[i]) + sparsityError,d_act);//Bishop (5.56)
            }
            else//in this case in oDs the bias term has to be removed
            {
                *oDs[i] = bitWiseMul(delPreColumn(*oDs[i+1]) * (*m_oWs[i]) + sparsityError,d_act);
            }
        }
        else
        {
            //Backpropagate first derivatives
            if(i == m_iN - 2)//in this case in oDs there is not the bias term to be removed
            {
                *oDs[i] = bitWiseMul((*oDs[i+1]) * (*m_oWs[i]),d_act);//Bishop (5.56)
            }
            else//in this case in oDs the bias term has to be removed
            {
                *oDs[i] = bitWiseMul(delPreColumn(*oDs[i+1]) * (*m_oWs[i]),d_act);
            }
        }

        if(double_larger_than_zero(m_fDropoutFraction))
        {
            *oDs[i] = bitWiseMul(*oDs[i],addPreColumn(*m_odOMs[i],1));
        }

    }
    if(m_odWs.empty())//Initialize m_odWs
    {
        for(int32_t i = 0; i < m_iN - 1; ++i)
        {
            m_odWs.push_back(FMatrix_ptr(new FMatrix()));
        }
    }
    for(int32_t i = 0; i < m_iN - 1; ++i)
    {
        if(i == m_iN - 2)
        {
            *m_odWs[i] = trans(*oDs[i+1]) * (*m_oAs[i]) / oDs[i+1]->rows();
        }
        else
        {
            *m_odWs[i] = trans(delPreColumn(*oDs[i+1])) * (*m_oAs[i]) / oDs[i+1]->rows();
        }
    }
//     std::cout << "end nnbp" << std::endl;

}
//updates weights and biases with calculated gradients
void FBNN::nnapplygrads(void )
{
//     std::cout << "start nnapplygrads" << std::endl;
    FMatrix dW;
    for(int32_t i = 0; i < m_iN - 1; ++i)
    {
        if(double_larger_than_zero(m_fWeithtPenaltyL2))
            dW = *m_odWs[i] + m_fWeithtPenaltyL2 * addPreColumn(delPreColumn(*m_oWs[i]),0);
        else
            dW = *m_odWs[i];
        dW = m_fLearningRate * dW;

        if(double_larger_than_zero(m_fMomentum))
        {
            *m_oVWs[i] = (*m_oVWs[i]) * m_fMomentum + dW;
            dW = *m_oVWs[i];
        }

        *m_oWs[i] -= dW;
    }
//     std::cout << "end nnapplygrads" << std::endl;
}

//evaluates performance of neural network
void ff::FBNN::nneval(Loss& loss, 
		      const FMatrix& train_x, 
		      const FMatrix& train_y, 
		      const FMatrix& valid_x, 
		      const FMatrix& valid_y)
{
//     std::cout << "start nneval" << std::endl;
    //training performance
    loss.train_error.push_back(nnff(train_x,train_y));

    //validation performance
    if(valid_x.rows() != 0 && valid_y.rows() != 0)
        loss.valid_error.push_back(nnff(valid_x,valid_y));

    //calc misclassification rate if softmax
    if(m_strOutput == "softmax")
    {
        loss.train_error_fraction.push_back(nntest(train_x,train_y));
        if(valid_x.rows() != 0 && valid_y.rows() != 0)
            loss.valid_error_fraction.push_back(nntest(valid_x,valid_y));
    }
//     std::cout << "end nneval" << std::endl;
}

void ff::FBNN::nneval(Loss& loss, const FMatrix& train_x, const FMatrix& train_y)
{
    FMatrix emptyM;
    nneval(loss,train_x,train_y,emptyM,emptyM);
}

double ff::FBNN::nntest(const FMatrix& x, const FMatrix& y)
{
//     std::cout << "start nntest" << std::endl;
    FColumn labels;
    nnpredict(x,y,labels);
    FColumn expected = rowMaxIndexes(y);
    std::vector<int32_t> bad = findUnequalIndexes(labels,expected);
//     std::cout << "end nntest" << std::endl;
    return double(bad.size()) / x.rows();//Haven't return bad vector.(nntest.m does)
}

void ff::FBNN::nnpredict(const FMatrix& x, const FMatrix& y, FColumn& labels)
{
//     std::cout << "start nnpredict" << std::endl;
    m_fTesting = true;
    nnff(x,zeros(x.rows(),m_oArch[m_iN - 1]));
    m_fTesting = false;
    labels = rowMaxIndexes(*m_oAs[m_iN - 1]);
//     std::cout << "end nnpredict" << std::endl;
}

}
