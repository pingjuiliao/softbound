## Softbound
a function pass, using the new pass manager


## to build llvm ( REQUIRED )
```
./setup.py
```

## to build shared library ( REQUIRED )
```
## build our libSoftbound.so first
make 
```

## to test if the compilation succeeds
```
## Requirement: 1) LLVM built 2) libSoftbound built
## this will compile ./test-suite/a.c with our pass
make        # in the REPO_ROOT
./a.exe
```

## to test if softbound can protect vulnerable codes 
./test-suite/a.c is a collection of tests.  
I am just using that for checking whether the pass works or not  
To perform test separately, do this :  
```
## Requirement: 1) LLVM built 2) libSoftbound built
## assume ${REPO_ROOT}/lib/libSoftbound.so exists...
cd ./test-suite
make test-xxxxx
./test-xxxxx.exe
```
Current progress:  
O test_strncpy  
O test_forloop  
O test_arbitW  
X test_runtime => TODO: propagate pointers!  


## Scripts
[Softbound.h](llvm-project-13.0.0.src/llvm/include/llvm/Transforms/Utils/Softbound.h)  
[Softbound.cpp](llvm-project-13.0.0.src/llvm/lib/Transforms/Utils/Softbound.cpp)  
[PassRegistry.def](llvm-project-13.0.0.src/llvm/lib/Passes/PassRegistry.def)  
[PassBuilder.cpp](llvm-project-13.0.0.src/llvm/lib/Passes/PassBuilder.cpp)  


## to debug
```
make debug
```


