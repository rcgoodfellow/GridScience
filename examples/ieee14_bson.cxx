#include "Grid.hxx"
#include "PowerFlow.hxx"
#include "ModelIO.hxx"
#include "Math.hxx"

using namespace cypress;
using namespace gridworks;
using std::cout;
using std::endl;

int main() {

  cout << "IEEE 14 Bus System Test from json" << endl;

  Grid grid = cypress::gridFromJson("ieee14.json");

  SMatrix<complex> Y = ymatrix(grid);

  Glob<complex> x = grid.flatStart();

  Jacobi J(&grid, Y, x);

  Glob<complex> sSch = cypress::schedule("ieee14_sched.json");
  
  scale(sSch, 100.0);

  PowerFlow pf(&grid, x, sSch, 5e-6);
  pf.run();
  
  cout << endl << pf.result_summary() << endl;
}
