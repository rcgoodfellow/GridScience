#ifndef _GW_GRID_
#define _GW_GRID_

#include "SMatrix.hxx"

#include <complex>
#include <cmath>
#include <vector>
#include <array>

namespace gridworks {

//Name scopes .................................................................
using complex = std::complex<double>;
using std::polar;
using std::vector;
using std::array;

//Forward Declarations ........................................................
struct Branch;
struct Line;
struct Transformer;
struct Bus;
struct Generator;
struct Load;
struct ShuntCap;

/*=============================================================================
 * A #Grid is a composition of #Bus, #Line, #Transformer, #Generator and 
 * #Load objects
 *===========================================================================*/
struct Grid {
  //data ----------------------------------------------------------------------
  vector<Bus*>          buses;
  vector<ShuntCap*>     shunt_caps;
  vector<Line*>         lines;
  vector<Transformer*>  transformers;
  vector<Generator*>    generators;
  vector<Load*>         loads;

  //methods -------------------------------------------------------------------
  Glob<complex> sCalc(Glob<complex> state, SMatrix<complex> Y);
  Glob<complex> flatStart();
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Given a #Grid object the $ymatrix function returns the admittance matrix
 * of the @grid in the form of a #SMatrix sparse matrix object
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
SMatrix<complex> ymatrix(Grid &grid);

/*=============================================================================
 * The #Neighbor class connects a #Bus to a neighboring #Bus via a #Branch
 *===========================================================================*/
struct Neighbor
{
  //data ----------------------------------------------------------------------
  Branch  *br;  //connecting branch
  Bus     *b;   //neighboring bus

  //constructors --------------------------------------------------------------
  Neighbor(Branch *br , Bus *b);
};

/*=============================================================================
 * The #Bus class represents a bus in a power system. The #Bus is the principle
 * decoupling object in the power grid as all components interconnect
 * indirectly through busses
 *===========================================================================*/
struct Bus {
  //data ----------------------------------------------------------------------
  int               id{-1};                   //identification key
  double            rating;               //rating in kV
  //TODO: Repalce this with the ShuntCap object
  complex           shunt_y;              //shunt capacitance
  vector<Neighbor>  neighbors;            //neighboring buses
  bool              slack{false};         //slack bus indicator
  Generator         *generator{nullptr};  //attached generator
  Load              *load{nullptr};       //attached load
  int               jidx[2]{-1,-1};       //jacobean index information
 
  //constructors --------------------------------------------------------------
  Bus(int id, double rating, complex shunt_y={0,0});
};

/*=============================================================================
 * A #ShuntCap is a shunt capacitor connected to a bus for a variaty or
 * purposes, voltage stability, power factor correction ect.
 *===========================================================================*/
struct ShuntCap {
  //data ----------------------------------------------------------------------
  int id{-1};     //the id of this shunt capacitor
  int bus_id;     //the id of the bus to which this shuntcap is attached
  complex y;      //the admittance of the capacitor

  //constructors --------------------------------------------------------------
  ShuntCap(int id, int bus_id, complex y) : id{id}, bus_id{bus_id}, y{y} {}
};

/*=============================================================================
 * A #Branch interconnects two #Bus objects, this is an abstract class
 *===========================================================================*/
struct Branch {
  //types ---------------------------------------------------------------------
  enum class Kind{ Line, Transformer };
  
  //data ----------------------------------------------------------------------
  int             id;       //the id of this branchmuffin
  Kind            kind;     //what kind of branch this is
  array<Bus*, 2>  b;        //the buses this branch interconnects
  array<int, 2>   bus_ids;  //the ids of the buses that this branch 
                            //interconnects
  
  //constructors --------------------------------------------------------------
  Branch(Kind kind);
  
  //methods -------------------------------------------------------------------
  //connect buses @b0 and @b1
  void connect(Bus *b0, Bus *b1);

  //return the impedance of the branch, all concrete subclasses must implement
  //this method
  virtual complex z() const = 0;
};

/*=============================================================================
 * A #Line is a type of #Branch, this is an abstract class
 *===========================================================================*/
struct Line : public Branch {
  //constructors --------------------------------------------------------------
  Line();
  
