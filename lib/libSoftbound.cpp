#include "libSoftbound.h" 

void updateFatPointer(unsigned ptr_id, uint64_t base, uint64_t bound) {
    LookupTable[ptr_id].ptr_base  = base ;
    LookupTable[ptr_id].ptr_bound = bound;
    printf("base: 0x%08lx, bound: 0x%08lx\n", base, bound) ;
}

