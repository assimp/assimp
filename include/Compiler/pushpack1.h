
// may be included multiple times - sets structure packing to 1 
// for all supported compilers. A poppack1.h include must follow
// each inclusion of this header.


/*

PACK_STRUCT must follow each structure declaration:

struct X
{
} PACK_STRUCT;

 */

//

#ifdef AI_PUSHPACK_IS_DEFINED
#	error poppack1.h must be included after pushpack1.h
#endif

#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
#	pragma pack(push,1)
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error Compiler not supported
#endif

#if defined(_MSC_VER)
// packing was changed after the inclusion of the header, propably missing #pragma pop
#	pragma warning (disable : 4103) 
#endif


#define AI_PUSHPACK_IS_DEFINED


