#include <stdio.h>
void bar(void) ;
void foo(void) ;


int
main(void) {
    bar() ;
    foo() ;
    return 0;
}

void
bar(void) {
    puts("bbbbbbbbbbbbbbbar") ;
}

void
foo(void) {
    puts("fffffffffffffffffoo") ;
}
