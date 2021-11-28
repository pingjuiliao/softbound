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



// Place a check before { strcpy, strncpy, memcpy, memset } 
void SoftboundPass::checkSizedSequentialOperation(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;
    // TODO (MAYBE): 
    // a constant list should be of much simple types.
    // ArrayRef<StringRef> => cannot use the forloop to capture
    const SmallVector<StringRef> DstSrcSizeFnList = \
    {"strncpy",  "memcpy" , "memset", "memmove", "read", "write", 
    "strncat", "fgets"};   \

    // TODO: make this simple 
    Function* Callee = CallI->getCalledFunction() ;
    StringRef FnName = Callee->getName() ;
    bool ShouldBeChecked = false ;
    for ( auto Name: DstSrcSizeFnList ) {
        if ( FnName.compare(Name) == 0 ) {
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

    // errs() << "\nSOFTBOUND-Checking CallInst" << *CallI << "\n" ;
    bool IsFGETS = FnName.compare("fgets") == 0 ;  
    // Dst 
    Value* DstPtr = CallI->getOperand(0) ; 
    // Size
    Value* SizeValue = FnName.compare("fgets")!=0? \
                       CallI->getOperand(2) : CallI->getOperand(1);

    auto GEP = dyn_cast<GetElementPtrInst>(DstPtr);
    if ( !GEP ) 
        return ;
           
    writeCheckCodeAfter(GEP, SizeValue);
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




