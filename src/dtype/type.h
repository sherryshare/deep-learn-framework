#ifndef FFDL_DTYPE_TYPE_H_
#define FFDL_DTYPE_TYPE_H_

#include <blaze/Math.h>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits.hpp>

// #include <common.h>

namespace ff
{
typedef blaze::DynamicMatrix<double, blaze::rowMajor> FMatrix;
typedef boost::shared_ptr<FMatrix> FMatrix_ptr;
typedef blaze::DynamicVector<int32_t, blaze::columnVector> FColumn;

template <class T>
struct is_matrix {
    const static bool value=false;
};

template <>
struct is_matrix<FMatrix> {
    const static bool value = true;
};
template<class T1, class T2>
struct is_one_matrix_one_arith {
    static const bool value = (boost::is_arithmetic<T1>::value || boost::is_arithmetic<T2>::value) && (is_matrix<T1>::value || is_matrix<T2>::value);
};//end struct is_one_matrix_one_arith;

template <class T1, class T2>
struct matrix_type {
    typedef typename boost::conditional<is_matrix<T1>::value, T1, T2>::type type;
};//end struct matrix_type
}//end namespace ff

#endif
