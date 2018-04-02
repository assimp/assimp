#include "simd.h"

namespace Assimp {
    bool CPUSupportsSSE2() {
#if defined(__x86_64__) || defined(_M_X64)
        //* x86_64 always has SSE2 instructions */
        return true;
#elif defined(__GNUC__) && defined(i386)
        // for GCC x86 we check cpuid
        unsigned int d;
        __asm__(
            "pushl %%ebx\n\t"
            "cpuid\n\t"
            "popl %%ebx\n\t"
            : "=d" ( d )
            :"a" ( 1 ) );
        return ( d & 0x04000000 ) != 0;
#elif (defined(_MSC_VER) && defined(_M_IX86))
        // also check cpuid for MSVC x86
        unsigned int d;
        __asm {
            xor     eax, eax
            inc eax
            push ebx
            cpuid
            pop ebx
            mov d, edx
        }
        return ( d & 0x04000000 ) != 0;
#else
        return false;
#endif
    }
}
