/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

/** @file Implementation of the MD5 parser class */

#include "AssimpPCH.h"

// internal headers
#include "MD5Loader.h"
#include "MaterialSystem.h"
#include "fast_atof.h"
#include "ParsingUtils.h"
#include "StringComparison.h"


using namespace Assimp;
using namespace Assimp::MD5;

#if _MSC_VER >= 1400
#	define sprintf sprintf_s
#endif

// ------------------------------------------------------------------------------------------------
MD5Parser::MD5Parser(char* buffer, unsigned int fileSize)
{
	ai_assert(NULL != buffer && 0 != fileSize);

	this->buffer = buffer;
	this->fileSize = fileSize;
	this->lineNumber = 0;

	DefaultLogger::get()->debug("MD5Parser begin");

	// parse the file header
	this->ParseHeader();

	// and read all sections until we're finished
	while (true)
	{
		this->mSections.push_back(Section());
		Section& sec = this->mSections.back();
		if(!this->ParseSection(sec))
		{
			break;
		}
	}

	if ( !DefaultLogger::isNullLogger())
	{
		char szBuffer[128]; // should be sufficiently large
		::sprintf(szBuffer,"MD5Parser end. Parsed %i sections",(int)this->mSections.size());
		DefaultLogger::get()->debug(szBuffer);
	}
}
// ------------------------------------------------------------------------------------------------
/*static*/ void MD5Parser::ReportError (const char* error, unsigned int line)
{
	char szBuffer[1024]; // you, listen to me, you HAVE TO BE sufficiently large
	::sprintf(szBuffer,"Line %i: %s",line,error);
	throw new ImportErrorException(szBuffer);
}
// ------------------------------------------------------------------------------------------------
/*static*/ void MD5Parser::ReportWarning (const char* warn, unsigned int line)
{
	char szBuffer[1024]; // you, listen to me, you HAVE TO BE sufficiently large
	::sprintf(szBuffer,"Line %i: %s",line,warn);
	DefaultLogger::get()->warn(szBuffer);
}
// ------------------------------------------------------------------------------------------------
void MD5Parser::ParseHeader()
{
	// parse and validate the file version
	SkipSpaces();
	if (0 != ASSIMP_strincmp(buffer,"MD5Version",10) ||
		!IsSpace(*(buffer+=10)))
	{
		this->ReportError("Invalid MD5 file: MD5Version tag has not been found");
	}
	SkipSpaces();
	unsigned int iVer = ::strtol10(buffer,(const char**)&buffer);
	if (10 != iVer)
	{
		this->ReportWarning("MD5 version tag is unknown (10 is expected)");
	}
	this->SkipLine();

	// print the command line options to the console
	char* sz = buffer;
	while (!IsLineEnd( *buffer++));
	DefaultLogger::get()->info(std::string(sz,(uintptr_t)(buffer-sz)));
	this->SkipSpacesAndLineEnd();
}
// ------------------------------------------------------------------------------------------------
bool MD5Parser::ParseSection(Section& out)
{
	// store the current line number for use in error messages
	out.iLineNumber = this->lineNumber;

	// first parse the name of the section
	char* sz = buffer;
	while (!IsSpaceOrNewLine( *buffer))buffer++;
	out.mName = std::string(sz,(uintptr_t)(buffer-sz));
	SkipSpaces();

	while (true)
	{
		if ('{' == *buffer)
		{
			// it is a normal section so read all lines
			buffer++;
			while (true)
			{
				if (!SkipSpacesAndLineEnd())
				{
					return false; // seems this was the last section
				}
				if ('}' == *buffer)
				{
					buffer++;
					break;
				}

				out.mElements.push_back(Element());
				Element& elem = out.mElements.back();

				elem.iLineNumber = lineNumber;
				elem.szStart = buffer;

				// terminate the line with zero - remove all spaces at the end
				while (!IsLineEnd( *buffer))buffer++;
				//const char* end = buffer;
				do {buffer--;}
				while (IsSpace(*buffer));
				buffer++;
				*buffer++ = '\0';
				//if (*end) ++lineNumber;
			}
			break;
		}
		else if (!IsSpaceOrNewLine(*buffer))
		{
			// it is an element at global scope. Parse its value and go on
			// FIX: for MD5ANIm files - frame 0 {...} is allowed
			sz = buffer;
			while (!IsSpaceOrNewLine( *buffer++));
			out.mGlobalValue = std::string(sz,(uintptr_t)(buffer-sz));
			continue;
		}
		break;
	}
	return SkipSpacesAndLineEnd();
}
// ------------------------------------------------------------------------------------------------

