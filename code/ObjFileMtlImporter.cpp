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

#include "ObjFileMtlImporter.h"
#include "ObjTools.h"
#include "ObjFileData.h"
#include "fast_atof.h"

namespace Assimp	{

// Material specific token
static const std::string DiffuseTexture      = "map_kd";
static const std::string AmbientTexture      = "map_ka";
static const std::string SpecularTexture     = "map_ks";
static const std::string OpacityTexture      = "map_d";
static const std::string BumpTexture1        = "map_bump";
static const std::string BumpTexture2        = "map_Bump";
static const std::string BumpTexture3        = "bump";
static const std::string NormalTexture       = "map_Kn";
static const std::string DisplacementTexture = "disp";
static const std::string SpecularityTexture  = "map_ns";

// -------------------------------------------------------------------
//	Constructor
ObjFileMtlImporter::ObjFileMtlImporter( std::vector<char> &buffer, 
									   const std::string & /*strAbsPath*/,
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
//	Destructor
ObjFileMtlImporter::~ObjFileMtlImporter()
{
	// empty
}

// -------------------------------------------------------------------
//	Private copy constructor
ObjFileMtlImporter::ObjFileMtlImporter(const ObjFileMtlImporter & /* rOther */ )
{
	// empty
}
	
// -------------------------------------------------------------------
//	Private copy constructor
ObjFileMtlImporter &ObjFileMtlImporter::operator = ( const ObjFileMtlImporter & /*rOther */ )
{
	return *this;
}

// -------------------------------------------------------------------
//	Loads the material description
void ObjFileMtlImporter::load()
{
	if ( m_DataIt == m_DataItEnd )
		return;

	while ( m_DataIt != m_DataItEnd )
	{
		switch (*m_DataIt)
		{
		case 'K':
			{
				++m_DataIt;
				if (*m_DataIt == 'a') // Ambient color
				{
					++m_DataIt;
					getColorRGBA( &m_pModel->m_pCurrentMaterial->ambient );
				}
				else if (*m_DataIt == 'd')	// Diffuse color
				{
					++m_DataIt;
					getColorRGBA( &m_pModel->m_pCurrentMaterial->diffuse );
				}
				else if (*m_DataIt == 's')
				{
					++m_DataIt;
					getColorRGBA( &m_pModel->m_pCurrentMaterial->specular );
				}
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'd':	// Alpha value
			{
				++m_DataIt;
				getFloatValue( m_pModel->m_pCurrentMaterial->alpha );
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'N':	// Shineness
			{
				++m_DataIt;
				switch(*m_DataIt) 
				{
				case 's':
					++m_DataIt;
					getFloatValue(m_pModel->m_pCurrentMaterial->shineness);
					break;
				case 'i': //Index Of refraction 
					++m_DataIt;
					getFloatValue(m_pModel->m_pCurrentMaterial->ior);
					break;
				}
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
				break;
			}
			break;
		

		case 'm':	// Texture
		case 'b':   // quick'n'dirty - for 'bump' sections
			{
				getTexture();
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'n':	// New material name
			{
				createMaterial();
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'i':	// Illumination model
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
//	Loads a color definition
void ObjFileMtlImporter::getColorRGBA( aiColor3D *pColor )
{
	ai_assert( NULL != pColor );
	
	float r, g, b;
	m_DataIt = getFloat<DataArrayIt>( m_DataIt, m_DataItEnd, r );
	pColor->r = r;
	
	m_DataIt = getFloat<DataArrayIt>( m_DataIt, m_DataItEnd, g );
	pColor->g = g;

	m_DataIt = getFloat<DataArrayIt>( m_DataIt, m_DataItEnd, b );
	pColor->b = b;
}

// -------------------------------------------------------------------
//	Loads the kind of illumination model.
void ObjFileMtlImporter::getIlluminationModel( int &illum_model )
{
	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	illum_model = atoi(m_buffer);
}

// -------------------------------------------------------------------
//	Loads a single float value. 
void ObjFileMtlImporter::getFloatValue( float &value )
{
	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	value = (float) fast_atof(m_buffer);
}

// -------------------------------------------------------------------
//	Creates a material from loaded data.
void ObjFileMtlImporter::createMaterial()
{	
	std::string line( "" );
	while ( !isNewLine( *m_DataIt ) ) {
		line += *m_DataIt;
		++m_DataIt;
	}
	
	std::vector<std::string> token;
	const unsigned int numToken = tokenize<std::string>( line, token, " " );
	std::string name( "" );
	if ( numToken == 1 ) {
		name = AI_DEFAULT_MATERIAL_NAME;
	} else {
		name = token[ 1 ];
	}

	std::map<std::string, ObjFile::Material*>::iterator it = m_pModel->m_MaterialMap.find( name );
	if ( m_pModel->m_MaterialMap.end() == it) {
		// New Material created
		m_pModel->m_pCurrentMaterial = new ObjFile::Material();	
		m_pModel->m_pCurrentMaterial->MaterialName.Set( name );
		m_pModel->m_MaterialLib.push_back( name );
		m_pModel->m_MaterialMap[ name ] = m_pModel->m_pCurrentMaterial;
	} else {
		// Use older material
		m_pModel->m_pCurrentMaterial = (*it).second;
	}
}

// -------------------------------------------------------------------
//	Gets a texture name from data.
void ObjFileMtlImporter::getTexture() {
	aiString *out( NULL );

	const char *pPtr( &(*m_DataIt) );
	if ( !ASSIMP_strincmp( pPtr, DiffuseTexture.c_str(), DiffuseTexture.size() ) ) {
		// Diffuse texture
		out = & m_pModel->m_pCurrentMaterial->texture;
	} else if ( !ASSIMP_strincmp( pPtr,AmbientTexture.c_str(),AmbientTexture.size() ) ) {
		// Ambient texture
		out = & m_pModel->m_pCurrentMaterial->textureAmbient;
	} else if (!ASSIMP_strincmp( pPtr, SpecularTexture.c_str(), SpecularTexture.size())) {
		// Specular texture
		out = & m_pModel->m_pCurrentMaterial->textureSpecular;
	} else if ( !ASSIMP_strincmp( pPtr, OpacityTexture.c_str(), OpacityTexture.size() ) ) {
		// Opacity texture
		out = & m_pModel->m_pCurrentMaterial->textureOpacity;
	} else if (!ASSIMP_strincmp( pPtr,"map_ka",6)) {
		// Ambient texture
		out = & m_pModel->m_pCurrentMaterial->textureAmbient;
	} else if ( !ASSIMP_strincmp( pPtr, BumpTexture1.c_str(), BumpTexture1.size() ) ||
		        !ASSIMP_strincmp( pPtr, BumpTexture2.c_str(), BumpTexture2.size() ) || 
		        !ASSIMP_strincmp( pPtr, BumpTexture3.c_str(), BumpTexture3.size() ) ) {
		// Bump texture 
		out = & m_pModel->m_pCurrentMaterial->textureBump;
	} else if (!ASSIMP_strincmp( pPtr,NormalTexture.c_str(), NormalTexture.size())) { 
		// Normal map
		out = & m_pModel->m_pCurrentMaterial->textureNormal;
	} else if (!ASSIMP_strincmp( pPtr, DisplacementTexture.c_str(), DisplacementTexture.size() ) ) {
		// Displacement texture
		out = &m_pModel->m_pCurrentMaterial->textureDisp;
	} else if (!ASSIMP_strincmp( pPtr, SpecularityTexture.c_str(),SpecularityTexture.size() ) ) {
		// Specularity scaling (glossiness)
		out = & m_pModel->m_pCurrentMaterial->textureSpecularity;
	} else {
		DefaultLogger::get()->error("OBJ/MTL: Encountered unknown texture type");
		return;
	}

	std::string strTexture;
	m_DataIt = getName<DataArrayIt>( m_DataIt, m_DataItEnd, strTexture );
	out->Set( strTexture );
}

// -------------------------------------------------------------------

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
