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
    if ( !HasInit ) {
        HasInit = initializeLinkage(F.getParent()) ; 
    }
    registerPointers(F) ;
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

// **********************
// Things to register: 
//  1) local variables (AllocaInst) 
//  2) global variables   TODO
//  3) function arguments TODO
// **********************

void SoftboundPass::registerPointers(Function &F) {

    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            //  1) local variables 
            if ( auto AllocaI = dyn_cast<AllocaInst>(&I) ) {
                Type* AllocatedTy = AllocaI->getAllocatedType() ;
                if ( AllocatedTy->isArrayTy() )  {
                    auto ArrTy = dyn_cast<ArrayType>(AllocatedTy) ;
                    registerArray(AllocaI, ArrTy) ;
                } else if ( AllocatedTy->isPointerTy() ) {
                    auto PtrTy = dyn_cast<PointerType>(AllocatedTy) ;
                    registerAllocatedPointer(AllocaI, PtrTy) ;
                }
            }
            // 1-c) malloc, calloc, realloc
            registerHeapAlloc(&I) ;
            // 1-d) phinode
            if ( auto PHI = dyn_cast<PHINode>(&I) ) 
                registerPHINode(PHI) ;
            // 2) global variable
            for ( auto &Val: I.operands() ) {
                auto GV = dyn_cast<GlobalVariable>(&Val) ;
                if ( !GV ) 
                    continue ;
                errs() << "[GlobalVal] "<< *GV << "\n" ;
                Type* GVTy = GV->getValueType() ;
                if ( GVTy->isArrayTy() ) { 
                    auto ArrTy = dyn_cast<ArrayType>(GVTy);
                    errs() << "[GlobalValArr] "<< *GV << "\n" ;
                    registerGlobalArray(GV, ArrTy) ;
                } /*else if ( GVTy->isPointerTy() ) {
                    auto PtrTy = dyn_cast<PointerType>(GVTy) ;
                    registerAllocatedPointer(GV, PtrTy) ;
                }*/
            } 
        }
    }
    //  2) global variables   TODO
    //  3) function arguments TODO
    
}

    

void SoftboundPass::checkPointers(Function &F) {
    // two style: 
    // 1) check each dereference
    // 2) check dereferences that use write operation

    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            
            // check dereferences
            checkDereference(I) ;

            // check Sequential functions
            checkSequentialWrite(I) ;
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
    PointerIDMap[AllocaI] = AssignedID ;
    AssignedID ++ ;
    ConstantInt *PtrID = IRB.getInt32( PointerIDMap[AllocaI] ) ; 
    Value* PtrBase = IRB.CreateBitCast(AllocaI, IRB.getInt8PtrTy()) ;
    Value* IntBase = IRB.CreatePtrToInt(PtrBase, IRB.getInt64Ty()) ;
    unsigned TotalBits = DL.getTypeStoreSize(ArrTy) ; 
    ConstantInt* TotalSize = IRB.getInt64(TotalBits) ;
    Value* IntBound = IRB.CreateAdd(IntBase, TotalSize) ;
    Value* PtrBound = IRB.CreateIntToPtr(IntBound, IRB.getInt8PtrTy()) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrBase, PtrBound}) ;
}


void SoftboundPass::registerGlobalArray(GlobalVariable* GV, ArrayType *ArrTy) {

    
    // and it's element must have size ( hardly not true )
    Type* ElemTy = ArrTy->getElementType() ;
    if ( !ElemTy->isSized() ) return;
    
    // return if already registered
    if ( PointerIDMap.find(GV) != PointerIDMap.end() ) 
        return ; 
    
    // Ready to write code, setup first
    Module* M = GV->getParent() ;
    DataLayout DL = M->getDataLayout() ; //used for type size
    Function *MainF=M->getFunction("main");
    // write code

    IRBuilder<> IRB(MainF->getEntryBlock().getFirstNonPHI()) ;
    PointerIDMap[ GV ] = AssignedID ;
    AssignedID ++ ;
    ConstantInt *PtrID = IRB.getInt32( PointerIDMap[ GV ] ) ; 
    Value* PtrBase = IRB.CreateBitCast(GV, IRB.getInt8PtrTy()) ;
    Value* IntBase = IRB.CreatePtrToInt(PtrBase, IRB.getInt64Ty()) ;
    unsigned TotalBits = DL.getTypeStoreSize(ArrTy) ; 
    ConstantInt* TotalSize = IRB.getInt64(TotalBits) ;
    Value* IntBound = IRB.CreateAdd(IntBase, TotalSize) ;
    Value* PtrBound = IRB.CreateIntToPtr(IntBound, IRB.getInt8PtrTy()) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrBase, PtrBound}) ;
}




