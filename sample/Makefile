OBJ_ROOT=../llvm-project-13.0.0.src/build

CC=${OBJ_ROOT}/bin/clang
CXX=${OBJ_ROOT}/bin/clang++
OPT=${OBJ_ROOT}/bin/opt
LLC=${OBJ_ROOT}/bin/llc

EMIT_LLVM_FLAG=-m64 -O2 -S -emit-llvm
LIBSOFTBOUND=-L../lib -lSoftbound -Wl,-rpath,../lib 

tests := test_arbitW.exe test_runtime.exe test_strncpy.exe
tests += test_forloop.exe

.PHONY: all


all: $(tests)
%.exe: %.bc 
	$(CC) -o $@ ${LIBSOFTBOUND} $<
%.bc: %.ll 
	$(OPT) -passes=softbound -o $@ $<
%.ll: %.c
	$(CC) ${EMIT_LLVM_FLAG} -o $@ $<
clean: 
	rm $(wildcard *.exe)
