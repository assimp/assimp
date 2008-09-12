

// Definitions for the Interchange File Format (IFF)
// Alexander Gessler, 2006
// Adapted for Assimp August 2008

#ifndef AI_IFF_H_INCLUDED
#define AI_IFF_H_INCLUDED

#include "ByteSwap.h"

namespace Assimp {
namespace IFF {

//! Describes an IFF chunk header
struct ChunkHeader
{
	//! Type of the chunk header - FourCC
	uint32_t type;

	//! Length of the chunk data, in bytes
	uint32_t length;
};


#define AI_IFF_FOURCC(a,b,c,d) ((uint32_t) (((uint8_t)a << 24u) | \
	((uint8_t)b << 16u) | ((uint8_t)c << 8u) | ((uint8_t)d)))


#define AI_IFF_FOURCC_FORM AI_IFF_FOURCC('F','O','R','M')


/////////////////////////////////////////////////////////////////////////////////
//! Read the file header and return the type of the file and its size
//! @param outFile Pointer to the file data. The buffer must at 
//!   least be 12 bytes large.
//! @param fileType Receives the type of the file
//! @return 0 if everything was OK, otherwise an error message
/////////////////////////////////////////////////////////////////////////////////
inline const char* ReadHeader(const uint8_t* outFile,uint32_t& fileType) 
{
	LE_NCONST ChunkHeader* head = (LE_NCONST ChunkHeader*) outFile;
	AI_LSWAP4(head->length);
	AI_LSWAP4(head->type);
	if(AI_IFF_FOURCC_FORM != head->type)
	{
		return "The file is not an IFF file: FORM chunk is missing";
	}
	fileType = *((uint32_t*)(head+1));
	AI_LSWAP4(fileType);
	return 0;
}


}}

#endif // !! AI_IFF_H_INCLUDED
