OBJ_ROOT=../llvm-project-13.0.0.src/build

CC=${OBJ_ROOT}/bin/clang
CXX=${OBJ_ROOT}/bin/clang++
OPT=${OBJ_ROOT}/bin/opt
LLC=${OBJ_ROOT}/bin/llc

EMIT_LLVM_FLAG=-m64 -S -emit-llvm
LIBSOFTBOUND=-L../lib -lSoftbound -Wl,-rpath,../lib 

tests := test_arbitW.exe test_runtime.exe test_strncpy.exe test_memset.exe
tests += test_forloop.exe test_malloc.exe test_calloc.exe 
tests += test_gruntime.exe test_gforloop.exe
tests += test_countup.exe

.PHONY: all use-clang


all: $(tests)
%.exe: %.bc 
	$(CC) -o $@ ${LIBSOFTBOUND} $<
%.bc: %.ll 
	$(OPT) -passes=softbound -o $@ $<
%.ll: %.c
	$(CC) -O1 ${EMIT_LLVM_FLAG} -o $@ $<
use-clang: test_forloop.c
	$(CC) -O1 ${LIBSOFTBOUND} -o clang_forloop.exe $<
clean: 
	rm $(wildcard *.exe) $(wildcard *.ll) $(wildcard *.bc)
