#include "sae/sae_from_config.h"

namespace ff {
SAE_ptr SAE_create(NervureConfigurePtr const pnc)
{
    srand(time(NULL));
    std::string activationFunction = pnc->get<std::string>("init.activation-function");
    double learningRate = pnc->get<double>("init.learning-rate");
    double inputZeroMaskedFraction = pnc->get<double>("init.input-zero-masked-fraction");
    std::string saeStructureStr = pnc->get<std::string>("init.sae-structure");
    
    std::cout << activationFunction << "\t" << learningRate << "\t" << inputZeroMaskedFraction << std::endl;
    std::vector<int> structure;
    std::stringstream ss;
    ss << saeStructureStr;
    int i;
    while(ss >> i)
    {
      structure.push_back(i);
    }
    
    Arch_t c(structure.size());
    for(size_t i = 0; i < structure.size(); ++ i)
    {
      c[i] = structure[i];
    }
    
    //train a 100 hidden unit SDAE and use it to initialize a FFNN
    //Setup and train a stacked denoising autoencoder (SDAE)
    std::cout << "Pretrain an SAE" << std::endl;
    std::cout << "c = " << c << "numel(c) = " << numel(c) << std::endl;
    SAE sae(c,activationFunction,learningRate,inputZeroMaskedFraction);
    return std::make_shared<SAE>(sae);
}


bool SAE_run(SAE_ptr const psae,std::string data_dir, NervureConfigurePtr const pnc)
{
//     if(!psae)
//       return false;
    FMatrix_ptr train_x = read_matrix_from_dir(data_dir);
    if(train_x == nullptr)
        return false;
    *train_x = (*train_x) / 255;
    Opts opts;
   opts.numpochs = pnc->get<int>("opt.num-epochs");
    std::cout << "numpochs = " << opts.numpochs << std::endl;
    opts.batchsize = pnc->get<int>("opt.batch-size");
    psae->SAETrain(*train_x,opts);
    return true;
}

void train_NN(SAE_ptr const psae, NervureConfigurePtr const pnc)
{
//     if(!psae)
//       return;
    std::string input_file = pnc->get<std::string>("path.input-file");
    std::string activationFunction = pnc->get<std::string>("init.activation-function");
    double learningRate = pnc->get<double>("init.learning-rate");
    std::string nnStructureStr = pnc->get<std::string>("init.nn-structure");
    TData d = read_data(input_file);
    if(d.train_x == nullptr || d.train_y == nullptr || d.test_x == nullptr || d.test_y == nullptr)
      return;
    *d.train_x = (*d.train_x) / 255;
    *d.test_x = (*d.test_x) / 255;

    //add for quick test
    *d.train_x = submatrix(*d.train_x,0UL,0UL,20,d.train_x->columns());
    *d.train_y = submatrix(*d.train_y,0UL,0UL,20,d.train_y->columns());
    *d.test_x = submatrix(*d.test_x,0UL,0UL,20,d.test_x->columns());
    *d.test_y = submatrix(*d.test_y,0UL,0UL,20,d.test_y->columns());


    //Use the SDAE to initialize a FFNN
    std::cout << "Train an FFNN" << std::endl;
    std::vector<int> structure;
    std::stringstream ss(nnStructureStr);
    int i;
    while(ss >> i)
    {
      structure.push_back(i);
    }  
    Arch_t cn(structure.size());
    for(size_t i = 0; i < structure.size(); ++ i)
    {
      cn[i] = structure[i];
    }
    std::cout << "cn = " << cn << "numel(cn) = " << numel(cn) << std::endl;
    FBNN nn(cn,activationFunction,learningRate);
    Opts opts;
    opts.numpochs = pnc->get<int>("opt.num-epochs");
    std::cout << "numpochs = " << opts.numpochs << std::endl;
    opts.batchsize = pnc->get<int>("opt.batch-size");
    //check if nn structure is correct
    std::vector<FMatrix_ptr> & m_oWs = nn.get_m_oWs();
    std::vector<FMatrix_ptr> & m_oVWs = nn.get_m_oVWs();
    std::vector<FMatrix_ptr> & m_oPs = nn.get_m_oPs();
    for(int j = 0; j < m_oWs.size(); j++) {
        std::cout << "W[" << j << "] = {" << m_oWs[j]->rows() << ", " << m_oWs[j]->columns() << "}" << std::endl;
        if(!m_oVWs.empty())
            std::cout << "vW[" << j << "] = {" << m_oVWs[j]->rows() << ", " << m_oVWs[j]->columns() << "}" << std::endl;
        if(!m_oPs.empty())
            std::cout << "P[" << j << "] = {" << m_oPs[j]->rows() << ", " << m_oPs[j]->columns() << "}" << std::endl;
    }

    for(int i = 0; i < m_oWs.size() - 1; i++)
    {
	if(m_oWs[i]->rows() != (psae->get_m_oAEs()[i]->get_m_oWs())[0]->rows() ||
	  m_oWs[i]->columns() != (psae->get_m_oAEs()[i]->get_m_oWs())[0]->columns() )
	{
	  std::cout << "FFNN structure doesn't match SAE structure! Train by default value from level " << i << "..." << std::endl;
	  break;
	}
        *m_oWs[i] = *(psae->get_m_oAEs()[i]->get_m_oWs())[0];//nn.W{i} = sae.ae{i}.W{1};
    }

    //Train the FFNN
    nn.train(*d.train_x,*d.train_y,opts);
    double error = nn.nntest(*d.test_x,*d.test_y);
    std::cout << "test error = " << error << std::endl;
    if(error >= 0.16)
        std::cout << "Too big error!" << std::endl;

}


};//end namespace ff
