//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
//
//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/Softbound.h"

using namespace llvm;


PreservedAnalyses SoftboundPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {

    // get pointer first
    harvestPointers(F) ;
    
    // intrument checks
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // handle sequential map
            checkSequentialCopy(I) ;

        }
    }
    return PreservedAnalyses::none();
}

 

void SoftboundPass::harvestPointers(Function &F) {
    Module *M  = F.getParent() ;
    LLVMContext &C = M->getContext() ; 
    DataLayout DL(M) ;

    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // only checks allocation intructions
            auto *AllocaI = dyn_cast<AllocaInst>(&I) ;
            if ( !AllocaI ) continue ;

            // and it must be an array
            Type* AllocaTy = AllocaI->getAllocatedType() ;
            ArrayType* ArrTy = dyn_cast<ArrayType>(AllocaTy); 
            if ( !ArrTy )  continue ;

            // and it's element must have size ( hardly not true )
            Type* ElemTy = ArrTy->getElementType() ;
            if ( !ElemTy->isSized() ) continue ;
            errs() << "============================================\n" ;
            errs() << *AllocaI << "\n" ;

            // auto ArrElemSize =  ArrTy->getScalarSizeInBits() / 8 ;
            // uint64_t ArrSize = AllocaI->getAllocationSizeInBits(DL) ;

            // map char[]
            IRBuilder<> IRB(AllocaI);
            // ID
            ConstantInt *PtrID = IRB.getInt32(assignedID++) ; 
            // base
            Value* BitcastI = IRB.CreateBitCast(AllocaI, IRB.getInt8PtrTy()) ;
            // bound
            // TypeSize TS = DL.getTypeAllocSize(ElemTy) * ArrTy->getNumElements();
            // Value* AddedBound = IRB.CreateAdd(AllocaI, IRB.getInt64(TS) ) ;
            // args
            /*
            SmallVector<Value*, 3> args ;
            args.emplace_back(PtrID);
            args.emplace_back(AllocaI);
            args.emplace_back(AddedBound) ;
            FunctionCallee CallUFP = M->getOrInsertFunction("updateFatPointer", IRB.getVoidTy()) ;
            IRB.CreateCall(CallUFP, args) ;
            */
        }
    }
}

void SoftboundPass::checkSequentialCopy(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;


    // check  
    Function* Caller = CallI->getFunction() ;
    Function* F = CallI->getCalledFunction() ;
    errs() << Caller->getName() << " gonna check : " << F->getName() << "\n" ;

}

