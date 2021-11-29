#include "libSoftbound.h" 


void _softbound_register(u8* base, unsigned long long size) {
    // re-register is allowed
    u8* bound = base + size ;
    for ( u8* p = base ; p != bound ; ++p ) {
        shadow[p].base = base ;
        shadow[p].bound= base+size-1;
    }
}

void _softbound_check(u8* pAccess, u8* pBased) {
   
    // 
    if ( shadow.find(pBased) == shadow.end() )
        return ;


    if ( shadow[pBased].base <= pAccess && 
            pAccess <= shadow[pBased].bound ) {
        std::cout << "[ " << static_cast<void*>(shadow[pBased].base) \
                  << ", " << static_cast<void*>(shadow[pBased].bound)\
                  << " ] : " << static_cast<void*>(pAccess) << std::endl;
        return ;
    }

    std::cout << "[ " << static_cast<void*>(shadow[pBased].base) \
              << ", " << static_cast<void*>(shadow[pBased].bound)\
              << " ] : BUT " << static_cast<void*>(pAccess) << std::endl ;
    shadow.clear(); // otherwise, double-free error
    exit(-1) ;
}

void _softbound_check_offset(u8* pAccess, size_t offset, u8* pBased ) {
    pAccess = std::max(pAccess, pAccess + offset - 1) ;
    _softbound_check(pAccess, pBased);
}


void _softbound_check_string(u8* pAccess, char* s0, char* s1, int size, u8* pBased) {
    
    pAccess += strlen(s0) ;
    size_t final_size = 0;
    if ( s1 )
        final_size =  strlen(s1) + 1 ;
    if ( size >= 0 ) 
        final_size = std::min(final_size, (size_t) size);

    _softbound_check_offset(pAccess, final_size, pBased) ;
}
