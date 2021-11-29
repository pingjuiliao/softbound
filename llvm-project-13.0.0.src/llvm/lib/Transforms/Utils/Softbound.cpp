//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
//
//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/Softbound.h"

using namespace llvm;

PreservedAnalyses SoftboundPass::run(Module &M, ModuleAnalysisManager &AM) {

    static bool HasInit = false ;

    if ( !HasInit ) {
        HasInit = initializeLinkage(&M) ; 
    }
    registerBaseBound(M) ;
    checkBaseBound(M) ;

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
    /*
    Function *UFP = M->getFunction(SOFTBOUND_UPDATE) ;
    if ( !UFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        UFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(), 
                SOFTBOUND_UPDATE, M) ;

    }*/
    
    // SOFTBOUND_CHECK: Check Fat Pointer
    Function *CFP = M->getFunction(SOFTBOUND_CHECK) ;
    if ( !CFP ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        CFP = Function::Create(FnTy, 
                GlobalValue::ExternalLinkage, 
                M->begin()->getAddressSpace(),
                SOFTBOUND_CHECK, M);
    }

    Function* CFP_OFFSET = M->getFunction(SOFTBOUND_CHECK_OFFSET) ;
    if ( !CFP_OFFSET ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        CFP_OFFSET = Function::Create(FnTy, 
                        GlobalValue::ExternalLinkage, 
                        M->begin()->getAddressSpace(), 
                        SOFTBOUND_CHECK_OFFSET, M) ;
    }

    Function* CFP_STRING = M->getFunction(SOFTBOUND_CHECK_STRING) ;
    if ( !CFP_STRING ) {
        FunctionType *FnTy = FunctionType::get(Type::getVoidTy(Ctx), true) ;
        CFP_STRING = Function::Create(FnTy, 
                        GlobalValue::ExternalLinkage, 
                        M->begin()->getAddressSpace(), 
                        SOFTBOUND_CHECK_STRING, M) ;
    }


    return true ;
}

// **********************
// Things to register: 
//  1) local variables (AllocaInst) 
//  2) global variables   TODO
//  3) function arguments TODO
// **********************

void SoftboundPass::registerBaseBound(Module &M) {
    
    for ( auto &F: M ) {
        for ( auto &BB: F ) {
            for ( auto &I: BB ) {
                // local stack buffer
                if ( auto AllocI = dyn_cast<AllocaInst>(&I) ) {
                    registerStackBuffer(AllocI) ;
                // heap buffers
                } else if ( auto CallI = dyn_cast<CallInst>(&I) ) {
                    registerHeapBuffer(CallI) ;
                }
                // global stack buffers
                for ( auto &OP: I.operands() ) {
                    if ( auto GV = dyn_cast<GlobalVariable>(&OP) ) {
                        registerStackBuffer(GV, F) ;  
                        continue ;
                    } 
                    auto CE = dyn_cast<ConstantExpr>(&OP); 
                    if (!CE)
                        continue ;
                    for ( auto &CEOP: CE->operands()  ) {
                        auto CEGV = dyn_cast<GlobalVariable>(&CEOP) ;
                        if( !CEGV ) 
                            continue ;
                        registerStackBuffer(CEGV, F);
                    }
                }  
            }
        }
    }   
}

    

void SoftboundPass::checkBaseBound(Module &M) {
    // two style: 
    // 1) check each dereference
    // 2) check dereferences that use write operation

    for ( auto &F: M ) {
        for ( auto &BB: F ) {
            for ( auto &I: BB ) {
                // check dereferences
                checkDereference(I) ;

                // check Sequential functions
                checkSizedSequentialOperation(I) ;
                
                // check String-based functions
                checkStringBasedSequentialOperation(I) ;

            }
        }    
    }
}




void SoftboundPass::registerStackBuffer(AllocaInst *AllocaI) {

    // it should be array type
    Type* Ty = AllocaI->getAllocatedType() ; 
    auto ArrTy = dyn_cast<ArrayType>(Ty) ;
    if ( !ArrTy ) return ;

    // and it's element must have size ( hardly not true )
    Type* ElemTy = ArrTy->getElementType() ;
    if ( !ElemTy->isSized() ) return;
    
    // Ready to write code, setup first
    Module* M = AllocaI->getFunction()->getParent() ;
    DataLayout DL = M->getDataLayout() ; //used for type size

    // write code
    IRBuilder<> IRB(AllocaI->getNextNode());
    Value* PtrBase = IRB.CreateBitCast(AllocaI, IRB.getInt8PtrTy()) ;
    unsigned NumBytes = DL.getTypeStoreSize(ArrTy) ; 
    ConstantInt* BufSize = IRB.getInt64(NumBytes) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER) ;
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrBase, BufSize}) ;
}


