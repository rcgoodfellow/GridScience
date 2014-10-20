#ifndef CYPRESS_MATH
#define CYPRESS_MATH

#include "Utility.hxx"

namespace cypress {

template<typename T, typename S>
gridworks::Glob<T>& scale(gridworks::Glob<T> &g, S scalar) {
  for(size_t i=0; i<g.sz; ++i){ g.data[i] /= scalar; }
  return g;
}

}
#endif
