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
    Module *M = F.getParent(); 
    LLVMContext &Ctx = M->getContext() ;
    Function *UFP =  M->getFunction("updateFatPointer") ;
    if ( !UFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        UFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                F.getAddressSpace(), 
                "updateFatPointer", M) ;
    }


    harvestPointers(F) ;
        
        

    return PreservedAnalyses::all();
}



void SoftboundPass::harvestPointers(Function &F) {
    Module* M = F.getParent() ;
    LLVMContext &C = M->getContext() ; 

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
            Type *VoidTy = Type::getVoidTy(C) ;
            Type *I8PtrTy = PointerType::getInt8PtrTy(C) ;
            Type *I32Ty =  IntegerType::getInt32Ty(C) ;
            ArrayRef<Type*> UFPArgsTy = {I32Ty, I8PtrTy, I8PtrTy} ;
            FunctionType *FuncUFPTy = FunctionType::get(VoidTy, UFPArgsTy, false);


            // map char[]
            IRBuilder<> IRB(AllocaI->getNextNode());
            ConstantInt *PtrID = IRB.getInt32(assignedID++) ; 
            Value* PtrBase = IRB.CreateBitCast(AllocaI, I8PtrTy) ;
            Value* GEPPtrBound = IRB.CreateInBoundsGEP(ArrTy, AllocaI, IRB.getInt64(ArrTy->getNumElements()) );
            Value* PtrBound = IRB.CreateBitCast(GEPPtrBound, I8PtrTy) ;

            Function* UFP = M->getFunction("updateFatPointer") ;
            
            // FunctionCallee UFP = M->getOrInsertFunction("updateFatPointer", VoidTy, I32Ty, I8PtrTy, I8PtrTy) ;
            IRB.CreateCall(UFP->getFunctionType(), UFP, {PtrID, PtrBase, PtrBound}) ;
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

