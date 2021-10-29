#ifndef LIBSOFTBOUND_H
#define LIBSOFTBOUND_H


#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus 
extern "C" {
#endif

const uint64_t NUM_PTR_MAX = 0x1000 ;

typedef struct FatPointer {
    uint64_t ptr ;
    uint8_t* ptr_base ;
    uint8_t* ptr_bound;
} FatPointer;

FatPointer LookupTable[NUM_PTR_MAX] ;

void updateFatPointer(unsigned ptr_id, uint8_t* base, uint8_t* bound) ; 


#ifdef __cplusplus
}
#endif 



#endif /* LIBSOFTBOUND_H */
