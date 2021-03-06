/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "IceCfg.h"
#include "IceInst.h"
#include "IceOperand.h"
#include "IceRegAlloc.h"
#include "IceTargetLowering.h"

// Implements the linear-scan algorithm.  Based on "Linear Scan
// Register Allocation in the Context of SSA Form and Register
// Constraints" by Hanspeter Mössenböck and Michael Pfeiffer,
// ftp://ftp.ssw.uni-linz.ac.at/pub/Papers/Moe02.PDF .  This
// implementation is modified to take affinity into account and allow
// two interfering variables to share the same register in certain
// cases.
//
// Requires running IceCfg::liveness(IceLiveness_RangesFull) in
// preparation.  Results are assigned to IceVariable::RegNum for each
// IceVariable.
void IceLinearScan::scan(const llvm::SmallBitVector &RegMask) {
  if (!RegMask.any())
    return;
  Unhandled.clear();
  Handled.clear();
  Inactive.clear();
  Active.clear();

  // Gather the live ranges of all variables and add them to the
  // Unhandled set.  TODO: Unhandled is a set<> which is based on a
  // balanced binary tree, so inserting live ranges for N variables is
  // O(N log N) complexity.  N may be proportional to the number of
  // instructions, thanks to temporary generation during lowering.  As
  // a result, it may be useful to design a better data structure for
  // storing Cfg->getVariables().
  const IceVarList &Vars = Cfg->getVariables();
  for (IceVarList::const_iterator I = Vars.begin(), E = Vars.end(); I != E;
       ++I) {
    IceVariable *Var = *I;
    if (Var == NULL)
      continue;
    if (Var->getLiveRange().isEmpty())
      continue;
    Unhandled.insert(IceLiveRangeWrapper(Var));
    if (Var->getRegNum() >= 0)
      Var->setLiveRangeInfiniteWeight();
  }

  // RegUses[I] is the number of live ranges (variables) that register
  // I is currently assigned to.  It can be greater than 1 as a result
  // of IceVariable::AllowRegisterOverlap.
  std::vector<int> RegUses(RegMask.size());
  // Unhandled is already set to all ranges in increasing order of
  // start points.
  assert(Active.empty());
  assert(Inactive.empty());
  assert(Handled.empty());
  UnorderedRanges::iterator Next;

  while (!Unhandled.empty()) {
    IceLiveRangeWrapper Cur = *Unhandled.begin();
    Unhandled.erase(Unhandled.begin());
    if (Cfg->Str.isVerbose(IceV_LinearScan))
      Cfg->Str << "\nConsidering  " << Cur << "\n";
    const llvm::SmallBitVector &TypeMask =
        Cfg->getTarget()->getRegisterSetForType(Cur.Var->getType());

    // Check for precolored ranges.  If Cur is precolored, it
    // definitely gets that register.  Previously processed live
    // ranges would have avoided that register due to it being
    // precolored.  Future processed live ranges won't evict that
    // register because the live range has infinite weight.
    if (Cur.Var->getRegNum() >= 0) {
      int RegNum = Cur.Var->getRegNum();
      Cur.Var->setRegNumTmp(RegNum);
      if (Cfg->Str.isVerbose(IceV_LinearScan))
        Cfg->Str << "Precoloring  " << Cur << "\n";
      Active.push_back(Cur);
      assert(RegUses[RegNum] >= 0);
      ++RegUses[RegNum];
      continue;
    }

    // Check for active ranges that have expired or become inactive.
    for (UnorderedRanges::iterator I = Active.begin(), E = Active.end(); I != E;
         I = Next) {
      Next = I;
      ++Next;
      IceLiveRangeWrapper Item = *I;
      bool Moved = false;
      if (Item.endsBefore(Cur)) {
        // Move Item from Active to Handled list.
        if (Cfg->Str.isVerbose(IceV_LinearScan))
          Cfg->Str << "Expiring     " << Item << "\n";
        Active.erase(I);
        Handled.push_back(Item);
        Moved = true;
      } else if (!Item.overlaps(Cur)) {
        // Move Item from Active to Inactive list.
        if (Cfg->Str.isVerbose(IceV_LinearScan))
          Cfg->Str << "Inactivating " << Item << "\n";
        Active.erase(I);
        Inactive.push_back(Item);
        Moved = true;
      }
      if (Moved) {
        // Decrement Item from RegUses[].
        int RegNum = Item.Var->getRegNumTmp();
        assert(RegNum >= 0);
        --RegUses[RegNum];
        assert(RegUses[RegNum] >= 0);
      }
    }

    // Check for inactive ranges that have expired or reactivated.
    for (UnorderedRanges::iterator I = Inactive.begin(), E = Inactive.end();
         I != E; I = Next) {
      Next = I;
      ++Next;
      IceLiveRangeWrapper Item = *I;
      if (Item.endsBefore(Cur)) {
        // Move Item from Inactive to Handled list.
        if (Cfg->Str.isVerbose(IceV_LinearScan))
          Cfg->Str << "Expiring     " << Item << "\n";
        Inactive.erase(I);
        Handled.push_back(Item);
      } else if (Item.overlaps(Cur)) {
        // Move Item from Inactive to Active list.
        if (Cfg->Str.isVerbose(IceV_LinearScan))
          Cfg->Str << "Reactivating " << Item << "\n";
        Inactive.erase(I);
        Active.push_back(Item);
        // Increment Item in RegUses[].
        int RegNum = Item.Var->getRegNumTmp();
        assert(RegNum >= 0);
        assert(RegUses[RegNum] >= 0);
        ++RegUses[RegNum];
      }
    }

    // Calculate available registers into Free[].
    llvm::SmallBitVector Free = RegMask & TypeMask;
    for (unsigned i = 0; i < RegMask.size(); ++i) {
      if (RegUses[i] > 0)
        Free[i] = false;
    }

    // Remove registers from the Free[] list where an Inactive range
    // overlaps with the current range.
    for (UnorderedRanges::const_iterator I = Inactive.begin(),
                                         E = Inactive.end();
         I != E; ++I) {
      IceLiveRangeWrapper Item = *I;
      if (Item.overlaps(Cur)) {
        int RegNum = Item.Var->getRegNumTmp();
        // Don't assert(Free[RegNum]) because in theory (though
        // probably never in practice) there could be two inactive
        // variables that were allowed marked with
        // AllowRegisterOverlap.
        Free[RegNum] = false;
      }
    }

    // Remove registers from the Free[] list where an Unhandled range
    // overlaps with the current range and is precolored.
    // Cur.endsBefore(*I) is an early exit check that turns a
    // guaranteed O(N^2) algorithm into expected linear complexity.
    for (OrderedRanges::const_iterator I = Unhandled.begin(),
                                       E = Unhandled.end();
         I != E && !Cur.endsBefore(*I); ++I) {
      IceLiveRangeWrapper Item = *I;
      int RegNum = Item.Var->getRegNum(); // Note: getRegNum not getRegNumTmp
      if (RegNum >= 0 && Item.overlaps(Cur)) {
        Free[RegNum] = false;
      }
    }

    // Print info about physical register availability.
    if (Cfg->Str.isVerbose(IceV_LinearScan)) {
      for (unsigned i = 0; i < RegMask.size(); ++i) {
        if (RegMask[i]) {
          Cfg->Str << Cfg->physicalRegName(i) << "(U=" << RegUses[i]
                   << ",F=" << Free[i] << ") ";
        }
      }
      Cfg->Str << "\n";
    }

    IceVariable *Prefer = Cur.Var->getPreferredRegister();
    int PreferReg = Prefer ? Prefer->getRegNumTmp() : -1;
    if (PreferReg >= 0 && (Cur.Var->getRegisterOverlap() || Free[PreferReg])) {
      // First choice: a preferred register that is either free or is
      // allowed to overlap with its linked variable.
      Cur.Var->setRegNumTmp(PreferReg);
      if (Cfg->Str.isVerbose(IceV_LinearScan))
        Cfg->Str << "Preferring   " << Cur << "\n";
      assert(RegUses[PreferReg] >= 0);
      ++RegUses[PreferReg];
      Active.push_back(Cur);
    } else if (Free.any()) {
      // Second choice: any free register.  TODO: After explicit
      // affinity is considered, is there a strategy better than just
      // picking the lowest-numbered available register?
      int RegNum = Free.find_first();
      Cur.Var->setRegNumTmp(RegNum);
      if (Cfg->Str.isVerbose(IceV_LinearScan))
        Cfg->Str << "Allocating   " << Cur << "\n";
      assert(RegUses[RegNum] >= 0);
      ++RegUses[RegNum];
      Active.push_back(Cur);
    } else {
      // Fallback: there are no free registers, so we look for the
      // lowest-weight register and see if Cur has higher weight.
      std::vector<IceRegWeight> Weights(RegMask.size());
      // Check Active ranges.
      for (UnorderedRanges::const_iterator I = Active.begin(), E = Active.end();
           I != E; ++I) {
        IceLiveRangeWrapper Item = *I;
        assert(Item.overlaps(Cur));
        int RegNum = Item.Var->getRegNumTmp();
        assert(RegNum >= 0);
        Weights[RegNum].addWeight(Item.range().getWeight());
      }
      // Same as above, but check Inactive ranges instead of Active.
      for (UnorderedRanges::const_iterator I = Inactive.begin(),
                                           E = Inactive.end();
           I != E; ++I) {
        IceLiveRangeWrapper Item = *I;
        int RegNum = Item.Var->getRegNumTmp();
        assert(RegNum >= 0);
        Weights[RegNum].addWeight(Item.range().getWeight());
      }
      // Check Unhandled ranges that overlap Cur and are precolored.
      // Cur.endsBefore(*I) is an early exit check that turns a
      // guaranteed O(N^2) algorithm into expected linear complexity.
      for (OrderedRanges::const_iterator I = Unhandled.begin(),
                                         E = Unhandled.end();
           I != E && !Cur.endsBefore(*I); ++I) {
        IceLiveRangeWrapper Item = *I;
        int RegNum = Item.Var->getRegNumTmp();
        if (RegNum < 0)
          continue;
        if (Item.overlaps(Cur))
          Weights[RegNum].setWeight(IceRegWeight::Inf);
      }

      // All the weights are now calculated.  Find the register with
      // smallest weight.
      int MinWeightIndex = RegMask.find_first();
      // MinWeightIndex must be valid because of the initial
      // RegMask.any() test.
      assert(MinWeightIndex >= 0);
      for (unsigned i = MinWeightIndex + 1; i < Weights.size(); ++i) {
        if (RegMask[i] && TypeMask[i] && Weights[i] < Weights[MinWeightIndex])
          MinWeightIndex = i;
      }

      if (Cur.range().getWeight() <= Weights[MinWeightIndex]) {
        // Cur doesn't have priority over any other live ranges, so
        // don't allocate any register to it, and move it to the
        // Handled state.
        Handled.push_back(Cur);
        if (Cur.range().getWeight().isInf()) {
          Cfg->setError("Unable to find a physical register for an "
                        "infinite-weight live range");
        }
      } else {
        // Evict all live ranges in Active that register number
        // MinWeightIndex is assigned to.
        for (UnorderedRanges::iterator I = Active.begin(), E = Active.end();
             I != E; I = Next) {
          Next = I;
          ++Next;
          IceLiveRangeWrapper Item = *I;
          if (Item.Var->getRegNumTmp() == MinWeightIndex) {
            if (Cfg->Str.isVerbose(IceV_LinearScan))
              Cfg->Str << "Evicting     " << Item << "\n";
            --RegUses[MinWeightIndex];
            assert(RegUses[MinWeightIndex] >= 0);
            Item.Var->setRegNumTmp(-1);
            Active.erase(I);
            Handled.push_back(Item);
          }
        }
        // Do the same for Inactive.
        for (UnorderedRanges::iterator I = Inactive.begin(), E = Inactive.end();
             I != E; I = Next) {
          Next = I;
          ++Next;
          IceLiveRangeWrapper Item = *I;
          if (Item.Var->getRegNumTmp() == MinWeightIndex) {
            if (Cfg->Str.isVerbose(IceV_LinearScan))
              Cfg->Str << "Evicting     " << Item << "\n";
            Item.Var->setRegNumTmp(-1);
            Inactive.erase(I);
            Handled.push_back(Item);
          }
        }
        // Assign the register to Cur.
        Cur.Var->setRegNumTmp(MinWeightIndex);
        ++RegUses[MinWeightIndex];
        assert(RegUses[MinWeightIndex] >= 0);
        Active.push_back(Cur);
        if (Cfg->Str.isVerbose(IceV_LinearScan))
          Cfg->Str << "Allocating   " << Cur << "\n";
      }
    }
    dump(Cfg->Str);
  }
  // Move anything Active or Inactive to Handled for easier handling.
  for (UnorderedRanges::iterator I = Active.begin(), E = Active.end(); I != E;
       I = Next) {
    Next = I;
    ++Next;
    Handled.push_back(*I);
    Active.erase(I);
  }
  for (UnorderedRanges::iterator I = Inactive.begin(), E = Inactive.end();
       I != E; I = Next) {
    Next = I;
    ++Next;
    Handled.push_back(*I);
    Inactive.erase(I);
  }
  dump(Cfg->Str);

  // Finish up by assigning RegNumTmp->RegNum for each IceVariable.
  for (UnorderedRanges::const_iterator I = Handled.begin(), E = Handled.end();
       I != E; ++I) {
    IceLiveRangeWrapper Item = *I;
    int RegNum = Item.Var->getRegNumTmp();
    if (Cfg->Str.isVerbose(IceV_LinearScan)) {
      if (RegNum < 0) {
        Cfg->Str << "Not assigning " << Item.Var << "\n";
      } else {
        Cfg->Str << (RegNum == Item.Var->getRegNum() ? "Reassigning "
                                                     : "Assigning ")
                 << Cfg->physicalRegName(RegNum) << "(r" << RegNum << ") to "
                 << Item.Var << "\n";
      }
    }
    Item.Var->setRegNum(Item.Var->getRegNumTmp());
  }

  // TODO: Consider running register allocation one more time, with
  // infinite registers, for two reasons.  First, evicted live ranges
  // get a second chance for a register.  Second, it allows coalescing
  // of stack slots.  If there is no time budget for the second
  // register allocation run, each unallocated variable just gets its
  // own slot.
  //
  // Another idea for coalescing stack slots is to initialize the
  // Unhandled list with just the unallocated variables, saving time
  // but not offering second-chance opportunities.
}

