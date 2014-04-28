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

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_OBJ_IMPORTER

#include "ObjFileParser.h"
#include "ObjFileMtlImporter.h"
#include "ObjTools.h"
#include "ObjFileData.h"
#include "ParsingUtils.h"
#include "../include/assimp/types.h"
#include "DefaultIOSystem.h"

namespace Assimp {

const std::string ObjFileParser::DEFAULT_MATERIAL = AI_DEFAULT_MATERIAL_NAME; 

// -------------------------------------------------------------------
//	Constructor with loaded data and directories.
ObjFileParser::ObjFileParser(std::vector<char> &Data,const std::string &strModelName, IOSystem *io ) :
	m_DataIt(Data.begin()),
	m_DataItEnd(Data.end()),
	m_pModel(NULL),
	m_uiLine(0),
	m_pIO( io )
{
	std::fill_n(m_buffer,BUFFERSIZE,0);

	// Create the model instance to store all the data
	m_pModel = new ObjFile::Model();
	m_pModel->m_ModelName = strModelName;
	
    // create default material and store it
	m_pModel->m_pDefaultMaterial = new ObjFile::Material();
	m_pModel->m_pDefaultMaterial->MaterialName.Set( DEFAULT_MATERIAL );
    m_pModel->m_MaterialLib.push_back( DEFAULT_MATERIAL );
	m_pModel->m_MaterialMap[ DEFAULT_MATERIAL ] = m_pModel->m_pDefaultMaterial;
	
	// Start parsing the file
	parseFile();
}

// -------------------------------------------------------------------
//	Destructor
ObjFileParser::~ObjFileParser()
{
	delete m_pModel;
	m_pModel = NULL;
}

// -------------------------------------------------------------------
//	Returns a pointer to the model instance.
ObjFile::Model *ObjFileParser::GetModel() const
{
	return m_pModel;
}

// -------------------------------------------------------------------
//	File parsing method.
void ObjFileParser::parseFile()
{
	if (m_DataIt == m_DataItEnd)
		return;

	while (m_DataIt != m_DataItEnd)
	{
		switch (*m_DataIt)
		{
		case 'v': // Parse a vertex texture coordinate
			{
				++m_DataIt;
				if (*m_DataIt == ' ' || *m_DataIt == '\t') {
					// read in vertex definition
					getVector3(m_pModel->m_Vertices);
				} else if (*m_DataIt == 't') {
					// read in texture coordinate ( 2D or 3D )
                    ++m_DataIt;
                    getVector( m_pModel->m_TextureCoord );
				} else if (*m_DataIt == 'n') {
					// Read in normal vector definition
					++m_DataIt;
					getVector3( m_pModel->m_Normals );
				}
			}
			break;

		case 'p': // Parse a face, line or point statement
		case 'l':
		case 'f':
			{
				getFace(*m_DataIt == 'f' ? aiPrimitiveType_POLYGON : (*m_DataIt == 'l' 
					? aiPrimitiveType_LINE : aiPrimitiveType_POINT));
			}
			break;

		case '#': // Parse a comment
			{
				getComment();
			}
			break;

		case 'u': // Parse a material desc. setter
			{
				getMaterialDesc();
			}
			break;

		case 'm': // Parse a material library or merging group ('mg')
			{
				if (*(m_DataIt + 1) == 'g')
					getGroupNumberAndResolution();
				else
					getMaterialLib();
			}
			break;

		case 'g': // Parse group name
			{
				getGroupName();
			}
			break;

		case 's': // Parse group number
			{
				getGroupNumber();
			}
			break;

		case 'o': // Parse object name
			{
				getObjectName();
			}
			break;
		
		default:
			{
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;
		}
	}
}

// -------------------------------------------------------------------
//	Copy the next word in a temporary buffer
void ObjFileParser::copyNextWord(char *pBuffer, size_t length)
{
	size_t index = 0;
	m_DataIt = getNextWord<DataArrayIt>(m_DataIt, m_DataItEnd);
	while ( m_DataIt != m_DataItEnd && !isSeparator(*m_DataIt) )
	{
		pBuffer[index] = *m_DataIt;
		index++;
		if (index == length-1)
			break;
		++m_DataIt;
	}

	ai_assert(index < length);
	pBuffer[index] = '\0';
}

// -------------------------------------------------------------------
// Copy the next line into a temporary buffer
void ObjFileParser::copyNextLine(char *pBuffer, size_t length)
{
	size_t index = 0u;

	// some OBJ files have line continuations using \ (such as in C++ et al)
	bool continuation = false;
	for (;m_DataIt != m_DataItEnd && index < length-1; ++m_DataIt) 
	{
		const char c = *m_DataIt;
		if (c == '\\') {
			continuation = true;
			continue;
		}
		
		if (c == '\n' || c == '\r') {
			if(continuation) {
				pBuffer[ index++ ] = ' ';
				continue;
			}
			break;
		}

		continuation = false;
		pBuffer[ index++ ] = c;
	}
	ai_assert(index < length);
	pBuffer[ index ] = '\0';
}

// -------------------------------------------------------------------
void ObjFileParser::getVector( std::vector<aiVector3D> &point3d_array ) {
    size_t numComponents( 0 );
    DataArrayIt tmp( m_DataIt );
    while( !IsLineEnd( *tmp ) ) {
        if( *tmp == ' ' ) {
            ++numComponents;
        }
        tmp++;
    }
    float x, y, z;
    if( 2 == numComponents ) {
        copyNextWord( m_buffer, BUFFERSIZE );
        x = ( float ) fast_atof( m_buffer );

        copyNextWord( m_buffer, BUFFERSIZE );
        y = ( float ) fast_atof( m_buffer );
        z = 0.0;
    } else if( 3 == numComponents ) {
        copyNextWord( m_buffer, BUFFERSIZE );
        x = ( float ) fast_atof( m_buffer );

        copyNextWord( m_buffer, BUFFERSIZE );
        y = ( float ) fast_atof( m_buffer );

        copyNextWord( m_buffer, BUFFERSIZE );
        z = ( float ) fast_atof( m_buffer );
    } else {
        ai_assert( !"Invalid number of components" );
    }
    point3d_array.push_back( aiVector3D( x, y, z ) );
    m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Get values for a new 3D vector instance
void ObjFileParser::getVector3(std::vector<aiVector3D> &point3d_array) {
	float x, y, z;
	copyNextWord(m_buffer, BUFFERSIZE);
	x = (float) fast_atof(m_buffer);	
	
	copyNextWord(m_buffer, BUFFERSIZE);
	y = (float) fast_atof(m_buffer);

    copyNextWord( m_buffer, BUFFERSIZE );
    z = ( float ) fast_atof( m_buffer );

	point3d_array.push_back( aiVector3D( x, y, z ) );
	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Get values for a new 2D vector instance
void ObjFileParser::getVector2( std::vector<aiVector2D> &point2d_array ) {
	float x, y;
	copyNextWord(m_buffer, BUFFERSIZE);
	x = (float) fast_atof(m_buffer);	
	
	copyNextWord(m_buffer, BUFFERSIZE);
	y = (float) fast_atof(m_buffer);

	point2d_array.push_back(aiVector2D(x, y));

	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Get values for a new face instance
void ObjFileParser::getFace(aiPrimitiveType type)
{
	copyNextLine(m_buffer, BUFFERSIZE);
	if (m_DataIt == m_DataItEnd)
		return;

	char *pPtr = m_buffer;
	char *pEnd = &pPtr[BUFFERSIZE];
	pPtr = getNextToken<char*>(pPtr, pEnd);
	if (pPtr == pEnd || *pPtr == '\0')
		return;

	std::vector<unsigned int> *pIndices = new std::vector<unsigned int>;
	std::vector<unsigned int> *pTexID = new std::vector<unsigned int>;
	std::vector<unsigned int> *pNormalID = new std::vector<unsigned int>;
	bool hasNormal = false;

	const int vSize = m_pModel->m_Vertices.size();
	const int vtSize = m_pModel->m_TextureCoord.size();
	const int vnSize = m_pModel->m_Normals.size();

	const bool vt = (!m_pModel->m_TextureCoord.empty());
	const bool vn = (!m_pModel->m_Normals.empty());
	int iStep = 0, iPos = 0;
	while (pPtr != pEnd)
	{
		iStep = 1;

		if (IsLineEnd(*pPtr))
			break;

		if (*pPtr=='/' )
		{
			if (type == aiPrimitiveType_POINT) {
				DefaultLogger::get()->error("Obj: Separator unexpected in point statement");
			}
			if (iPos == 0)
			{
				//if there are no texture coordinates in the file, but normals
				if (!vt && vn) {
					iPos = 1;
					iStep++;
				}
			}
			iPos++;
		}
		else if ( isSeparator(*pPtr) )
		{
			iPos = 0;
		}
		else 
		{
			//OBJ USES 1 Base ARRAYS!!!!
			const int iVal = atoi( pPtr );

			// increment iStep position based off of the sign and # of digits
			int tmp = iVal;
			if (iVal < 0)
			    ++iStep;
			while ( ( tmp = tmp / 10 )!=0 )
				++iStep;

			if ( iVal > 0 )
			{
				// Store parsed index
				if ( 0 == iPos )
				{
					pIndices->push_back( iVal-1 );
				}
				else if ( 1 == iPos )
				{	
					pTexID->push_back( iVal-1 );
				}
				else if ( 2 == iPos )
				{
					pNormalID->push_back( iVal-1 );
					hasNormal = true;
				}
				else
				{
					reportErrorTokenInFace();
				}
			}
			else if ( iVal < 0 )
			{
				// Store relatively index
				if ( 0 == iPos )
				{
					pIndices->push_back( vSize + iVal );
				}
				else if ( 1 == iPos )
				{
					pTexID->push_back( vtSize + iVal );
				}
				else if ( 2 == iPos )
				{
					pNormalID->push_back( vnSize + iVal );
					hasNormal = true;
				}
				else
				{
					reportErrorTokenInFace();
				}
			}
		}
		pPtr += iStep;
	}

	if ( pIndices->empty() ) 
	{
		DefaultLogger::get()->error("Obj: Ignoring empty face");
		m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
		return;
	}

	ObjFile::Face *face = new ObjFile::Face( pIndices, pNormalID, pTexID, type );
	
	// Set active material, if one set
	if (NULL != m_pModel->m_pCurrentMaterial) 
		face->m_pMaterial = m_pModel->m_pCurrentMaterial;
	else 
		face->m_pMaterial = m_pModel->m_pDefaultMaterial;

	// Create a default object, if nothing is there
	if ( NULL == m_pModel->m_pCurrent )
		createObject( "defaultobject" );
	
	// Assign face to mesh
	if ( NULL == m_pModel->m_pCurrentMesh )
	{
		createMesh();
	}
	
	// Store the face
	m_pModel->m_pCurrentMesh->m_Faces.push_back( face );
	m_pModel->m_pCurrentMesh->m_uiNumIndices += (unsigned int)face->m_pVertices->size();
	m_pModel->m_pCurrentMesh->m_uiUVCoordinates[ 0 ] += (unsigned int)face->m_pTexturCoords[0].size(); 
	if( !m_pModel->m_pCurrentMesh->m_hasNormals && hasNormal ) 
	{
		m_pModel->m_pCurrentMesh->m_hasNormals = true;
	}
	// Skip the rest of the line
	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Get values for a new material description
void ObjFileParser::getMaterialDesc()
{
	// Each material request a new object.
	// Sometimes the object is already created (see 'o' tag by example), but it is not initialized !
	// So, we create a new object only if the current on is already initialized !
	if (m_pModel->m_pCurrent != NULL &&
		(	m_pModel->m_pCurrent->m_Meshes.size() > 1 ||
			(m_pModel->m_pCurrent->m_Meshes.size() == 1 && m_pModel->m_Meshes[m_pModel->m_pCurrent->m_Meshes[0]]->m_Faces.size() != 0)	)
		)
		m_pModel->m_pCurrent = NULL;

	// Get next data for material data
	m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
	if (m_DataIt == m_DataItEnd)
		return;

	char *pStart = &(*m_DataIt);
	while ( m_DataIt != m_DataItEnd && !isSeparator(*m_DataIt) )
		++m_DataIt;

	// Get name
	std::string strName(pStart, &(*m_DataIt));
	if ( strName.empty())
		return;

	// Search for material
	std::map<std::string, ObjFile::Material*>::iterator it = m_pModel->m_MaterialMap.find( strName );
	if ( it == m_pModel->m_MaterialMap.end() )
	{
		// Not found, use default material
		m_pModel->m_pCurrentMaterial = m_pModel->m_pDefaultMaterial;
		DefaultLogger::get()->error("OBJ: failed to locate material " + strName + ", skipping");
	}
	else
	{
		// Found, using detected material
		m_pModel->m_pCurrentMaterial = (*it).second;
		if ( needsNewMesh( strName ))
		{
			createMesh();	
		}
		m_pModel->m_pCurrentMesh->m_uiMaterialIndex = getMaterialIndex( strName );
	}

	// Skip rest of line
	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Get a comment, values will be skipped
void ObjFileParser::getComment()
{
	while (m_DataIt != m_DataItEnd)
	{
		if ( '\n' == (*m_DataIt))
		{
			++m_DataIt;
			break;
		}
		else
		{
			++m_DataIt;
		}
	}
}

// -------------------------------------------------------------------
//	Get material library from file.
void ObjFileParser::getMaterialLib()
{
	// Translate tuple
	m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
	if (m_DataIt ==  m_DataItEnd)
		return;
	
	char *pStart = &(*m_DataIt);
	while (m_DataIt != m_DataItEnd && !isNewLine(*m_DataIt))
		m_DataIt++;

	// Check for existence
	const std::string strMatName(pStart, &(*m_DataIt));
	IOStream *pFile = m_pIO->Open(strMatName);

	if (!pFile )
	{
		DefaultLogger::get()->error("OBJ: Unable to locate material file " + strMatName);
		m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
		return;
	}

	// Import material library data from file
	std::vector<char> buffer;
	BaseImporter::TextFileToBuffer(pFile,buffer);
	m_pIO->Close( pFile );

	// Importing the material library 
	ObjFileMtlImporter mtlImporter( buffer, strMatName, m_pModel );			
}

// -------------------------------------------------------------------
//	Set a new material definition as the current material.
void ObjFileParser::getNewMaterial()
{
	m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
	m_DataIt = getNextWord<DataArrayIt>(m_DataIt, m_DataItEnd);
	if ( m_DataIt == m_DataItEnd )
		return;

	char *pStart = &(*m_DataIt);
	std::string strMat( pStart, *m_DataIt );
	while ( m_DataIt != m_DataItEnd && isSeparator( *m_DataIt ) )
		m_DataIt++;
	std::map<std::string, ObjFile::Material*>::iterator it = m_pModel->m_MaterialMap.find( strMat );
	if ( it == m_pModel->m_MaterialMap.end() )
	{
		// Show a warning, if material was not found
		DefaultLogger::get()->warn("OBJ: Unsupported material requested: " + strMat);
		m_pModel->m_pCurrentMaterial = m_pModel->m_pDefaultMaterial;
	}
	else
	{
		// Set new material
		if ( needsNewMesh( strMat ) )
		{
			createMesh();	
		}
		m_pModel->m_pCurrentMesh->m_uiMaterialIndex = getMaterialIndex( strMat );
	}

	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
int ObjFileParser::getMaterialIndex( const std::string &strMaterialName )
{
	int mat_index = -1;
	if ( strMaterialName.empty() )
		return mat_index;
	for (size_t index = 0; index < m_pModel->m_MaterialLib.size(); ++index)
	{
		if ( strMaterialName == m_pModel->m_MaterialLib[ index ])
		{
			mat_index = (int)index;
			break;
		}
	}
	return mat_index;
}

// -------------------------------------------------------------------
//	Getter for a group name.  
void ObjFileParser::getGroupName()
{
	std::string strGroupName;
   
	m_DataIt = getName<DataArrayIt>(m_DataIt, m_DataItEnd, strGroupName);
	if ( isEndOfBuffer( m_DataIt, m_DataItEnd ) )
		return;

	// Change active group, if necessary
	if ( m_pModel->m_strActiveGroup != strGroupName )
	{
		// Search for already existing entry
		ObjFile::Model::ConstGroupMapIt it = m_pModel->m_Groups.find(strGroupName);
		
		// We are mapping groups into the object structure
		createObject( strGroupName );
		
		// New group name, creating a new entry
		if (it == m_pModel->m_Groups.end())
		{
			std::vector<unsigned int> *pFaceIDArray = new std::vector<unsigned int>;
			m_pModel->m_Groups[ strGroupName ] = pFaceIDArray;
			m_pModel->m_pGroupFaceIDs = (pFaceIDArray);
		}
		else
		{
			m_pModel->m_pGroupFaceIDs = (*it).second;
		}
		m_pModel->m_strActiveGroup = strGroupName;
	}
	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Not supported
void ObjFileParser::getGroupNumber()
{
	// Not used

	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Not supported
void ObjFileParser::getGroupNumberAndResolution()
{
	// Not used

	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}

// -------------------------------------------------------------------
//	Stores values for a new object instance, name will be used to 
//	identify it.
void ObjFileParser::getObjectName()
{
	m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
	if (m_DataIt == m_DataItEnd)
		return;
	char *pStart = &(*m_DataIt);
	while ( m_DataIt != m_DataItEnd && !isSeparator( *m_DataIt ) )
		++m_DataIt;

	std::string strObjectName(pStart, &(*m_DataIt));
	if (!strObjectName.empty()) 
	{
		// Reset current object
		m_pModel->m_pCurrent = NULL;
		
		// Search for actual object
		for (std::vector<ObjFile::Object*>::const_iterator it = m_pModel->m_Objects.begin();
			it != m_pModel->m_Objects.end();
			++it)
		{
			if ((*it)->m_strObjName == strObjectName)
			{
				m_pModel->m_pCurrent = *it;
				break;
			}
		}

		// Allocate a new object, if current one was not found before
		if ( NULL == m_pModel->m_pCurrent )
			createObject(strObjectName);
	}
	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
}
// -------------------------------------------------------------------
//	Creates a new object instance
void ObjFileParser::createObject(const std::string &strObjectName)
{
	ai_assert( NULL != m_pModel );
	//ai_assert( !strObjectName.empty() );

	m_pModel->m_pCurrent = new ObjFile::Object;
	m_pModel->m_pCurrent->m_strObjName = strObjectName;
	m_pModel->m_Objects.push_back( m_pModel->m_pCurrent );
	

	createMesh();

	if( m_pModel->m_pCurrentMaterial )
	{
		m_pModel->m_pCurrentMesh->m_uiMaterialIndex = 
			getMaterialIndex( m_pModel->m_pCurrentMaterial->MaterialName.data );
		m_pModel->m_pCurrentMesh->m_pMaterial = m_pModel->m_pCurrentMaterial;
	}		
}
// -------------------------------------------------------------------
//	Creates a new mesh
void ObjFileParser::createMesh()
{
	ai_assert( NULL != m_pModel );
	m_pModel->m_pCurrentMesh = new ObjFile::Mesh;
	m_pModel->m_Meshes.push_back( m_pModel->m_pCurrentMesh );
	unsigned int meshId = m_pModel->m_Meshes.size()-1;
	if ( NULL != m_pModel->m_pCurrent )
	{
		m_pModel->m_pCurrent->m_Meshes.push_back( meshId );
	}
	else
	{
		DefaultLogger::get()->error("OBJ: No object detected to attach a new mesh instance.");
	}
}

// -------------------------------------------------------------------
//	Returns true, if a new mesh must be created.
bool ObjFileParser::needsNewMesh( const std::string &rMaterialName )
{
	if(m_pModel->m_pCurrentMesh == 0)
	{
		// No mesh data yet
		return true;
	}
	bool newMat = false;
	int matIdx = getMaterialIndex( rMaterialName );
	int curMatIdx = m_pModel->m_pCurrentMesh->m_uiMaterialIndex;
	if ( curMatIdx != int(ObjFile::Mesh::NoMaterial) || curMatIdx != matIdx )
	{
		// New material -> only one material per mesh, so we need to create a new 
		// material
		newMat = true;
	}
	return newMat;
}

// -------------------------------------------------------------------
//	Shows an error in parsing process.
void ObjFileParser::reportErrorTokenInFace()
{		
	m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
	DefaultLogger::get()->error("OBJ: Not supported token in face description detected");
}

// -------------------------------------------------------------------

}	// Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
