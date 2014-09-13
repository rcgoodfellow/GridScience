#include "PowerFlow.hxx"

using namespace gridworks;

PowerFlow::PowerFlow(Grid *g, Glob<complex> state, Glob<complex> sSch,
    double thresh)
  : G{g}, 
    Y{ymatrix(*g)}, 
    state{state}, 
    sSch{sSch},
    dSch(g->buses.size()),
    J{G, Y, state},
    dX(J.m->n),
    dS(J.m->n),
    thresh{thresh}
{ 

  init_mkl();
  
  calc_sCalc();
  calc_dSch();
  calc_dS();
}

PowerFlow::~PowerFlow()
{
  dss_delete(mkl_handle, dss_solve_opt);
  mkl_free_buffers();
}

void PowerFlow::calc_sCalc()
{
  sCalc = G->sCalc(state, Y); 
}
    
void PowerFlow::calc_dSch()
{
  size_t n = G->buses.size();
  for(size_t i=0; i<n; ++i) 
    dSch.data[i] = sSch.data[i] - sCalc.data[i];
}
    
void PowerFlow::calc_dS()
{
  for(size_t i=0; i<G->buses.size(); ++i)
  {
    Bus &b = *G->buses[i];
    if(b.slack) { continue; }
    if(b.generator) { dS.data[b.jidx[0]] = dSch.data[i].real(); }
    if(!b.generator) 
    { 
      dS.data[b.jidx[0]] = dSch.data[i].real();
      dS.data[b.jidx[1]] = dSch.data[i].imag(); 
    }
  }
}
    
void PowerFlow::update_state()
{
  using std::abs;
  using std::arg;
  using std::polar;

  for(size_t i=0; i<G->buses.size(); ++i)
  {
    Bus &b = *G->buses[i]; 
    complex &vi = state.data[i];

    if(b.slack) { continue; }
    if(b.generator) 
    { 
      vi = polar( abs(vi), 
                  arg(vi) + dX.data[b.jidx[0]] );
    }
    if(!b.generator) 
    { 
      vi = polar( abs(vi) + abs(vi) * dX.data[b.jidx[1]], 
                  arg(vi) + dX.data[b.jidx[0]] );
    }
  }
}
    
void PowerFlow::mkl_death()
{
  throw std::runtime_error{
    "MKL has exploded with error: " + std::to_string(mkl_err)
  };
}
    
void PowerFlow::init_mkl()
{
  //create
  mkl_err = dss_create(mkl_handle, dss_opt);
  if(mkl_err != MKL_DSS_SUCCESS) { mkl_death(); }
}

void PowerFlow::step()
{
  //structure
  mkl_err = dss_define_structure(
      mkl_handle, dss_struct_opt, 
      J.m->r, J.m->n, J.m->n, J.m->c, J.m->s);
  if(mkl_err != MKL_DSS_SUCCESS) { mkl_death(); }

  //reorder
  mkl_err = dss_reorder(mkl_handle, dss_reorder_opt, 0);
  if(mkl_err != MKL_DSS_SUCCESS) { mkl_death(); }

  //factor
  mkl_err = dss_factor_real(mkl_handle, dss_factor_opt, J.m->v);
  if(mkl_err != MKL_DSS_SUCCESS) { mkl_death(); }

  //solve
  mkl_err = 
    dss_solve_real(mkl_handle, dss_solve_opt, dS.data, nRhs, dX.data);
  if(mkl_err != MKL_DSS_SUCCESS) { mkl_death(); }
 
  update_state();
  calc_sCalc();
  calc_dSch();
  calc_dS();

  J.update();
  
  ++steps;
}
    
double PowerFlow::max_dX()
{
  double max{0};
  for(int i=0; i<J.m->n; ++i)
  {
    max = std::max<double>(max, std::abs(dX.data[i])); 
  }
  return max;
}

double PowerFlow::max_dS()
{
  double max{0};
  for(int i=0; i<J.m->n; ++i)
  {
    max = std::max<double>(max, std::abs(dS.data[i])); 
  }
  return max;
}

void PowerFlow::run()
{
  steps = 0;
  step();
  while(max_dS() > thresh)
  {
    step();
  }
}
