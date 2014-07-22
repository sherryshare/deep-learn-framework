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
    , m_bPushAckReceived(true)//Used in parameter push
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
                 const FMatrix& valid_y)
{
    int32_t ibatchNum = train_x.rows() / opts.batchsize + (train_x.rows() % opts.batchsize != 0);
//     FMatrix L = zeros(opts.numpochs * ibatchNum, 1);
//     m_oLp = FMatrix_ptr(new FMatrix(L));
    m_oLp = FMatrix_ptr(new FMatrix(opts.numpochs * ibatchNum, 1));
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

            (*m_oLp)(i*ibatchNum+j,0) = nnff(batch_x,batch_y);
            nnbp();
            nnapplygrads();
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
        std::cout << "Mini-batch mean squared error on training set is " << columnMean(submatrix(*m_oLp,i*ibatchNum,0UL,ibatchNum,m_oLp->columns())) << std::endl;
        m_fLearningRate *= m_fScalingLearningRate;
// 	  std::cout << "end numpochs " << i << std::endl;
    }
}
void FBNN::train(const FMatrix& train_x,
                 const FMatrix& train_y,
                 const Opts& opts)
{
    FMatrix emptyM;
    train(train_x,train_y,opts,emptyM,emptyM);
}

//trains a neural net
void FBNN::train(const FMatrix& train_x,
                 const Opts& opts,
                 ffnet::NetNervureFromFile& ref_NNFF,
                 const ffnet::EndpointPtr_t& pEP,
                 const int32_t sae_index)
{
    m_opTrain_x = FMatrix_ptr(new FMatrix(train_x));// Copy and store train_x
    m_sOpts = opts;
    m_iBatchNum = m_opTrain_x->rows() / m_sOpts.batchsize + (m_opTrain_x->rows() % m_sOpts.batchsize != 0);
//     FMatrix L = zeros(m_sOpts.numpochs * m_iBatchNum, 1);
//     m_oLp = FMatrix_ptr(new FMatrix(L));
    m_oLp = FMatrix_ptr(new FMatrix(m_sOpts.numpochs * m_iBatchNum, 1));
//       std::cout << "numpochs = " << m_sOpts.numpochs << std::endl;
    m_ivEpoch = 0;
    std::cout << "start numpochs " << m_ivEpoch << std::endl;
    //int32_t elapsedTime...
    randperm(m_opTrain_x->rows(),m_oRandVec);
    std::cout << "start batch: ";
    m_ivBatch = 0;
    std::cout << " " << m_ivBatch << std::endl;
    //wait push ack then pull
    boost::shared_lock<RWMutex> rlock(m_g_RWMutex);
    while (!m_bPushAckReceived) {
        m_cond_ack.wait(rlock);
        std::cout << "Wait push ack!" << std::endl;
    }
    rlock.unlock();
    //pull under network conditions
    std::cout << "Need to pull weights!" << std::endl;
    boost::shared_ptr<PullParaReq> pullReqMsg(new PullParaReq());
    pullReqMsg->sae_index() = sae_index;
    ref_NNFF.send(pullReqMsg,pEP);
}

void FBNN::train_after_pull(const int32_t sae_index,
                            ffnet::NetNervureFromFile& ref_NNFF,
                            const ffnet::EndpointPtr_t& pEP
                           )
{
    //in batch loop
    int32_t curBatchSize = m_sOpts.batchsize;
    if(m_ivBatch == m_iBatchNum - 1 && m_opTrain_x->rows() % m_sOpts.batchsize != 0)
        curBatchSize = m_opTrain_x->rows() % m_sOpts.batchsize;
    FMatrix batch_x(curBatchSize,m_opTrain_x->columns());
    for(int32_t r = 0; r < curBatchSize; ++r)//randperm()
        row(batch_x,r) = row(*m_opTrain_x,m_oRandVec[m_ivBatch * m_sOpts.batchsize + r]);

    //Add noise to input (for use in denoising autoencoder)
    if(m_fInputZeroMaskedFraction != 0)
        batch_x = bitWiseMul(batch_x,(rand(curBatchSize,m_opTrain_x->columns())>m_fInputZeroMaskedFraction));

    (*m_oLp)(m_ivEpoch*m_iBatchNum+m_ivBatch,0) = nnff(batch_x,batch_x);
    nnbp();
    nnapplygrads();
    //push under network conditions
    std::cout << "Need to push weights!" << std::endl;
    //set push ack false
    boost::unique_lock<RWMutex> wlock(m_g_RWMutex);
    set_push_ack(false);
    m_cond_ack.notify_one();
    wlock.unlock();
    //set push package
    boost::shared_ptr<PushParaReq> pushReqMsg(new PushParaReq());
    copy(get_m_odWs().begin(),get_m_odWs().end(),std::back_inserter(pushReqMsg->dWs()));
    pushReqMsg->sae_index() = sae_index;
    ref_NNFF.send(pushReqMsg,pEP);
    ++m_ivBatch;
    train_after_push(sae_index,ref_NNFF,pEP);
}

void FBNN::train_after_push(const int32_t sae_index,
                            ffnet::NetNervureFromFile& ref_NNFF,
                            const ffnet::EndpointPtr_t& pEP
                           )
{
    if(m_ivBatch < m_iBatchNum) {
        std::cout << " " << m_ivBatch << std::endl;
        //wait push ack then pull
        boost::shared_lock<RWMutex> rlock(m_g_RWMutex);
        while (!m_bPushAckReceived) {
            m_cond_ack.wait(rlock);
            std::cout << "Wait push ack!" << std::endl;
        }
        rlock.unlock();
        //pull under network conditions
        std::cout << "Need to pull weights!" << std::endl;
        boost::shared_ptr<PullParaReq> pullReqMsg(new PullParaReq());
        pullReqMsg->sae_index() = sae_index;
        ref_NNFF.send(pullReqMsg,pEP);
    }
    else if(m_ivEpoch < m_sOpts.numpochs) {
//         std::cout << std::endl;
        //std::cout << "elapsed time: " << elapsedTime << "s" << std::endl;
        //loss calculate use nneval
        nneval(m_oLoss, *m_opTrain_x, *m_opTrain_x);
        std::cout << "Full-batch train mse = " << m_oLoss.train_error.back() << std::endl;
        //std::cout << "epoch " << m_ivEpoch+1 << " / " <<  m_sOpts.numpochs << " took " << elapsedTime << " seconds." << std::endl;
        std::cout << "Mini-batch mean squared error on training set is " << columnMean(submatrix(*m_oLp,m_ivEpoch*m_iBatchNum,0UL,m_iBatchNum,m_oLp->columns())) << std::endl;
        m_fLearningRate *= m_fScalingLearningRate;
        std::cout << "end numpochs " << m_ivEpoch << std::endl;
        ++m_ivEpoch;
        m_ivBatch = 0;//reset for next epoch loop
        std::cout << "start numpochs " << m_ivEpoch << std::endl;
        //int32_t elapsedTime...
        randperm(m_opTrain_x->rows(),m_oRandVec);
        std::cout << "start batch: ";
        train_after_push(sae_index,ref_NNFF,pEP);
    }
    else {
        std::cout << "End training!" << std::endl;
    }
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
    m_fTesting = true;
    //training performance
    loss.train_error.push_back(nnff(train_x,train_y));

    //validation performance
    if(valid_x.rows() != 0 && valid_y.rows() != 0)
        loss.valid_error.push_back(nnff(valid_x,valid_y));

    m_fTesting = false;
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


