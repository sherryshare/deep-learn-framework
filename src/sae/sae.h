/*
 * This shall be header file for SAE
 */
#pragma once
#include "common/common.h"
#include "nn/fbnn.h"

namespace ff
{
  class SAE;
  typedef std::shared_ptr<SAE> SAE_ptr;
  class SAE
  {
    public:
        SAE(const Arch_t & arch,std::string activationFunction = "sigm", double learningRate = 1, double inputZeroMaskedFraction = 0.5);
        void    SAETrain(const FMatrix & train_x, const Opts & opts, const SAE_ptr & pSAE = nullptr);
	std::vector<FBNN_ptr> & get_m_oAEs(void){return m_oAEs;};

    protected:
        std::vector<FBNN_ptr>        m_oAEs;
        std::string    m_strActivationFunction;
        double          m_fLearningRate;
        double          m_fInputZeroMaskedFraction;
  };//end class SAE
  
};//end namespace ff

