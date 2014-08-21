#include "sae/sae_from_config.h"

namespace ff {
SAE_ptr SAE_create(const NervureConfigurePtr& pnc)
{
    srand(time(NULL));
    const std::string activationFunction = getActivationFunctionFromNervureConfigure(pnc);
    const double learningRate = getLearningRateFromNervureConfigure(pnc);
    const double inputZeroMaskedFraction = getInputZeroMaskedFractionFromNervureConfigure(pnc);
    const int32_t maxSynchronicStep = getMaxSynchronicStepFromNervureConfigure(pnc);
    std::cout << activationFunction << "\t" << learningRate << "\t" << inputZeroMaskedFraction << std::endl;
    std::cout << "Max synchronic step = " << maxSynchronicStep << std::endl;
    Arch_t sae_arch;
    getArchFromNervureConfigure(pnc,"sae",sae_arch);

    //train a 100 hidden unit SDAE and use it to initialize a FFNN
    //Setup and train a stacked denoising autoencoder (SDAE)
    std::cout << "Pretrain an SAE" << std::endl;
    std::cout << "sae_arch = " << sae_arch << "numel(sae_arch) = " << numel(sae_arch) << std::endl;
//     SAE sae(sae_arch,activationFunction,learningRate,inputZeroMaskedFraction);
    return SAE_ptr(new SAE(sae_arch,activationFunction,learningRate,
                           inputZeroMaskedFraction,maxSynchronicStep-1));//count from 0
}

void getArchFromNervureConfigure(const NervureConfigurePtr& pnc,
                                 const std::string& structure_name,
                                 Arch_t& arch) {
    std::string structure_field = static_cast<std::string>("init.") + structure_name + "-structure";
    std::string readStructureStr = pnc->get<std::string>(structure_field);
    std::vector<unsigned long> structure;
    std::stringstream ss;
    ss << readStructureStr;
    unsigned long ul_nodeNumber;
    while(ss >> ul_nodeNumber)
    {
        structure.push_back(ul_nodeNumber);
    }

    arch.resize(structure.size());
    for(size_t i = 0; i < structure.size(); ++i)
    {
        arch[i] = structure[i];
    }
}

bool SAE_run(const SAE_ptr& psae,
             const std::string& data_dir, 
             const NervureConfigurePtr& pnc)
{
    FMatrix_ptr train_x = read_matrix_from_dir(data_dir);
    if(train_x == NULL)
        return false;
    *train_x = (*train_x) / 255;
    Opts opts;
    getOptsFromNervureConfigure(pnc,opts);
    psae->SAETrain(*train_x,opts);
    return true;
}

void train_NN(const SAE_ptr& psae, const NervureConfigurePtr& pnc)
{
    const std::string input_file = getInputFileNameFromNervureConfigure(pnc);
    const std::string activationFunction = getActivationFunctionFromNervureConfigure(pnc);
    const double learningRate = getLearningRateFromNervureConfigure(pnc);
    TData d = read_data(input_file);
    if(d.train_x == NULL || d.train_y == NULL || d.test_x == NULL || d.test_y == NULL)
        return;
    *d.train_x = (*d.train_x) / 255;
    *d.test_x = (*d.test_x) / 255;

    //add for quick test
//     *d.train_x = submatrix(*d.train_x,0UL,0UL,20,d.train_x->columns());
//     *d.train_y = submatrix(*d.train_y,0UL,0UL,20,d.train_y->columns());
//     *d.test_x = submatrix(*d.test_x,0UL,0UL,20,d.test_x->columns());
//     *d.test_y = submatrix(*d.test_y,0UL,0UL,20,d.test_y->columns());


    //Use the SDAE to initialize a FFNN
    Arch_t nn_arch;
    getArchFromNervureConfigure(pnc,"nn",nn_arch);
    std::cout << "nn_arch = " << nn_arch << "numel(nn_arch) = " << numel(nn_arch) << std::endl;
    FBNN nn(nn_arch,activationFunction,learningRate);
    Opts opts;
    getOptsFromNervureConfigure(pnc,opts);
    //check if nn structure is correct
    const std::vector<FMatrix_ptr> & m_oWs = nn.get_m_oWs();
    const std::vector<FMatrix_ptr> & m_oVWs = nn.get_m_oVWs();
    const std::vector<FMatrix_ptr> & m_oPs = nn.get_m_oPs();
    for(int j = 0; j < m_oWs.size(); ++j) {
        std::cout << "W[" << j << "] = {" << m_oWs[j]->rows() << ", " << m_oWs[j]->columns() << "}" << std::endl;
        if(!m_oVWs.empty())
            std::cout << "vW[" << j << "] = {" << m_oVWs[j]->rows() << ", " << m_oVWs[j]->columns() << "}" << std::endl;
        if(!m_oPs.empty())
            std::cout << "P[" << j << "] = {" << m_oPs[j]->rows() << ", " << m_oPs[j]->columns() << "}" << std::endl;
    }

    for(int i = 0; i < m_oWs.size() - 1; ++i)
    {
        if(m_oWs[i]->rows() != (psae->get_m_oAEs()[i]->get_m_oWs())[0]->rows() ||
                m_oWs[i]->columns() != (psae->get_m_oAEs()[i]->get_m_oWs())[0]->columns() )
        {
            std::cout << "FFNN structure doesn't match SAE structure! Train by default value from level " << i << "..." << std::endl;
            break;
        }
        nn.set_m_oWs_column((psae->get_m_oAEs()[i]->get_m_oWs())[0],i);//nn.W{i} = sae.ae{i}.W{1};
    }

    //Train the FFNN
    nn.train(*d.train_x,*d.train_y,opts);
    double error = nn.nntest(*d.test_x,*d.test_y);
    std::cout << "test error = " << error << std::endl;
    if(error >= 0.16)
        std::cout << "Too big error!" << std::endl;

}


}//end namespace ff
