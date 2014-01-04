// -*- Mode: c++ -*-
#ifndef _IceInst_h
#define _IceInst_h

#include <vector>

#include "IceDefs.h"
#include "IceTypes.h"

class IceInst {
public:
  enum IceInstType {
    // Alphabetical order
    Alloca,
    Arithmetic,
    Assign, // not part of LLVM/PNaCl bitcode
    Br,
    Call,
    Conversion,
    Fcmp,
    Icmp,
    Load,
    Phi,
    Ret,
    Select,
    Store,
    Switch,
    Target // target-specific low-level ICE
  };
  IceInstType getKind(void) const { return Kind; }
  IceVariable *getDest(unsigned I) const {
    return I < Dests.size() ? Dests[I] : NULL;
  }
  IceOperand *getSrc(unsigned I) const {
    return I < Srcs.size() ? Srcs[I] : NULL;
  }
  unsigned getDestSize(void) const { return Dests.size(); }
  unsigned getSrcSize(void) const { return Srcs.size(); }
  bool isDeleted(void) const { return Deleted; }
  // If an instruction is deleted as a result of replacing it with
  // equivalent instructions, only call setDeleted() *after* inserting
  // the new instructions because of the cascading deletes from
  // reference counting.
  void setDeleted(void);
  void updateVars(IceCfgNode *Node);
  void findAddressOpt(IceCfg *Cfg, const IceCfgNode *Node);
  void doAddressOpt(IceVariable *&Base, IceVariable *&Index,
                    int &Shift, int32_t &Offset);
  void replaceOperands(const IceCfgNode *Node, unsigned Index,
                       const IceOpList &NewOperands);
  virtual void removeUse(IceVariable *Variable);
  void markLastUses(IceCfg *Cfg);
  virtual IceInstList genCodeX8632(IceCfg *Cfg, IceRegManager *RegManager,
                                   IceInst *Next, bool &DeleteCurInst,
                                   bool &DeleteNextInst);
  virtual void dump(IceOstream &Str) const;
  virtual void dumpExtras(IceOstream &Str) const;
  void dumpSources(IceOstream &Str) const;
  void dumpDests(IceOstream &Str) const;
protected:
  IceInst(IceInstType Kind, IceType Type);
  void addDest(IceVariable *Dest);
  void addSource(IceOperand *Src);
  const IceInstType Kind;
  const IceType Type;
  bool Deleted;
  IceVarList Dests;
  IceOpList Srcs;
private:
  // instruction type
  // list of IceOperand LHS definitions
  //   this can be empty list if instruction is dead (use counts of LHS
  //   operands are 0) but we need to preserve exceptional side effects
  // list of IceInst RHS uses
  // which RHS operand live ranges are known to end here
  // do we tag/retain deleted instructions?
};

IceOstream& operator<<(IceOstream &Str, const IceInst *I);

class IceInstAlloca : public IceInst {
public:
  IceInstAlloca(uint32_t Size, uint32_t Align) :
    IceInst(Alloca, IceType_i8), Size(Size), Align(Align) {}
  virtual void dump(IceOstream &Str) const;
private:
  const uint32_t Size;
  const uint32_t Align;
};

class IceInstArithmetic : public IceInst {
public:
  enum IceArithmetic {
    // Ordered by http://llvm.org/docs/LangRef.html#binary-operations
    Add,
    Fadd,
    Sub,
    Fsub,
    Mul,
    Fmul,
    Udiv,
    Sdiv,
    Fdiv,
    Urem,
    Srem,
    Frem,
    // Ordered by http://llvm.org/docs/LangRef.html#bitwise-binary-operations
    Shl,
    Lshr,
    Ashr,
    And,
    Or,
    Xor,
    Invalid,
  };
  IceInstArithmetic(IceArithmetic Op, IceType Type, IceVariable *Dest,
                    IceOperand *Source1, IceOperand *Source2);
  IceArithmetic getOp(void) const { return Op; }
  bool isCommutative(void) const;
  virtual void dump(IceOstream &Str) const;

private:
  const IceArithmetic Op;
};

class IceInstAssign : public IceInst {
public:
  IceInstAssign(IceType Type, IceVariable *Dest, IceOperand *Source);
  virtual void dump(IceOstream &Str) const;
private:
};