// skip all spaces ... handle EOL correctly
#define	AI_MD5_SKIP_SPACES()  if(!SkipSpaces(&sz)) \
	MD5Parser::ReportWarning("Unexpected end of line",(*eit).iLineNumber);

	// read a triple float in brackets: (1.0 1.0 1.0)
#define AI_MD5_READ_TRIPLE(vec) \
	AI_MD5_SKIP_SPACES(); \
	if ('(' != *sz++) \
		MD5Parser::ReportWarning("Unexpected token: ( was expected",(*eit).iLineNumber); \
	AI_MD5_SKIP_SPACES(); \
	sz = fast_atof_move(sz,(float&)vec.x); \
	AI_MD5_SKIP_SPACES(); \
	sz = fast_atof_move(sz,(float&)vec.y); \
	AI_MD5_SKIP_SPACES(); \
	sz = fast_atof_move(sz,(float&)vec.z); \
	AI_MD5_SKIP_SPACES(); \
	if (')' != *sz++) \
		MD5Parser::ReportWarning("Unexpected token: ) was expected",(*eit).iLineNumber);

	// parse a string, enclosed in quotation marks or not
#define AI_MD5_PARSE_STRING(out) \
	bool bQuota = *sz == '\"'; \
	const char* szStart = sz; \
	while (!IsSpaceOrNewLine(*sz))++sz; \
	const char* szEnd = sz; \
	if (bQuota) \
	{ \
		szStart++; \
		if ('\"' != *(szEnd-=1)) \
		{ \
			MD5Parser::ReportWarning("Expected closing quotation marks in string", \
				(*eit).iLineNumber); \
		} \
	} \
	out.length = (size_t)(szEnd - szStart); \
	::memcpy(out.data,szStart,out.length); \
	out.data[out.length] = '\0';

