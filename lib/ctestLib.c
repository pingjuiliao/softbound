#include <stdio.h>
#include <string.h>
#include "libSoftbound.h"

void
bufovfl(void) {
    char buf[100] ;
    _softbound_update(0, (uint8_t *) &buf, (uint8_t *) &buf[10]) ;
    strcpy(buf, "Hello world ! hahahahhahahahahahhaha") ;
    puts(buf) ;
}
int
main(int argc, char** argv) {
    bufovfl();
    return 0;
}
