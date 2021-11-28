#include "libSoftbound.h" 


void _softbound_register(u8* base, unsigned long long size) {
    // re-register is allowed
    std::cout << "Register Key : " << static_cast<void*>(base) << "\n" ;
    u8* bound = base + size ;
    for ( u8* p = base ; p != bound ; ++p ) {
        shadow[p].base = base ;
        shadow[p].bound= base+size-1;
        std::cout << static_cast<void*>(p) << " is registered with [" \
                     << static_cast<void*>(base) << ", "  \
                     << static_cast<void*>(bound) << ") ! \n" ;
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
