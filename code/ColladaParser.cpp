/** Implementation of the Collada parser helper*/
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
#include "ColladaParser.h"
#include "fast_atof.h"
#include "ParsingUtils.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ColladaParser::ColladaParser( const std::string& pFile)
	: mFileName( pFile)
{
	mRootNode = NULL;
	mUnitSize = 1.0f;
	mUpDirection = UP_Z;

	// generate a XML reader for it
	mReader = irr::io::createIrrXMLReader( pFile.c_str());
	if( !mReader)
		ThrowException( "Unable to open file.");

	// start reading
	ReadContents();
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ColladaParser::~ColladaParser()
{
	delete mReader;
	for( NodeLibrary::iterator it = mNodeLibrary.begin(); it != mNodeLibrary.end(); ++it)
		delete it->second;
}

// ------------------------------------------------------------------------------------------------
// Reads the contents of the file
void ColladaParser::ReadContents()
{
	while( mReader->read())
	{
		// handle the root element "COLLADA"
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "COLLADA"))
			{
				ReadStructure();
			} else
			{
				DefaultLogger::get()->debug( boost::str( boost::format( "Ignoring global element \"%s\".") % mReader->getNodeName()));
				SkipElement();
			}
		} else
		{
			// skip everything else silently
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads the structure of the file
void ColladaParser::ReadStructure()
{
	while( mReader->read())
	{
		// beginning of elements
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "asset"))
				ReadAssetInfo();
			else if( IsElement( "library_geometries"))
				ReadGeometryLibrary();
			else if( IsElement( "library_visual_scenes"))
				ReadSceneLibrary();
			else if( IsElement( "scene"))
				ReadScene();
			else
				SkipElement();
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads asset informations such as coordinate system informations and legal blah
void ColladaParser::ReadAssetInfo()
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "unit"))
			{
				// read unit data from the element's attributes
				int attrIndex = GetAttribute( "meter");
				mUnitSize = mReader->getAttributeValueAsFloat( attrIndex);

				// consume the trailing stuff
				if( !mReader->isEmptyElement())
					SkipElement();
			} 
			else if( IsElement( "up_axis"))
			{
				// read content, strip whitespace, compare
				const char* content = GetTextContent();
				if( strncmp( content, "X_UP", 4) == 0)
					mUpDirection = UP_X;
				else if( strncmp( content, "Y_UP", 4) == 0)
					mUpDirection = UP_Y;
				else
					mUpDirection = UP_Z;

				// check element end
				TestClosing( "up_axis");
			} else
			{
				SkipElement();
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads the geometry library contents
void ColladaParser::ReadGeometryLibrary()
{
	SkipElement();
}

// ------------------------------------------------------------------------------------------------
// Reads the library of node hierarchies and scene parts
void ColladaParser::ReadSceneLibrary()
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			// a visual scene - generate root node under its ID and let ReadNode() do the recursive work
			if( IsElement( "visual_scene"))
			{
				// read ID. Is optional according to the spec, but how on earth should a scene_instance refer to it then?
				int indexID = GetAttribute( "id");
				const char* attrID = mReader->getAttributeValue( indexID);

				// read name if given. 
				int indexName = TestAttribute( "name");
				const char* attrName = "unnamed";
				if( indexName > -1)
					attrName = mReader->getAttributeValue( indexName);

				// TODO: (thom) support SIDs
				assert( TestAttribute( "sid") == -1);

				// create a node and store it in the library under its ID
				Node* node = new Node;
				node->mID = attrID;
				node->mName = attrName;
				mNodeLibrary[node->mID] = node;

				ReadSceneNode( node);
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads a scene node's contents including children and stores it in the given node
void ColladaParser::ReadSceneNode( Node* pNode)
{
	// quit immediately on <bla/> elements
	if( mReader->isEmptyElement())
		return;

	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "lookat"))
				ReadNodeTransformation( pNode, TF_LOOKAT);
			else if( IsElement( "matrix"))
				ReadNodeTransformation( pNode, TF_MATRIX);
			else if( IsElement( "rotate"))
				ReadNodeTransformation( pNode, TF_ROTATE);
			else if( IsElement( "scale"))
				ReadNodeTransformation( pNode, TF_SCALE);
			else if( IsElement( "skew"))
				ReadNodeTransformation( pNode, TF_SKEW);
			else if( IsElement( "translate"))
				ReadNodeTransformation( pNode, TF_TRANSLATE);
			else if( IsElement( "node"))
			{
				Node* child = new Node;
				int attrID = TestAttribute( "id");
				if( attrID > -1)
					child->mID = mReader->getAttributeValue( attrID);

				int attrName = TestAttribute( "name");
				if( attrName > -1)
					child->mName = mReader->getAttributeValue( attrName);

				// TODO: (thom) support SIDs
				assert( TestAttribute( "sid") == -1);

				pNode->mChildren.push_back( child);
				child->mParent = pNode;

				// read on recursively from there
				ReadSceneNode( child);
			} else if( IsElement( "instance_node"))
			{
				// test for it, in case we need to implement it
				assert( false);
				SkipElement();
			} else
			{
				// skip everything else for the moment
				SkipElement();
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads a node transformation entry of the given type and adds it to the given node's transformation list.
void ColladaParser::ReadNodeTransformation( Node* pNode, TransformType pType)
{
	std::string tagName = mReader->getNodeName();

	// how many parameters to read per transformation type
	static const unsigned int sNumParameters[] = { 9, 4, 3, 3, 7, 16 };
	const char* content = GetTextContent();

	// read as many parameters and store in the transformation
	Transform tf;
	tf.mType = pType;
	for( unsigned int a = 0; a < sNumParameters[pType]; a++)
	{
		// read a number
		content = fast_atof_move( content, tf.f[a]);
		// skip whitespace after it
		SkipSpacesAndLineEnd( &content);
	}

	// place the transformation at the queue of the node
	pNode->mTransforms.push_back( tf);

	// and consum the closing tag
	TestClosing( tagName.c_str());
}

// ------------------------------------------------------------------------------------------------
// Reads the collada scene
void ColladaParser::ReadScene()
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "instance_visual_scene"))
			{
				// should be the first and only occurence
				if( mRootNode)
					ThrowException( "Invalid scene containing multiple root nodes");

				// read the url of the scene to instance. Should be of format "#some_name"
				int urlIndex = GetAttribute( "url");
				const char* url = mReader->getAttributeValue( urlIndex);
				if( url[0] != '#')
					ThrowException( "Unknown reference format");

				// find the referred scene, skip the leading # 
				NodeLibrary::const_iterator sit = mNodeLibrary.find( url+1);
				if( sit == mNodeLibrary.end())
					ThrowException( boost::str( boost::format( "Unable to resolve visual_scene reference \"%s\".") % url));
				mRootNode = sit->second;
			} else
			{
				SkipElement();
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			break;
		} 
	}
}

