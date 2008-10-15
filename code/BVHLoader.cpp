/** Implementation of the BVH loader */
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

#include "AssimpPCH.h"
#include "BVHLoader.h"
#include "fast_atof.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
BVHLoader::BVHLoader()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
BVHLoader::~BVHLoader()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool BVHLoader::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// check file extension 
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);
	for( std::string::iterator it = extension.begin(); it != extension.end(); ++it)
		*it = tolower( *it);

	if( extension == ".bvh")
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void BVHLoader::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	mFileName = pFile;

	// read file into memory
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open file " + pFile + ".");

	size_t fileSize = file->FileSize();
	if( fileSize == 0)
		throw new ImportErrorException( "File is too small.");

	mBuffer.resize( fileSize);
	file->Read( &mBuffer.front(), 1, fileSize);

	// start reading
	mReader = mBuffer.begin();
	mLine = 1;
	ReadStructure( pScene);
}

// ------------------------------------------------------------------------------------------------
// Reads the file
void BVHLoader::ReadStructure( aiScene* pScene)
{
	// first comes hierarchy
	std::string header = GetNextToken();
	if( header != "HIERARCHY")
		ThrowException( "Expected header string \"HIERARCHY\".");
	ReadHierarchy( pScene);

	// then comes the motion data
	std::string motion = GetNextToken();
	if( motion != "MOTION")
		ThrowException( "Expected beginning of motion data \"MOTION\".");
	ReadMotion( pScene);
}

// ------------------------------------------------------------------------------------------------
// Reads the hierarchy
void BVHLoader::ReadHierarchy( aiScene* pScene)
{
	std::string root = GetNextToken();
	if( root != "ROOT")
		ThrowException( "Expected root node \"ROOT\".");

	// Go read the hierarchy from here
	pScene->mRootNode = ReadNode();
}

// ------------------------------------------------------------------------------------------------
// Reads a node and recursively its childs and returns the created node;
aiNode* BVHLoader::ReadNode()
{
	// first token is name
	std::string nodeName = GetNextToken();
	if( nodeName.empty() || nodeName == "{")
		ThrowException( boost::str( boost::format( "Expected node name, but found \"%s\".") % nodeName));

	// HACK: (thom) end nodes are called "End Site". If the name of the node is "Site", we know it's going to be an end node
	if( nodeName == "Site")
		nodeName = "End Site";

	// then an opening brace should follow
	std::string openBrace = GetNextToken();
	if( openBrace != "{")
		ThrowException( boost::str( boost::format( "Expected opening brace \"{\", but found \"%s\".") % openBrace));

	// Create a node
	aiNode* node = new aiNode( nodeName);
	std::vector<aiNode*> childNodes;

	// now read the node's contents
	while( 1)
	{
		std::string token = GetNextToken();
		
		// node offset to parent node
		if( token == "OFFSET")
			ReadNodeOffset( node);
		else if( token == "CHANNELS")
			ReadNodeChannels( node);
		else if( token == "JOINT")
		{
			// child node follows
			aiNode* child = ReadNode();
			childNodes.push_back( child);
		} else 
		if( token == "End")
		{
			// HACK: (thom) end child node follows. Full token is "End Site", then no name, then a node.
			// But I don't want to write another function for this, so I simply leave the "Site" for ReadNode() as a node name
			aiNode* child = ReadNode();
			childNodes.push_back( child);
		} else
		if( token == "}")
		{
			// we're done with that part of the hierarchy
			break;
		} else
		{
			// everything else is a parse error
			ThrowException( boost::str( boost::format( "Unknown keyword \"%s\".") % token));
		}
	}

	// add the child nodes if there are any
	if( childNodes.size() > 0)
	{
		node->mNumChildren = childNodes.size();
		node->mChildren = new aiNode*[node->mNumChildren];
		std::copy( childNodes.begin(), childNodes.end(), node->mChildren);
	}

	// and return the sub-hierarchy we built here
	return node;
}

