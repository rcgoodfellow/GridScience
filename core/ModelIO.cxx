#include "ModelIO.hxx"

using namespace gridworks;
using namespace cypress;
using mongo::BSONObj;
using mongo::BSONElement;
using mongo::fromjson;
using std::ifstream;
using std::string;
using std::vector;
using std::to_string;
using std::find_if;
using std::cout;
using std::endl;
using std::runtime_error;
using std::istreambuf_iterator;


Grid 
cypress::gridFromJson(string filename) {
  
  string src = readFile(filename);

  BSONObj bsob = fromjson(src);
  BSONElement grid_elem = bsob["grid"];
  if(grid_elem.eoo()) {
    throw runtime_error("the supplied grid object does not contain a grid");
  }
  BSONObj grid_bse = grid_elem.Obj();

  Grid grid;
  grid.buses = getBuses(grid_bse);

  grid.generators = getGenerators(grid_bse);
  resolveGeneratorBuses(grid);

  grid.shunt_caps = getShuntCapacitors(grid_bse);
  resolveShuntCaps(grid);

  grid.lines = getLines(grid_bse);
  resolveBranches(grid.lines, grid.buses);

  grid.transformers = getTransformers(grid_bse);
  resolveBranches(grid.transformers, grid.buses);

  return grid;

}

string 
cypress::readFile(string filename) {
  //try to read input source
  ifstream ifs(filename);
  if(!ifs.good()) {
    throw runtime_error("Unable to read file " + filename);
  }

  //string to hold source
  string str;

  //figure out the size of the input and reserve the required
  //capacity in the string
  ifs.seekg(0, std::ios::end);
  str.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  //read the input from the file into the string
  str.assign((istreambuf_iterator<char>(ifs)),
              istreambuf_iterator<char>());
 
  //return by rvalue
  return move(str);
}

BSONElement 
cypress::getRequiredElement(const BSONObj &obj, const string &name) {
  BSONElement be = obj[name];
  if(be.eoo()) {
    throw runtime_error(be.fieldName() + 
        string(" does not contain the required element ") + name);
  }
  return be;
}

int 
cypress::getRequiredInt(const BSONObj &obj, const string &name) {
  return getRequiredElement(obj, name).Int();
}

double 
cypress::getRequiredDouble(const BSONObj &obj, const string &name) {
  return getRequiredElement(obj, name).Double();
}

string 
cypress::getRequiredString(const BSONObj &obj, const string &name) {
  return getRequiredElement(obj, name).str();
}

vector<BSONElement> 
cypress::getRequiredArray(const BSONObj &obj, const string &name) {
  return getRequiredElement(obj, name).Array();
}

bool 
cypress::getOptionalBool(const BSONObj &obj, const string &name,
                         bool fallback) {
  BSONElement be = obj[name];
  if(be.eoo()){ return fallback; }
  return be.Bool();
}

double 
cypress::getOptionalDouble(const BSONObj &obj, const string &name, 
                           double fallback) {
  BSONElement be = obj[name];
  if(be.eoo()){ return fallback; }
  return be.Double();
}


vector<Bus*> 
cypress::getBuses(const BSONObj &grid) {
  vector<Bus*> buses;

  //grab the busses bson element, if not there throw
  BSONElement buses_bson = grid["buses"];
  if(buses_bson.eoo()) {
    throw runtime_error("the supplied grid object does not contain buses");
  }
 
  //iterate over the bus bson objects transforming each into a bus object
  //and placing in the buses array
  vector<BSONElement> bus_elems = buses_bson.Array(); 
  for(const BSONElement &be : bus_elems) {
    BSONObj bo = be.Obj();
    size_t id = getRequiredInt(bo, "id");
    double rating = getRequiredDouble(bo, "rating");
    bool slack = getOptionalBool(bo, "slack", false);
    Bus *b = new Bus(id, rating);
    b->slack = slack;
    buses.push_back(b);
  }

  cout << "Found " << buses.size() << " buses" << endl;

  return move(buses);
}

vector<Generator*> 
cypress::getGenerators(const BSONObj &grid) {
  vector<Generator*> gens;

  BSONElement gens_bson = getRequiredElement(grid, "generators");
  vector<BSONElement> gen_elems = gens_bson.Array();

  for(const BSONElement &be : gen_elems) {
    BSONObj bo = be.Obj();
    int id = getRequiredInt(bo, "id");
    string model = getRequiredString(bo, "model");
    int bus = getRequiredInt(bo, "bus");
    if(model == "static") {
      vector<BSONElement> velem = getRequiredArray(bo, "v");
      if(velem.size() != 2) {
        throw runtime_error("complex numbers must be an array of two doubles");
      }
      double vm = velem[0].Double(),
             va = velem[1].Double();
      Generator *g = new StaticGen(std::polar(vm, va));
      g->id = id;
      g->bus_id = bus;
      gens.push_back(g);
    }
  }

  cout << "Found " << gens.size() << " generators" << endl;

  return move(gens);
}

