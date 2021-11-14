#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_runtime(char* s) ;
int* gptr ;

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
    int small[3] = {1,2,3} ;
    int medium[6]= {1,2,3,4,5,6} ;
    int large[11] = {1,2,3,4,5,6,7,8,9,10,11} ;

    switch(i % 3 ) {
        case 0 :
            gptr = small ;
            break ;
        case 1 :
            gptr = medium ;
            break ;
        case 2 :
            gptr = large ; 
            break ;
    }
    
    // only large buffer is a valid store
    gptr[8] = 1337 ;

}


