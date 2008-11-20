/** Defines the parser helper class for the collada loader */

/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

#ifndef AI_COLLADAPARSER_H_INC
#define AI_COLLADAPARSER_H_INC

#include "./irrXML/irrXMLWrapper.h"

namespace Assimp
{

/** Parser helper class for the Collada loader. Does all the XML reading and builds internal data structures from it, 
 * but leaves the resolving of all the references to the loader.
*/
class ColladaParser
{
	friend class ColladaLoader;
public:
	/** Transformation types that can be applied to a node */
	enum TransformType
	{
		TF_LOOKAT,
		TF_ROTATE,
		TF_TRANSLATE,
		TF_SCALE,
		TF_SKEW,
		TF_MATRIX
	};

	/** Contains all data for one of the different transformation types */
	struct Transform
	{
		TransformType mType;
		float f[16]; ///< Interpretation of data depends on the type of the transformation 
	};

	/** A node in a scene hierarchy */
	struct Node
	{
		std::string mName;
		std::string mID;
		Node* mParent;
		std::vector<Node*> mChildren;

		/** Operations in order to calculate the resulting transformation to parent. */
		std::vector<Transform> mTransforms;

		Node() { mParent = NULL; }
		~Node() { for( std::vector<Node*>::iterator it = mChildren.begin(); it != mChildren.end(); ++it) delete *it; }
	};

	/** Data source array */
	struct Data
	{
		std::vector<float> mValues;
	};

	/** Accessor to a data array */
	struct Accessor
	{
		size_t mCount;   // in number of objects
		size_t mOffset;  // in number of values
		size_t mStride;  // Stride in number of values
		std::vector<std::string> mParams; // names of the data streams in the accessors. Empty string tells to ignore. 
		size_t mSubOffset[4]; // Suboffset inside the object for the common 4 elements. For a vector, thats XYZ, for a color RGBA and so on.
							  // For example, SubOffset[0] denotes which of the values inside the object is the vector X component.
		std::string mSource;   // URL of the source array
		mutable const Data* mData; // Pointer to the source array, if resolved. NULL else

		Accessor() 
		{ 
			mCount = 0; mOffset = 0; mStride = 0; mData = NULL; 
			mSubOffset[0] = mSubOffset[1] = mSubOffset[2] = mSubOffset[3] = 0;
		}
	};

	/** A single face in a mesh */
	struct Face
	{
		std::vector<size_t> mIndices;
	};

	/** Different types of input data to a vertex or face */
	enum InputType
	{
		IT_Invalid,
		IT_Vertex,  // special type for per-index data referring to the <vertices> element carrying the per-vertex data.
		IT_Position,
		IT_Normal,
		IT_Texcoord,
		IT_Color
	};

	/** An input channel for mesh data, referring to a single accessor */
	struct InputChannel
	{
		InputType mType;      // Type of the data
		size_t mIndex;		  // Optional index, if multiple sets of the same data type are given
		size_t mOffset;       // Index offset in the indices array of per-face indices. Don't ask, can't explain that any better.
		std::string mAccessor; // ID of the accessor where to read the actual values from.
		mutable const Accessor* mResolved; // Pointer to the accessor, if resolved. NULL else

		InputChannel() { mType = IT_Invalid; mIndex = 0; mOffset = 0; mResolved = NULL; }
	};

	/** Contains data for a single mesh */
	struct Mesh
	{
		std::string mVertexID; // just to check if there's some sophisticated addressing involved... which we don't support, and therefore should warn about.
		std::vector<InputChannel> mPerVertexData; // Vertex data addressed by vertex indices

		// actual mesh data, assembled on encounter of a <p> element. Verbose format, not indexed
		std::vector<aiVector3D> mPositions;
		std::vector<aiVector3D> mNormals;
		std::vector<aiVector2D> mTexCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
		std::vector<aiColor4D> mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

		// Faces. Stored are only the number of vertices for each face. 1 == point, 2 == line, 3 == triangle, 4+ == poly
		std::vector<size_t> mFaceSize;
	};

protected:
	/** Constructor from XML file */
	ColladaParser( const std::string& pFile);

	/** Destructor */
	~ColladaParser();

	/** Reads the contents of the file */
	void ReadContents();

	/** Reads the structure of the file */
	void ReadStructure();

	/** Reads asset informations such as coordinate system informations and legal blah */
	void ReadAssetInfo();

	/** Reads the geometry library contents */
	void ReadGeometryLibrary();

	/** Reads a mesh from the geometry library */
	void ReadMesh( Mesh* pMesh);

	/** Reads a data array holding a number of floats, and stores it in the global library */
	void ReadFloatArray();

