#ifndef CYPRESS_MODELIO
#define CYPRESS_MODELIO

#include "Grid.hxx"

#include <mongo/client/dbclient.h>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace cypress {

gridworks::Grid 
gridFromJson(std::string filename);

std::string 
readFile(std::string);

mongo::BSONElement 
getRequiredElement(const mongo::BSONObj &obj, const std::string &name);

int 
getRequiredInt(const mongo::BSONObj &obj, const std::string &name);

double 
getRequiredDouble(const mongo::BSONObj &obj, const std::string &name);

std::string 
getRequiredString(const mongo::BSONObj &obj, const std::string &name);

std::vector<mongo::BSONElement> 
getRequiredArray(const mongo::BSONObj &obj, const std::string &name);

bool 
getOptionalBool(const mongo::BSONObj &obj, const std::string &name,
                bool fallback);

double 
getOptionalDouble(const mongo::BSONObj &obj, const std::string &name, 
                  double fallback);

std::vector<gridworks::Bus*> 
getBuses(const mongo::BSONObj &grid);

std::vector<gridworks::Generator*> 
getGenerators(const mongo::BSONObj &grid);

void 
resolveGeneratorBuses(gridworks::Grid &g);

std::vector<gridworks::ShuntCap*> 
getShuntCapacitors(mongo::BSONObj grid_obj);

void 
resolveShuntCaps(gridworks::Grid &grid);

std::vector<gridworks::Line*> 
getLines(const mongo::BSONObj &grid_elem);

template <class T>
void resolveBranches(std::vector<T*> &brs, 
                     std::vector<gridworks::Bus*> &buses) {
  for(gridworks::Branch *br : brs) {
    auto b0 = std::find_if(buses.begin(), buses.end(),
        [br](const gridworks::Bus *b){ return b->id == br->bus_ids[0]; });

    if(b0 == buses.end()) {
      throw std::runtime_error("line " + std::to_string(br->id) +
          " references bus " + std::to_string(br->bus_ids[0]) +
          " which does not exist");
    }
    
    auto b1 = std::find_if(buses.begin(), buses.end(),
        [br](const gridworks::Bus *b){ return b->id == br->bus_ids[1]; });
    
    if(b1 == buses.end()) {
      throw std::runtime_error("line " + std::to_string(br->id) +
          " references bus " + std::to_string(br->bus_ids[1]) +
          " which does not exist");
    }

    br->b[0] = *b0;
    br->b[1] = *b1;

    (*b0)->neighbors.push_back(gridworks::Neighbor(br, *b1));
    (*b1)->neighbors.push_back(gridworks::Neighbor(br, *b0));

  }
}

std::vector<gridworks::Transformer*> 
getTransformers(const mongo::BSONObj &grid_bse);

gridworks::Glob<std::complex<double>>
schedule(std::string filename);

}

#endif
