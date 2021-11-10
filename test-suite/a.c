#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_arbitW(char* s) ;
void test_strncpy(char* s) ;
void test_forloop(char *s) ;
void test_runtime(char* s) ;
char gbuf[20] ;

int
main(int argc, char** argv) { 
    if ( argc < 2 ) {
        fprintf(stderr, "Usage: %s <bufovfl> <bufovfl> <Integer>\n", argv[0]) ;
        exit(-1) ;
    }
    test_strncpy(argv[1]) ;
    test_forloop(argv[1]) ;
    test_arbitW(argv[1]) ;
    return 0;
}


void
test_strncpy(char* s) {
    char buf[20] ;
    strncpy(buf, s, 128);
    puts(buf) ;
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
    for ( int i = 0; i < 11; ++i ) {
        printf("%d\n", large[i]) ; 
    }

}


