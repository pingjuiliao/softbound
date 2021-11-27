#ifndef LIBSOFTBOUND_H
#define LIBSOFTBOUND_H

#ifdef __cplusplus
    #include <iostream>
    #include <cstdlib>
    #include <map>
#else 
    #include <stdlib.h>
#endif 

typedef unsigned char u8;

typedef struct {
    u8* base ;
    u8* bound;
} FatPointer;

#ifdef __cplusplus
std::map<u8*, FatPointer> shadow;
#endif


#ifdef __cplusplus 
extern "C" {
#endif

void _softbound_register(u8*, unsigned long long) ; 
void _softbound_check(u8*, u8*) ;
// void softbound_register(uint8_t*, uint64_t) ;



#ifdef __cplusplus
}
#endif 



#endif /* LIBSOFTBOUND_H */
