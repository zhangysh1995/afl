/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   Copyright 2015, 2016 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.

 */

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"
#include "edge.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

  class AFLCoverage : public ModulePass {

    public:

      static char ID;
      AFLCoverage() : ModulePass(ID) { }

      bool runOnModule(Module &M) override;

//       StringRef getPassName() const override {
//        return "American Fuzzy Lop Instrumentation";
//       }

  };

}


char AFLCoverage::ID = 0;


bool AFLCoverage::runOnModule(Module &M) {

    LLVMContext &C = M.getContext();

    IntegerType *Int64Ty = IntegerType::getInt64Ty(C);
    IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

    // the pointer to the struct
    StructType *PtrStructTy = StructType::create(C, "struct.Edge");
    PointerType *PPStructTy = PointerType::get(PtrStructTy, 0);

    Type *FuncVoidTy = FunctionType::getVoidTy(C);


    /* Show a banner */

    char be_quiet = 0;

    if (isatty(2) && !getenv("AFL_QUIET")) {

        SAYF(cCYA
                     "afl-llvm-pass "
                     cBRI
                     VERSION
                     cRST
                     " by <lszekeres@google.com>\n");

    } else be_quiet = 1;

    /* Decide instrumentation ratio */

    char *inst_ratio_str = getenv("AFL_INST_RATIO");
    unsigned int inst_ratio = 100;

    if (inst_ratio_str) {

        if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
            inst_ratio > 100)
            FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

    }

    // retrieve the function
//    Constant *find_edge = M.getOrInsertFunction("find_edge",
//                                                PtrStructTy,
//                                                PtrStructTy,
//                                                Int32Ty,
//                                                NULL);
//
//    Constant *create_edge = M.getOrInsertFunction("new_edge",
//                                                  PtrStructTy,
//                                                  Int32Ty,
//                                                  NULL);
//
//    Constant *add_edge = M.getOrInsertFunction("add_edge",
//                                               FuncVoidTy,
//                                               PtrStructTy,
//                                               PtrStructTy,
//                                               NULL);
//
//    Constant *update_count = M.getOrInsertFunction("update_count",
//                                                   FuncVoidTy,
//                                                   PtrStructTy,
//                                                   Int32Ty,
//                                                   NULL);

    Constant *update = M.getOrInsertFunction("checkThenUpdate",
                                             FuncVoidTy,
                                             PPStructTy,
                                             Int32Ty,
                                             NULL);

    // pointer to map head, passed from `afl-fuzz`
    GlobalVariable *AFLMapPtr =
            new GlobalVariable(M, PPStructTy, false,
                               GlobalValue::ExternalLinkage, 0, "__afl_map_ptr");

    // _prev_
    GlobalVariable *AFLPrevLoc = new GlobalVariable(
            M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
            0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

    /* Instrument all the things! */

    int inst_blocks = 0;

    for (auto &F : M) {
        if (F.isIntrinsic())
            continue;


    for (auto &BB : F) {

        BasicBlock::iterator IP = BB.getFirstInsertionPt();

        IRBuilder<> IRB(&(*IP));

        if (AFL_R(100) >= inst_ratio) continue;


        /* Make up cur_loc */

        unsigned int cur_loc = AFL_R(MAP_SIZE);

        ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);


        /* Load prev_loc */

        LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
        PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
        Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, Int32Ty);

        /* Load SHM pointer */

        LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
        MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        // hash of the edg
        Value *MapPtrIdx = IRB.CreateXor(PrevLocCasted, CurLoc);

        // update the trace
        IRB.CreateCall(update, {MapPtr, MapPtrIdx});


//        // find the edge
//        Value *new_edge = IRB.CreateCall(find_edge, {AFLMapPtr, MapPtrIdx});
//
//        /* how do we add edge */
//        IRBuilder<> addBuilder(add);
//        // Create the new edge
//        Value *create_call = addBuilder.CreateCall(create_edge, {AFLMapPtr, MapPtrIdx});
//
//        // Insert to the edge hashmap
//        addBuilder.CreateCall(add_edge, {AFLMapPtr, create_call});
//
//        /* how do we update edge */
//        IRBuilder<> updateBuilder(update);
//        // update the count
//        updateBuilder.CreateCall(update_count, {AFLMapPtr, MapPtrIdx});
//
//        // Is the edge new?
//        Value* edge_exits = IRB.CreatePtrDiff(NullPTR, new_edge);
//
//        Value* cond = IRB.CreateICmp(CmpInst::Predicate::ICMP_EQ, edge_exits,
//                ConstantInt::get(IntegerType::getInt64Ty(C), 0));
//
//        // new edge -> add
//        // seen edge -> update
//        IRB.CreateCondBr(cond, update, add);


        /* Set prev_loc to cur_loc >> 1 */
        StoreInst *Store =
                IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
        Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        inst_blocks++;

    }
}
  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u locations (%s mode, ratio %u%%).",
             inst_blocks, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }


  return true;

}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_OptimizerLast, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
