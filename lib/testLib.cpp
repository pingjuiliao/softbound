#include <stdio.h>
#include <string.h>
#include "libSoftbound.h"
#include <iostream>
using namespace std; 
void
bufovfl(void) {
    char buf[10 ] ;
    updateFatPointer(0, (uint64_t) &buf, (uint64_t) &buf[10]) ;
    strcpy(buf, "Hello world ! hahahahhahahahahahhaha") ;
    cout << buf  << "\n";
}
int
main(int argc, char** argv) {
    bufovfl();
    return 0;
}