// ------------------------------------------------------------------------------------------------
MD5MeshParser::MD5MeshParser(SectionList& mSections)
{
	DefaultLogger::get()->debug("MD5MeshParser begin");

	// now parse all sections
	for (SectionList::const_iterator
		iter =  mSections.begin(), iterEnd = mSections.end();
		iter != iterEnd;++iter)
	{
		if ((*iter).mGlobalValue.length())
		{
			if ( !::strcmp("numMeshes",(*iter).mName.c_str()))
			{
				unsigned int iNumMeshes;
				if((iNumMeshes = ::strtol10((*iter).mGlobalValue.c_str())))
				{
					mMeshes.reserve(iNumMeshes);
				}
			}
			else if ( !::strcmp("numJoints",(*iter).mName.c_str()))
			{
				unsigned int iNumJoints;
				if((iNumJoints = ::strtol10((*iter).mGlobalValue.c_str())))
				{
					mJoints.reserve(iNumJoints);
				}
			}
		}
		else if (!::strcmp("joints",(*iter).mName.c_str()))
		{
			// now read all elements
			// "origin"	-1 ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000000 0.707107 )
			for (ElementList::const_iterator
				eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();
				eit != eitEnd; ++eit)
			{
				mJoints.push_back(BoneDesc());
				BoneDesc& desc = mJoints.back();

				const char* sz = (*eit).szStart;

				AI_MD5_PARSE_STRING(desc.mName);
				AI_MD5_SKIP_SPACES();

				// negative values can occur here ...
				bool bNeg = false;
				if ('-' == *sz){sz++;bNeg = true;}
				else if ('+' == *sz){sz++;}
				desc.mParentIndex = (int)::strtol10(sz,&sz);
				if (bNeg)desc.mParentIndex *= -1;

				AI_MD5_READ_TRIPLE(desc.mPositionXYZ);
				AI_MD5_READ_TRIPLE(desc.mRotationQuat); // normalized quaternion, so w is not there
			}
		}
		else if (!::strcmp("mesh",(*iter).mName.c_str()))
		{
			mMeshes.push_back(MeshDesc());
			MeshDesc& desc = mMeshes.back();

			// now read all elements
			for (ElementList::const_iterator
				eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();
				eit != eitEnd; ++eit)
			{
				const char* sz = (*eit).szStart;

				// shader attribute
				if (!ASSIMP_strincmp(sz,"shader",6) &&
					IsSpaceOrNewLine(*(sz+=6)++))
				{
					// don't expect quotation marks
					AI_MD5_SKIP_SPACES();
					AI_MD5_PARSE_STRING(desc.mShader);
				}
				// numverts attribute
				else if (!ASSIMP_strincmp(sz,"numverts",8) &&
					IsSpaceOrNewLine(*(sz+=8)++))
				{
					// reserve enough storage
					AI_MD5_SKIP_SPACES();
					unsigned int iNumVertices;
					if((iNumVertices = ::strtol10(sz)))
						desc.mVertices.resize(iNumVertices);
				}
				// numtris attribute
				else if (!ASSIMP_strincmp(sz,"numtris",7) &&
					IsSpaceOrNewLine(*(sz+=7)++))
				{
					// reserve enough storage
					AI_MD5_SKIP_SPACES();
					unsigned int iNumTris;
					if((iNumTris = ::strtol10(sz)))
						desc.mFaces.resize(iNumTris);
				}
				// numweights attribute
				else if (!ASSIMP_strincmp(sz,"numweights",10) &&
					IsSpaceOrNewLine(*(sz+=10)++))
				{
					// reserve enough storage
					AI_MD5_SKIP_SPACES();
					unsigned int iNumWeights;
					if((iNumWeights = ::strtol10(sz)))
						desc.mWeights.resize(iNumWeights);
				}
				// vert attribute
				// "vert 0 ( 0.394531 0.513672 ) 0 1"
				else if (!ASSIMP_strincmp(sz,"vert",4) &&
					IsSpaceOrNewLine(*(sz+=4)++))
				{
					AI_MD5_SKIP_SPACES();
					unsigned int idx = ::strtol10(sz,&sz);
					AI_MD5_SKIP_SPACES();
					if (idx >= desc.mVertices.size())
						desc.mVertices.resize(idx+1);

					VertexDesc& vert = desc.mVertices[idx];	
					if ('(' != *sz++)
						MD5Parser::ReportWarning("Unexpected token: ( was expected",(*eit).iLineNumber);
					AI_MD5_SKIP_SPACES();
					sz = fast_atof_move(sz,(float&)vert.mUV.x);
					AI_MD5_SKIP_SPACES();
					sz = fast_atof_move(sz,(float&)vert.mUV.y);
					AI_MD5_SKIP_SPACES();
					if (')' != *sz++)
						MD5Parser::ReportWarning("Unexpected token: ) was expected",(*eit).iLineNumber);
					AI_MD5_SKIP_SPACES();
					vert.mFirstWeight = ::strtol10(sz,&sz);
					AI_MD5_SKIP_SPACES();
					vert.mNumWeights = ::strtol10(sz,&sz);
				}
				// tri attribute
				// "tri 0 15 13 12"
				else if (!ASSIMP_strincmp(sz,"tri",3) &&
					IsSpaceOrNewLine(*(sz+=3)++))
				{
					AI_MD5_SKIP_SPACES();
					unsigned int idx = ::strtol10(sz,&sz);
					if (idx >= desc.mFaces.size())
						desc.mFaces.resize(idx+1);

					aiFace& face = desc.mFaces[idx];	
					face.mIndices = new unsigned int[face.mNumIndices = 3];
					for (unsigned int i = 0; i < 3;++i)
					{
						AI_MD5_SKIP_SPACES();
						face.mIndices[i] = ::strtol10(sz,&sz);
					}
				}
				// weight attribute
				// "weight 362 5 0.500000 ( -3.553583 11.893474 9.719339 )"
				else if (!ASSIMP_strincmp(sz,"weight",6) &&
					IsSpaceOrNewLine(*(sz+=6)++))
				{
					AI_MD5_SKIP_SPACES();
					unsigned int idx = ::strtol10(sz,&sz);
					AI_MD5_SKIP_SPACES();
					if (idx >= desc.mWeights.size())
						desc.mWeights.resize(idx+1);

					WeightDesc& weight = desc.mWeights[idx];	
					weight.mBone = ::strtol10(sz,&sz);
					AI_MD5_SKIP_SPACES();
					sz = fast_atof_move(sz,weight.mWeight);
					AI_MD5_READ_TRIPLE(weight.vOffsetPosition);
				}
			}
		}
	}
	DefaultLogger::get()->debug("MD5MeshParser end");
}

