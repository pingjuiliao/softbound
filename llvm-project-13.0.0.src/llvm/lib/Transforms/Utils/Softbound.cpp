//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
//
//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/Softbound.h"

using namespace llvm;

PreservedAnalyses SoftboundPass::run(Function &F, FunctionAnalysisManager &AM) {

    static bool HasInit = false ;
    if ( !HasInit ) 
        HasInit = initializeLinkage(F.getParent()) ; 
    harvestPointers(F) ;
    checkPointers(F) ;

    return PreservedAnalyses::all();
}


bool SoftboundPass::initializeLinkage(Module* M) {

    LLVMContext &Ctx = M->getContext() ;
    if ( M->empty() ) {
        errs() << "Initialization Failed \n" ;
        return false ; 
    } 
        
    // SOFTBOUND_UPDATE
    Function *UFP =  M->getFunction(SOFTBOUND_UPDATE) ;
    if ( !UFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        UFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(), // get a random function 
                SOFTBOUND_UPDATE, M) ;
    }
    
    // SOFTBOUND_PROPAGATE
    Function *PFP = M->getFunction(SOFTBOUND_PROPAGATE) ;
    if ( !PFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        PFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(), 
                SOFTBOUND_PROPAGATE, M) ;

    }
    return true ;
}

void SoftboundPass::harvestPointers(Function &F) {
    Module* M = F.getParent() ;
    // LLVMContext &Ctx = M->getContext() ; 

    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            if ( auto *Alloca = dyn_cast<AllocaInst>(&I) ) {
                updateArrayBaseBound(Alloca) ;
                continue ;
            } 
            propagatePointers(I) ;
        }
    }
}

void SoftboundPass::updateArrayBaseBound(AllocaInst *AllocaI) {

    Module* M = AllocaI->getFunction()->getParent() ;

    // and it must be an array
    Type* AllocaTy = AllocaI->getAllocatedType() ;
    ArrayType* ArrTy = dyn_cast<ArrayType>(AllocaTy); 
    if ( !ArrTy ) return  ;

    // and it's element must have size ( hardly not true )
    Type* ElemTy = ArrTy->getElementType() ;
    if ( !ElemTy->isSized() ) return  ;


    // map char[]
    IRBuilder<> IRB(AllocaI->getNextNode());
    ConstantInt *PtrID = IRB.getInt32(AssignedID) ; 
    Value* PtrBase = IRB.CreateBitCast(AllocaI, IRB.getInt8PtrTy()) ;
    Value* GEPPtrBound = IRB.CreateInBoundsGEP(ArrTy, AllocaI, IRB.getInt64(ArrTy->getNumElements()) );
    Value* PtrBound = IRB.CreateBitCast(GEPPtrBound, IRB.getInt8PtrTy()) ;

    Function* UFP = M->getFunction(SOFTBOUND_UPDATE) ;
    
    IRB.CreateCall(UFP->getFunctionType(), UFP, {PtrID, PtrBase, PtrBound}) ;
    PointerIDMap[AllocaI] = AssignedID ;
    AssignedID ++ ;

}

void SoftboundPass::checkPointers(Function &F) {
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // memcpy, strcpy
            checkSequentialCopy(I) ;
        }
    }
}


void SoftboundPass::propagatePointers(Instruction &I) {
    
    Module *M = I.getFunction()->getParent() ;

    bool IsPtrTy = I.getType()->isPtrOrPtrVectorTy() ;
    bool IsArrTy = I.getType()->isArrayTy() ;
    if ( !IsPtrTy && !IsArrTy  ) return ;
    
    if ( !I.getNumOperands() ) return ; // no operands...

    // GEP, BitCast, ....
    Value* Op0 = I.getOperand(0) ;
    if ( PointerIDMap.find(Op0) == PointerIDMap.end() ) return ;

    errs() << I << " pass should be propagated\n" ;
    // propagate
    IRBuilder<> IRB(I.getNextNode()) ;
    ConstantInt* DstPtrID = IRB.getInt32(AssignedID) ;
    ConstantInt* SrcPtrID = IRB.getInt32(PointerIDMap[Op0]) ;
    Function* PFP = M->getFunction(SOFTBOUND_PROPAGATE) ;
    IRB.CreateCall(PFP->getFunctionType(), PFP, 
                    {DstPtrID, SrcPtrID}) ;
    PointerIDMap[ &I ] = AssignedID ;
    AssignedID ++ ;

}


void SoftboundPass::checkSequentialCopy(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;
    const SmallVector<StringRef, 8> SizedFn = {"memcpy", "strncpy"} ; 
    // check  
    // Function* Caller = CallI->getFunction() ;
    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;

    if ( !FnName.contains("cpy") ) 
        return ;
    if ( !FnName.contains("strcpy")) {
        // These function follows the Fn(dst, src, size) format
        // Dst : if it's in our FatPointer lookup table, check!
        // Src : dont care
        // Size: if dst in map, check 
        if ( CallI->getNumOperands() < 3 ) return ;

        Value* DstPtr = CallI->getOperand(0) ; 
        if ( PointerIDMap.find(DstPtr) == PointerIDMap.end()) {
            errs() << "cannot instrument buggy function" << FnName ;
            errs() << "DST is " << *DstPtr << "\n" ;
            return ;
        } 
        Value* CpySize = CallI->getOperand(2) ;
        if ( !CpySize->getType()->isIntegerTy() ) {
            errs() << "3rd argument is not size\n" ;
            return ;
        }
        /*
        IRBuilder<> IRB(CallI->getPrevNode()) ;
        ConstantInt* PtrID = IRB.getInt32(PointerIDMap[Dst]) ;
        */
        errs() << "CHECKED !!!!!\n" ;
         
    }


}