  //methods -------------------------------------------------------------------
  //return the charging admittance of the line, all concrete subclasses must
  //implement this method
  virtual complex cy() = 0; //charging admittance
};

/*=============================================================================
 * A #SimpleLine is a simple concrete implementation of a #Line
 *===========================================================================*/
struct SimpleLine : public Line {
  //data ----------------------------------------------------------------------
  complex   _z,     //impedance
            _cy;    //charging admittance 

  //constructors --------------------------------------------------------------
  explicit SimpleLine(complex impedance, complex charging_admittance = {0,0});


  //methods -------------------------------------------------------------------
  complex z() const override;
  complex cy() override;
};

/*=============================================================================
 * A #Transformer is a type of #Branch, this is an abstract class.
 *===========================================================================*/
struct Transformer : public Branch {
  //constructors --------------------------------------------------------------
  Transformer();

  //methods -------------------------------------------------------------------
  //return the turns ratio of the transformer, all concrete subclasses must
  //implement this method
  virtual complex tr() = 0;
};

/*=============================================================================
 * A #SimpleTransformer is a simple concrete implementation of a #Transformer
 *===========================================================================*/
struct SimpleTransformer : public Transformer {
  //data ----------------------------------------------------------------------
  complex   _z,   //impedance
            _tr;  //turns ratio

  //constructors --------------------------------------------------------------
  SimpleTransformer(complex impedance, complex turns_ratio);

  //methods -------------------------------------------------------------------
  complex z() const override;
  complex tr() override;
};

/*=============================================================================
 * #Generator is an abstract class that defines the interface that all
 * concrete generator classes must implement
 *===========================================================================*/
struct Generator {
  //data ----------------------------------------------------------------------
  int       id{-1},         //id of the generator
            bus_id{-1};     //id of the bus this generator is connected to
  Bus       *bus{nullptr};  //pointer to the attached bus

  //methods -------------------------------------------------------------------
  //returns the complex voltage at this bus, must be implemented by concrete
  //subclasses
  virtual complex v(double t) const = 0;
};


/*=============================================================================
 * A #StaticGen is a static voltage implementation of a #Generator
 *===========================================================================*/
struct StaticGen : public Generator {
  //data ----------------------------------------------------------------------
  complex   _v;           //static values at which the generator holds the
                          //attached buses voltage
  StaticGen(complex v);
  complex v(double t) const override;
};

/*=============================================================================
 * #Load is an abstract class that defines the interface that all
 * concrete Load classes must implement
 *===========================================================================*/
struct Load {

};


/*=============================================================================
 * #JacobiStructureInfo 
 *===========================================================================*/
struct JacobiStructureInfo {
  //data ----------------------------------------------------------------------
  int n[2]{0,0};              //size of the 1st and 2nd jacobi partitions
  int s[4]{0,0,0,0};          //not quite sure wtf this is at the moment

  //methods -------------------------------------------------------------------
  //returns the sum of the n array
  int N();

  //returns the sum of the s array
  int S();

  //returns a string representaiton of the #JacobiStructureInfo object
  std::string toString();
};

/*=============================================================================
 * The #Jacobi encapsulates the powerflow jacobean sparse matrix #SMatrix
 * object and the supporting information to make it functional
 *===========================================================================*/
struct Jacobi {

  //data ----------------------------------------------------------------------
  Grid                                *g;   //pointer to the grid object for
                                            //which this jacobean is defined
                                            
  JacobiStructureInfo                 jsi;  //keeps track of the structural
                                            //information for this jacobean

  std::shared_ptr<SMatrix<double>>    m;    //pointer to the underlying sparse
                                            //matrix object

  SMatrix<complex>                    y;    //the admittance matrix used to 
                                            //construct this jacobean
                                            
  Glob<complex>                       x;    //the input voltage and magnitudes
                                            //combined into one vector used to
                                            //creat this jacobean

  //constructors --------------------------------------------------------------
  Jacobi(Grid *g, SMatrix<complex> y, Glob<complex> x);

  //methods -------------------------------------------------------------------
  //computes the structural information for this jacobean and stores in the
  //%jsi data member
  void computeStructureInfo();

  //computes the mapping information from each bus from its natural index to its
  //jacobean indicies
  void computeMapInfo();

  //update the jacobian based on the input information in the data member %x
  void update();

};

}

#endif