// ------------------------------------------------------------------------------------------------
// Aborts the file reading with an exception
void ColladaParser::ThrowException( const std::string& pError) const
{
	throw new ImportErrorException( boost::str( boost::format( "%s - %s") % mFileName % pError));
}

// ------------------------------------------------------------------------------------------------
// Skips all data until the end node of the current element
void ColladaParser::SkipElement()
{
	// nothing to skip if it's an <element />
	if( mReader->isEmptyElement())
		return;

	// copy the current node's name because it'a pointer to the reader's internal buffer, 
	// which is going to change with the upcoming parsing 
	std::string element = mReader->getNodeName();
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
			if( mReader->getNodeName() == element)
				break;
	}
}

// ------------------------------------------------------------------------------------------------
// Tests for the closing tag of the given element, throws an exception if not found
void ColladaParser::TestClosing( const char* pName)
{
	// read closing tag
	if( !mReader->read())
		ThrowException( boost::str( boost::format( "Unexpected end of file while reading end of \"%s\" element.") % pName));
	if( mReader->getNodeType() != irr::io::EXN_ELEMENT_END || strcmp( mReader->getNodeName(), pName) != 0)
		ThrowException( boost::str( boost::format( "Expected end of \"%s\" element.") % pName));
}

// ------------------------------------------------------------------------------------------------
// Returns the index of the named attribute or -1 if not found. Does not throw, therefore useful for optional attributes
int ColladaParser::GetAttribute( const char* pAttr) const
{
	int index = TestAttribute( pAttr);
	if( index != -1)
		return index;

	// attribute not found -> throw an exception
	ThrowException( boost::str( boost::format( "Expected attribute \"%s\" at element \"%s\".") % pAttr % mReader->getNodeName()));
	return -1;
}

