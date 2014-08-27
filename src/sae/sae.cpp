#include "sae.h"

namespace ff
{
SAE::SAE(const Arch_t& arch,
         const std::string& activationFunction,
         const double learningRate,
         const double inputZeroMaskedFraction,
         const int32_t maxSynchronicStep
        )
    : m_strActivationFunction(activationFunction)
    , m_fLearningRate(learningRate)
    , m_fInputZeroMaskedFraction(inputZeroMaskedFraction)
{
    std::cout << "SAE initialize!" << std::endl;
    for(size_t i = 1; i < numel(arch); ++i)
    {
        Arch_t t(3UL);
        t[0] = arch[i-1];
        t[1] = arch[i];
        t[2] = arch[i-1];
        m_oAEs.push_back(FBNN_ptr(new FBNN(t,m_strActivationFunction,m_fLearningRate,
                                           m_fInputZeroMaskedFraction,maxSynchronicStep)));
    }
    std::cout << "Finish initialize!" << std::endl;
}

void SAE::SAETrain(const FMatrix& train_x, const Opts& opts)
{
    std::cout << "Start training SAE." << std::endl;
    size_t num_ae = m_oAEs.size();
    FMatrix x = train_x;
    for( size_t i = 0; i < num_ae; ++i)
    {
        std::cout << "Training AE " << i+1 << " / " << num_ae << ", ";
        std::cout << "x = (" << x.rows() << ", " << x.columns() << ")"<< std::endl;
        m_oAEs[i]->train(x, x, opts);
        if(i < num_ae - 1)//No need to get output from last level
        {
            m_oAEs[i]->nnff(x, x);
            x = *(m_oAEs[i]->get_m_oAs())[1];
            x = delPreColumn(x);
        }
    }
    std::cout << "End training SAE." << std::endl;
}

void SAE::SAETest(const FMatrix& train_x, const Opts& opts)
{
    std::cout << "Start testing SAE." << std::endl;
    size_t num_ae = m_oAEs.size();
    FMatrix x = train_x;
    for( size_t i = 0; i < num_ae; ++i)
    {
        std::cout << "Testing AE " << i+1 << " / " << num_ae << ", ";
        std::cout << "x = (" << x.rows() << ", " << x.columns() << ")"<< std::endl;
        m_oAEs[i]->AEtest(x,opts);
        if(i < num_ae - 1)//No need to get output from last level
        {
            m_oAEs[i]->nnff(x, x);
            x = *(m_oAEs[i]->get_m_oAs())[1];
            x = delPreColumn(x);
        }
    }
    std::cout << "End testing SAE." << std::endl;
}



void SAE::SAETrain(const FMatrix& train_x,
                   const Opts& opts,
                   ffnet::NetNervureFromFile& ref_NNFF,
                   const ffnet::EndpointPtr_t& pEP,
                   TimePoint& startTime,
                   int32_t defaultSynchronicStep
                  )
{
    m_iDefaultSynchronicStep = defaultSynchronicStep;
    std::cout << "Start training SAE." << std::endl;
    m_pTrain_x = FMatrix_ptr(new FMatrix(train_x));
    m_iAEIndex = 0;
    m_sOpts = opts;
    std::cout << "Training AE " << m_iAEIndex+1 << " / " << m_oAEs.size() << ", ";
    std::cout << "x = (" << m_pTrain_x->rows() << ", " << m_pTrain_x->columns() << ")"<< std::endl;
    m_oAEs[m_iAEIndex]->train(*m_pTrain_x, m_sOpts, ref_NNFF, pEP,
                              m_iAEIndex, startTime, m_iDefaultSynchronicStep);
}

bool SAE::train_after_end_AE(ffnet::NetNervureFromFile& ref_NNFF,
                             const ffnet::EndpointPtr_t& pEP,
                             TimePoint& startTime
                            )
{
//     std::cout << "Start train_after_end_AE:" << std::endl;
    if(m_iAEIndex < m_oAEs.size() - 1)//No need to get output from last level
    {
        m_oAEs[m_iAEIndex]->nnff(*m_pTrain_x, *m_pTrain_x);
        m_pTrain_x = (m_oAEs[m_iAEIndex]->get_m_oAs())[1];
        m_pTrain_x = FMatrix_ptr(new FMatrix(delPreColumn(*m_pTrain_x)));
    }
    ++m_iAEIndex;
    if(m_iAEIndex < m_oAEs.size())
    {
        std::cout << "Training AE " << m_iAEIndex+1 << " / " << m_oAEs.size() << ", ";
        std::cout << "x = (" << m_pTrain_x->rows() << ", " << m_pTrain_x->columns() << ")"<< std::endl;
        m_oAEs[m_iAEIndex]->train(*m_pTrain_x, m_sOpts, ref_NNFF, pEP,
                                  m_iAEIndex,startTime, m_iDefaultSynchronicStep);
//         std::cout << "End train_after_end_AE." << std::endl;
    }
    else
    {
//         std::cout << "End train_after_end_AE." << std::endl;
        std::cout << "End training SAE." << std::endl;
        return true;
    }
    return false;
}



}//end namespace ff
