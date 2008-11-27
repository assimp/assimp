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
using namespace Assimp::Collada;

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
	for( MeshLibrary::iterator it = mMeshLibrary.begin(); it != mMeshLibrary.end(); ++it)
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
			if( strcmp( mReader->getNodeName(), "asset") != 0)
				ThrowException( "Expected end of \"asset\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads the image library contents
void ColladaParser::ReadImageLibrary()
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "image"))
			{
				// read ID. Another entry which is "optional" by design but obligatory in reality
				int attrID = GetAttribute( "id");
				std::string id = mReader->getAttributeValue( attrID);

				// create an entry and store it in the library under its ID
				mImageLibrary[id] = Image();

				// read on from there
				ReadImage( mImageLibrary[id]);
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "library_images") != 0)
				ThrowException( "Expected end of \"library_images\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads an image entry into the given image
void ColladaParser::ReadImage( Collada::Image& pImage)
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "init_from"))
			{
				// element content is filename - hopefully
				const char* content = GetTextContent();
				pImage.mFileName = content;
				TestClosing( "init_from");
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "image") != 0)
				ThrowException( "Expected end of \"image\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads the material library
void ColladaParser::ReadMaterialLibrary()
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "material"))
			{
				// read ID. By now you propably know my opinion about this "specification"
				int attrID = GetAttribute( "id");
				std::string id = mReader->getAttributeValue( attrID);

				// create an entry and store it in the library under its ID
				mMaterialLibrary[id] = Material();
				// read on from there
				ReadMaterial( mMaterialLibrary[id]);
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "library_materials") != 0)
				ThrowException( "Expected end of \"library_materials\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads a material entry into the given material
void ColladaParser::ReadMaterial( Collada::Material& pMaterial)
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "instance_effect"))
			{
				// referred effect by URL
				int attrUrl = GetAttribute( "url");
				const char* url = mReader->getAttributeValue( attrUrl);
				if( url[0] != '#')
					ThrowException( "Unknown reference format");

				pMaterial.mEffect = url;

				SkipElement();
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "material") != 0)
				ThrowException( "Expected end of \"image\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads the effect library
void ColladaParser::ReadEffectLibrary()
{
}

// ------------------------------------------------------------------------------------------------
// Reads an effect entry into the given effect
void ColladaParser::ReadEffect( Collada::Effect* pEffect)
{
}

// ------------------------------------------------------------------------------------------------
// Reads the geometry library contents
void ColladaParser::ReadGeometryLibrary()
{
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "geometry"))
			{
				// read ID. Another entry which is "optional" by design but obligatory in reality
				int indexID = GetAttribute( "id");
				std::string id = mReader->getAttributeValue( indexID);

				// TODO: (thom) support SIDs
				assert( TestAttribute( "sid") == -1);

				// a <geometry> always contains a single <mesh> element inside, so we just skip that element in advance
				TestOpening( "mesh");

				// create a mesh and store it in the library under its ID
				Mesh* mesh = new Mesh;
				mMeshLibrary[id] = mesh;
				// read on from there
				ReadMesh( mesh);

				// check for the closing tag of the outer <geometry> element, the inner closing of <mesh> has been consumed by ReadMesh()
				TestClosing( "geometry");
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "library_geometries") != 0)
				ThrowException( "Expected end of \"library_geometries\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads a mesh from the geometry library
void ColladaParser::ReadMesh( Mesh* pMesh)
{
	// I'm doing a dirty state parsing here because I don't want to open another submethod for it.
	// There's a <source> tag defining the name for the accessor inside, and possible a <float_array>
	// with it's own ID. This string contains the current source's ID if parsing is inside a <source> element.
	std::string presentSourceID;

	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "source"))
			{
				// beginning of a source element - store ID for the inner elements
				int attrID = GetAttribute( "id");
				presentSourceID = mReader->getAttributeValue( attrID);
			}
			else if( IsElement( "float_array"))
			{
				ReadFloatArray();
			}
			else if( IsElement( "technique_common"))
			{
				// I don't fucking care for your profiles bullshit
			}
			else if( IsElement( "accessor"))
			{
				ReadAccessor( presentSourceID);
			} 
			else if( IsElement( "vertices"))
			{
				// read per-vertex mesh data
				ReadVertexData( pMesh);
			}
			else if( IsElement( "triangles") || IsElement( "lines") || IsElement( "linestrips")
				|| IsElement( "polygons") || IsElement( "polylist") || IsElement( "trifans") || IsElement( "tristrips")) 
			{
				// read per-index mesh data and faces setup
				ReadIndexData( pMesh);
			} else
			{
				// ignore the rest
				SkipElement();
			}
		}
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "source") == 0)
			{
				// end of <source> - reset present source ID
				presentSourceID.clear();
			}
			else if( strcmp( mReader->getNodeName(), "technique_common") == 0)
			{
				// end of another meaningless element - read over it
			} 
			else if( strcmp( mReader->getNodeName(), "mesh") == 0)
			{
				// end of <mesh> element - we're done here
				break;
			} else
			{
				// everything else should be punished
				ThrowException( "Expected end of \"mesh\" element.");
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads a data array holding a number of floats, and stores it in the global library
void ColladaParser::ReadFloatArray()
{
	// read attributes
	int indexID = GetAttribute( "id");
	std::string id = mReader->getAttributeValue( indexID);
	int indexCount = GetAttribute( "count");
	unsigned int count = (unsigned int) mReader->getAttributeValueAsInt( indexCount);
	const char* content = GetTextContent();

	// read values and store inside an array in the data library
	mDataLibrary[id] = Data();
	Data& data = mDataLibrary[id];
	data.mValues.reserve( count);
	for( unsigned int a = 0; a < count; a++)
	{
		if( *content == 0)
			ThrowException( "Expected more values while reading float_array contents.");

		float value;
		// read a number
		content = fast_atof_move( content, value);
		data.mValues.push_back( value);
		// skip whitespace after it
		SkipSpacesAndLineEnd( &content);
	}

	// test for closing tag
	TestClosing( "float_array");
}

// ------------------------------------------------------------------------------------------------
// Reads an accessor and stores it in the global library
void ColladaParser::ReadAccessor( const std::string& pID)
{
	// read accessor attributes
	int attrSource = GetAttribute( "source");
	const char* source = mReader->getAttributeValue( attrSource);
	if( source[0] != '#')
		ThrowException( boost::str( boost::format( "Unknown reference format in url \"%s\".") % source));
	int attrCount = GetAttribute( "count");
	unsigned int count = (unsigned int) mReader->getAttributeValueAsInt( attrCount);
	int attrOffset = TestAttribute( "offset");
	unsigned int offset = 0;
	if( attrOffset > -1)
		offset = (unsigned int) mReader->getAttributeValueAsInt( attrOffset);
	int attrStride = TestAttribute( "stride");
	unsigned int stride = 1;
	if( attrStride > -1)
		stride = (unsigned int) mReader->getAttributeValueAsInt( attrStride);

	// store in the library under the given ID
	mAccessorLibrary[pID] = Accessor();
	Accessor& acc = mAccessorLibrary[pID];
	acc.mCount = count;
	acc.mOffset = offset;
	acc.mStride = stride;
	acc.mSource = source+1; // ignore the leading '#'

	// and read the components
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "param"))
			{
				// read data param
				int attrName = TestAttribute( "name");
				std::string name;
				if( attrName > -1)
				{
					name = mReader->getAttributeValue( attrName);

					// analyse for common type components and store it's sub-offset in the corresponding field
					if( name == "X") acc.mSubOffset[0] = acc.mParams.size();
					else if( name == "Y") acc.mSubOffset[1] = acc.mParams.size();
					else if( name == "Z") acc.mSubOffset[2] = acc.mParams.size();
					else if( name == "R") acc.mSubOffset[0] = acc.mParams.size();
					else if( name == "G") acc.mSubOffset[1] = acc.mParams.size();
					else if( name == "B") acc.mSubOffset[2] = acc.mParams.size();
					else if( name == "A") acc.mSubOffset[3] = acc.mParams.size();
					else if( name == "S") acc.mSubOffset[0] = acc.mParams.size();
					else if( name == "T") acc.mSubOffset[1] = acc.mParams.size();
					else
						DefaultLogger::get()->warn( boost::str( boost::format( "Unknown accessor parameter \"%s\". Ignoring data channel.") % name));
				}

				acc.mParams.push_back( name);

				// skip remaining stuff of this element, if any
				SkipElement();
			} else
			{
				ThrowException( "Unexpected sub element in tag \"accessor\".");
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "accessor") != 0)
				ThrowException( "Expected end of \"accessor\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads input declarations of per-vertex mesh data into the given mesh
void ColladaParser::ReadVertexData( Mesh* pMesh)
{
	// extract the ID of the <vertices> element. Not that we care, but to catch strange referencing schemes we should warn about
	int attrID= GetAttribute( "id");
	pMesh->mVertexID = mReader->getAttributeValue( attrID);

	// a number of <input> elements
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "input"))
			{
				ReadInputChannel( pMesh->mPerVertexData);
			} else
			{
				ThrowException( "Unexpected sub element in tag \"vertices\".");
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "vertices") != 0)
				ThrowException( "Expected end of \"vertices\" element.");

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads input declarations of per-index mesh data into the given mesh
void ColladaParser::ReadIndexData( Mesh* pMesh)
{
	std::vector<size_t> vcount;
	std::vector<InputChannel> perIndexData;

	// read primitive count from the attribute
	int attrCount = GetAttribute( "count");
	size_t numPrimitives = (size_t) mReader->getAttributeValueAsInt( attrCount);

	// distinguish between polys and triangles
	std::string elementName = mReader->getNodeName();
	PrimitiveType primType = Prim_Invalid;
	if( IsElement( "lines"))
		primType = Prim_Lines;
	else if( IsElement( "linestrips"))
		primType = Prim_LineStrip;
	else if( IsElement( "polygons"))
		primType = Prim_Polygon;
	else if( IsElement( "polylist"))
		primType = Prim_Polylist;
	else if( IsElement( "triangles"))
		primType = Prim_Triangles;
	else if( IsElement( "trifans"))
		primType = Prim_TriFans;
	else if( IsElement( "tristrips"))
		primType = Prim_TriStrips;

	assert( primType != Prim_Invalid);

	// also a number of <input> elements, but in addition a <p> primitive collection and propably index counts for all primitives
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "input"))
			{
				ReadInputChannel( perIndexData);
			} 
			else if( IsElement( "vcount"))
			{
				// case <polylist> - specifies the number of indices for each polygon
				const char* content = GetTextContent();
				vcount.reserve( numPrimitives);
				for( unsigned int a = 0; a < numPrimitives; a++)
				{
					if( *content == 0)
						ThrowException( "Expected more values while reading vcount contents.");
					// read a number
					vcount.push_back( (size_t) strtol10( content, &content));
					// skip whitespace after it
					SkipSpacesAndLineEnd( &content);
				}
				
				TestClosing( "vcount");
			}
			else if( IsElement( "p"))
			{
				// now here the actual fun starts - these are the indices to construct the mesh data from
				ReadPrimitives( pMesh, perIndexData, numPrimitives, vcount, primType);
			} else
			{
				ThrowException( "Unexpected sub element in tag \"vertices\".");
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( mReader->getNodeName() != elementName)
				ThrowException( boost::str( boost::format( "Expected end of \"%s\" element.") % elementName));

			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Reads a single input channel element and stores it in the given array, if valid 
void ColladaParser::ReadInputChannel( std::vector<InputChannel>& poChannels)
{
	InputChannel channel;
	
	// read semantic
	int attrSemantic = GetAttribute( "semantic");
	std::string semantic = mReader->getAttributeValue( attrSemantic);
	channel.mType = GetTypeForSemantic( semantic);

	// read source
	int attrSource = GetAttribute( "source");
	const char* source = mReader->getAttributeValue( attrSource);
	if( source[0] != '#')
		ThrowException( boost::str( boost::format( "Unknown reference format in url \"%s\".") % source));
	channel.mAccessor = source+1; // skipping the leading #, hopefully the remaining text is the accessor ID only

	// read index offset, if per-index <input>
	int attrOffset = TestAttribute( "offset");
	if( attrOffset > -1)
		channel.mOffset = mReader->getAttributeValueAsInt( attrOffset);

	// store, if valid type
	if( channel.mType != IT_Invalid)
		poChannels.push_back( channel);

	// skip remaining stuff of this element, if any
	SkipElement();
}

// ------------------------------------------------------------------------------------------------
// Reads a <p> primitive index list and assembles the mesh data into the given mesh
void ColladaParser::ReadPrimitives( Mesh* pMesh, std::vector<InputChannel>& pPerIndexChannels, 
								   size_t pNumPrimitives, const std::vector<size_t>& pVCount, PrimitiveType pPrimType)
{
	// determine number of indices coming per vertex 
	// find the offset index for all per-vertex channels
	size_t numOffsets = 1;
	size_t perVertexOffset = -1; // invalid value
	BOOST_FOREACH( const InputChannel& channel, pPerIndexChannels)
	{
		numOffsets = std::max( numOffsets, channel.mOffset+1);
		if( channel.mType == IT_Vertex)
			perVertexOffset = channel.mOffset;
	}

	// determine the expected number of indices 
	size_t expectedPointCount = 0;
	switch( pPrimType)
	{
		case Prim_Polylist:
		{
			BOOST_FOREACH( size_t i, pVCount)
				expectedPointCount += i;
			break;
		}
		case Prim_Lines:
			expectedPointCount = 2 * pNumPrimitives;
			break;
		case Prim_Triangles:
			expectedPointCount = 3 * pNumPrimitives;
			break;
		default:
			// other primitive types don't state the index count upfront... we need to guess
			break;
	}

	// and read all indices into a temporary array
	std::vector<size_t> indices;
	if( expectedPointCount > 0)
		indices.reserve( expectedPointCount * numOffsets);

	const char* content = GetTextContent();
	while( *content != 0)
	{
		// read a value 
		unsigned int value = strtol10( content, &content);
		indices.push_back( size_t( value));
		// skip whitespace after it
		SkipSpacesAndLineEnd( &content);
	}

	// complain if the index count doesn't fit
	if( expectedPointCount > 0 && indices.size() != expectedPointCount * numOffsets)
		ThrowException( "Expected different index count in <p> element.");
	else if( expectedPointCount == 0 && (indices.size() % numOffsets) != 0)
		ThrowException( "Expected different index count in <p> element.");

	// find the data for all sources
	BOOST_FOREACH( InputChannel& input, pMesh->mPerVertexData)
	{
		if( input.mResolved)
			continue;

		// find accessor
		input.mResolved = &ResolveLibraryReference( mAccessorLibrary, input.mAccessor);
		// resolve accessor's data pointer as well, if neccessary
		const Accessor* acc = input.mResolved;
		if( !acc->mData)
			acc->mData = &ResolveLibraryReference( mDataLibrary, acc->mSource);
	}
	// and the same for the per-index channels
	BOOST_FOREACH( InputChannel& input, pPerIndexChannels)
	{
		if( input.mResolved)
			continue;

		// ignore vertex pointer, it doesn't refer to an accessor
		if( input.mType == IT_Vertex)
		{
			// warn if the vertex channel does not refer to the <vertices> element in the same mesh
			if( input.mAccessor != pMesh->mVertexID)
				ThrowException( "Unsupported vertex referencing scheme. I fucking hate Collada.");
			continue;
		}

		// find accessor
		input.mResolved = &ResolveLibraryReference( mAccessorLibrary, input.mAccessor);
		// resolve accessor's data pointer as well, if neccessary
		const Accessor* acc = input.mResolved;
		if( !acc->mData)
			acc->mData = &ResolveLibraryReference( mDataLibrary, acc->mSource);
	}


	// now assemble vertex data according to those indices
	std::vector<size_t>::const_iterator idx = indices.begin();

	// For continued primitives, the given count does not come all in one <p>, but only one primitive per <p>
	size_t numPrimitives = pNumPrimitives;
	if( pPrimType == Prim_TriFans || pPrimType == Prim_Polygon)
		numPrimitives = 1;

	for( size_t a = 0; a < numPrimitives; a++)
	{
		// determine number of points for this primitive
		size_t numPoints = 0;
		switch( pPrimType)
		{
			case Prim_Lines: numPoints = 2; break;
			case Prim_Triangles: numPoints = 3; break;
			case Prim_Polylist: numPoints = pVCount[a]; break;
			case Prim_TriFans: 
			case Prim_Polygon: numPoints = indices.size() / numOffsets; break;
			default:
				// LineStrip and TriStrip not supported due to expected index unmangling
				ThrowException( "Unsupported primitive type.");
				break;
		}

		// store the face size to later reconstruct the face from
		pMesh->mFaceSize.push_back( numPoints);

		// gather that number of vertices
		for( size_t b = 0; b < numPoints; b++)
		{
			// read all indices for this vertex. Yes, in a hacky static array
			assert( numOffsets < 20);
			static size_t vindex[20];
			for( size_t offsets = 0; offsets < numOffsets; ++offsets)
				vindex[offsets] = *idx++;

			// extract per-vertex channels using the global per-vertex offset
			BOOST_FOREACH( const InputChannel& input, pMesh->mPerVertexData)
				ExtractDataObjectFromChannel( input, vindex[perVertexOffset], pMesh);
			// and extract per-index channels using there specified offset
			BOOST_FOREACH( const InputChannel& input, pPerIndexChannels)
				ExtractDataObjectFromChannel( input, vindex[input.mOffset], pMesh);
		}
	}

	// if I ever get my hands on that guy who invented this steaming pile of indirection...
	TestClosing( "p");
}

// ------------------------------------------------------------------------------------------------
// Extracts a single object from an input channel and stores it in the appropriate mesh data array 
void ColladaParser::ExtractDataObjectFromChannel( const InputChannel& pInput, size_t pLocalIndex, Mesh* pMesh)
{
	// ignore vertex referrer - we handle them that separate
	if( pInput.mType == IT_Vertex)
		return;

	const Accessor& acc = *pInput.mResolved;
	if( pLocalIndex >= acc.mCount)
		ThrowException( boost::str( boost::format( "Invalid data index (%d/%d) in primitive specification") % pLocalIndex % acc.mCount));

	// get a pointer to the start of the data object referred to by the accessor and the local index
	const float* dataObject = &(acc.mData->mValues[0]) + acc.mOffset + pLocalIndex* acc.mStride;
	
	// assemble according to the accessors component sub-offset list. We don't care, yet, what kind of object exactly we're extracting here
	float obj[4];
	for( size_t c = 0; c < 4; ++c)
		obj[c] = dataObject[acc.mSubOffset[c]];

	// now we reinterpret it according to the type we're reading here
	switch( pInput.mType)
	{
		case IT_Position: // ignore all position streams except 0 - there can be only one position
			if( pInput.mIndex == 0)
				pMesh->mPositions.push_back( aiVector3D( obj[0], obj[1], obj[2])); 
			break;
		case IT_Normal: // ignore all normal streams except 0 - there can be only one normal
			if( pInput.mIndex == 0)
				pMesh->mNormals.push_back( aiVector3D( obj[0], obj[1], obj[2])); 
			break;
		case IT_Texcoord: // up to 4 texture coord sets are fine, ignore the others
			if( pInput.mIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS)
				pMesh->mTexCoords[pInput.mIndex].push_back( aiVector2D( obj[0], obj[1])); 
			break;
		case IT_Color: // up to 4 color sets are fine, ignore the others
			if( pInput.mIndex < AI_MAX_NUMBER_OF_COLOR_SETS)
				pMesh->mColors[pInput.mIndex].push_back( aiColor4D( obj[0], obj[1], obj[2], obj[3])); 
			break;
	}
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
			if( strcmp( mReader->getNodeName(), "library_visual_scenes") != 0)
				ThrowException( "Expected end of \"library_visual_scenes\" element.");

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
//				assert( TestAttribute( "sid") == -1);

				pNode->mChildren.push_back( child);
				child->mParent = pNode;

				// read on recursively from there
				ReadSceneNode( child);
			} else if( IsElement( "instance_node"))
			{
				// test for it, in case we need to implement it
				assert( false);
				SkipElement();
			} else if( IsElement( "instance_geometry"))
			{
				// Reference to a mesh, we possible material associations
				ReadNodeGeometry( pNode);
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
// Reads a mesh reference in a node and adds it to the node's mesh list
void ColladaParser::ReadNodeGeometry( Node* pNode)
{
	// referred mesh is given as an attribute of the <instance_geometry> element
	int attrUrl = GetAttribute( "url");
	const char* url = mReader->getAttributeValue( attrUrl);
	if( url[0] != '#')
		ThrowException( "Unknown reference format");
	
	Collada::MeshInstance instance;
	instance.mMesh = url+1; // skipping the leading #

	// read material associations. Ignore additional elements inbetween
	while( mReader->read())
	{
		if( mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if( IsElement( "instance_material"))
			{
				// read ID of the geometry subgroup and the target material
				int attrGroup = GetAttribute( "symbol");
				std::string group = mReader->getAttributeValue( attrGroup);
				int attrMaterial = GetAttribute( "target");
				const char* urlMat = mReader->getAttributeValue( attrMaterial);
				if( urlMat[0] != '#')
					ThrowException( "Unknown reference format");
				std::string mat = mReader->getAttributeValue( attrMaterial+1);

				// store the association
				instance.mMaterials[group] = mat;
			} else
			{
				SkipElement();
			}
		} 
		else if( mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if( strcmp( mReader->getNodeName(), "instance_geometry") == 0)
				break;
		} 
	}
	
	// store it
	pNode->mMeshes.push_back( instance);
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
// Tests for an opening element of the given name, throws an exception if not found
void ColladaParser::TestOpening( const char* pName)
{
	// read element start
	if( !mReader->read())
		ThrowException( boost::str( boost::format( "Unexpected end of file while beginning of \"%s\" element.") % pName));
	// whitespace in front is ok, just read again if found
	if( mReader->getNodeType() == irr::io::EXN_TEXT)
		if( !mReader->read())
			ThrowException( boost::str( boost::format( "Unexpected end of file while reading beginning of \"%s\" element.") % pName));

	if( mReader->getNodeType() != irr::io::EXN_ELEMENT || strcmp( mReader->getNodeName(), pName) != 0)
		ThrowException( boost::str( boost::format( "Expected start of \"%s\" element.") % pName));
}

// ------------------------------------------------------------------------------------------------
// Tests for the closing tag of the given element, throws an exception if not found
void ColladaParser::TestClosing( const char* pName)
{
	// read closing tag
	if( !mReader->read())
		ThrowException( boost::str( boost::format( "Unexpected end of file while reading end of \"%s\" element.") % pName));
	// whitespace in front is ok, just read again if found
	if( mReader->getNodeType() == irr::io::EXN_TEXT)
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
				float angle = tf.f[3] * float( AI_MATH_PI) / 180.0f;
				aiVector3D axis( tf.f[0], tf.f[1], tf.f[2]);
				aiMatrix4x4::Rotation( angle, axis, rot);
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

// ------------------------------------------------------------------------------------------------
// Determines the input data type for the given semantic string
Collada::InputType ColladaParser::GetTypeForSemantic( const std::string& pSemantic)
{
	if( pSemantic == "POSITION")
		return IT_Position;
	else if( pSemantic == "TEXCOORD")
		return IT_Texcoord;
	else if( pSemantic == "NORMAL")
		return IT_Normal;
	else if( pSemantic == "COLOR")
		return IT_Color;
	else if( pSemantic == "VERTEX")
		return IT_Vertex;

	DefaultLogger::get()->warn( boost::str( boost::format( "Unknown vertex input type \"%s\". Ignoring.") % pSemantic));
	return IT_Invalid;
}
