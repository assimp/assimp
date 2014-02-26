/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file metadata.h
 *  @brief Defines the data structures for holding node meta information.
 */
#ifndef __AI_METADATA_H_INC__
#define __AI_METADATA_H_INC__

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------------------------
/**
  * Container for holding metadata.
  *
  * Metadata is a key-value store using string keys and values.
  */
 // -------------------------------------------------------------------------------
struct aiMetadata 
{
	/** Length of the mKeys and mValues arrays, respectively */
	unsigned int mNumProperties;

	/** Arrays of keys, may not be NULL. Entries in this array may not be NULL as well. */
	C_STRUCT aiString* mKeys;

	/** Arrays of values, may not be NULL. Entries in this array may be NULL if the
	  * corresponding property key has no assigned value. */
	C_STRUCT aiString* mValues;

#ifdef __cplusplus

	/** Constructor */
	aiMetadata()
	{
		// set all members to zero by default
		mKeys = NULL;
		mValues = NULL;
		mNumProperties = 0;
	}


	/** Destructor */
	~aiMetadata()
	{
		if (mKeys)
			delete [] mKeys;
		if (mValues)
			delete [] mValues;
	}


	inline bool Get(const aiString& key, aiString& value)
	{
		for (unsigned i=0; i<mNumProperties; ++i) {
			if (mKeys[i]==key) {
				value=mValues[i];
				return true;
			}
		}
		return false;
	}
#endif // __cplusplus
};

#ifdef __cplusplus
} //extern "C" {
#endif

#endif // __AI_METADATA_H_INC__


