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
    static bool HasInitialized = false ;
    if ( !HasInitialized ) {
        Module *M = F.getParent() ;
        LLVMContext &C = M->getContext() ;
        Type *VoidTy = Type::getVoidTy(C) ;
        Type *I8PtrTy = PointerType::getInt8PtrTy(C) ;
        Type *I32Ty =  IntegerType::getInt32Ty(C) ;
        ArrayRef<Type*> UFPArgsTy = {I32Ty, I8PtrTy, I8PtrTy} ;
        FunctionType *FuncUFPTy = FunctionType::get(VoidTy, UFPArgsTy, false);
        M->getOrInsertFunction("updateFatPointer", FuncUFPTy) ;
        // Function* UFP = Function::Create(FuncUFPTy, Function::ExternalLinkage, "updateFatPointer", M) ;
    }

    // get pointer first
    harvestPointers(F) ;
    
    // intrument checks
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // handle sequential map
            checkSequentialCopy(I) ;

        }
    }
    return PreservedAnalyses::all();
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
        Type *VoidTy = Type::getVoidTy(C) ;
        Type *I8PtrTy = PointerType::getInt8PtrTy(C) ;
        Type *I32Ty =  IntegerType::getInt32Ty(C) ;
        ArrayRef<Type*> UFPArgsTy = {I32Ty, I8PtrTy, I8PtrTy} ;
        FunctionType *FuncUFPTy = FunctionType::get(VoidTy, UFPArgsTy, false);
        
            
            // map char[]
            IRBuilder<> IRB(AllocaI->getNextNode());
            // ID
            ConstantInt *PtrID = IRB.getInt32(assignedID++) ; 
            // base
            PointerType *VoidPtrTy = PointerType::get(Type::getVoidTy(C), 0) ;
            Value* PtrBase = IRB.CreateBitCast(AllocaI, IRB.getInt8PtrTy()) ;

            // bound
            unsigned TS = DL.getTypeAllocSize(ElemTy) * ArrTy->getNumElements();
            errs() << "Size : " << TS << "\n" ;
            Value* PtrBound = IRB.CreateInBoundsGEP(ArrTy, AllocaI, IRB.getInt64(ArrTy->getNumElements()) );
            

            ArrayRef<Value*> args = {PtrID, PtrBase, PtrBound} ;
            Function* CFP = M->getFunction("checkFatPointer") ;
            errs() << "CFP: " <<  CFP << "\n" ;
            Function* UFP = M->getFunction("updateFatPointer") ;
            errs() << "UFP: " << UFP << "\n" ;
            /*
            IRB.CreateCall(FuncUFPTy, UFP, args) ;
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