void SoftboundPass::registerStackBuffer(GlobalVariable *GV, Function &UsedFunc) {
    Type* Ty = GV->getValueType() ;
    auto ArrTy = dyn_cast<ArrayType>(Ty) ;
    if ( !ArrTy )
        return ;

    Type* ElemTy = ArrTy->getElementType() ;
    if ( !ElemTy->isSized() ) 
        return ;

    Module *M = UsedFunc.getParent() ;
    DataLayout DL= M->getDataLayout() ;

    IRBuilder<> IRB(UsedFunc.getEntryBlock().getFirstNonPHI());
    Value* PtrBase = IRB.CreateBitCast(GV, IRB.getInt8PtrTy()) ;
    unsigned NumBytes = DL.getTypeStoreSize(ArrTy) ;
    ConstantInt* BufSize = IRB.getInt64(NumBytes) ;
    Function* RFP = M->getFunction(SOFTBOUND_REGISTER);
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrBase, BufSize}) ;
}

void SoftboundPass::registerHeapBuffer(CallInst* CallI) {

    // settings
    Module *M = CallI->getFunction()->getParent() ;
    Function *RFP =  M->getFunction(SOFTBOUND_REGISTER) ;
    const SmallVector<StringRef, 4> AllocFnList = { "malloc", 
                                                 "calloc", 
                                                 "realloc" } ;

    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;
    bool ShouldBeUpdated = false ;
    for ( auto Name: AllocFnList ) {
        if ( FnName.compare(Name) == 0) {
            ShouldBeUpdated = true ;
            break ;
        }
    }

    if ( !ShouldBeUpdated ) 
        return ;

    IRBuilder<> IRB(CallI->getNextNode()) ;
    Value* AllocSize ;
    if ( FnName.compare("malloc") == 0 ) {
        AllocSize = CallI->getOperand(0) ;
    } else {
        AllocSize = IRB.CreateMul(CallI->getOperand(0), CallI->getOperand(1)) ;
    }
    // errs() << "Register on " << FnName << "\n" ;
    if ( !AllocSize ) {
        errs() << *CallI << " does not have allocation size \n";
        return ;
    }
    Value* PtrBase  = CallI ;
    Value* ZExtSize = IRB.CreateZExt(AllocSize, IRB.getInt64Ty()) ;
    IRB.CreateCall(RFP->getFunctionType(), RFP, {PtrBase, ZExtSize}) ;

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

    errs() << "SOFTBOUND-Checking GEPInst " << *GEP << "\n"; 
    
    writeCheckCodeAfter(GEP, 0);

}


// These function perform sized sequential load/store operations
void SoftboundPass::checkSizedSequentialOperation(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;
    
    // a constant list should be of much simple types.
    std::map<StringRef, std::pair<unsigned, unsigned>> SizedFnMap = {
        { "strncpy", {0, 2} }, {"snprintf", {0, 1}},
        { "memcpy",  {0, 2} }, {"memmove",{0, 2}}, 
        { "llvm.memcpy.p0i8.p0i8.i64", {0, 2} },
        { "read",    {1, 2} }, {"write"  ,{1, 2}},
        { "recv",    {1, 2} }, {"send"   ,{1, 2}}, 
        { "recvfrom",{1, 2} }, {"sendto", {1, 2}},
        { "fgets",   {0, 1} }, {"fread",  {0, 1}}
    };

    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;
    
    if ( SizedFnMap.find(FnName) == SizedFnMap.end() ) 
        return ;
    
    // These function follows the Fn(dst, src, size) format
    // Dst : if it's in our FatPointer lookup table, check!
    // Src : dont care
    // Size: if dst in map, check 
    if ( CallI->getNumOperands() < 3 ) return ;

    // Dst 
    unsigned DstOp = SizedFnMap[FnName].first ;
    Value* DstPtr = CallI->getOperand(DstOp) ; 
    // Size
    unsigned SizeOp = SizedFnMap[FnName].second ;
    Value* SizeValue = CallI->getOperand(SizeOp) ;

    auto GEP = dyn_cast<GetElementPtrInst>(DstPtr);
    if ( !GEP ) 
        return ;
           
    Value* BasedPtr= GEP->getPointerOperand();
    // writeCheckCodeAfter(GEP, SizeValue);
    Module *M = CallI->getFunction()->getParent() ;
    Function* CFP_OFFSET = M->getFunction(SOFTBOUND_CHECK_OFFSET);

    IRBuilder<> IRB(CallI) ;
    IRB.CreateCall(CFP_OFFSET->getFunctionType(), CFP_OFFSET,
             {GEP, SizeValue, BasedPtr}) ;

    errs() << "SOFTBOUND-Checking CallInst " << FnName << "\n" ; 
}