void SoftboundPass::registerAllocatedPointer(AllocaInst *AllocaI, PointerType *PtrTy) {

    Module* M = AllocaI->getFunction()->getParent() ;
    DataLayout DL = M->getDataLayout() ;

    // write code
    IRBuilder<> IRB(AllocaI->getNextNode()) ;
    PointerIDMap[AllocaI] = AssignedID ;
    AssignedID ++ ;
    ConstantInt *PtrID = IRB.getInt32( PointerIDMap[ AllocaI ] ) ;
    Value* PtrNull = IRB.CreateIntToPtr(IRB.getInt64(0), IRB.getInt8PtrTy()) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrNull, PtrNull}) ;
}

void SoftboundPass::registerHeapAlloc(Instruction *I) {

    // settings
    Module *M = I->getFunction()->getParent() ;
    Function *RFP =  M->getFunction(SOFTBOUND_REGISTER) ;
    const SmallVector<StringRef, 4> AllocFnList = { "malloc", 
                                                 "calloc", 
                                                 "realloc" } ;
    // TODO: handle 'new' operator, which is not a CallInst
    auto CallI = dyn_cast<CallInst>(I) ;
    if ( !CallI ) 
        return  ;
    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;
    bool ShouldBeUpdated = false ;
    for ( auto Name: AllocFnList ) {
        if ( FnName == Name ) {
            ShouldBeUpdated = true ;
            break ;
        }
    }

    if ( !ShouldBeUpdated ) 
        return ;

    IRBuilder<> IRB(CallI->getNextNode()) ;
    Value* AllocSize ;
    if ( FnName == "malloc" ) {
        AllocSize = CallI->getOperand(0) ;
    } else {
        AllocSize = IRB.CreateMul(CallI->getOperand(0), CallI->getOperand(1)) ;
    }
    if ( !AllocSize ) {
        errs() << *CallI << " does not have allocation size \n";
        return ;
    }   
    PointerIDMap[ CallI ] = AssignedID ;
    AssignedID ++ ;
    ConstantInt* PtrID = IRB.getInt32( PointerIDMap[ CallI ] ) ;
    Value* PtrBase  = CallI ;
    Value* IntBase  = IRB.CreatePtrToInt(CallI, IRB.getInt64Ty()) ;
    Value* ZExtSize = IRB.CreateZExt(AllocSize, IRB.getInt64Ty()) ;
    Value* IntBound = IRB.CreateAdd(IntBase, ZExtSize) ;
    Value* PtrBound = IRB.CreateIntToPtr(IntBound, IRB.getInt8PtrTy()) ;
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrBase, PtrBound}) ;

}

void SoftboundPass::registerPHINode(PHINode* PHI) {
    
    Function* F = PHI->getFunction() ;
    Module*   M = F->getParent() ;

    if ( !PHI->getType()->isPointerTy() ) {
        errs() << *PHI << " is not pointer Type\n" ;
        return ;
    }

    if ( PHI->getNumOperands() < 2 ) {
        errs() << *PHI << " has no multiple incoming values\n" ;
        return ;
    }
    // write code
    IRBuilder<> IRB(F->getEntryBlock().getFirstNonPHI()) ;
    PointerIDMap[PHI] = AssignedID ;
    AssignedID ++ ;
    ConstantInt *PtrID = IRB.getInt32( PointerIDMap[PHI] ) ;
    Value* PtrNull = IRB.CreateIntToPtr(IRB.getInt64(0), IRB.getInt8PtrTy());
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrID, PtrNull, PtrNull}) ;

    for ( auto &Op: PHI->incoming_values() ) {
        auto OpI = dyn_cast<Instruction>(&Op) ;
        if ( !OpI ) { 
            errs() << "registerPHINode: operands not a instruction\n" ;
            continue ;
        }
        writeUpdateCodeAfter(OpI, PointerIDMap[ PHI ]) ;
    }
}


void SoftboundPass::writeUpdateCodeAfter(Instruction *I, 
                                                      unsigned DstID) {
    // global settings
    Module* M = I->getFunction()->getParent() ;
    
    auto GEP = dyn_cast<GetElementPtrInst>(I) ;
    if ( !GEP ) {
        errs() << "Anther Inst to overwrite pointers/arrays\n" ;
        errs() << "writeUpdateCodeAfter: " << *I << "failed\n" ;
        return ;
    }
    
    if ( !GEP->getNumOperands() ) {
        errs() << "error: GEP has no opereands\n" ;
        return ;
    }

    Value* SrcPtr = getDeclaration(GEP->getOperand(0)) ; 
    if ( PointerIDMap.find(SrcPtr) == PointerIDMap.end() ) {
        errs() << "writeUpdateCodeAfter: "
               << " cannot find source pointer for " << *SrcPtr 
               << "\n" ;
        return ;
    }

    IRBuilder<> IRB(GEP->getNextNode()) ;
    Value* SrcPtrID = IRB.getInt32(PointerIDMap[SrcPtr]) ; 
    Value* DstPtrID = IRB.getInt32(DstID) ;
    Function* UFP = M->getFunction(SOFTBOUND_UPDATE) ;
    IRB.CreateCall(UFP->getFunctionType(), UFP, {DstPtrID, SrcPtrID}) ;
    errs() << "[UPDATE Code] success : " << *GEP << "\n" ;

}



