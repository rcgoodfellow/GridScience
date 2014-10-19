#include "Grid.hxx"

using namespace gridworks;
  
  
Neighbor::Neighbor(Branch *br , Bus *b) : br{br}, b{b} {}
  
Bus::Bus(size_t id, double rating, complex shunt_y) 
  : id{id}, rating{rating}, shunt_y{shunt_y} {}

Branch::Branch(Kind kind) : kind{kind} {}
  
void Branch::connect(Bus *b0, Bus *b1){ 
  b[0] = b0; b[1] = b1; 
  b0->neighbors.push_back({this, b1});
  b1->neighbors.push_back({this, b0});
}

Line::Line() : Branch(Kind::Line) {}
  
SimpleLine::SimpleLine(complex impedance, complex charging_admittance) 
  : _z(impedance), _cy{charging_admittance} {}
  
complex SimpleLine::z() const { return _z; }
complex SimpleLine::cy() { return _cy; }
  
SimpleTransformer::SimpleTransformer(complex impedance, complex turns_ratio)
  : _z{impedance}, _tr{turns_ratio} {}

Transformer::Transformer() : Branch(Kind::Transformer) {}
  
complex SimpleTransformer::z() const { return _z; }
complex SimpleTransformer::tr() { return _tr; }
  
StaticGen::StaticGen(complex v) : _v{v} {}
complex StaticGen::v(double) { return _v; }

Glob<complex> Grid::sCalc(Glob<complex> x, SMatrix<complex> Y)
{
  using std::abs;
  using std::arg;
  using std::pow;

  Glob<complex> sCalc(buses.size());

  for(size_t i=0; i<buses.size(); ++i)
  {
    Bus &b = *buses[i];

    complex vi = x[i],
            yii = Y[{i,i}];

    double P =  pow(abs(vi), 2) * yii.real(),
           Q = -pow(abs(vi), 2) * yii.imag();

    for(size_t ni=0; ni<b.neighbors.size(); ++ni)
    {
      int j = b.neighbors[ni].b->id;

      complex vj = x[j],
              yij = Y[{i,j}];

      double mag = abs(vi)  * abs(vj) * abs(yij),
              ang = arg(yij) + arg(vj) - arg(vi);

      P += mag * cos(ang);
      Q -= mag * sin(ang);
    }

    sCalc.data[i] = {P,Q};
  }
  
  return sCalc;

}

SMatrix<complex> gridworks::ymatrix(Grid &g) {

  SMatrix<complex> m(g.buses.size(), 
      g.lines.size()*2 + g.transformers.size()*2 + g.buses.size());

  int x{0};
  for(size_t i=0; i<g.buses.size(); ++i) {
    m.r[i] = x;
    x += g.buses[i]->neighbors.size() + 1;
  }
  m.r[g.buses.size()] = x;

  m.zero();

  auto apply = 
  [&g, &m](size_t i) {

    Bus &b = *g.buses[i];
    int off = m.r[i];
    m.c[off] = i;
    m.v[off] += b.shunt_y;
    
    for(size_t j=1; j<=b.neighbors.size(); ++j)
    {
      Neighbor n = b.neighbors[j-1]; 
      m.c[off+j] = n.b->id;
      std::cout << i << " " << n.b->id << std::endl;
      switch(n.br->kind)
      {
        case Branch::Kind::Line:
        {
          Line &l = *static_cast<Line*>(n.br);
          complex y = 1.0/l.z();
          m.v[off+j] = -y;
          m.v[off] += y + 0.5 * l.cy();
          break;
        }

        case Branch::Kind::Transformer:
        {
          Transformer &t = *static_cast<Transformer*>(n.br);
          complex y = 1.0/t.z();
          m.v[off+j] = -(1.0/t.tr().real())*y;
          if(b.rating > n.b->rating)
            m.v[off] += std::pow(std::abs(1.0/t.tr().real()), 2)*y;
          else
            m.v[off] += y;
          break;
        }
      }
    }
  };

  for(size_t i=0; i<g.buses.size(); ++i)
  {
    apply(i);
  }

  return m;
}



//Jacobi ----------------------------------------------------------------------


Jacobi::Jacobi(Grid *g, SMatrix<complex> y, Glob<complex> x) 
  :g(g), y{y}, x{x}
{ 
  computeStructureInfo();
  m = std::make_shared<SMatrix<double>>(jsi.N(), jsi.S());
  computeMapInfo();
  update();
}


