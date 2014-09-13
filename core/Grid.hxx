#ifndef _GW_GRID_
#define _GW_GRID_

#include "SMatrix.hxx"

#include <complex>
#include <cmath>
#include <vector>
#include <array>

namespace gridworks {

using complex = std::complex<double>;
using std::polar;
using std::vector;
using std::array;

struct Branch;
struct Line;
struct Transformer;
struct Bus;
struct Generator;
struct Load;

struct Grid {
  vector<Bus*> buses;
  vector<Line*> lines;
  vector<Transformer*> transformers;
  vector<Generator*> generators;
  vector<Load*> loads;

  Glob<complex> sCalc(Glob<complex> state, SMatrix<complex> Y);
};

SMatrix<complex> ymatrix(Grid &g);

struct Neighbor
{
  Neighbor(Branch *br , Bus *b);
  Branch *br;
  Bus *b;
};

struct Bus {
  Bus(size_t id, double rating, complex shunt_y={0,0});

  size_t id;
  double rating;
  complex shunt_y;
  vector<Neighbor> neighbors; 
  bool slack{false};
  Generator *generator{nullptr};
  Load *load{nullptr};
  int jidx[2]{-1,-1};
};

struct Branch {
  enum class Kind{ Line, Transformer };
  Kind kind;

  Branch(Kind kind);

  array<Bus*, 2> b;
  virtual complex z() const = 0;  //branch impedance
  
  void connect(Bus *b0, Bus *b1);
};

struct Line : public Branch {
  Line();
  virtual complex cy() = 0; //charging admittance
};

struct SimpleLine : public Line {
  explicit SimpleLine(complex impedance, complex charging_admittance = {0,0});
  complex _z, _cy;
  complex z() const override;
  complex cy() override;
};

struct Transformer : public Branch {
  Transformer();
  virtual complex tr() = 0;
};

struct SimpleTransformer : public Transformer {
  SimpleTransformer(complex impedance, complex turns_ratio);
  complex _z, _tr;
  complex z() const override;
  complex tr() override;
};

struct Generator {
  Bus *bus;
  virtual complex v(double t) = 0;
};

struct Load {

};

struct StaticGen : public Generator {
  StaticGen(complex v);
  complex _v;
  complex v(double t) override;
};


struct JacobiStructureInfo {
  int n[2]{0,0};
  int s[4]{0,0,0,0};
  int N() { return n[0] + n[1]; }
  int S() { return s[0] + s[1] + s[2] + s[3]; }

  std::string toString()
  {
    std::stringstream ss;
    ss
      << "n: (" << n[0] << "," << n[1] << ")" << std::endl
      << "s: (" << s[0] << "," << s[1] << "," << s[2] << "," << s[3] << ")" 
                << std::endl
      << "N: " << N() << std::endl
      << "S: " << S();

    return ss.str();
  }
};

struct Jacobi {

  Grid *g;
  JacobiStructureInfo jsi;
  std::shared_ptr<SMatrix<double>> m;
  SMatrix<complex> y;
  Glob<complex> x;

  Jacobi(Grid *g, SMatrix<complex> y, Glob<complex> x);

  void computeStructureInfo();
  void computeMapInfo();
  void update();

};

}

#endif
