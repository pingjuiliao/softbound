OBJ_ROOT=./llvm-project-13.0.0.src/build

CC=${OBJ_ROOT}/bin/clang
OPT=${OBJ_ROOT}/bin/opt
LLC=${OBJ_ROOT}/bin/llc

all: a-debug
a: a-dis
	$(OPT) -passes=softbound -o ./a.bc a.ll
	$(CC) -o ./a.exe a.bc 
a-debug: a-dis
	$(OPT) -passes=softbound -disable-output a.ll -debug-pass-manager
a-dis: sample/a.c
	$(CC) -m64 -O2 -S -emit-llvm -o ./a.ll sample/a.c
clean:
	rm *.bc *.ll *.exe 
