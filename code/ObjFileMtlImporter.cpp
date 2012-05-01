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
void ObjFileMtlImporter::getTexture()
{
	aiString *out = NULL;

	// FIXME: just a quick'n'dirty hack, consider cleanup later

	// Diffuse texture
	if (!ASSIMP_strincmp(&(*m_DataIt),"map_kd",6))
		out = & m_pModel->m_pCurrentMaterial->texture;

	// Ambient texture
	else if (!ASSIMP_strincmp(&(*m_DataIt),"map_ka",6))
		out = & m_pModel->m_pCurrentMaterial->textureAmbient;

	// Specular texture
	else if (!ASSIMP_strincmp(&(*m_DataIt),"map_ks",6))
		out = & m_pModel->m_pCurrentMaterial->textureSpecular;

	// Opacity texture
	else if (!ASSIMP_strincmp(&(*m_DataIt),"map_d",5))
		out = & m_pModel->m_pCurrentMaterial->textureOpacity;

	// Ambient texture
	else if (!ASSIMP_strincmp(&(*m_DataIt),"map_ka",6))
		out = & m_pModel->m_pCurrentMaterial->textureAmbient;

	// Bump texture
	else if (!ASSIMP_strincmp(&(*m_DataIt),"map_bump",8) || !ASSIMP_strincmp(&(*m_DataIt),"bump",4))
		out = & m_pModel->m_pCurrentMaterial->textureBump;

	// Specularity scaling (glossiness)
	else if (!ASSIMP_strincmp(&(*m_DataIt),"map_ns",6))
		out = & m_pModel->m_pCurrentMaterial->textureSpecularity;

	else
	{
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