class IceInstBr : public IceInst {
public:
  // Conditional branch
  IceInstBr(IceCfgNode *Node, IceOperand *Source,
            uint32_t LabelTrue, uint32_t LabelFalse);
  // Unconditional branch
  IceInstBr(IceCfgNode *Node, uint32_t Label);
  uint32_t getLabelTrue(void) const { return LabelIndexTrue; }
  // Fall-through
  uint32_t getLabelFalse(void) const { return LabelIndexFalse; }
  virtual void dump(IceOstream &Str) const;
private:
  // TODO: consider making this a list of edges.
  const bool IsConditional;
  uint32_t LabelIndexFalse; // fall-through
  uint32_t LabelIndexTrue;
};

// TODO: implement
class IceInstCall : public IceInst {
public:
  IceInstCall(IceType Type, bool Tail=false) :
    IceInst(Call, Type), Tail(Tail) {}
  virtual void removeUse(IceVariable *Variable) {}
  virtual void dump(IceOstream &Str) const;
private:
  const bool Tail;
};

// TODO: implement
class IceInstConversion : public IceInst {
public:
  enum IceConvert {
    // Ordered by http://llvm.org/docs/LangRef.html#conversion-operations
    Trunc,
    Zext,
    Sext,
    Fptrunc,
    Fpext,
    Fptoui,
    Fptosi,
    Uitofp,
    Sitofp,
    Ptrtoint,
    Inttoptr,
    Bitcast,
  };
  IceInstConversion(IceConvert Conversion, IceType Type,
                    IceOperand *Dest, IceOperand *Source);
private:
};

// TODO: implement
class IceInstFcmp : public IceInst {
public:
  enum IceFCond {
    // Ordered by http://llvm.org/docs/LangRef.html#id254
    False,
    Oeq,
    Ogt,
    Oge,
    Olt,
    Ole,
    One,
    Ord,
    Ueq,
    Ugt,
    Uge,
    Ult,
    Ule,
    Une,
    Uno,
    True,
  };
  IceInstFcmp(IceFCond Condition, IceType Type, IceOperand *Dest,
              IceOperand *Source1, IceOperand *Source2);
private:
  IceFCond Condition;
};

class IceInstIcmp : public IceInst {
public:
  enum IceICond {
    // Ordered by http://llvm.org/docs/LangRef.html#id249
    Eq,
    Ne,
    Ugt,
    Uge,
    Ult,
    Ule,
    Sgt,
    Sge,
    Slt,
    Sle,
  };
  IceInstIcmp(IceICond Condition, IceType Type, IceVariable *Dest,
              IceOperand *Source1, IceOperand *Source2);
  IceICond getCondition(void) const { return Condition; }
  virtual void dump(IceOstream &Str) const;
private:
  IceICond Condition;
};

class IceInstLoad : public IceInst {
public:
  IceInstLoad(IceType Type, IceVariable *Dest, IceOperand *SourceAddr);
  virtual void dump(IceOstream &Str) const;
private:
};

class IceInstPhi : public IceInst {
public:
  IceInstPhi(IceType Type, IceVariable *Dest);
  void addArgument(IceOperand *Source, uint32_t Label);
  IceInst *lower(IceCfg *Cfg, IceCfgNode *Node);
  IceOperand *getOperandForTarget(uint32_t Target) const;
  virtual void dump(IceOstream &Str) const;
private:
  IceEdgeList Labels; // corresponding to Srcs
};

class IceInstRet : public IceInst {
public:
  IceInstRet(IceType Type, IceOperand *Source = NULL);
  virtual void dump(IceOstream &Str) const;
private:
};

// TODO: implement
class IceInstSelect : public IceInst {
public:
  IceInstSelect(IceType Type, IceOperand *Dest, IceOperand *Condition,
                IceOperand *Source1, IceOperand *Source2);
private:
};

// TODO: implement
// Put SourceData as Srcs[0] and SourceAddr as Srcs[1]
class IceInstStore : public IceInst {
public:
  IceInstStore(IceType Type, IceOperand *SourceAddr, IceOperand *SourceData);
private:
};

// TODO: implement
class IceInstSwitch : public IceInst {
public:
  IceInstSwitch(IceType Type, IceOperand *Source,
                int32_t LabelDefault);
  void addBranch(IceType Type, IceOperand *Source, int32_t Label);
private:
};

class IceInstTarget : public IceInst {
public:
  void setRegState(const IceRegManager *State);
  virtual void dump(IceOstream &Str) const;
  virtual void dumpExtras(IceOstream &Str) const;
protected:
  IceInstTarget(IceType Type) : IceInst(Target, Type), RegState(NULL) {}
  IceRegManager *RegState;
private:
};

#endif // _IceInst_h
