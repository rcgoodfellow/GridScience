#ifndef GW_POWERFLOW
#define GW_POWERFLOW

#include "Grid.hxx"
#include <mkl_dss.h>
#include <mkl_types.h>
#include <cassert>

namespace gridworks {

  struct PowerFlow
  {
    Grid *G;
    SMatrix<complex> Y;
    Glob<complex> state, sSch, sCalc, dSch;
    Jacobi J;
    Glob<double> dX, dS;
    int steps{0};
    const double thresh;
   
    //MKL stuff
    _MKL_DSS_HANDLE_t mkl_handle;
    _INTEGER_t nRhs{1}, mkl_err{ MKL_DSS_SUCCESS };
    _INTEGER_t 
      dss_opt{ MKL_DSS_MSG_LVL_INFO + 
               MKL_DSS_TERM_LVL_ERROR + 
               MKL_DSS_ZERO_BASED_INDEXING
             },
      dss_struct_opt{ MKL_DSS_NON_SYMMETRIC },
      dss_reorder_opt{ MKL_DSS_AUTO_ORDER },
      dss_factor_opt{ MKL_DSS_INDEFINITE },
      dss_solve_opt{ MKL_DSS_DEFAULTS };
    //---------

    PowerFlow(Grid *g, Glob<complex> state, Glob<complex> sSch,
        double thresh = 0.001);

    ~PowerFlow();

    void calc_sCalc();
    void calc_dSch();
    void calc_dS();
    void update_state();
    void mkl_death();
    void init_mkl();
    void step();
    double max_dX();
    double max_dS();
    void run();

  };

}

#endif
