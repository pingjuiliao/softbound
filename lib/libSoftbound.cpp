#include "libSoftbound.h" 

void _softbound_register(unsigned ptr_id, uint8_t* base, uint8_t* bound) {
    if ( base != NULL && bound != NULL  ) {
        LookupTable[ptr_id].ptr_base  = base ;
        LookupTable[ptr_id].ptr_bound = bound;
    }
}

void _softbound_check(unsigned ptr_id, uint8_t* ptr) {
    uint8_t* base = LookupTable[ptr_id].ptr_base ;
    uint8_t* bound= LookupTable[ptr_id].ptr_bound;
    if ( base <= ptr && ptr < bound ) {  
        printf("[VALID] [%p, %p), and %p is inside\n", base, bound, ptr) ;
        return ;
    }
    printf("[ABORT] [%p, %p), but ptr: %p\n", base, bound, ptr) ;
    _softbound_abort() ;
}
void _softbound_update(unsigned dst_ptr_id, unsigned src_ptr_id) {
    LookupTable[dst_ptr_id].ptr_base  = LookupTable[src_ptr_id].ptr_base  ;
    LookupTable[dst_ptr_id].ptr_bound = LookupTable[src_ptr_id].ptr_bound ;
    puts("Updating pointer") ;
}

void _softbound_abort(void) {
    puts("pointer exceed base or bound") ;
    exit(-1) ;
}
