#ifndef FFDL_NN_ARCH_H_
#define FFDL_NN_ARCH_H_

#include "dtype/type.h"

namespace ff{
//  typedef FMatrix Arch_t;
  
//   typedef blaze::StaticVector<int, 3UL, blaze::columnVector> Arch_t;
  typedef blaze::DynamicVector<int32_t, blaze::columnVector> Arch_t;
  size_t     numel(const Arch_t& a);

}//end namespace ff

#endif
