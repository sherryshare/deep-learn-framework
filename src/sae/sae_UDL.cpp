#include "sae/sae_UDL.h"

namespace ff {
SAE_ptr SAE_create(void )
{
    srand(time(NULL));
    //train a 100 hidden unit SDAE and use it to initialize a FFNN
    //Setup and train a stacked denoising autoencoder (SDAE)
    std::cout << "Pretrain an SAE" << std::endl;
    Arch_t c(2UL);
    c[0] = 784;
    c[1] = 100;
//    ff::Arch_t c(3UL);
//    c[0] = 784;
//    c[1] = 3000;
//    c[2] = 500;
    std::cout << "c = " << c << "numel(c) = " << numel(c) << std::endl;
    SAE sae(c);
    return std::make_shared<SAE>(sae);
}


bool SAE_run(SAE_ptr psae,std::string data_dir)
{
//     if(!psae)
//       return false;
    FMatrix_ptr train_x = read_matrix_from_dir(data_dir);
    if(train_x == nullptr)
        return false;
    *train_x = (*train_x) / 255;
    Opts opts;
//    opts.numpochs = 2;//50
    std::cout << "numpochs = " << opts.numpochs << std::endl;
    psae->SAETrain(*train_x,opts);
    return true;
}

void train_NN(SAE_ptr psae,std::string input_file)
{
//     if(!psae)
//       return;
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
    Arch_t cn(3UL);
    cn[0] = 784;
    cn[1] = 100;
    cn[2] = 10;
//    ff:Arch_t cn(4UL);
//    cn[0] = 784;
//    cn[1] = 3000;
//    cn[2] = 500;
//    cn[3] = 10;
    std::cout << "cn = " << cn << "numel(cn) = " << numel(cn) << std::endl;
//    ff::FBNN nn(cn,"sigm",0.1);
    FBNN nn(cn,"sigm",1);
    Opts opts;
//    opts.numpochs = 3;//200
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
        *m_oWs[i] = *(psae->get_m_oAEs()[i]->get_m_oWs())[0];//nn.W{i} = sae.ae{i}.W{1};
    }


//     std::cout << "d.train_x = (" << d.train_x->rows() << "," << d.train_x->columns() << ")" << std::endl;

    //Train the FFNN
    nn.train(*d.train_x,*d.train_y,opts);
    double error = nn.nntest(*d.test_x,*d.test_y);
    std::cout << "test error = " << error << std::endl;
    if(error >= 0.16)
        std::cout << "Too big error!" << std::endl;

}


};//end namespace ff