// ------------------------------------------------------------------------------------------------
// Tests the present element for the presence of one attribute, returns its index or throws an exception if not found
int ColladaParser::TestAttribute( const char* pAttr) const
{
	for( int a = 0; a < mReader->getAttributeCount(); a++)
		if( strcmp( mReader->getAttributeName( a), pAttr) == 0)
			return a;

	return -1;
}

// ------------------------------------------------------------------------------------------------
// Reads the text contents of an element, throws an exception if not given. Skips leading whitespace.
const char* ColladaParser::GetTextContent()
{
	// present node should be the beginning of an element
	if( mReader->getNodeType() != irr::io::EXN_ELEMENT || mReader->isEmptyElement())
		ThrowException( "Expected opening element");

	// read contents of the element
	if( !mReader->read())
		ThrowException( "Unexpected end of file while reading asset up_axis element.");
	if( mReader->getNodeType() != irr::io::EXN_TEXT)
		ThrowException( "Invalid contents in element \"up_axis\".");

	// skip leading whitespace
	const char* text = mReader->getNodeData();
	SkipSpacesAndLineEnd( &text);

	return text;
}

// ------------------------------------------------------------------------------------------------
// Calculates the resulting transformation fromm all the given transform steps
aiMatrix4x4 ColladaParser::CalculateResultTransform( const std::vector<Transform>& pTransforms) const
{
	aiMatrix4x4 res;

	for( std::vector<Transform>::const_iterator it = pTransforms.begin(); it != pTransforms.end(); ++it)
	{
		const Transform& tf = *it;
		switch( tf.mType)
		{
			case TF_LOOKAT:
				// TODO: (thom)
				assert( false);
				break;
			case TF_ROTATE:
			{
				aiMatrix4x4 rot;
				aiMatrix4x4::Rotation( tf.f[3], aiVector3D( tf.f[0], tf.f[1], tf.f[2]), rot);
				res *= rot;
				break;
			}
			case TF_TRANSLATE:
			{
				aiMatrix4x4 trans;
				aiMatrix4x4::Translation( aiVector3D( tf.f[0], tf.f[1], tf.f[2]), trans);
				res *= trans;
				break;
			}
			case TF_SCALE:
			{
				aiMatrix4x4 scale( tf.f[0], 0.0f, 0.0f, 0.0f, 0.0f, tf.f[1], 0.0f, 0.0f, 0.0f, 0.0f, tf.f[2], 0.0f, 
					0.0f, 0.0f, 0.0f, 1.0f);
				res *= scale;
				break;
			}
			case TF_SKEW:
				// TODO: (thom)
				assert( false);
				break;
			case TF_MATRIX:
			{
				aiMatrix4x4 mat( tf.f[0], tf.f[1], tf.f[2], tf.f[3], tf.f[4], tf.f[5], tf.f[6], tf.f[7],
					tf.f[8], tf.f[9], tf.f[10], tf.f[11], tf.f[12], tf.f[13], tf.f[14], tf.f[15]);
				res *= mat;
				break;
			}
			default: 
				assert( false);
				break;
		}
	}

	return res;
}