	/** Reads an accessor and stores it in the global library under the given ID - 
	 * accessors use the ID of the parent <source> element
	 */
	void ReadAccessor( const std::string& pID);

	/** Reads input declarations of per-vertex mesh data into the given mesh */
	void ReadVertexData( Mesh* pMesh);

	/** Reads input declarations of per-index mesh data into the given mesh */
	void ReadIndexData( Mesh* pMesh);

	/** Reads a single input channel element and stores it in the given array, if valid */
	void ReadInputChannel( std::vector<InputChannel>& poChannels);

	/** Reads a <p> primitive index list and assembles the mesh data into the given mesh */
	void ReadPrimitives( Mesh* pMesh, std::vector<InputChannel>& pPerIndexChannels, 
		size_t pNumPrimitives, const std::vector<size_t>& pVCount, bool pIsPolylist);

	/** Extracts a single object from an input channel and stores it in the appropriate mesh data array */
	void ExtractDataObjectFromChannel( const InputChannel& pInput, size_t pLocalIndex, Mesh* pMesh);

	/** Reads the library of node hierarchies and scene parts */
	void ReadSceneLibrary();

	/** Reads a scene node's contents including children and stores it in the given node */
	void ReadSceneNode( Node* pNode);

	/** Reads a node transformation entry of the given type and adds it to the given node's transformation list. */
	void ReadNodeTransformation( Node* pNode, TransformType pType);

	/** Reads the collada scene */
	void ReadScene();

protected:
	/** Aborts the file reading with an exception */
	void ThrowException( const std::string& pError) const;

	/** Skips all data until the end node of the current element */
	void SkipElement();

	/** Compares the current xml element name to the given string and returns true if equal */
	bool IsElement( const char* pName) const { assert( mReader->getNodeType() == irr::io::EXN_ELEMENT); return strcmp( mReader->getNodeName(), pName) == 0; }

	/** Tests for the opening tag of the given element, throws an exception if not found */
	void TestOpening( const char* pName);

	/** Tests for the closing tag of the given element, throws an exception if not found */
	void TestClosing( const char* pName);

	/** Checks the present element for the presence of the attribute, returns its index or throws an exception if not found */
	int GetAttribute( const char* pAttr) const;

	/** Returns the index of the named attribute or -1 if not found. Does not throw, therefore useful for optional attributes */
	int TestAttribute( const char* pAttr) const;

	/** Reads the text contents of an element, throws an exception if not given. Skips leading whitespace. */
	const char* GetTextContent();

	/** Calculates the resulting transformation fromm all the given transform steps */
	aiMatrix4x4 CalculateResultTransform( const std::vector<Transform>& pTransforms) const;

	/** Determines the input data type for the given semantic string */
	InputType GetTypeForSemantic( const std::string& pSemantic);

	/** Finds the item in the given library by its reference, throws if not found */
	template <typename Type> const Type& ResolveLibraryReference( const std::map<std::string, Type>& pLibrary, const std::string& pURL) const;

protected:
	/** Filename, for a verbose error message */
	std::string mFileName;

	/** XML reader */
	irr::io::IrrXMLReader* mReader;

	/** All data arrays found in the file by ID. Might be referred to by actually everyone. Collada, you are a steaming pile of indirection. */
	typedef std::map<std::string, Data> DataLibrary;
	DataLibrary mDataLibrary;

	/** Same for accessors which define how the data in a data array is accessed. */
	typedef std::map<std::string, Accessor> AccessorLibrary;
	AccessorLibrary mAccessorLibrary;

	/** Mesh library: mesh by ID */
	typedef std::map<std::string, Mesh*> MeshLibrary;
	MeshLibrary mMeshLibrary;

	/** node library: root node of the hierarchy part by ID */
	typedef std::map<std::string, Node*> NodeLibrary;
	NodeLibrary mNodeLibrary;

	/** Pointer to the root node. Don't delete, it just points to one of the nodes in the node library. */
	Node* mRootNode;

	/** Size unit: how large compared to a meter */
	float mUnitSize;

	/** Which is the up vector */
	enum { UP_X, UP_Y, UP_Z } mUpDirection;
};

// ------------------------------------------------------------------------------------------------
// Finds the item in the given library by its reference, throws if not found
template <typename Type> 
const Type& ColladaParser::ResolveLibraryReference( const std::map<std::string, Type>& pLibrary, const std::string& pURL) const
{
	std::map<std::string, Type>::const_iterator it = pLibrary.find( pURL);
	if( it == pLibrary.end())
		ThrowException( boost::str( boost::format( "Unable to resolve library reference \"%s\".") % pURL));
	return it->second;
}

} // end of namespace Assimp

#endif // AI_COLLADAPARSER_H_INC
