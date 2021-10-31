#include "libSoftbound.h" 

void _softbound_update(unsigned ptr_id, uint8_t* base, uint8_t* bound) {
    LookupTable[ptr_id].ptr_base  = base ;
    LookupTable[ptr_id].ptr_bound = bound;
    printf("base: %p, bound: %p, size: %lu\n", base, bound, (uint64_t)bound - (uint64_t)base ) ;
}

void _softbound_check(unsigned ptr_id, uint8_t* ptr) {
    uint8_t* base = LookupTable[ptr_id].ptr_base ;
    uint8_t* bound= LookupTable[ptr_id].ptr_bound;
    if ( base < ptr && ptr < bound ) 
        return ;
    _softbound_abort() ;
}
void _softbound_propagate(unsigned dst_ptr_id, unsigned src_ptr_id) {
    LookupTable[dst_ptr_id].ptr_base  = LookupTable[src_ptr_id].ptr_base  ;
    LookupTable[dst_ptr_id].ptr_bound = LookupTable[src_ptr_id].ptr_bound ;

}

void _softbound_abort(void) {
    puts("pointer exceed base or bound") ;
    exit(-1) ;
}
