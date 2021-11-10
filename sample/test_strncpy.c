#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_strncpy(char* s) ;

int
main(int argc, char** argv) { 
    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s <bufovfl>\n", argv[0]) ;
        exit(-1) ;
    }
    test_strncpy(argv[1]) ;
    return 0;
}


void
test_strncpy(char* s) {
    char buf[20] ;
    strncpy(buf, s, 128);
    puts(buf) ;
}

