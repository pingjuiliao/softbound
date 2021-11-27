CC=clang
CXX=clang++
LOAD_LIB=-L.. -lSoftbound -Wl,-rpath,..
INCLUDE=-I..

all: ctest cxxtest
ctest: ctestLib.c
	$(CC) -o $@ ${INCLUDE} ${LOAD_LIB} $<
cxxtest: cxxtestLib.cpp
	$(CXX) -o $@ ${INCLUDE} ${LOAD_LIB} $<
clean:
	rm ctest cxxtest
