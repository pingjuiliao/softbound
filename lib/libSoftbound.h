#ifndef LIBSOFTBOUND_H
#define LIBSOFTBOUND_H
#ifdef __cplusplus 
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const uint64_t NUM_PTR_MAX = 0x1000 ;

typedef struct FatPointer {
    uint64_t ptr ;
    uint8_t* ptr_base ;
    uint8_t* ptr_bound;
} FatPointer;

FatPointer LookupTable[NUM_PTR_MAX] ;


void _softbound_register(unsigned ptr_id, uint8_t* base, uint8_t* bound) ; 
void _softbound_check(unsigned ptr_id, uint8_t* ptr) ;
void _softbound_update(unsigned dst_ptr_id, unsigned src_ptr_id) ;
void _softbound_abort(void) ;


#ifdef __cplusplus
}
#endif 



#endif /* LIBSOFTBOUND_H */
