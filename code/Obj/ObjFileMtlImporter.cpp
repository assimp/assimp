/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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

#include <stdlib.h>
#include "ObjFileMtlImporter.h"
#include "ObjTools.h"
#include "ObjFileData.h"
#include <assimp/fast_atof.h>
#include <assimp/ParsingUtils.h>
#include <assimp/material.h>
#include <assimp/DefaultLogger.hpp>

namespace Assimp    {

// Material specific token (case insensitive compare)
static const char *DiffuseTexture       = "map_Kd";
static const char *AmbientTexture       = "map_Ka";
static const char *SpecularTexture      = "map_Ks";
static const char *OpacityTexture       = "map_d";
static const char *EmissiveTexture1     = "map_emissive";
static const char *EmissiveTexture2     = "map_Ke";
static const char *BumpTexture1         = "map_bump";
static const char *BumpTexture2         = "bump";
static const char *NormalTexture        = "map_Kn";
static const char *ReflectionTexture    = "refl";
static const char *DisplacementTexture1 = "map_disp";
static const char *DisplacementTexture2 = "disp";
static const char *SpecularityTexture   = "map_ns";

// texture option specific token
static const char *BlendUOption       = "-blendu";
static const char *BlendVOption       = "-blendv";
static const char *BoostOption        = "-boost";
static const char *ModifyMapOption    = "-mm";
static const char *OffsetOption       = "-o";
static const char *ScaleOption        = "-s";
static const char *TurbulenceOption   = "-t";
static const char *ResolutionOption   = "-texres";
static const char *ClampOption        = "-clamp";
static const char *BumpOption         = "-bm";
static const char *ChannelOption      = "-imfchan";
static const char *TypeOption         = "-type";

// -------------------------------------------------------------------
//  Constructor
ObjFileMtlImporter::ObjFileMtlImporter( std::vector<char> &buffer,
                                       const std::string &,
                                       ObjFile::Model *pModel ) :
    m_DataIt( buffer.begin() ),
    m_DataItEnd( buffer.end() ),
    m_pModel( pModel ),
    m_uiLine( 0 )
{
    ai_assert( NULL != m_pModel );
    if ( NULL == m_pModel->m_pDefaultMaterial )
    {
        m_pModel->m_pDefaultMaterial = new ObjFile::Material;
        m_pModel->m_pDefaultMaterial->MaterialName.Set( "default" );
    }
    load();
}

// -------------------------------------------------------------------
//  Destructor
ObjFileMtlImporter::~ObjFileMtlImporter()
{
    // empty
}

// -------------------------------------------------------------------
//  Loads the material description
void ObjFileMtlImporter::load()
{
    if ( m_DataIt == m_DataItEnd )
        return;

    while ( m_DataIt != m_DataItEnd )
    {
        switch (*m_DataIt)
        {
        case 'k':
        case 'K':
            {
                ++m_DataIt;
                if (*m_DataIt == 'a') // Ambient color
                {
                    ++m_DataIt;
                    getColorRGBA( &m_pModel->m_pCurrentMaterial->ambient );
                }
                else if (*m_DataIt == 'd')  // Diffuse color
                {
                    ++m_DataIt;
                    getColorRGBA( &m_pModel->m_pCurrentMaterial->diffuse );
                }
                else if (*m_DataIt == 's')
                {
                    ++m_DataIt;
                    getColorRGBA( &m_pModel->m_pCurrentMaterial->specular );
                }
                else if (*m_DataIt == 'e')
                {
                    ++m_DataIt;
                    getColorRGBA( &m_pModel->m_pCurrentMaterial->emissive );
                }
                m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
            }
            break;
        case 'T':
            {
                ++m_DataIt;
                if (*m_DataIt == 'f') // Material transmission
                {
                    ++m_DataIt;
                    getColorRGBA( &m_pModel->m_pCurrentMaterial->transparent);
                }
                m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
            }
            break;
        case 'd':
            {
                if( *(m_DataIt+1) == 'i' && *( m_DataIt + 2 ) == 's' && *( m_DataIt + 3 ) == 'p' ) {
                    // A displacement map
                    getTexture();
                } else {
                    // Alpha value
                    ++m_DataIt;
                    getFloatValue( m_pModel->m_pCurrentMaterial->alpha );
                    m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
                }
            }
            break;

        case 'N':
        case 'n':
            {
                ++m_DataIt;
                switch(*m_DataIt)
                {
                case 's':   // Specular exponent
                    ++m_DataIt;
                    getFloatValue(m_pModel->m_pCurrentMaterial->shineness);
                    break;
                case 'i':   // Index Of refraction
                    ++m_DataIt;
                    getFloatValue(m_pModel->m_pCurrentMaterial->ior);
                    break;
                case 'e':   // New material
                    createMaterial();
                    break;
                }
                m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
            }
            break;

        case 'm':   // Texture
        case 'b':   // quick'n'dirty - for 'bump' sections
        case 'r':   // quick'n'dirty - for 'refl' sections
            {
                getTexture();
                m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
            }
            break;

        case 'i':   // Illumination model
            {
                m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
                getIlluminationModel( m_pModel->m_pCurrentMaterial->illumination_model );
                m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
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
//  Loads a color definition
void ObjFileMtlImporter::getColorRGBA( aiColor3D *pColor )
{
    ai_assert( NULL != pColor );

    ai_real r( 0.0 ), g( 0.0 ), b( 0.0 );
    m_DataIt = getFloat<DataArrayIt>( m_DataIt, m_DataItEnd, r );
    pColor->r = r;

    // we have to check if color is default 0 with only one token
    if( !IsLineEnd( *m_DataIt ) ) {
        m_DataIt = getFloat<DataArrayIt>( m_DataIt, m_DataItEnd, g );
        m_DataIt = getFloat<DataArrayIt>( m_DataIt, m_DataItEnd, b );
    }
    pColor->g = g;
    pColor->b = b;
}

// -------------------------------------------------------------------
//  Loads the kind of illumination model.
void ObjFileMtlImporter::getIlluminationModel( int &illum_model )
{
    m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
    illum_model = atoi(m_buffer);
}

// -------------------------------------------------------------------
//  Loads a single float value.
void ObjFileMtlImporter::getFloatValue( ai_real &value )
{
    m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
    value = (ai_real) fast_atof(m_buffer);
}

// -------------------------------------------------------------------
//  Creates a material from loaded data.
void ObjFileMtlImporter::createMaterial()
{
    std::string line( "" );
    while( !IsLineEnd( *m_DataIt ) ) {
        line += *m_DataIt;
        ++m_DataIt;
    }

    std::vector<std::string> token;
    const unsigned int numToken = tokenize<std::string>( line, token, " \t" );
    std::string name( "" );
    if ( numToken == 1 ) {
        name = AI_DEFAULT_MATERIAL_NAME;
    } else {
        // skip newmtl and all following white spaces
        std::size_t first_ws_pos = line.find_first_of(" \t");
        std::size_t first_non_ws_pos = line.find_first_not_of(" \t", first_ws_pos);
        if (first_non_ws_pos != std::string::npos) {
            name = line.substr(first_non_ws_pos);
        }
    }

    name = trim_whitespaces(name);

    std::map<std::string, ObjFile::Material*>::iterator it = m_pModel->m_MaterialMap.find( name );
    if ( m_pModel->m_MaterialMap.end() == it) {
        // New Material created
        m_pModel->m_pCurrentMaterial = new ObjFile::Material();
        m_pModel->m_pCurrentMaterial->MaterialName.Set( name );
        m_pModel->m_MaterialLib.push_back( name );
        m_pModel->m_MaterialMap[ name ] = m_pModel->m_pCurrentMaterial;

        if (m_pModel->m_pCurrentMesh) {
            m_pModel->m_pCurrentMesh->m_uiMaterialIndex = static_cast<unsigned int>(m_pModel->m_MaterialLib.size() - 1);
        }
    } else {
        // Use older material
        m_pModel->m_pCurrentMaterial = (*it).second;
    }
}

// -------------------------------------------------------------------
//  Gets a texture name from data.
void ObjFileMtlImporter::getTexture() {
    aiString *out( NULL );
    int clampIndex = -1;

    const char *pPtr( &(*m_DataIt) );
    if ( !ASSIMP_strincmp( pPtr, DiffuseTexture, strlen( DiffuseTexture ) ) ) {
        // Diffuse texture
        out = & m_pModel->m_pCurrentMaterial->texture;
        clampIndex = ObjFile::Material::TextureDiffuseType;
    } else if ( !ASSIMP_strincmp( pPtr,AmbientTexture, strlen( AmbientTexture ) ) ) {
        // Ambient texture
        out = & m_pModel->m_pCurrentMaterial->textureAmbient;
        clampIndex = ObjFile::Material::TextureAmbientType;
    } else if ( !ASSIMP_strincmp( pPtr, SpecularTexture, strlen(SpecularTexture ) ) ) {
        // Specular texture
        out = & m_pModel->m_pCurrentMaterial->textureSpecular;
        clampIndex = ObjFile::Material::TextureSpecularType;
    } else if ( !ASSIMP_strincmp( pPtr, DisplacementTexture1, strlen( DisplacementTexture1 ) ) ||
                !ASSIMP_strincmp( pPtr, DisplacementTexture2, strlen( DisplacementTexture2 ) ) ) {
        // Displacement texture
        out = &m_pModel->m_pCurrentMaterial->textureDisp;
        clampIndex = ObjFile::Material::TextureDispType;
    } else if ( !ASSIMP_strincmp( pPtr, OpacityTexture, strlen(OpacityTexture ) ) ) {
        // Opacity texture
        out = & m_pModel->m_pCurrentMaterial->textureOpacity;
        clampIndex = ObjFile::Material::TextureOpacityType;
    } else if ( !ASSIMP_strincmp( pPtr, EmissiveTexture1, strlen( EmissiveTexture1 ) ) ||
                !ASSIMP_strincmp( pPtr, EmissiveTexture2, strlen( EmissiveTexture2 ) ) ) {
        // Emissive texture
        out = & m_pModel->m_pCurrentMaterial->textureEmissive;
        clampIndex = ObjFile::Material::TextureEmissiveType;
    } else if ( !ASSIMP_strincmp( pPtr, BumpTexture1, strlen(BumpTexture1 ) ) ||
                !ASSIMP_strincmp( pPtr, BumpTexture2, strlen(BumpTexture2 ) ) ) {
        // Bump texture
        out = & m_pModel->m_pCurrentMaterial->textureBump;
        clampIndex = ObjFile::Material::TextureBumpType;
    } else if ( !ASSIMP_strincmp( pPtr,NormalTexture, strlen( NormalTexture ) ) ) {
        // Normal map
        out = & m_pModel->m_pCurrentMaterial->textureNormal;
        clampIndex = ObjFile::Material::TextureNormalType;
    } else if( !ASSIMP_strincmp( pPtr, ReflectionTexture, strlen(ReflectionTexture ) ) ) {
        // Reflection texture(s)
        //Do nothing here
        return;
    } else if ( !ASSIMP_strincmp( pPtr, SpecularityTexture, strlen( SpecularityTexture ) ) ) {
        // Specularity scaling (glossiness)
        out = & m_pModel->m_pCurrentMaterial->textureSpecularity;
        clampIndex = ObjFile::Material::TextureSpecularityType;
    } else {
        ASSIMP_LOG_ERROR("OBJ/MTL: Encountered unknown texture type");
        return;
    }

    bool clamp = false;
    getTextureOption(clamp, clampIndex, out);
    m_pModel->m_pCurrentMaterial->clamp[clampIndex] = clamp;

    std::string texture;
    m_DataIt = getName<DataArrayIt>( m_DataIt, m_DataItEnd, texture );
    if ( nullptr != out ) {
        out->Set( texture );
    }
}

/* /////////////////////////////////////////////////////////////////////////////
 * Texture Option
 * /////////////////////////////////////////////////////////////////////////////
 * According to http://en.wikipedia.org/wiki/Wavefront_.obj_file#Texture_options
 * Texture map statement can contains various texture option, for example:
 *
 *  map_Ka -o 1 1 1 some.png
 *  map_Kd -clamp on some.png
 *
 * So we need to parse and skip these options, and leave the last part which is
 * the url of image, otherwise we will get a wrong url like "-clamp on some.png".
 *
 * Because aiMaterial supports clamp option, so we also want to return it
 * /////////////////////////////////////////////////////////////////////////////
 */
void ObjFileMtlImporter::getTextureOption(bool &clamp, int &clampIndex, aiString *&out) {
    m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);

    // If there is any more texture option
    while (!isEndOfBuffer(m_DataIt, m_DataItEnd) && *m_DataIt == '-') {
        const char *pPtr( &(*m_DataIt) );
        //skip option key and value
        int skipToken = 1;

        if (!ASSIMP_strincmp(pPtr, ClampOption, strlen( ClampOption ))) {
            DataArrayIt it = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
            char value[3];
            CopyNextWord(it, m_DataItEnd, value, sizeof(value) / sizeof(*value));
            if (!ASSIMP_strincmp(value, "on", 2)) {
                clamp = true;
            }

            skipToken = 2;
        } else if( !ASSIMP_strincmp( pPtr, TypeOption, strlen( TypeOption ) ) ) {
            DataArrayIt it = getNextToken<DataArrayIt>( m_DataIt, m_DataItEnd );
            char value[ 12 ];
            CopyNextWord( it, m_DataItEnd, value, sizeof( value ) / sizeof( *value ) );
            if( !ASSIMP_strincmp( value, "cube_top", 8 ) ) {
                clampIndex = ObjFile::Material::TextureReflectionCubeTopType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[ 0];
            } else if( !ASSIMP_strincmp( value, "cube_bottom", 11 ) ) { 
                clampIndex = ObjFile::Material::TextureReflectionCubeBottomType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[ 1 ];
            } else if( !ASSIMP_strincmp( value, "cube_front", 10 ) ) {
                clampIndex = ObjFile::Material::TextureReflectionCubeFrontType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[2];
            } else if( !ASSIMP_strincmp( value, "cube_back", 9 ) ) {
                clampIndex = ObjFile::Material::TextureReflectionCubeBackType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[3];
            } else if( !ASSIMP_strincmp( value, "cube_left", 9 ) ) {
                clampIndex = ObjFile::Material::TextureReflectionCubeLeftType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[4];
            } else if( !ASSIMP_strincmp( value, "cube_right", 10 ) ) {
                clampIndex = ObjFile::Material::TextureReflectionCubeRightType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[5];
            } else if( !ASSIMP_strincmp( value, "sphere", 6 ) ) {
                clampIndex = ObjFile::Material::TextureReflectionSphereType;
                out = &m_pModel->m_pCurrentMaterial->textureReflection[0];
            }

            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, BlendUOption, strlen(BlendUOption))
                || !ASSIMP_strincmp(pPtr, BlendVOption, strlen(BlendVOption))
                || !ASSIMP_strincmp(pPtr, BoostOption, strlen(BoostOption))
                || !ASSIMP_strincmp(pPtr, ResolutionOption, strlen(ResolutionOption))
                || !ASSIMP_strincmp(pPtr, BumpOption, strlen(BumpOption))
                || !ASSIMP_strincmp(pPtr, ChannelOption, strlen(ChannelOption))) {
            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, ModifyMapOption, strlen(ModifyMapOption))) {
            skipToken = 3;
        } else if (  !ASSIMP_strincmp(pPtr, OffsetOption, strlen(OffsetOption ))
                || !ASSIMP_strincmp(pPtr, ScaleOption, strlen( ScaleOption ))
                || !ASSIMP_strincmp(pPtr, TurbulenceOption, strlen(TurbulenceOption))
                ) {
            skipToken = 4;
        }

        for (int i = 0; i < skipToken; ++i) {
            m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
        }
    }
}

// -------------------------------------------------------------------

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
