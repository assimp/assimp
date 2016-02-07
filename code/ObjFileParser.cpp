/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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


#ifndef ASSIMP_BUILD_NO_OBJ_IMPORTER

#include "ObjFileParser.h"
#include "ObjFileMtlImporter.h"
#include "ObjTools.h"
#include "ObjFileData.h"
#include "ParsingUtils.h"
#include "DefaultIOSystem.h"
#include "BaseImporter.h"
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <assimp/Importer.hpp>
#include <cstdlib>


namespace Assimp {

const std::string ObjFileParser::DEFAULT_MATERIAL = AI_DEFAULT_MATERIAL_NAME;

// -------------------------------------------------------------------
//  Constructor with loaded data and directories.
ObjFileParser::ObjFileParser(const std::vector<char> &data,const std::string &modelName, IOSystem *io, ProgressHandler* progress ) :
    m_DataBuffer(data),
    m_pModel(NULL),
    m_uiLine(0),
    m_pIO( io ),
    m_progress(progress)
{
    // Create the model instance to store all the data
    m_pModel = new ObjFile::Model();
    m_pModel->m_ModelName = modelName;

    // create default material and store it
    m_pModel->m_pDefaultMaterial = new ObjFile::Material;
    m_pModel->m_pDefaultMaterial->MaterialName.Set( DEFAULT_MATERIAL );
    m_pModel->m_MaterialLib.push_back( DEFAULT_MATERIAL );
    m_pModel->m_MaterialMap[ DEFAULT_MATERIAL ] = m_pModel->m_pDefaultMaterial;

    // Start parsing the file
    parseFile();
}

// -------------------------------------------------------------------
//  Destructor
ObjFileParser::~ObjFileParser()
{
    delete m_pModel;
    m_pModel = NULL;
}

// -------------------------------------------------------------------
//  Returns a pointer to the model instance.
ObjFile::Model *ObjFileParser::GetModel() const
{
    return m_pModel;
}

// -------------------------------------------------------------------
//  File parsing method.
void ObjFileParser::parseFile()
{
    //! Iterator to current position in buffer
    ConstDataArrayIt dataIt = m_DataBuffer.begin();
    //! Iterator to end position of buffer
    const ConstDataArrayIt dataItEnd = m_DataBuffer.end();

    if (dataIt == dataItEnd)
        return;

    //! Helper buffer
    std::vector<char> helperBuffer;

    // only update every 100KB or it'll be too slow
    const unsigned int updateProgressEveryBytes = 100 * 1024;
    unsigned int progressCounter = 0;
    const unsigned int bytesToProcess = std::distance(dataIt, dataItEnd);
    const unsigned int progressTotal = bytesToProcess;
    unsigned int processed = 0;

    ConstDataArrayIt lastDataIt = dataIt;

    while (dataIt != dataItEnd)
    {
        // Handle progress reporting
        processed += std::distance(lastDataIt, dataIt);
        lastDataIt = dataIt;
        if (processed > (progressCounter * updateProgressEveryBytes))
        {
            progressCounter++;
            m_progress->UpdateFileRead(processed, progressTotal);
        }

        // take the next line and copy it into a helper buffer
        // all subsequant parsing should use the helper buffer
        copyNextLine(helperBuffer, dataIt, dataItEnd);

        if (helperBuffer[0] == '\0')
        {
            // either empty line, or end of file
            if (dataIt == dataItEnd)
            {
                // end of file
                return;
            }
            // else empty line, so skip
            continue;
        }

        //! Iterator to current position in helper buffer
        ConstDataArrayIt helperIt = helperBuffer.begin();
        //! Iterator to end of helper buffer
        const ConstDataArrayIt helperItEnd = helperBuffer.end();

        // parse line
        switch (*helperIt)
        {
        case 'v': // Parse a vertex texture coordinate
            {
                if (++helperIt != helperItEnd) {
                    if (*helperIt == ' ' || *helperIt == '\t') {
                        // read in vertex definition
                        getVector3(m_pModel->m_Vertices, ++helperIt, helperItEnd);
                    } else if (*helperIt == 't') {
                        // read in texture coordinate ( 2D or 3D )
                        getVector( m_pModel->m_TextureCoord, ++helperIt, helperItEnd);
                    } else if (*helperIt == 'n') {
                        // Read in normal vector definition
                        getVector3( m_pModel->m_Normals, ++helperIt, helperItEnd);
                    }
                    // else unknown line
                }
                // else no more data
            }
            break;

        case 'p': // Parse a face, line or point statement
        case 'l':
        case 'f':
            {
                aiPrimitiveType primType = (*helperIt == 'f') ? aiPrimitiveType_POLYGON :
                                           (*helperIt == 'l') ? aiPrimitiveType_LINE :
                                                                aiPrimitiveType_POINT;
                getFace(primType, ++helperIt, helperItEnd);
            }
            break;

        case '#': // Parse a comment
            {
                // just ignore it
            }
            break;

        case 'u': // Parse a material desc. setter
            {
                getMaterialDesc(++helperIt, helperItEnd);
            }
            break;

        case 'm': // Parse a material library or merging group ('mg')
            {
                if (*(helperIt + 1) == 'g')
                    getGroupNumberAndResolution();
                else {
                    getMaterialLib(++helperIt, helperItEnd);
                }
            }
            break;

        case 'g': // Parse group name
            {
                getGroupName(++helperIt, helperItEnd);
            }
            break;

        case 's': // Parse group number
            {
                getGroupNumber();
            }
            break;

        case 'o': // Parse object name
            {
                getObjectName(++helperIt, helperItEnd);
            }
            break;
        default:
            {
                // unknown line, skip
            }
            break;
        }
    }
}

// -------------------------------------------------------------------
//  Copy the next word in a temporary buffer
bool ObjFileParser::getNextFloat(ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd, float &result)
{
    std::vector<char> tmpBuffer;
    dataIt = getNextWord<ConstDataArrayIt>(dataIt, dataItEnd);
    while( dataIt != dataItEnd && !IsSpaceOrNewLine( *dataIt ) ) {
        tmpBuffer.push_back(*dataIt);
        ++dataIt;
    }

    if (tmpBuffer.size() == 0)
    {
        return false;
    }

    tmpBuffer.push_back('\0');

    result = fast_atof(&tmpBuffer[0]);
    return true;
}

// -------------------------------------------------------------------
// Copy the next line into a temporary buffer
void ObjFileParser::copyNextLine(std::vector<char> &buffer, ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    // clear old data out. This is O(1) since a char is a "trivially-destructable type"
    buffer.clear();
    // some OBJ files have line continuations using \ (such as in C++ et al)
    bool continuation = false;
    for (;dataIt != dataItEnd; ++dataIt)
    {
        const char c = *dataIt;
        if (c == '\\') {
            continuation = true;
            continue;
        }

        if (c == '\n' || c == '\r') {
            if(continuation) {
                buffer.push_back(' ');
                continue;
            }
            // end of line, update dataIt to point to the start of the next
            dataIt = skipLine<ConstDataArrayIt>(dataIt, dataItEnd, m_uiLine );
            break;
        }

        continuation = false;
        buffer.push_back(c);
    }
    // add a NULL terminator
    buffer.push_back('\0');
}

// -------------------------------------------------------------------
void ObjFileParser::getVector( std::vector<aiVector3D> &point3d_array, ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd) {
    size_t numComponents( 0 );
    float components[3];
    while( dataIt != dataItEnd ) {
        if (!getNextFloat(dataIt, dataItEnd, components[numComponents]))
        {
            // failed
            break;
        }
        numComponents++;
        if (numComponents == 3)
        {
            // 3 is the max
            break;
        }
    }

    if( 2 == numComponents ) {
        components[2] = 0.0f;
    } else if( 3 != numComponents ) {
        throw DeadlyImportError( "OBJ: Invalid number of components" );
    }
    point3d_array.push_back( aiVector3D( components[0], components[1], components[2] ) );
}

// -------------------------------------------------------------------
//  Get values for a new 3D vector instance
void ObjFileParser::getVector3(std::vector<aiVector3D> &point3d_array, ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd) {
    float x, y, z;
    if (!getNextFloat(dataIt, dataItEnd, x) ||
        !getNextFloat(dataIt, dataItEnd, y) ||
        !getNextFloat(dataIt, dataItEnd, z))
    {
        throw DeadlyImportError( "OBJ: Invalid number of components" );
    }
    else
    {
        point3d_array.push_back( aiVector3D( x, y, z ) );
    }
}

// -------------------------------------------------------------------
//  Get values for a new 2D vector instance
void ObjFileParser::getVector2( std::vector<aiVector2D> &point2d_array, ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd) {
    float x, y;
    if (!getNextFloat(dataIt, dataItEnd, x) ||
        !getNextFloat(dataIt, dataItEnd, y))
    {
        throw DeadlyImportError( "OBJ: Invalid number of components" );
    }
    else
    {
        point2d_array.push_back( aiVector2D( x, y ) );
    }
}

// -------------------------------------------------------------------
//  Get values for a new face instance
void ObjFileParser::getFace(aiPrimitiveType type, ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    ConstDataArrayIt pPtr = getNextToken<ConstDataArrayIt>(dataIt, dataItEnd);
    if (pPtr == dataItEnd || *pPtr == '\0')
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
    while (pPtr != dataItEnd)
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
        else if( IsSpaceOrNewLine( *pPtr ) )
        {
            iPos = 0;
        }
        else
        {
            //OBJ USES 1 Base ARRAYS!!!!
            const int iVal = atoi( &pPtr[0] );

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

    if ( pIndices->empty() ) {
        DefaultLogger::get()->error("Obj: Ignoring empty face");

        // clean up
        delete pNormalID;
        delete pTexID;
        delete pIndices;

        return;
    }

    ObjFile::Face *face = new ObjFile::Face( pIndices, pNormalID, pTexID, type );

    // Set active material, if one set
    if( NULL != m_pModel->m_pCurrentMaterial ) {
        face->m_pMaterial = m_pModel->m_pCurrentMaterial;
    } else {
        face->m_pMaterial = m_pModel->m_pDefaultMaterial;
    }

    // Create a default object, if nothing is there
    if( NULL == m_pModel->m_pCurrent ) {
        createObject( "defaultobject" );
    }

    // Assign face to mesh
    if ( NULL == m_pModel->m_pCurrentMesh ) {
        createMesh( "defaultobject" );
    }

    // Store the face
    m_pModel->m_pCurrentMesh->m_Faces.push_back( face );
    m_pModel->m_pCurrentMesh->m_uiNumIndices += (unsigned int)face->m_pVertices->size();
    m_pModel->m_pCurrentMesh->m_uiUVCoordinates[ 0 ] += (unsigned int)face->m_pTexturCoords[0].size();
    if( !m_pModel->m_pCurrentMesh->m_hasNormals && hasNormal ) {
        m_pModel->m_pCurrentMesh->m_hasNormals = true;
    }
}

// -------------------------------------------------------------------
//  Get values for a new material description
void ObjFileParser::getMaterialDesc(ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    // Get next data for material data
    dataIt = getNextToken<ConstDataArrayIt>(dataIt, dataItEnd);
    if (dataIt == dataItEnd) {
        return;
    }

    // In some cases we should ignore this 'usemtl' command, this variable helps us to do so
    bool skip = false;

    // Get name
    std::string strName(dataIt, dataItEnd);
    strName = trim_whitespaces(strName);

    if (strName.empty())
        skip = true;

    // If the current mesh has the same material, we simply ignore that 'usemtl' command
    // There is no need to create another object or even mesh here
    if (m_pModel->m_pCurrentMaterial && m_pModel->m_pCurrentMaterial->MaterialName == aiString(strName))
        skip = true;

    if (!skip)
    {
        // Search for material
        std::map<std::string, ObjFile::Material*>::iterator it = m_pModel->m_MaterialMap.find(strName);
        if (it == m_pModel->m_MaterialMap.end())
        {
            // Not found, use default material
            m_pModel->m_pCurrentMaterial = m_pModel->m_pDefaultMaterial;
            DefaultLogger::get()->error("OBJ: failed to locate material " + strName + ", skipping");
            strName = m_pModel->m_pDefaultMaterial->MaterialName.C_Str();
        }
        else
        {
            // Found, using detected material
            m_pModel->m_pCurrentMaterial = (*it).second;
        }

        if (needsNewMesh(strName))
            createMesh(strName);

        m_pModel->m_pCurrentMesh->m_uiMaterialIndex = getMaterialIndex(strName);
    }
}

// -------------------------------------------------------------------
//  Get material library from file.
void ObjFileParser::getMaterialLib(ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    // Translate tuple
    dataIt = getNextToken<ConstDataArrayIt>(dataIt, dataItEnd);
    if( dataIt == dataItEnd ) {
        return;
    }

    // Check for existence
    const std::string strMatName(dataIt, dataItEnd);
    std::string absName;
    if ( m_pIO->StackSize() > 0 ) {
        std::string path = m_pIO->CurrentDirectory();
        if ( '/' != *path.rbegin() ) {
          path += '/';
        }
        absName = path + strMatName;
    } else {
        absName = strMatName;
    }
    IOStream *pFile = m_pIO->Open( absName );

    if (!pFile ) {
        DefaultLogger::get()->error( "OBJ: Unable to locate material file " + strMatName );
        return;
    }

    // Import material library data from file.
    // Some exporters (e.g. Silo) will happily write out empty
    // material files if the model doesn't use any materials, so we
    // allow that.
    std::vector<char> buffer;
    BaseImporter::TextFileToBuffer( pFile, buffer, BaseImporter::ALLOW_EMPTY );
    m_pIO->Close( pFile );

    // Importing the material library
    ObjFileMtlImporter mtlImporter( buffer, strMatName, m_pModel );
}

// -------------------------------------------------------------------
//  Set a new material definition as the current material.
void ObjFileParser::getNewMaterial(ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    dataIt = getNextToken<ConstDataArrayIt>(dataIt, dataItEnd);
    dataIt = getNextWord<ConstDataArrayIt>(dataIt, dataItEnd);
    if( dataIt == dataItEnd ) {
        return;
    }

    const char *pStart = &(*dataIt);
    while( dataIt != dataItEnd && IsSpaceOrNewLine( *dataIt ) ) {
        ++dataIt;
    }
    std::string strMat( pStart, *dataIt );
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
            createMesh( strMat );
        }
        m_pModel->m_pCurrentMesh->m_uiMaterialIndex = getMaterialIndex( strMat );
    }
}