void SoftboundPass::checkDereference(Instruction &I) {
    // GEP: Get Element Pointer
    // LLVM tends to use this for every dereferences
    auto GEP = dyn_cast<GetElementPtrInst>(&I) ;
    if ( !GEP ) 
        return ;


    if ( !GEP->getNumOperands() ) {
        errs() << "GEP no operands\n" ;
        return ;
    }
    
    // Opitimization Move: remove checking if it's &ptr[0] 
    // [DANGER]
    if ( GEP->hasAllZeroIndices() ) {
        return ;
    }
    
    writeCheckCodeAfter(GEP, 0);

}



// Place a check before { strcpy, strncpy, memcpy, memset } 
void SoftboundPass::checkSequentialWrite(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;
    // TODO (MAYBE): 
    // a constant list should be of much simple types.
    // ArrayRef<StringRef> => cannot use the forloop to capture
    const SmallVector<StringRef> CheckFnList = \
    {"strncpy",  "memcpy" , "memset", "memmove", "read", "write"};   \
     //"fgets", "strcpy", "snprintf", "strcat", "strncat", "scanf"  \


    // TODO: make this simple 
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
    
    // These function follows the Fn(dst, src, size) format
    // Dst : if it's in our FatPointer lookup table, check!
    // Src : dont care
    // Size: if dst in map, check 
    if ( CallI->getNumOperands() < 3 ) return ;

    errs() << "\nSOFTBOUND-Checking CallInst" << *CallI << "\n" ;
    // Dst 
    Value* DstPtr = CallI->getOperand(0) ; 
    // Size
    Value* SizeValue = CallI->getOperand(2) ;

    auto GEP = dyn_cast<GetElementPtrInst>(DstPtr);
    if ( !GEP ) 
        return ;
        
    writeCheckCodeAfter(GEP, SizeValue);
}

Value* SoftboundPass::getDeclaration(Value* V) {

    Value* Ptr = V ;
    while ( Ptr ) {
        
        if ( PointerIDMap.find(Ptr) != PointerIDMap.end() ) 
            break ;

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
        // TODO: some operation might not be Operand[0]
        NextV = PtrU->getOperand(0) ; // bitcast, GEP 
        /* DEBUG 
        errs() << "===GetDeclaration==================" \
               << "\nUpdate: \n" << *Ptr << "  backtrack to  " \
               << *NextV << "\n=======================\n";
        */
        Ptr = NextV ; 
    }
    if ( !Ptr ) {
        errs() << "[Error] getDeclaration cannot backtrace to nothing\n" ;
        return nullptr ;
    }
    errs() << "getDeclaration Success: " << *Ptr << " FOUND !\n"; 
    return Ptr ;
}

void SoftboundPass::writeCheckCodeAfter(GetElementPtrInst* GEP, Value* SizeVal) {


    // global settings
    Module* M = GEP->getFunction()->getParent() ;
    Function* CFP = M->getFunction(SOFTBOUND_CHECK) ;
    
    // resources from GEP
    Value* Ptr = getDeclaration(GEP->getPointerOperand()) ;
    if ( PointerIDMap.find(Ptr) == PointerIDMap.end() ) {
        errs() << "Unregistered pointer found: " << *GEP << "\n" ;
        return ;
    }

    IRBuilder<> IRB(GEP->getNextNode());
    ConstantInt* PtrID = IRB.getInt32(PointerIDMap[Ptr]) ;

    if ( !SizeVal ) {
        IRB.CreateCall(CFP->getFunctionType(), CFP, {PtrID, GEP}) ;
        return ;
    }
    Value* Int64Ptr = IRB.CreatePtrToInt(GEP, IRB.getInt64Ty()) ;
    Value* Int64Size= IRB.CreateZExt(SizeVal, IRB.getInt64Ty()) ;
    Value* AddedPtr = IRB.CreateAdd(Int64Ptr, Int64Size) ;
    AddedPtr = IRB.CreateSub(AddedPtr, IRB.getInt64(1)) ;
    Value* OffsetPtr= IRB.CreateIntToPtr(AddedPtr, IRB.getInt8PtrTy()) ;
    
    // SOFTBOUND_CHECK(ptr_id, ptr);
    IRB.CreateCall(CFP->getFunctionType(), CFP, {PtrID, OffsetPtr}); 
}




void SoftboundPass::updateStoreToPointer(StoreInst *StoreI) {
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

