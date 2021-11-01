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
        
    // SOFTBOUND_REGISTER
    Function *RFP =  M->getFunction(SOFTBOUND_REGISTER) ;
    if ( !RFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        RFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(), // get a random function 
                SOFTBOUND_REGISTER, M) ;
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
    
    // SOFTBOUND_CHECK: Check Fat Pointer
    Function *CFP = M->getFunction(SOFTBOUND_CHECK) ;
    if ( !CFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        PFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(),
                SOFTBOUND_CHECK, M);
    }

    return true ;
}

void SoftboundPass::harvestPointers(Function &F) {

    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // local variable
            if ( auto AllocaI = dyn_cast<AllocaInst>(&I) ) {

                Type* AllocatedTy = AllocaI->getAllocatedType() ;
                if ( AllocatedTy->isArrayTy() )  {
                    auto ArrTy = dyn_cast<ArrayType>(AllocatedTy) ;
                    registerArray(AllocaI, ArrTy) ;
                } else if ( AllocatedTy->isPointerTy() ) {
                    auto PtrTy = dyn_cast<PointerType>(AllocatedTy) ;
                    registerPointer(AllocaI, PtrTy) ;
                }
            }
            // TODO: global variable

            // TODO: function argument
            // TODO: Propagate 
        }
    }
}

void SoftboundPass::registerArray(AllocaInst *AllocaI, ArrayType *ArrTy) {

    
    // and it's element must have size ( hardly not true )
    Type* ElemTy = ArrTy->getElementType() ;
    if ( !ElemTy->isSized() ) return;
    

    // Ready to write code, setup first
    Module* M = AllocaI->getFunction()->getParent() ;
    DataLayout DL = M->getDataLayout() ; //used for type size

    // write code
    IRBuilder<> IRB(AllocaI->getNextNode());
    ConstantInt *PtrID = IRB.getInt32(AssignedID) ; 
    PointerIDMap[AllocaI] = AssignedID ;
    AssignedID ++ ;
    Value* PtrBase = IRB.CreateBitCast(AllocaI, IRB.getInt8PtrTy()) ;
    Value* IntBase = IRB.CreatePtrToInt(PtrBase, IRB.getInt64Ty()) ;
    unsigned TotalBits = DL.getTypeStoreSize(ArrTy) ; 
    ConstantInt* TotalSize = IRB.getInt64(TotalBits) ;
    Value* IntBound = IRB.CreateAdd(IntBase, TotalSize) ;
    Value* PtrBound = IRB.CreateIntToPtr(IntBound, IRB.getInt8PtrTy()) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrBase, PtrBound}) ;

}