// ------------------------------------------------------------------------------------------------
// Reads a node offset for the given node
void BVHLoader::ReadNodeOffset( aiNode* pNode)
{
	// Offset consists of three floats to read
	aiVector3D offset;
	offset.x = GetNextTokenAsFloat();
	offset.y = GetNextTokenAsFloat();
	offset.z = GetNextTokenAsFloat();

	// build a transformation matrix from it
	pNode->mTransformation = aiMatrix4x4( 1.0f, 0.0f, 0.0f, offset.x, 0.0f, 1.0f, 0.0f, offset.y,
		0.0f, 0.0f, 1.0f, offset.z, 0.0f, 0.0f, 0.0f, 1.0f);
}

// ------------------------------------------------------------------------------------------------
// Reads the animation channels for the given node
void BVHLoader::ReadNodeChannels( aiNode* pNode)
{
	// number of channels. Use the float reader because we're lazy
	float numChannelsFloat = GetNextTokenAsFloat();
	unsigned int numChannels = (unsigned int) numChannelsFloat;

	// TODO: (thom) proper channel parsing. For the moment I just skip the number of tokens
	for( unsigned int a = 0; a < numChannels; a++)
		GetNextToken();
}

// ------------------------------------------------------------------------------------------------
// Reads the motion data
void BVHLoader::ReadMotion( aiScene* pScene)
{
	// Read number of frames
	std::string tokenFrames = GetNextToken();
	if( tokenFrames != "Frames:")
		ThrowException( boost::str( boost::format( "Expected frame count \"Frames:\", but found \"%s\".") % tokenFrames));

	float numFramesFloat = GetNextTokenAsFloat();
	unsigned int numFrames = (unsigned int) numFramesFloat;

	// Read frame duration
	std::string tokenDuration1 = GetNextToken();
	std::string tokenDuration2 = GetNextToken();
	if( tokenDuration1 != "Frame" || tokenDuration2 != "Time:")
		ThrowException( boost::str( boost::format( "Expected frame duration \"Frame Time:\", but found \"%s %s\".") % tokenDuration1 % tokenDuration2));

	float frameDuration = GetNextTokenAsFloat();

	// resize value array accordingly
	// ************* Continue here ********
	//mMotionValues.resize( boost::extents[numFrames][numChannels]);
}

// ------------------------------------------------------------------------------------------------
// Retrieves the next token
std::string BVHLoader::GetNextToken()
{
	// skip any preceeding whitespace
	while( mReader != mBuffer.end())
	{
		if( !isspace( *mReader))
			break;

		// count lines
		if( *mReader == '\n')
			mLine++;

		++mReader;
	}

	// collect all chars till the next whitespace. BVH is easy in respect to that.
	std::string token;
	while( mReader != mBuffer.end())
	{
		if( isspace( *mReader))
			break;

		token.push_back( *mReader);
		++mReader;

		// little extra logic to make sure braces are counted correctly
		if( token == "{" || token == "}")
			break;
	}

	// empty token means end of file, which is just fine
	return token;
}

// ------------------------------------------------------------------------------------------------
// Reads the next token as a float
float BVHLoader::GetNextTokenAsFloat()
{
	std::string token = GetNextToken();
	if( token.empty())
		ThrowException( "Unexpected end of file while trying to read a float");

	// check if the float is valid by testing if the atof() function consumed every char of the token
	const char* ctoken = token.c_str();
	float result = 0.0f;
	ctoken = fast_atof_move( ctoken, result);

	if( ctoken != token.c_str() + token.length())
		ThrowException( boost::str( boost::format( "Expected a floating point number, but found \"%s\".") % token));

	return result;
}

// ------------------------------------------------------------------------------------------------
// Aborts the file reading with an exception
void BVHLoader::ThrowException( const std::string& pError)
{
	throw new ImportErrorException( boost::str( boost::format( "%s:%d - %s") % mFileName % mLine % pError));
}
