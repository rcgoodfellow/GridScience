#ifndef GW_SMATRIX
#define GW_SMATRIX

#include "Utility.hxx"
#include <mkl.h>
#include <memory>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace gridworks
{
  template <class T>
  struct SMatrix
  {
    T         *v;
    MKL_INT   *c;
    MKL_INT   *r;

    MKL_INT           n, s;

    SMatrix(MKL_INT n, MKL_INT s) : n{n}, s{s}
    {
      v = aalloc<T>(s);
      c = aalloc<MKL_INT>(s);
      r = aalloc<MKL_INT>(n);

      memset(v, 0, sizeof(T)*s);
      for(int i=0; i<s; ++i) { c[i] = -1; }
      for(int i=0; i<n; ++i) { r[i] = -1; }
    }

    void zero()
    {
      for(int i=0; i<s; ++i) v[i] = T{};
    }

    T& operator[](std::pair<MKL_INT, MKL_INT> ix)
    {
      MKL_INT cb = r[ix.first],
              ce = r[ix.first + 1];

      for(MKL_INT i=cb; i<ce; ++i)
      {
        if(c[i] == ix.second) { return v[i]; }
      }

      throw std::out_of_range{
        "CSR Out of Range (" 
          + std::to_string(ix.first) + "," + std::to_string(ix.second) + ")"
      };
    }
    
    T at(std::pair<MKL_INT, MKL_INT> ix)
    {
      MKL_INT cb = r[ix.first],
              ce = r[ix.first + 1];

      for(MKL_INT i=cb; i<ce; ++i)
      {
        if(c[i] == ix.second) { return v[i]; }
      }

      return T{};
    }

    std::string toString()
    {
      std::stringstream ss;

      ss << "v: [";
      int x = 1, nr = r[x];
      for(size_t i=0; i<s-1; ++i) 
      { 
        ss << v[i] << ","; 
        if(i+1 >= nr && x < n)
        {
          ss << std::endl;
          nr = r[++x];
        }
      }
      ss << v[s-1] << "]" << std::endl << std::endl;
      
      ss << "c: [";
      x = 1, nr = r[x];
      for(size_t i=0; i<s-1; ++i) 
      { 
        ss << c[i] << ","; 
        if(i+1 >= nr && x < n)
        {
          ss << std::endl;
          nr = r[++x];
        }
      }
      ss << c[s-1] << "]" << std::endl << std::endl;
      
      ss << "r: [";
      for(size_t i=0; i<n; ++i) { ss << r[i] << ","; }
      ss << r[n] << "]" << std::endl;

      return ss.str();
    }

    std::string toCsv()
    {
      std::stringstream ss;
      ss << std::setprecision(11);
      for(int i = 0; i<n; ++i)
      {
        for(int j = 0; j<n-1; ++j)
        {
          ss << at({i,j}) << ",";
        }
        ss << at({i,n-1});
        ss << std::endl;
      }
      return ss.str();
    }
  };

}

#endif