void SoftboundPass::registerPointer(AllocaInst *AllocaI, PointerType *PtrTy) {

    Module* M = AllocaI->getFunction()->getParent() ;
    DataLayout DL = M->getDataLayout() ;

    // write code
    IRBuilder<> IRB(AllocaI->getNextNode()) ;
    ConstantInt *PtrID = IRB.getInt32(AssignedID) ;
    PointerIDMap[AllocaI] = AssignedID ;
    AssignedID ++ ;
    Value* PtrNull = IRB.CreateIntToPtr(IRB.getInt64(0), IRB.getInt8PtrTy()) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrNull, PtrNull}) ;

}
void SoftboundPass::checkPointers(Function &F) {
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // memcpy, strcpy
            checkSequentialWrite(I) ;
            checkStore(I) ;
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
void SoftboundPass::checkStore(Instruction &I) {
    auto *StoreI = dyn_cast<StoreInst>(&I) ;
    if ( !StoreI ) return ;

    // assert(StoreI->getNumOperands() >= 2 ) ;
    errs() << "SOFTBOUND-Checking StoreInst " << *StoreI << "\n" ;
    Value* DstPtr = StoreI->getOperand(1) ; // GEP? 
    Value* DefinedPtr = getDefinition(DstPtr) ;
    writeCheckCode(StoreI, DefinedPtr, DstPtr) ;

}


// Place a check before { strcpy, strncpy, memcpy, memset } 
void SoftboundPass::checkSequentialWrite(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;
    // TODO (MAYBE): 
    // a constant list should be of much simple types.
    // ArrayRef<StringRef> => cannot use the forloop to capture
    const SmallVector<StringRef> CheckFnList = {"strcpy", "strncpy",
                                          "memcpy" , "memset" };


    // TODO: make this 
    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;
    bool ShouldBeChecked = false ;
    for ( auto Name: CheckFnList ) {
        if ( FnName.contains(Name) ) {
            ShouldBeChecked = true ;
            break ;
        }
    }


    if ( !ShouldBeChecked )
        return ;
    if ( !FnName.contains("strcpy")) {
        // These function follows the Fn(dst, src, size) format
        // Dst : if it's in our FatPointer lookup table, check!
        // Src : dont care
        // Size: if dst in map, check 
        if ( CallI->getNumOperands() < 3 ) return ;

        errs() << "\nSOFTBOUND-Checking CallInst" << *CallI << "\n" ;
        // Dst 
        Value* DstPtr = CallI->getOperand(0) ; 
        Value* DefinedPtr = getDefinition(DstPtr) ;  
        // Size
        auto CpySize = dyn_cast<ConstantInt>(CallI->getOperand(2)) ;
        if ( !CpySize || !CpySize->getType()->isIntegerTy() ) {
            errs() << "3rd argument is not size\n" ;
            return ;
        } 
        // [dst, dst+size) or [dst, dst+size-1]
        uint64_t u64Size = CpySize->getZExtValue() - 1 ;

        writeCheckCode(CallI, DefinedPtr, DstPtr, u64Size);
        errs() << FnName << " CHECK " << *DefinedPtr << "!!!\n" ;
         
    }
}

Value* SoftboundPass::getDefinition(Value* V) {

    Value* Ptr = V ;
    while ( !isa<AllocaInst>(Ptr) ) {

        auto PtrU = dyn_cast<User>(Ptr) ;
        if ( !PtrU ) {
            errs() << *Ptr << " is not a definition nor does it a User type\n" ;
            return nullptr ;
        }

        unsigned NumOps = PtrU->getNumOperands() ;
        if ( !NumOps ) {
            errs() << *PtrU << " does not have operands....\n" ;
            return nullptr;
        }
        Value* NextV ;
        if ( isa<PHINode>(PtrU) ) {
            // TODO: this value (Op1) may not be correct
            NextV = PtrU->getOperand(1) ;
        } else {
            NextV = PtrU->getOperand(0) ; // bitcast, GEP 
        }
        errs() << "===================================" \
               << "\nUpdate: \n" << *Ptr << " backtrack to" \
               << *NextV << "\n=======================\n";
        Ptr = NextV ; 
    }
    errs() << "Success: " << *Ptr << " FOUND !\n"; 
    return Ptr ;
}


void SoftboundPass::writeCheckCode(Instruction *I, Value* FatPtr, Value* AccessPtr, uint64_t offset) {
    
    if ( PointerIDMap.find(FatPtr) == PointerIDMap.end() ) {
        errs() << "Cannot place check before: " << *I \
               << "\nbecause " << *FatPtr << " is not registered \n" ; 
        return ;
    }
    auto AccessPtrInst = dyn_cast<Instruction>(AccessPtr) ;
    if ( !AccessPtrInst ) {
        // hardly happen: we use GEP to make dereferences...
        errs() << "AccessPtr is not a instruction\n" ;
        return ;
    }
    
    Module *M = I->getFunction()->getParent() ; 
    Function *CFP = M->getFunction(SOFTBOUND_CHECK) ; 
    // Don't use IRBuilder<> IRB(I->getPrevNode()) ;
    IRBuilder<> IRB(AccessPtrInst->getNextNode()) ;
    ConstantInt* PtrID = IRB.getInt32(PointerIDMap[FatPtr]) ;
    
    if ( !offset ) {
        IRB.CreateCall(CFP->getFunctionType(), CFP, {PtrID, AccessPtr}) ;
        return ;
    }
    // NOTE: this method uses PtrToInt and IntToPtr
    // Though we can finish this in GEP, but GEP requires weird
    // type match. e.g. it must be [20 * i8] if it's an array pointer
    Value* Int64Ptr = IRB.CreatePtrToInt(AccessPtr, IRB.getInt64Ty()) ;
    Value* AddedPtr = IRB.CreateAdd(Int64Ptr, IRB.getInt64(offset)) ;
    Value* NewPtr   = IRB.CreateIntToPtr(AddedPtr, IRB.getInt8PtrTy()) ;
    // SOFTBOUND_CHECK(ptr_id, ptr) ;
    IRB.CreateCall(CFP->getFunctionType(), CFP, {PtrID, NewPtr} );

}


