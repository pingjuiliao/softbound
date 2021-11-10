#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_forloop(char *s) ;
char gbuf[20] ;

int
main(int argc, char** argv) { 
    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s <bufovfl>\n", argv[0]) ;
        exit(-1) ;
    }
    test_forloop(argv[1]) ;
    return 0;
}

void
test_forloop(char* s) {
    size_t i ;
    char buf[20];
    for ( i = 0; s[i] != '\0' ; ++i ) {
        buf[i] = s[i] ;
    }
    puts(buf) ;
}

