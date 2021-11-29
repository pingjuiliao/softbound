#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int
main(int argc, char** argv) {
    if ( argc < 2 ) {
        fprintf(stderr, "%s <size>\n", argv[0]) ;
        exit(-1) ;
    }
    char buf[10] ;
    size_t size = 0; 
    size = atoi(argv[1]) ;
    
    printf("fgets..ing  : size == %zu\n", size) ;
    fgets(buf, size++, stdin) ;
    printf("read..ing   : size == %zu\n", size) ;
    read(0, buf, size++) ;
    printf("write..ing  : size == %zu\n", size) ;
    write(1, buf, size++) ;
    printf("\nmemmove..ing  : size == %zu\n", size) ;
    memmove(buf, "qwkfopwkopkpofkpowkqpf", size) ;
   
    return 0 ;

}
