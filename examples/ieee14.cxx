#include "Grid.hxx"
#include "PowerFlow.hxx"
#include <iostream>
#include <fstream>
#include "IP.hxx"

using namespace gridworks;
using ip::Network;
using ip::Host;
using ip::Link;

using std::cout;
using std::endl;
  
Grid grid;
Network network;

void init_buses()
{
  
  Bus *b = new Bus(0, 69);
  b->slack = true;
  b->generator = new StaticGen({1,0});
  grid.buses.push_back(b);

  b = new Bus(1, 69);
  b->generator = new StaticGen({1,0});
  grid.buses.push_back(b);

  b = new Bus(2, 69);
  b->generator = new StaticGen({1,0});
  grid.buses.push_back(b);

  b = new Bus(3, 69);
  grid.buses.push_back(b);

  b = new Bus(4, 69);
  grid.buses.push_back(b);

  b = new Bus(5, 13);
  b->generator = new StaticGen({1,0});
  grid.buses.push_back(b);

  b = new Bus(6, 18);
  grid.buses.push_back(b);

  b = new Bus(7, 13.8);
  b->generator = new StaticGen({1,0});
  grid.buses.push_back(b);

  b = new Bus(8, 13.8, {0,0.19});
  grid.buses.push_back(b);

  b = new Bus(9, 13.8);
  grid.buses.push_back(b);

  b = new Bus(10, 13.8);
  grid.buses.push_back(b);

  b = new Bus(11, 13.8);
  grid.buses.push_back(b);

  b = new Bus(12, 13.8);
  grid.buses.push_back(b);

  b = new Bus(13, 13.8);
  grid.buses.push_back(b);
}

void init_lines()
{
  SimpleLine *l;
  auto &b = grid.buses;

  l = new SimpleLine({0.01938, 0.05917}, {0, 0.0528});
  l->connect(b[0], b[1]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.05403, 0.22304}, {0, 0.0492});
  l->connect(b[0], b[4]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.04699, 0.19797}, {0, 0.0438});
  l->connect(b[1], b[2]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.05811, 0.17632}, {0, 0.0340});
  l->connect(b[1], b[3]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.05695, 0.17388}, {0, 0.0346});
  l->connect(b[1], b[4]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.06701, 0.17103}, {0, 0.0128});
  l->connect(b[2], b[3]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.01335, 0.04211});
  l->connect(b[4], b[3]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.09498, 0.19890});
  l->connect(b[5], b[10]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.12291, 0.25581});
  l->connect(b[5], b[11]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.06615, 0.13027});
  l->connect(b[5], b[12]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.03181, 0.08450});
  l->connect(b[8], b[9]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.12711, 0.27038});
  l->connect(b[8], b[13]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.08205, 0.19207});
  l->connect(b[9], b[10]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.22092, 0.19989});
  l->connect(b[11], b[12]);
  grid.lines.push_back(l);

  l = new SimpleLine({0.17093, 0.34802});
  l->connect(b[12], b[13]);
  grid.lines.push_back(l);

}

void init_transformers() {

  SimpleTransformer *t;
  auto &b = grid.buses;

  t = new SimpleTransformer({0.0, 0.209120}, {0.978, 1.0});
  t->connect(b[3], b[6]);
  grid.transformers.push_back(t);

  t = new SimpleTransformer({0.0, 0.556180}, {0.969, 1.0});
  t->connect(b[3], b[8]);
  grid.transformers.push_back(t);
  
  t = new SimpleTransformer({0.0, 0.252020}, {0.932, 1.0}); 
  t->connect(b[4], b[5]);
  grid.transformers.push_back(t);

  t = new SimpleTransformer({0.0, 0.176150}, {1.0, 1.0});
  t->connect(b[7], b[6]);
  grid.transformers.push_back(t);

  t = new SimpleTransformer({0.0, 0.110011}, {1.0, 1.0});
  t->connect(b[6], b[8]);
  grid.transformers.push_back(t);

}

void show_bus_topo()
{
  cout << "Bus Topology" << endl;
  for(const Bus *b : grid.buses)
  {
    cout << b->id << ": ";
    for(const Neighbor n : b->neighbors)
    {
      cout << n.b->id << " ";
    }
    cout << endl;
  }
}

void init_ip() {
  for(const Bus *bus : grid.buses) {
    network.hosts.push_back(new Host(bus->id));
  }
  for(const Line *l : grid.lines) {
    Link *lnk = new Link(100, abs(l->z())*250);
    Host *a = network.hosts[l->b[0]->id];
    Host *b = network.hosts[l->b[1]->id];
    connect(a,lnk,b);
    network.links.push_back(lnk);
  }
  for(const Transformer *t : grid.transformers) {
    Link *lnk = new Link(100, abs(t->z())*250);
    Host *a = network.hosts[t->b[0]->id];
    Host *b = network.hosts[t->b[1]->id];
    connect(a,lnk,b);
    network.links.push_back(lnk);
  }

}

int main() {

  init_buses();
  init_lines();
  init_transformers();
  show_bus_topo();

  init_ip();
  std::ofstream ofs("ieee14.topdl");
  ofs << network.topDL();


  SMatrix<complex> Y = ymatrix(grid);
  cout << Y.toCsv() << endl;

  Glob<complex> x(14);
  for(int i=0; i<14; ++i) { x.data[i] = std::polar(1.0,0.0); }
  //Initial voltage magnitudes for PV buses
  x.data[0] = std::polar(1.06, 0.0);
  x.data[1] = std::polar(1.045, 0.0);
  x.data[2] = std::polar(1.01, 0.0);
  x.data[5] = std::polar(1.07, 0.0);
  x.data[7] = std::polar(1.09, 0.0);
  Jacobi J(&grid, Y, x);
  
  //Scheduled power per bus
  Glob<complex> sSch(14);

  sSch.data[0] = {232.4,	-16.9};
  sSch.data[1] = {18.3,	29.7};
  sSch.data[2] = {-94.2,	4.4};
  sSch.data[3] = {-47.8,	3.9};
  sSch.data[4] = {-7.6,	-1.6};
  sSch.data[5] = {-11.2,	4.7};
  sSch.data[6] = {0,	0};
  sSch.data[7] = {0,	17.4};
  sSch.data[8] = {-29.5,	-16.6};
  sSch.data[9] = {-9,	-5.8};
  sSch.data[10] = {-3.5,	-1.8};
  sSch.data[11] = {-6.1,	-1.6};
  sSch.data[12] = {-13.5,	-5.8};
  sSch.data[13] = {-14.9,	-5};

  for(size_t i=0; i<14; ++i) sSch.data[i] /= 100.0;

  cout << J.m->toCsv() << endl;

  PowerFlow pf{&grid, x, sSch, 5e-6};
  pf.run();
  
  std::cout << "Powerflow converged in " << pf.steps << " steps" << std::endl;
  std::cout << "Max mismatch " << pf.max_dS() << std::endl;

  std::cout << "result:" << std::endl;
  for(size_t i=0; i<grid.buses.size(); ++i)
  {
    std::cout << "[" << i << "] : "
              << "(" << std::abs(pf.state.data[i])
              << "," << deg(std::arg(pf.state.data[i])) << ")  "
              << pf.sCalc.data[i] * 100.0
              << std::endl;
  }
}