void SoftboundPass::checkStringBasedSequentialOperation(Instruction &I) {

    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;
    
    std::map<StringRef, SmallVector<unsigned, 4> > StringFnMap = {
        { "strcpy",  {0, 1} },     // {"sprintf", {0, 1}},
        { "strcat",  {0, 0, 1} },  
        {"strncat",  {0, 0, 1, 2}} 
    };
    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;
    
    if ( StringFnMap.find(FnName) == StringFnMap.end() ) 
        return ;
    
    // These function follows the Fn(dst, src, size) format
    // Dst : if it's in our FatPointer lookup table, check!
    // Src : dont care
    // Size: if dst in map, check 
    if ( CallI->getNumOperands() < 2 ) 
        return ;

    
    
    // writeCheckCodeAfter(GEP, SizeValue);
    Module *M = CallI->getFunction()->getParent() ;
    Function* CFP_STRING = M->getFunction(SOFTBOUND_CHECK_STRING);

    IRBuilder<> IRB(CallI) ;
    // Dst 
    unsigned DstOp = StringFnMap[FnName][0] ;
    Value* DstPtr = CallI->getOperand(DstOp) ; 
    // Str0
    unsigned StrOp0 = StringFnMap[FnName][1] ;
    Value* Str0 = CallI->getOperand(StrOp0) ;
    // Str1
    Value* Str1 ;
    if (  StringFnMap[FnName].size() >= 3 ) {
        unsigned StrOp1 = StringFnMap[FnName][2] ;
        Str1 = CallI->getOperand(StrOp1);
    } else {
        Str1 = IRB.getInt64(0) ;
    }
    // bounded size: for strncat
    Value* SizeValue ;
    if ( StringFnMap[FnName].size() >= 4 ) {
        SizeValue = CallI->getOperand(StringFnMap[FnName][3]);
    } else {
        SizeValue = IRB.getInt64(-1) ;
    }

    // 
    auto GEP = dyn_cast<GetElementPtrInst>(DstPtr);
    if ( !GEP ) 
        return ;
           
    Value* BasedPtr= GEP->getPointerOperand();
    IRB.CreateCall(CFP_STRING->getFunctionType(), CFP_STRING,
             {GEP, Str0, Str1, SizeValue, BasedPtr}) ;

    errs() << "SOFTBOUND-Checking CallInst " << FnName << "\n" ; 
}

void SoftboundPass::writeCheckCodeAfter(GetElementPtrInst* GEP, Value* SizeVal) {


    // global settings
    Module* M = GEP->getFunction()->getParent() ;
    Function* CFP = M->getFunction(SOFTBOUND_CHECK) ;
    
    // resources from GEP
    Value* BasedPtr = GEP->getPointerOperand() ;
    if ( !BasedPtr ) 
        return ;

    IRBuilder<> IRB(GEP->getNextNode());
    
    if ( !SizeVal ) {
        IRB.CreateCall(CFP->getFunctionType(), CFP, {GEP, BasedPtr}) ;
        return ;
    }
    Value* Int64Ptr = IRB.CreatePtrToInt(GEP, IRB.getInt64Ty()) ;
    Value* Int64Size= IRB.CreateZExt(SizeVal, IRB.getInt64Ty()) ;
    Value* AddedPtr = IRB.CreateAdd(Int64Ptr, Int64Size) ;
    AddedPtr = IRB.CreateSub(AddedPtr, IRB.getInt64(1)) ;
    Value* OffsetPtr= IRB.CreateIntToPtr(AddedPtr, IRB.getInt8PtrTy()) ;
    
    // SOFTBOUND_CHECK(ptr_id, ptr);
    IRB.CreateCall(CFP->getFunctionType(), CFP, {OffsetPtr, BasedPtr}); 
}




