#include "ObjFileMtlImporter.h"
#include "../include/aiTypes.h"
#include "../include/aiAssert.h"
#include "ObjTools.h"
#include "ObjFileData.h"
#include "fast_atof.h"


namespace Assimp
{

// -------------------------------------------------------------------
ObjFileMtlImporter::ObjFileMtlImporter( std::vector<char> &buffer, 
									   const std::string &strAbsPath,
									   ObjFile::Model *pModel ) :
	m_DataIt( buffer.begin() ),
	m_DataItEnd( buffer.end() ),
	m_uiLine( 0 ),
	m_pModel( pModel )
{
	ai_assert ( NULL != m_pModel );
	if ( NULL == m_pModel->m_pDefaultMaterial )
	{
		m_pModel->m_pDefaultMaterial = new ObjFile::Material();
		m_pModel->m_pDefaultMaterial->MaterialName.Set( "default" );
	}
	load();
}

// -------------------------------------------------------------------
ObjFileMtlImporter::~ObjFileMtlImporter()
{
	// empty
}

// -------------------------------------------------------------------
ObjFileMtlImporter::ObjFileMtlImporter(const ObjFileMtlImporter &rOther)
{
	// empty
}
	
// -------------------------------------------------------------------
ObjFileMtlImporter &ObjFileMtlImporter::operator = (
	const ObjFileMtlImporter &rOther)
{
	return *this;
}

// -------------------------------------------------------------------
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
					getColorRGBA( &m_pModel->m_pCurrentMaterial->ambient );
				}
				else if (*m_DataIt == 'd')	// Diffuse color
				{
					getColorRGBA( &m_pModel->m_pCurrentMaterial->diffuse );
				}
				else if (*m_DataIt == 's')
				{
					getColorRGBA( &m_pModel->m_pCurrentMaterial->specular );
				}
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'd':	// Alpha value
			{
				getFloatValue( m_pModel->m_pCurrentMaterial->alpha );
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'N':	// Shineness
			{
				getIlluminationModel( m_pModel->m_pCurrentMaterial->illumination_model );
				m_DataIt = skipLine<DataArrayIt>( m_DataIt, m_DataItEnd, m_uiLine );
			}
			break;

		case 'm':	// Texture
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
void ObjFileMtlImporter::getColorRGBA( aiColor3D *pColor )
{
	ai_assert( NULL != pColor );
	
	float r, g, b;
	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	r = (float) fast_atof(m_buffer);

	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	g = (float) fast_atof(m_buffer);

	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	b = (float) fast_atof(m_buffer);

	pColor->r = r;
	pColor->g = g;
	pColor->b = b;
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getIlluminationModel( int &illum_model )
{
	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	illum_model = atoi(m_buffer);
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getFloatValue( float &value )
{
	m_DataIt = CopyNextWord<DataArrayIt>( m_DataIt, m_DataItEnd, m_buffer, BUFFERSIZE );
	value = (float) fast_atof(m_buffer);
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::createMaterial()
{	
	std::string strName;
	m_DataIt = getName<DataArrayIt>( m_DataIt, m_DataItEnd, strName );
	if ( m_DataItEnd == m_DataIt )
		return;

	std::map<std::string, ObjFile::Material*>::iterator it = m_pModel->m_MaterialMap.find( strName );
	if ( m_pModel->m_MaterialMap.end() == it)
	{
		// New Material created
		m_pModel->m_pCurrentMaterial = new ObjFile::Material();	
		m_pModel->m_pCurrentMaterial->MaterialName.Set( strName );
		m_pModel->m_MaterialLib.push_back( strName );
		m_pModel->m_MaterialMap[ strName ] = m_pModel->m_pCurrentMaterial;
	}
	else
	{
		// Use older material
		m_pModel->m_pCurrentMaterial = (*it).second;
	}
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getTexture()
{
	std::string strTexture;
	m_DataIt = getName<DataArrayIt>( m_DataIt, m_DataItEnd, strTexture );
	if ( m_DataItEnd == m_DataIt )
		return;

	m_pModel->m_pCurrentMaterial->texture.Set( strTexture );
}

// -------------------------------------------------------------------

} // Namespace Assimp
