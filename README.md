## Softbound

## to build llvm
```
./setup.py
```


## to test if the pass work
```
## make our libSoftbound.so first
cd lib 
make 

## this will compile ./test-suite/a.c with our pass
cd ..
make
./a.exe
```

## to test 
./test-suite/a.c is a collection of tests.  
I am just using that for checking whether the pass works or not  
To perform test separately, do this :  
```
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


