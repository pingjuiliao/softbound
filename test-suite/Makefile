OBJ_ROOT=../llvm-project-13.0.0.src/build

CC=${OBJ_ROOT}/bin/clang
CXX=${OBJ_ROOT}/bin/clang++
OPT=${OBJ_ROOT}/bin/opt
LLC=${OBJ_ROOT}/bin/llc

EMIT_LLVM_FLAG=-m64 -O2 -S -emit-llvm
LIBSOFTBOUND=-L../lib -lSoftbound -Wl,-rpath,../lib 

all: test_arbitW test_forloop test_runtime test_strncpy
test_arbitW: test_arbitW.c ../lib/libSoftbound.so
	$(CC) ${EMIT_LLVM_FLAG} -o test_arbitW.ll test_arbitW.c
	$(OPT) -passes=softbound -o ./test_arbitW.bc test_arbitW.ll
	$(CC) -o test_arbitW.exe ${LIBSOFTBOUND} test_arbitW.bc
test_forloop: test_forloop.c ../lib/libSoftbound.so
	$(CC) ${EMIT_LLVM_FLAG} -o test_forloop.ll test_forloop.c
	$(OPT) -passes=softbound -o ./test_forloop.bc test_forloop.ll
	$(CC) -o test_forloop.exe ${LIBSOFTBOUND} test_forloop.bc
test_runtime: test_runtime.c ../lib/libSoftbound.so
	$(CC) ${EMIT_LLVM_FLAG} -o test_runtime.ll test_runtime.c
	$(OPT) -passes=softbound -o ./test_runtime.bc test_runtime.ll
	$(CC) -o test_runtime.exe ${LIBSOFTBOUND} test_runtime.bc
test_strncpy: test_strncpy.c ../lib/libSoftbound.so
	$(CC) ${EMIT_LLVM_FLAG} -o test_strncpy.ll test_strncpy.c
	$(OPT) -passes=softbound -o ./test_strncpy.bc test_strncpy.ll
	$(CC) -o test_strncpy.exe ${LIBSOFTBOUND} test_strncpy.bc
clean:
	rm *.bc *.ll *.exe
