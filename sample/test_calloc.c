#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_callocation(int argc, char** argv) ;

int
main(int argc, char** argv) { 
    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s <bufovfl>\n", argv[0]) ;
        exit(-1) ;
    }
    test_callocation(argc, argv) ;
    return 0;
}

void
test_callocation(int argc, char** argv) {
    int i ;
    int* buf;
    buf = (int *) calloc(argc-1, sizeof(int)) ;
    for ( i = 0 ; i < argc-1 ; ++i  ) {
        buf[i] = atoi(argv[i]);
    }
    buf[9] = 1337 ; // argc-1 >= 10
    for ( i = 0 ; i < argc-1 ; ++i  ) {
        printf("%d ", buf[i]) ;
    }
    puts("\nBye");
}