// ------------------------------------------------------------------------------------------------
MD5AnimParser::MD5AnimParser(SectionList& mSections)
{
	DefaultLogger::get()->debug("MD5AnimParser begin");

	fFrameRate = 24.0f;
	mNumAnimatedComponents = 0xffffffff;

	// now parse all sections
	for (SectionList::const_iterator
		iter =  mSections.begin(), iterEnd = mSections.end();
		iter != iterEnd;++iter)
	{
		if (!::strcmp("hierarchy",(*iter).mName.c_str()))
		{
			// now read all elements
			// "sheath"	0 63 6 
			for (ElementList::const_iterator
				eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();
				eit != eitEnd; ++eit)
			{
				mAnimatedBones.push_back ( AnimBoneDesc () );
				AnimBoneDesc& desc = mAnimatedBones.back();

				const char* sz = (*eit).szStart;
				AI_MD5_PARSE_STRING(desc.mName);
				AI_MD5_SKIP_SPACES();

				// parent index
				// negative values can occur here ...
				bool bNeg = false;
				if ('-' == *sz){sz++;bNeg = true;}
				else if ('+' == *sz){sz++;}
				desc.mParentIndex = (int)::strtol10(sz,&sz);
				if (bNeg)desc.mParentIndex *= -1;

				// flags (highest is 2^6-1)
				AI_MD5_SKIP_SPACES();
				if(63 < (desc.iFlags = ::strtol10(sz,&sz)))
				{
					MD5Parser::ReportWarning("Invalid flag combination in hierarchy section",
						(*eit).iLineNumber);
				}
				AI_MD5_SKIP_SPACES();

				// index of the first animation keyframe component for this joint
				desc.iFirstKeyIndex = ::strtol10(sz,&sz);
			}
		}
		else if(!::strcmp("baseframe",(*iter).mName.c_str()))
		{
			// now read all elements
			// ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000242 0.707107 )
			for (ElementList::const_iterator
				eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();
				eit != eitEnd; ++eit)
			{
				const char* sz = (*eit).szStart;

				mBaseFrames.push_back ( BaseFrameDesc () );
				BaseFrameDesc& desc = mBaseFrames.back();

				AI_MD5_READ_TRIPLE(desc.vPositionXYZ);
				AI_MD5_READ_TRIPLE(desc.vRotationQuat);
			}
		}
		else if(!::strcmp("frame",(*iter).mName.c_str()))
		{
			if (!(*iter).mGlobalValue.length())
			{
				MD5Parser::ReportWarning("A frame section must have a frame index",
					(*iter).iLineNumber);
				continue;
			}

			mFrames.push_back ( FrameDesc () );
			FrameDesc& desc = mFrames.back();
			desc.iIndex = ::strtol10((*iter).mGlobalValue.c_str());

			// we do already know how much storage we will presumably need
			if (0xffffffff != mNumAnimatedComponents)
				desc.mValues.reserve(mNumAnimatedComponents);

			// now read all elements
			// (continous list of float values)
			for (ElementList::const_iterator
				eit = (*iter).mElements.begin(), eitEnd = (*iter).mElements.end();
				eit != eitEnd; ++eit)
			{
				const char* sz = (*eit).szStart;
				while (SkipSpaces(sz,&sz))
				{
					float f;
					sz = fast_atof_move(sz,f);
					desc.mValues.push_back(f);
				}
			}
		}
		else if(!::strcmp("numFrames",(*iter).mName.c_str()))
		{
			unsigned int iNum;
			if((iNum = ::strtol10((*iter).mGlobalValue.c_str())))
			{
				mFrames.reserve(iNum);
			}
		}
		else if(!::strcmp("numJoints",(*iter).mName.c_str()))
		{
			unsigned int iNum;
			if((iNum = ::strtol10((*iter).mGlobalValue.c_str())))
			{
				mAnimatedBones.reserve(iNum);

				// try to guess the number of animated components if that element is not given
				if (0xffffffff == mNumAnimatedComponents)
					mNumAnimatedComponents = iNum * 6;
			}
		}
		else if(!::strcmp("numAnimatedComponents",(*iter).mName.c_str()))
		{
			unsigned int iNum;
			if((iNum = ::strtol10((*iter).mGlobalValue.c_str())))
			{
				mAnimatedBones.reserve(iNum);
			}
		}
		else if(!::strcmp("frameRate",(*iter).mName.c_str()))
		{
			fast_atof_move((*iter).mGlobalValue.c_str(),this->fFrameRate);
		}
	}
	DefaultLogger::get()->debug("MD5AnimParser end");
}

#undef AI_MD5_SKIP_SPACES
#undef AI_MD5_READ_TRIPLE
#undef AI_MD5_PARSE_STRING
