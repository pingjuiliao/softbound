CC=clang
CXX=clang++
all: ctestLib testLib
ctestLib: ctestLib.c libSoftbound
	$(CC) -o ctestLib.exe -L. -lSoftbound -Wl,-rpath,. ctestLib.c
testLib: testLib.cpp libSoftbound
	$(CXX) -o testLib.exe -L. -lSoftbound -Wl,-rpath,. testLib.cpp
libSoftbound: libSoftbound.cpp
	$(CXX) -shared -o ./libSoftbound.so libSoftbound.cpp 
clean:
	rm libSoftbound.so testLib.exe ctestLib.exe
