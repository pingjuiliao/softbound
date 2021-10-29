#include "libSoftbound.h" 

void updateFatPointer(unsigned ptr_id, uint8_t* base, uint8_t* bound) {
    LookupTable[ptr_id].ptr_base  = base ;
    LookupTable[ptr_id].ptr_bound = bound;
    printf("base: %p, bound: %p, size: %lu\n", base, bound, (uint64_t)bound - (uint64_t)base ) ;
}

