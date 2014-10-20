#ifndef GW_UTILITY
#define GW_UTILITY

#include <complex>
#include <mkl.h>
#include <mm_malloc.h>

#define MKL_Complex16 std::complex<double>

namespace gridworks {

constexpr size_t ALIGNMENT{64};

template <class T>
T* aalloc(size_t n) { return (T*)_mm_malloc(sizeof(T)*n, ALIGNMENT); }
  
template <class T>
struct Glob
{
  T* data{nullptr};
  size_t sz;

  Glob() = default;
  explicit Glob(size_t sz) : sz{sz}
  {
    data = aalloc<T>(sz);
  }
  ~Glob()
  {
    //_mm_free(data);
  }

  T& operator[](size_t i){ return data[i]; }

};
  
constexpr double rad(double deg) { return deg * (M_PI / 180.0); }
constexpr double deg(double rad) { return rad * (180.0 / M_PI); }

}

#endif
