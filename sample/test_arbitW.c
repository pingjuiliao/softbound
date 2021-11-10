#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_arbitW(char* s) ;
char gbuf[20] ;

int
main(int argc, char** argv) { 
    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s <Integer>\n", argv[0]) ;
        exit(-1) ;
    }
    test_arbitW(argv[1]) ;
    return 0;
}


void
test_arbitW(char* s) {
    int arr[] = {1,2,3,4,5} ;
    int *p ;
    int idx = atoi(s) ;
    p = &arr[idx] ;
    *p = 1337 ;
    printf("Array: ");
    for ( int i = 0 ; i < 5; ++i ) {
        printf("%d ", arr[i]);
    }
    puts("\ntest4 done") ;
}


