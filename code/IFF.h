

// Definitions for the Interchange File Format (IFF)
// Alexander Gessler, 2006
// Adapted to Assimp August 2008

#ifndef AI_IFF_H_INCLUDED
#define AI_IFF_H_INCLUDED

#include "ByteSwap.h"

namespace Assimp	{
namespace IFF		{

#include "./../include/assimp/Compiler/pushpack1.h"

/////////////////////////////////////////////////////////////////////////////////
//! Describes an IFF chunk header
/////////////////////////////////////////////////////////////////////////////////
struct ChunkHeader
{
	//! Type of the chunk header - FourCC
	uint32_t type;

	//! Length of the chunk data, in bytes
	uint32_t length;
} PACK_STRUCT;


/////////////////////////////////////////////////////////////////////////////////
//! Describes an IFF sub chunk header
/////////////////////////////////////////////////////////////////////////////////
struct SubChunkHeader
{
	//! Type of the chunk header - FourCC
	uint32_t type;

	//! Length of the chunk data, in bytes
	uint16_t length;
} PACK_STRUCT;

#include "./../include/assimp/Compiler/poppack1.h"


#define AI_IFF_FOURCC(a,b,c,d) ((uint32_t) (((uint8_t)a << 24u) | \
	((uint8_t)b << 16u) | ((uint8_t)c << 8u) | ((uint8_t)d)))


#define AI_IFF_FOURCC_FORM AI_IFF_FOURCC('F','O','R','M')


/////////////////////////////////////////////////////////////////////////////////
//! Load a chunk header
//! @param outFile Pointer to the file data - points to the chunk data afterwards
//! @return Pointer to the chunk header
/////////////////////////////////////////////////////////////////////////////////
inline ChunkHeader* LoadChunk(uint8_t*& outFile)
{
	ChunkHeader* head = (ChunkHeader*) outFile;
	AI_LSWAP4(head->length);
	AI_LSWAP4(head->type);
	outFile += sizeof(ChunkHeader);
	return head;
}

/////////////////////////////////////////////////////////////////////////////////
//! Load a sub chunk header
//! @param outFile Pointer to the file data - points to the chunk data afterwards
//! @return Pointer to the sub chunk header
/////////////////////////////////////////////////////////////////////////////////
inline SubChunkHeader* LoadSubChunk(uint8_t*& outFile)
{
	SubChunkHeader* head = (SubChunkHeader*) outFile;
	AI_LSWAP2(head->length);
	AI_LSWAP4(head->type);
	outFile += sizeof(SubChunkHeader);
	return head;
}

/////////////////////////////////////////////////////////////////////////////////
//! Read the file header and return the type of the file and its size
//! @param outFile Pointer to the file data. The buffer must at 
//!   least be 12 bytes large.
//! @param fileType Receives the type of the file
//! @return 0 if everything was OK, otherwise an error message
/////////////////////////////////////////////////////////////////////////////////
inline const char* ReadHeader(uint8_t* outFile,uint32_t& fileType) 
{
	ChunkHeader* head = LoadChunk(outFile);
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
