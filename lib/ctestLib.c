#include <stdio.h>
#include <string.h>
#include "libSoftbound.h"

void
bufovfl(void) {
    char buf[10 ] ;
    updateFatPointer(0, (uint64_t) &buf, (uint64_t) &buf[10]) ;
    strcpy(buf, "Hello world ! hahahahhahahahahahhaha") ;
    puts(buf) ;
}
int
main(int argc, char** argv) {
    bufovfl();
    return 0;
}