void 
cypress::resolveGeneratorBuses(Grid &g) {
  for(Generator *gen : g.generators) {
    auto bus = find_if(g.buses.begin(), g.buses.end(),
        [gen](const Bus *b) {
          return gen->bus_id == b->id;
        });
    if(bus == g.buses.end()) {
      throw runtime_error(
          "generator " + to_string(gen->id) +
          " references " + to_string(gen->bus_id) +
          " which does not exist");
    }
    gen->bus = *bus;
    (*bus)->generator = gen;
  }
}

vector<ShuntCap*> 
cypress::getShuntCapacitors(BSONObj grid_obj) {
  vector<ShuntCap*> caps;

  vector<BSONElement> cap_elems = 
    getRequiredArray(grid_obj, "shunt_capacitors");

  for(const BSONElement &be : cap_elems) {
   BSONObj bo = be.Obj();
   int id = getRequiredInt(bo, "id");
   vector<BSONElement> y_vec =
     getRequiredArray(bo, "y");
   double g = y_vec[0].Double(),
          b = y_vec[1].Double();
   int bus_id = getRequiredInt(bo, "bus");
   ShuntCap *sc = new ShuntCap(id, bus_id, {g,b});
   caps.push_back(sc);
  }

  cout << "found " << caps.size() << " shunt capacitors" << endl;
  
  return move(caps);
}

void 
cypress::resolveShuntCaps(Grid &grid) {
  for(ShuntCap *sc : grid.shunt_caps) {
    auto bus = find_if(grid.buses.begin(), grid.buses.end(),
        [sc](const Bus *b) {
          return sc->bus_id == b->id;
        });
    if(bus == grid.buses.end()) {
      throw runtime_error(
          "Shunt Capacitor with id " + to_string(sc->id) +
          " references bus " + to_string(sc->bus_id) +
          " which does not exist");
    }
   (*bus)->shunt_y = sc->y; 
  }
}

vector<Line*> 
cypress::getLines(const BSONObj &grid_elem) {
  vector<Line*> lines;
  
  vector<BSONElement> le_arr = getRequiredArray(grid_elem, "lines");
  for(const BSONElement &be : le_arr) {
    BSONObj bo = be.Obj();
    int id = getRequiredInt(bo, "id");
    string model = getRequiredString(bo, "model");
    Line *l{nullptr};
    if(model == "simple") {
      vector<BSONElement> z_arr = getRequiredArray(bo, "z");
      double r = z_arr[0].Double(),
             x = z_arr[1].Double();
      double charg_b = getOptionalDouble(bo, "charging_b", 0.0);
      vector<BSONElement> b_arr = getRequiredArray(bo, "buses");
      int b0 = b_arr[0].Int(),
          b1 = b_arr[1].Int();
      l = new SimpleLine({r,x}, {0, charg_b});
      l->id = id;
      l->bus_ids[0] = b0;
      l->bus_ids[1] = b1;
    }
    else {
      throw runtime_error("unknown line model type " + model);
    }
    lines.push_back(l);
  }
  cout << "found " << lines.size() << " lines" << endl;
  return lines;
}

vector<Transformer*> 
cypress::getTransformers(const BSONObj &grid_bse) {

  vector<Transformer*> tfmrs;

  vector<BSONElement> tfe_arr = 
    getRequiredArray(grid_bse, "transformers");

  for(const BSONElement &be : tfe_arr) {
    const BSONObj bo = be.Obj();
    int id = getRequiredInt(bo, "id");
    vector<BSONElement> z_arr = getRequiredArray(bo, "z");
    double r = z_arr[0].Double(),
           x = z_arr[1].Double();
    vector<BSONElement> t_arr = getRequiredArray(bo, "t");
    double tr = t_arr[0].Double(),
           ti = t_arr[1].Double();
    vector<BSONElement> b_arr = getRequiredArray(bo, "buses");
    double b0 = b_arr[0].Int(),
           b1 = b_arr[1].Int();
    string model = getRequiredString(bo, "model");
    Transformer *tfmr{nullptr};
    if(model == "simple") {
      tfmr = new SimpleTransformer({r,x}, {tr, ti});
      tfmr->id = id;
      tfmr->bus_ids[0] = b0;
      tfmr->bus_ids[1] = b1;
    }
    else {
      throw runtime_error("unknown transformer model " + model);
    }
    tfmrs.push_back(tfmr);
  }
  cout << "Found " << tfmrs.size() << " transformers" << endl;

  return tfmrs;
}

gridworks::Glob<complex>
cypress::schedule(std::string filename) {
  string src = readFile(filename);
  BSONObj s_json = fromjson(src);
  vector<BSONElement> items = getRequiredArray(s_json, "schedule");
  Glob<complex> sch(items.size());
  for(const BSONElement &e : items) {
    BSONObj obj = e.Obj();
    int bus_id = getRequiredInt(obj, "id");
    vector<BSONElement> p_arr = getRequiredArray(obj, "p");
    double p = p_arr[0].Double(),
           q = p_arr[1].Double();
    sch[bus_id] = {p,q};
  }
  return sch;
}