void Jacobi::computeStructureInfo() {
  
  for(size_t i=0; i<g->buses.size(); ++i)
  {
    Bus &b = *g->buses[i];
    if(!b.slack) {
      b.jidx[0] = jsi.n[0]++; jsi.s[0]++; 
      if(!b.generator){ jsi.s[1]++; }
      for(size_t j=0; j<b.neighbors.size(); ++j) {
        Bus &o = *g->buses[b.neighbors[j].b->id];
        if(!o.slack) jsi.s[0]++;
        if(!o.generator) jsi.s[1]++;
      }
    }
    if(!b.generator) {
      b.jidx[1] = jsi.n[1]++; 
      jsi.s[2]++; jsi.s[3]++;
      for(size_t j=0; j<b.neighbors.size(); ++j) {
        Bus &o = *g->buses[b.neighbors[j].b->id];
        if(!o.slack) jsi.s[2]++;
        if(!o.generator) jsi.s[3]++;
      }
    }
  }
  for(size_t i=0; i<g->buses.size(); ++i)
  {
    Bus &b = *g->buses[i];
    if(!b.generator) { b.jidx[1] += jsi.n[0]; }
  }
    
}

int qi_cmp (const void * a, const void * b)
{
  return ( *(MKL_INT*)a - *(MKL_INT*)b );
}

void Jacobi::computeMapInfo() {

  auto s1 = 
  [this](Bus &b, int i) 
  {
    int x{0};
    m->c[i++] = b.jidx[0];
    ++x;
    for(size_t j=0; j<b.neighbors.size(); ++j) 
    { 
      Bus &nbr = *g->buses[b.neighbors[j].b->id];
      if(!nbr.slack) 
      { 
        m->c[i++] = nbr.jidx[0];
        ++x;
      }
    }
    return x;
  };
  
  auto s2 = 
  [this](Bus &b, int i) 
  {
    int x{0};
    if(!b.generator)
    {
      m->c[i++] = b.jidx[1];
      x++;
    }
    for(size_t j=0; j<b.neighbors.size(); ++j) 
    { 
      Bus &nbr = *g->buses[b.neighbors[j].b->id];
      if(!nbr.generator) 
      { 
        m->c[i++] = nbr.jidx[1];
        ++x;
      }
    }
    return x;
  };

  m->r[0] = 0;
  int rv{0}, ri{1};
  //dP indices
  for(size_t i=0; i<g->buses.size(); ++i)
  {
    Bus &b = *g->buses[i];
    if(!b.slack) {
      rv += s1(b, rv); 
      rv += s2(b, rv);
      m->r[ri++] = rv; 
    }
  }
  //dQ indicies
  for(size_t i=0; i<g->buses.size(); ++i)
  {
    Bus &b = *g->buses[i];
    if(!b.generator) {
      rv += s1(b, rv); 
      rv += s2(b, rv);
      m->r[ri++] = rv; 
    }
  }

  m->r[jsi.N()] = jsi.S();

  for(int i=0; i<jsi.N(); ++i)
  {
    qsort(&m->c[ m->r[i] ], m->r[i+1] - m->r[i], sizeof(MKL_INT), qi_cmp);
  }
}

void Jacobi::update()
{
  auto gradient = 
  [this](MKL_INT i)
  {
    Bus &b = *g->buses[i];
    if(b.slack) { return; }
    
    SMatrix<complex> &Y = y;
    SMatrix<double> &M = *m;
    Glob<complex> &X = x;

    for(size_t ni=0; ni<b.neighbors.size(); ++ni)
    {
      int j = b.neighbors[ni].b->id;
      Bus &nbr = *g->buses[j];
      
      double vi = std::abs(X[i]),
             vj = std::abs(X[j]),
             ym = std::abs(Y[{i,j}]),

             ti = std::arg(X[i]),
             tj = std::arg(X[j]),
             ya = std::arg(Y[{i,j}]);
      
      double mag = vi * vj * ym,
              ang = ya + tj - ti,

              dPdA =  mag * sin(ang),
              dPdM =  mag * cos(ang),
              dQdA = -mag * cos(ang),
              dQdM = -mag * sin(ang);

      //dP
      if(!b.slack)
      {
        M[{b.jidx[0], b.jidx[0]}] += dPdA;
        if(!b.generator) { M[{b.jidx[0], b.jidx[1]}] += dPdM; }

        if(!nbr.slack)
        {
          M[{b.jidx[0], nbr.jidx[0]}] = -dPdA;
          if(!nbr.generator) { M[{b.jidx[0], nbr.jidx[1]}] = dPdM; }
        }
      }

      //dQ
      if(!b.generator)
      {
        M[{b.jidx[1], b.jidx[0]}] -= dQdA;
        if(!b.generator) { M[{b.jidx[1], b.jidx[1]}] += dQdM; }

        if(!nbr.slack)
        {
          M[{b.jidx[1], nbr.jidx[0]}] = dQdA;
          if(!nbr.generator) { M[{b.jidx[1], nbr.jidx[1]}] = dQdM; }
        }
      }

    }

    if(!b.generator)
    {
      M[{b.jidx[0], b.jidx[1]}] 
        += 2.0 * std::pow(std::abs(X[i]), 2) * Y[{i,i}].real();

      M[{b.jidx[1], b.jidx[1]}] 
        -= 2.0 * std::pow(std::abs(X[i]), 2) * Y[{i,i}].imag();
    }
  
  };
      
  m->zero();
  for(size_t i=0; i<g->buses.size(); ++i)
  {
    gradient(i);
  }


}
