// -*- Mode: c++ -*-
#ifndef _IceDefs_h
#define _IceDefs_h

#include <stdint.h>

#include <list>
#include <ostream>
#include <string>
#include <vector>

class IceCfg;
class IceCfgNode;
class IceInst;
class IceInstPhi;
class IceOperand;
class IceVariable;
class IceConstant;
class IceRegManager;

// typedefs of containers
typedef std::string IceString;
typedef std::list<IceInst *> IceInstList;
typedef std::vector<IceOperand *> IceOpList;
typedef std::vector<IceVariable *> IceVarList;
typedef std::vector<uint32_t> IceEdgeList;

// The IceOstream class wraps a std::ostream and an IceCfg pointer, so
// that dump routines have access to the IceCfg object and can print
// labels and variable names.
class IceOstream {
public:
  IceOstream(std::ostream &Stream, IceCfg *Cfg) : Stream(Stream), Cfg(Cfg) {}
  std::ostream &Stream;
  IceCfg *const Cfg;
};

inline IceOstream& operator<<(IceOstream &Str, const char *S) {
  Str.Stream << S;
  return Str;
}

inline IceOstream& operator<<(IceOstream &Str, const IceString &S) {
  Str.Stream << S;
  return Str;
}

inline IceOstream& operator<<(IceOstream &Str, uint32_t U) {
  Str.Stream << U;
  return Str;
}

#endif // _IceDefs_h
