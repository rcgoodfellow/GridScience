#ifndef _GW_IP_
#define _GW_IP_

#include <string>
#include <vector>
#include <sstream>

namespace gridworks { namespace ip {

struct Network;
struct Host;
struct Link;

struct Network {
  std::vector<Host*> hosts;
  std::vector<Link*> links;
  std::string topDL() const;
};

struct Neighbor {
  Link *l;
  Host *h; 
  Neighbor(Link *l, Host *h);
};

struct Host {
  Host(size_t id);
  size_t id;
  std::vector<Neighbor> neighbors;
  std::string topDL() const;
};

struct Link {
  Link(double capacity, double latency);
  double capacity, latency;
  Host *endpoints[2] = {nullptr, nullptr};
  std::string topDL() const;
};

void connect(Host *a, Link *l, Host *b);

}}

#endif
