// -*- Mode: c++ -*-
/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef _IceInstX8632_h
#define _IceInstX8632_h

#include "IceDefs.h"
#include "IceInst.h"

#include "IceTargetLowering.h"

class IceTargetX8632 : public IceTargetLowering {
public:
  IceTargetX8632(IceCfg *Cfg)
      : IceTargetLowering(Cfg), PhysicalRegisters(IceVarList(Reg_NUM)) {}
  virtual IceRegManager *makeRegManager(IceCfgNode *Node);
  virtual IceInstTarget *makeAssign(IceVariable *Dest, IceOperand *Src);
  virtual IceVariable *getPhysicalRegister(unsigned RegNum);
  virtual IceString *getRegNames(void) const { return RegNames; }
  virtual llvm::SmallBitVector getRegisterMask(void) const;
  enum Registers {
    Reg_eax = 0,
    Reg_ecx = 1,
    Reg_edx = 2,
    Reg_ebx = 3,
    Reg_esp = 4,
    Reg_ebp = 5,
    Reg_esi = 6,
    Reg_edi = 7,
    Reg_NUM = 8
  };

protected:
  virtual IceInstList lowerAlloca(const IceInstAlloca *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);
  virtual IceInstList lowerArithmetic(const IceInstArithmetic *Inst,
                                      const IceInst *Next,
                                      bool &DeleteNextInst);
  virtual IceInstList lowerAssign(const IceInstAssign *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);
  virtual IceInstList lowerBr(const IceInstBr *Inst, const IceInst *Next,
                              bool &DeleteNextInst);
  virtual IceInstList lowerCall(const IceInstCall *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerCast(const IceInstCast *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerFcmp(const IceInstFcmp *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerIcmp(const IceInstIcmp *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerLoad(const IceInstLoad *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerPhi(const IceInstPhi *Inst, const IceInst *Next,
                               bool &DeleteNextInst);
  virtual IceInstList lowerRet(const IceInstRet *Inst, const IceInst *Next,
                               bool &DeleteNextInst);
  virtual IceInstList lowerSelect(const IceInstSelect *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);
  virtual IceInstList lowerStore(const IceInstStore *Inst, const IceInst *Next,
                                 bool &DeleteNextInst);
  virtual IceInstList lowerSwitch(const IceInstSwitch *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);

private:
  IceVarList PhysicalRegisters;
  static IceString RegNames[];
};

class IceTargetX8632S : public IceTargetX8632 {
public:
  IceTargetX8632S(IceCfg *Cfg) : IceTargetX8632(Cfg) {}
  virtual IceRegManager *makeRegManager(IceCfgNode *Node) { return NULL; }
  virtual IceInstTarget *makeAssign(IceVariable *Dest, IceOperand *Src);
  virtual llvm::SmallBitVector getRegisterMask(void) const;

protected:
  virtual IceInstList lowerAlloca(const IceInstAlloca *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);
  virtual IceInstList lowerArithmetic(const IceInstArithmetic *Inst,
                                      const IceInst *Next,
                                      bool &DeleteNextInst);
  virtual IceInstList lowerAssign(const IceInstAssign *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);
  virtual IceInstList lowerBr(const IceInstBr *Inst, const IceInst *Next,
                              bool &DeleteNextInst);
  virtual IceInstList lowerCall(const IceInstCall *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerCast(const IceInstCast *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerFcmp(const IceInstFcmp *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerIcmp(const IceInstIcmp *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerLoad(const IceInstLoad *Inst, const IceInst *Next,
                                bool &DeleteNextInst);
  virtual IceInstList lowerPhi(const IceInstPhi *Inst, const IceInst *Next,
                               bool &DeleteNextInst);
  virtual IceInstList lowerRet(const IceInstRet *Inst, const IceInst *Next,
                               bool &DeleteNextInst);
  virtual IceInstList lowerSelect(const IceInstSelect *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);
  virtual IceInstList lowerStore(const IceInstStore *Inst, const IceInst *Next,
                                 bool &DeleteNextInst);
  virtual IceInstList lowerSwitch(const IceInstSwitch *Inst,
                                  const IceInst *Next, bool &DeleteNextInst);

private:
};

// Two-address arithmetic instructions.
class IceInstX8632Arithmetic : public IceInstTarget {
public:
  enum OpKind {
    Add = IceInstArithmetic::Add,
    Fadd = IceInstArithmetic::Fadd,
    Sub = IceInstArithmetic::Sub,
    Fsub = IceInstArithmetic::Fsub,
    Mul = IceInstArithmetic::Mul,
    Fmul = IceInstArithmetic::Fmul,
    Udiv = IceInstArithmetic::Udiv,
    Sdiv = IceInstArithmetic::Sdiv,
    Fdiv = IceInstArithmetic::Fdiv,
    Urem = IceInstArithmetic::Urem,
    Srem = IceInstArithmetic::Srem,
    Frem = IceInstArithmetic::Frem,
    Shl = IceInstArithmetic::Shl,
    Lshr = IceInstArithmetic::Lshr,
    Ashr = IceInstArithmetic::Ashr,
    And = IceInstArithmetic::And,
    Or = IceInstArithmetic::Or,
    Xor = IceInstArithmetic::Xor,
    Invalid /* = IceInstArithmetic::OpKind_NUM*/,
  };
  IceInstX8632Arithmetic(IceCfg *Cfg, OpKind Op, IceVariable *Dest,
                         IceOperand *Source);
  IceInstX8632Arithmetic(IceCfg *Cfg, IceInstArithmetic::OpKind Op,
                         IceVariable *Dest, IceOperand *Source);
  OpKind getOp(void) const { return Op; }
  bool isCommutative(void) const;
  virtual void dump(IceOstream &Str) const;

private:
  const OpKind Op;
};

class IceInstX8632Br : public IceInstTarget {
public:
  IceInstX8632Br(IceCfg *Cfg, IceCfgNode *TargetTrue, IceCfgNode *TargetFalse,
                 IceInstIcmp::IceICond Condition);
  IceCfgNode *getTargetTrue(void) const { return TargetTrue; }
  IceCfgNode *getTargetFalse(void) const { return TargetFalse; }
  virtual void dump(IceOstream &Str) const;

private:
  IceInstIcmp::IceICond Condition;
  IceCfgNode *TargetTrue;
  IceCfgNode *TargetFalse;
};

class IceInstX8632Call : public IceInstTarget {
public:
  IceInstX8632Call(IceCfg *Cfg, IceVariable *Dest, IceOperand *CallTarget,
                   bool Tail);
  IceOperand *getCallTarget(void) const { return CallTarget; }
  virtual void dump(IceOstream &Str) const;

private:
  IceOperand *CallTarget;
  const bool Tail;
};

class IceInstX8632Icmp : public IceInstTarget {
public:
  IceInstX8632Icmp(IceCfg *Cfg, IceOperand *Src1, IceOperand *Src2);
  virtual void dump(IceOstream &Str) const;

private:
};

// TODO: Are Load and Store really just Assigns?
class IceInstX8632Load : public IceInstTarget {
public:
  IceInstX8632Load(IceCfg *Cfg, IceVariable *Dest, IceOperand *Base,
                   IceOperand *Index, IceOperand *Shift, IceOperand *Offset);
  virtual void dump(IceOstream &Str) const;

private:
};

class IceInstX8632Store : public IceInstTarget {
public:
  IceInstX8632Store(IceCfg *Cfg, IceOperand *Value, IceOperand *Base,
                    IceOperand *Index, IceOperand *Shift, IceOperand *Offset);
  virtual void dump(IceOstream &Str) const;

private:
};

class IceInstX8632Mov : public IceInstTarget {
public:
  IceInstX8632Mov(IceCfg *Cfg, IceVariable *Dest, IceOperand *Source);
  virtual bool isRedundantAssign(void) const;
  virtual void dump(IceOstream &Str) const;

private:
};

class IceInstX8632Push : public IceInstTarget {
public:
  IceInstX8632Push(IceCfg *Cfg, IceOperand *Source);
  virtual void dump(IceOstream &Str) const;
};

class IceInstX8632Ret : public IceInstTarget {
public:
  IceInstX8632Ret(IceCfg *Cfg, IceVariable *Source = NULL);
  virtual void dump(IceOstream &Str) const;

private:
};

#endif // _IceInstX8632_h
