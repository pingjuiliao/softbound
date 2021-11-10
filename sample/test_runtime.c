#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_runtime(char* s) ;
char gbuf[20] ;

int
main(int argc, char** argv) { 
    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s <Integer>\n", argv[0]) ;
        exit(-1) ;
    }
    test_runtime(argv[1]) ;
    return 0;
}

void
test_runtime(char* s) {
    int i = atoi(s) ;
    int small[] = {1,2,3} ;
    int medium[]= {1,2,3,4,5,6} ;
    int large[] = {1,2,3,4,5,6,7,8,9,10,11} ;

    int* buf ;
    switch(i % 3 ) {
        case 0 :
            buf = small ;
            break ;
        case 1 :
            buf = medium ;
            break ;
        case 2 :
            buf = large ; 
            break ;
    }

    buf[8] = 1337 ;
    puts("Printing out large buffer") ;
    for ( int i = 0; i < 11; ++i ) { // this will be translated to llvm.memcpy...
        printf("%d\n", large[i]) ; 
    }

}