// ======================== Dump routines ======================== //

void IceLiveRangeWrapper::dump(IceOstream &Str) const {
  char buf[30];
  sprintf(buf, "%2d", Var->getRegNumTmp());
  Str << "R=" << buf << "  V=" << Var << "  Range=";
  range().dump(Str);
}

IceOstream &operator<<(IceOstream &Str, const IceLiveRangeWrapper &R) {
  R.dump(Str);
  return Str;
}

void IceLinearScan::dump(IceOstream &Str) const {
  if (!Cfg->Str.isVerbose(IceV_LinearScan))
    return;
  Cfg->Str.setCurrentNode(NULL);
  Str << "**** Current regalloc state:\n";
  Str << "++++++ Handled:\n";
  for (UnorderedRanges::const_iterator I = Handled.begin(), E = Handled.end();
       I != E; ++I) {
    Cfg->Str << *I << "\n";
  }
  Str << "++++++ Unhandled:\n";
  for (OrderedRanges::const_iterator I = Unhandled.begin(), E = Unhandled.end();
       I != E; ++I) {
    Cfg->Str << *I << "\n";
  }
  Str << "++++++ Active:\n";
  for (UnorderedRanges::const_iterator I = Active.begin(), E = Active.end();
       I != E; ++I) {
    Cfg->Str << *I << "\n";
  }
  Str << "++++++ Inactive:\n";
  for (UnorderedRanges::const_iterator I = Inactive.begin(), E = Inactive.end();
       I != E; ++I) {
    Cfg->Str << *I << "\n";
  }
}
