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
    
    // SOFTBOUND_UPDATE
    Function *UFP = M->getFunction(SOFTBOUND_UPDATE) ;
    if ( !UFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        UFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(), 
                SOFTBOUND_UPDATE, M) ;

    }
    
    // SOFTBOUND_CHECK: Check Fat Pointer
    Function *CFP = M->getFunction(SOFTBOUND_CHECK) ;
    if ( !CFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        UFP = Function::Create(FnTy, 
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
            
            // Propagate/update 
            if ( auto StoreI = dyn_cast<StoreInst>(&I) ) {
                errs() << "Suspictious pointer: " << *StoreI << "\n" ;
                updatePointer(StoreI) ;     
            }
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


void SoftboundPass::updatePointer(StoreInst *StoreI) {
    Module* M = StoreI->getFunction()->getParent() ;

    // ************************
    // 1. get DstPtrID: the pointer should be updated
    // ************************

    // check if it's our registered pointer
    Value* LoadPtrVal = StoreI->getOperand(1) ;
    auto LoadPtrInst  = dyn_cast<Instruction>(LoadPtrVal) ;
    // the instruction loads pointers before the store (StoreI)
    if ( !LoadPtrInst || !LoadPtrInst->getNumOperands() ) {
        errs() << "[EXCEPTION] store inst does not have an instuction" 
               << " to load pointers\n" ;
        return ;
    }
    errs() << *LoadPtrInst << " is LoadPtrInst \n" ;
    // assume it's always GEP
    Value* DstPtr = LoadPtrInst->getOperand(0) ;
    if ( !DstPtr->getType()->isPointerTy() ) 
        return ;
    if ( PointerIDMap.find(DstPtr) == PointerIDMap.end() )
        return ;
    unsigned DstPtrID = PointerIDMap[DstPtr] ; 
    
    errs() << "DstPointer Found " << *DstPtr ;
    // ***********************
    // 2. get SrcPtrID: the pointer 
    // ***********************
    Value* LoadValVal = StoreI->getOperand(0) ;
    if ( !LoadValVal->getType()->isPointerTy() ) 
        return ;
    
    Value* SrcPtr = getDeclaration(LoadValVal) ;
    if ( !SrcPtr->getType()->isPointerTy() || 
            SrcPtr->getType()->isArrayTy() ) 
        return ;
    if ( PointerIDMap.find(SrcPtr) == PointerIDMap.end() ) 
        return ;
    unsigned SrcPtrID = PointerIDMap[SrcPtr] ;

    errs() << "\n\nSOFTBOUND-Updating Pointer " << *StoreI ;  

    // propagate
    IRBuilder<> IRB(LoadPtrInst->getNextNode()) ;
    ConstantInt* DstPtrIDCI = IRB.getInt32(DstPtrID) ;
    ConstantInt* SrcPtrIDCI = IRB.getInt32(SrcPtrID) ;
    Function* UFP = M->getFunction(SOFTBOUND_UPDATE) ;
    IRB.CreateCall(UFP->getFunctionType(), UFP, {DstPtrIDCI, SrcPtrIDCI}) ;

}
void SoftboundPass::checkStore(Instruction &I) {
    auto *StoreI = dyn_cast<StoreInst>(&I) ;
    if ( !StoreI ) return ;

    // assert(StoreI->getNumOperands() >= 2 ) ;
    errs() << "\n\nSOFTBOUND-Checking StoreInst " << *StoreI << "\n" ;
    Value* DstPtr = StoreI->getOperand(1) ; // GEP? 
    Value* DefinedPtr = getDeclaration(DstPtr) ;
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
        Value* DefinedPtr = getDeclaration(DstPtr) ;  
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

Value* SoftboundPass::getDeclaration(Value* V) {

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


