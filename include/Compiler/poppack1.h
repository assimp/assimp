
// may be included multiple times - resets structure packing to default
// for all supported compilers. A pushpack1.h include must preceed
// each inclusion of this header.

#ifndef AI_PUSHPACK_IS_DEFINED
#	error pushpack1.h must be included after poppack1.h
#endif

// reset packing to the original value
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT

#undef AI_PUSHPACK_IS_DEFINED
