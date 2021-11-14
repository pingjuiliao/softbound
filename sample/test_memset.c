#include <stdio.h>
#include <string.h>

int
main(void) {
    char buf[20] ;
    memset(buf, 'a', sizeof(buf)) ;
    buf[19] = '\0';
    puts(buf) ;
    return 0; 
}
