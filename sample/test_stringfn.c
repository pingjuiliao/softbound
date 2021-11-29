#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int
main(int argc, char** argv) {
    if ( argc < 2 ) {
        fprintf(stderr, "%s <bufovfl>\n", argv[0]) ;
        exit(-1) ;
    }
    char buf[10] ;
    int r = 0;
    // strlen(argv[1]) < 10
    strcpy(buf, argv[1]) ;
    r = puts(buf) ;
    if ( r < 0 ) exit(-1); 

    memset(buf, 0, sizeof(buf)) ;
    strncat(buf, argv[1], 9) ;
    strncat(buf, argv[1], 0) ;
    r = puts(buf) ;
    if ( r < 0  ) exit(-1) ;
    memset(buf, 0, sizeof(buf)) ;
    strcat(buf, argv[1]);
    strcat(buf, argv[1]);


    return 0 ;

}
