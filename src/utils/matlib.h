#ifndef FFDL_UTILS_MATLIB_H_
#define FFDL_UTILS_MATLIB_H_

//This file is used to extend some operations from matlab
#include "dtype/type.h"
#include "common/common.h"

namespace ff
{

double        rand(void);

FMatrix      rand(const int32_t m, const int32_t n);

void randperm(const int32_t n, std::vector<int32_t>& iVector);

template<class T>
size_t      numel(const T& t, typename boost::enable_if<is_matrix<T>, void>::type* p = NULL)
{
    return t.rows() * t.columns();
}

FMatrix  zeros(const int32_t m, const int32_t n);

FMatrix     ones(const int32_t m, const int32_t n);

///TODO : performance issue here, we should use lazy evaluation here!
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type add(const FMatrix& m, const T& v)
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            res(i, j) = m(i, j) + v;
        }
    }
    return res;
}

/////////////////////////////////
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type sub(const FMatrix& m, const T& v)
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            res(i, j) = m(i, j) - v;
        }
    }
    return res;
}
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type sub(const T& v, const FMatrix& m )
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            res(i, j) = v - m(i, j);
        }
    }
    return res;
}

/////////////////
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type mul(const FMatrix& m, const T& v)
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            res(i, j) = m(i, j) * v;
        }
    }
    return res;
}

///////////////////
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type div(const FMatrix& m, const T& v)
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            res(i, j) = m(i, j) / v;
        }
    }
    return res;
}
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type div(const T& v, const FMatrix& m )
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            res(i, j) = v / m(i, j);
        }
    }
    return res;
}
////////////////
FMatrix trans(const FMatrix& m);

template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type largerThan(const FMatrix& m, const T& v)
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            if(m(i, j) > v)
                res(i, j) = 1;
            else
                res(i, j) = 0;
        }
    }
    return res;
}
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type largerThan(const T& v, const FMatrix& m )
{
    FMatrix res(m.rows(), m.columns());
    for(size_t i = 0; i < m.rows(); ++i)
    {
        for(size_t j = 0; j < m.columns(); ++j)
        {
            if(v > m(i, j))
                res(i, j) = 1;
            else
                res(i, j) = 0;
        }
    }
    return res;
}
///////////////////
FMatrix bitWiseMul(const FMatrix& m, const FMatrix& m1);

////////////////////
template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, FMatrix>::type addPreColumn(const FMatrix& m, const T& v )//add a column before m
{
    FMatrix res(m.rows(),m.columns() + 1);
    column(res,0) = v;
    submatrix(res,0UL,1UL,m.rows(),m.columns()) = m;
    return res;
}
FMatrix     delPreColumn(const FMatrix& m );

FMatrix     columnMean(const FMatrix& m );

FColumn     rowMaxIndexes(const FMatrix& m );

std::vector<int32_t>	findUnequalIndexes(const FColumn& c, const FColumn& c1);

double     matrixSum(const FMatrix& m );

FMatrix bitWiseSquare(const FMatrix& m);

FMatrix bitWiseLog(const FMatrix& m);

FMatrix repmat(const FMatrix& m, const int32_t rowX, const int32_t columnX);

};//end namespace ff
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator + (const ff::FMatrix& t1, const T& t)
{
    return ff::add(t1, t);
}
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator + (const T& t1, const ff::FMatrix& t )
{
    return ff::add(t, t1);
}

//////////////////////////////
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator -  (const ff::FMatrix& t1, const T& t )
{
    return ff::sub(t1, t);
}
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator - (const T& t1, const ff::FMatrix& t )
{
    return ff::sub(t1, t);
}

///////////////
//     template <class T>
//     typename std::enable_if<std::is_arithmetic<T>::value, ff::FMatrix>::type operator * (const ff::FMatrix& t1, const T& t)
//     {
//       return ff::mul(t1, t);
//     }
//     template <class T>
//     typename std::enable_if<std::is_arithmetic<T>::value, ff::FMatrix>::type operator * (const T& t1, const ff::FMatrix& t )
//     {
//       return ff::mul(t, t1);
//     }

////////////////
//     template <class T>
//     typename std::enable_if<std::is_arithmetic<T>::value, ff::FMatrix>::type operator / (const ff::FMatrix& t1, const T& t )
//     {
//       return ff::div(t1, t);
//     }
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator / (const T& t1, const ff::FMatrix& t )
{
    return ff::div(t1, t);
}
////////////////
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator > (const ff::FMatrix& t1, const T& t)
{
    return ff::largerThan(t1, t);
}
template <class T>
typename boost::enable_if<boost::is_arithmetic<T>, ff::FMatrix>::type operator > (const T& t1, const ff::FMatrix& t )
{
    return ff::largerThan(t1, t);
}

#endif
