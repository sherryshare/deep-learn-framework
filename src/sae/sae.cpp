#include "sae.h"

namespace ff
{
SAE::SAE(const Arch_t& arch,
         const std::string& activationFunction,
         const double learningRate,
         const double inputZeroMaskedFraction)
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
        m_oAEs.push_back(FBNN_ptr(new  FBNN(t,m_strActivationFunction,m_fLearningRate,m_fInputZeroMaskedFraction)));
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
        m_oAEs[i]->nnff(x, x);
        x = *(m_oAEs[i]->get_m_oAs())[1];
        x = delPreColumn(x);
    }
    std::cout << "End training SAE." << std::endl;
}

void SAE::SAETrain(const FMatrix& train_x, const Opts& opts, const DLWorker* pDLWorker)
{
    std::cout << "Start training SAE." << std::endl;
    size_t num_ae = m_oAEs.size();
    FMatrix x = train_x;
    for( size_t i = 0; i < num_ae; ++i)
    {
        std::cout << "Training AE " << i+1 << " / " << num_ae << ", ";
        std::cout << "x = (" << x.rows() << ", " << x.columns() << ")"<< std::endl;
        m_oAEs[i]->train(x, /*x, */opts, pDLWorker, i);
        m_oAEs[i]->nnff(x, x);
        x = *(m_oAEs[i]->get_m_oAs())[1];
        x = delPreColumn(x);
    }
    std::cout << "End training SAE." << std::endl;
}
}//end namespace ff