#include "IP.hxx"

using namespace gridworks;
using namespace gridworks::ip;
using std::string;
using std::stringstream;
using std::endl;
using std::to_string;
using std::min;
using std::max;

string Network::topDL() const
{
  stringstream ss;
  ss << "<experiment>" << endl;
  ss << "<version>1.0</version>" << endl << endl;
 
  //lcl-rmt substrate
  ss << "<substrates>" << endl;
  ss << "<name>lcl-dtr</name>" << endl;

  ss << "<capacity>" << endl;
  ss << "<rate>" << 1000 << "</rate>" << endl;
  ss << "<kind>max</kind>" << endl;
  ss << "</capacity>" << endl;
  
  ss << "<latency>" << endl;
  ss << "<time>" << 0 << "</time>" << endl;
  ss << "<kind>average</kind>" << endl;
  ss << "</latency>" << endl;
  ss << "</substrates>" << endl << endl;

  //lcl
  ss << "<elements>" << endl;
  ss << "<computer>" << endl;

  ss << "<name>lcl</name>" << endl;
  ss << "<interface>" << endl;
  ss << "<substrate>lcl-dtr</substrate>" << endl;
  ss << "<name>" << "inf" << 0 << "</name>" << endl;
  ss << "</interface>" << endl;
  ss << "<attribute>" << endl;
  ss << "<attribute>testbed</attribute><value>deter</value>" << endl;
  ss << "</attribute>" << endl << endl;
  
  ss << "</computer>" << endl;
  ss << "</elements>" << endl;
  
  //rmt
  ss << "<elements>" << endl;
  ss << "<computer>" << endl;

  ss << "<name>rmt</name>" << endl;
  ss << "<interface>" << endl;
  ss << "<substrate>lcl-dtr</substrate>" << endl;
  ss << "<name>" << "inf" << 0 << "</name>" << endl;
  ss << "</interface>" << endl;
  ss << "<attribute>" << endl;
  ss << "<attribute>testbed</attribute><value>local</value>" << endl;
  ss << "</attribute>" << endl << endl;

  ss << "</computer>" << endl;
  ss << "</elements>" << endl;

  for(const Link *l : links) { ss << l->topDL() << endl; }
  for(const Host *h : hosts) { ss << h->topDL() << endl; }
  
  ss << "</experiment>" << endl;
  return ss.str();
}

string Host::topDL() const
{
  stringstream ss;
  ss << "<elements>" << endl;
  ss << "<computer>" << endl;

  ss << "<name>h" << id << "</name>" << endl;
  size_t inf{0};
  for(const Neighbor &nbr : neighbors) {
    ss << "<interface>" << endl;
    ss << "<substrate>" 
       << "s" << min(id, nbr.h->id) << "-" <<  max(id, nbr.h->id) 
       <<"</substrate>" << endl;
    ss << "<name>" << "inf" << inf << "</name>" << endl;
    ss << "</interface>" << endl;
    ss << "<attribute>" << endl;
    ss << "<attribute>testbed</attribute><value>deter</value>" << endl;
    ss << "</attribute>" 
       << endl;
  }

  ss << "</computer>" << endl;
  ss << "</elements>" << endl;
  return ss.str();
}

string Link::topDL() const
{
  stringstream ss;
  size_t sa = endpoints[0]->id, sb = endpoints[1]->id;
  ss << "<substrates>" << endl;
  ss << "<name>s" 
     << min(sa,sb) << "-" << max(sa,sb) << "</name>" 
     << endl;

  ss << "<capacity>" << endl;
  ss << "<rate>" << capacity << "</rate>" << endl;
  ss << "<kind>max</kind>" << endl;
  ss << "</capacity>" << endl;
  
  ss << "<latency>" << endl;
  ss << "<time>" << latency << "</time>" << endl;
  ss << "<kind>average</kind>" << endl;
  ss << "</latency>" << endl;


  ss << "</substrates>" << endl;
  return ss.str();
}

Host::Host(size_t id) : id{id} {}

Link::Link(double capacity, double latency) 
  : capacity{capacity}, latency{latency} {}

Neighbor::Neighbor(Link *l, Host *h) : l{l}, h{h} {}

void ip::connect(Host *a, Link *l, Host *b) {
  l->endpoints[0] = a;
  l->endpoints[1] = b;
  a->neighbors.push_back(Neighbor(l, b)); 
  b->neighbors.push_back(Neighbor(l, a));
}