// -------------------------------------------------------------------
int ObjFileParser::getMaterialIndex( const std::string &strMaterialName )
{
    int mat_index = -1;
    if( strMaterialName.empty() ) {
        return mat_index;
    }
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
//  Getter for a group name.
void ObjFileParser::getGroupName(ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    std::string strGroupName;

    dataIt = getNextToken<ConstDataArrayIt>(dataIt, dataItEnd);
    dataIt = getName<ConstDataArrayIt>(dataIt, dataItEnd, strGroupName);
    if( isEndOfBuffer( dataIt, dataItEnd ) ) {
        return;
    }

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
}

// -------------------------------------------------------------------
//  Not supported
void ObjFileParser::getGroupNumber()
{
    // Not used
}

// -------------------------------------------------------------------
//  Not supported
void ObjFileParser::getGroupNumberAndResolution()
{
    // Not used
}

// -------------------------------------------------------------------
//  Stores values for a new object instance, name will be used to
//  identify it.
void ObjFileParser::getObjectName(ConstDataArrayIt &dataIt, const ConstDataArrayIt dataItEnd)
{
    dataIt = getNextToken<ConstDataArrayIt>(dataIt, dataItEnd);
    if( dataIt == dataItEnd ) {
        return;
    }

    std::string strObjectName(dataIt, dataItEnd);
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
        if( NULL == m_pModel->m_pCurrent ) {
            createObject( strObjectName );
        }
    }
}
// -------------------------------------------------------------------
//  Creates a new object instance
void ObjFileParser::createObject(const std::string &objName)
{
    ai_assert( NULL != m_pModel );

    m_pModel->m_pCurrent = new ObjFile::Object;
    m_pModel->m_pCurrent->m_strObjName = objName;
    m_pModel->m_Objects.push_back( m_pModel->m_pCurrent );

    createMesh( objName  );

    if( m_pModel->m_pCurrentMaterial )
    {
        m_pModel->m_pCurrentMesh->m_uiMaterialIndex =
            getMaterialIndex( m_pModel->m_pCurrentMaterial->MaterialName.data );
        m_pModel->m_pCurrentMesh->m_pMaterial = m_pModel->m_pCurrentMaterial;
    }
}
// -------------------------------------------------------------------
//  Creates a new mesh
void ObjFileParser::createMesh( const std::string &meshName )
{
    ai_assert( NULL != m_pModel );
    m_pModel->m_pCurrentMesh = new ObjFile::Mesh( meshName );
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
//  Returns true, if a new mesh must be created.
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
    if ( curMatIdx != int(ObjFile::Mesh::NoMaterial) && curMatIdx != matIdx )
    {
        // New material -> only one material per mesh, so we need to create a new
        // material
        newMat = true;
    }
    return newMat;
}

// -------------------------------------------------------------------
//  Shows an error in parsing process.
void ObjFileParser::reportErrorTokenInFace()
{
    DefaultLogger::get()->error("OBJ: Not supported token in face description detected");
}

// -------------------------------------------------------------------

}   // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
