#include <stdio.h>
#include <string.h>
#include "libSoftbound.h"
#include <iostream>
using namespace std; 
void
bufovfl(void) {
    char buf[100] ;
    updateFatPointer(0, (uint8_t *) &buf, (uint8_t *) &buf[10]) ;
    strcpy(buf, "Hello world ! hahahahhahahahahahhaha") ;
    cout << buf  << "\n";
}
void
intarray(void) {
    int list[10] = {12313,2,14,5253,235,6235,25125,23,65765,0} ;
    updateFatPointer(0, (uint8_t *) &list, (uint8_t *) &list[10]) ;
    for ( int i = 0; i < 10; ++i ) {
        printf("%d-", list[i]) ;
    }
    puts("end of array") ;
}

int
main(int argc, char** argv) {
    intarray() ;
    bufovfl();
    return 0;
}
