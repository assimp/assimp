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
#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_Q3BSP_IMPORTER

#include "DefaultIOSystem.h"
#include "Q3BSPFileImporter.h"
#include "Q3BSPZipArchive.h"
#include "Q3BSPFileParser.h"
#include "../contrib/zlib/zlib.h"
#include <vector>

namespace Assimp
{

using namespace Q3BSP;

// ------------------------------------------------------------------------------------------------
Q3BSPFileImporter::Q3BSPFileImporter()
{
}

// ------------------------------------------------------------------------------------------------
Q3BSPFileImporter::~Q3BSPFileImporter()
{
}

// ------------------------------------------------------------------------------------------------
bool Q3BSPFileImporter::CanRead( const std::string& rFile, IOSystem* pIOHandler, bool checkSig ) const
{
	bool isBSPData = false;
	if ( checkSig )
		isBSPData = SimpleExtensionCheck( rFile, "pk3" );

	return isBSPData;
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::GetExtensionList(std::set<std::string>& extensions)
{
	extensions.insert("pk3");
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::InternReadFile(const std::string &rFile, aiScene* pScene, IOSystem* pIOHandler)
{
	Q3BSPZipArchive Archive( rFile );
	if ( !Archive.isOpen() )
	{
		throw new DeadlyImportError( "Failed to open file " + rFile + "." );
	}

	std::string archiveName( "" ), mapName( "" );
	separateMapName( rFile, archiveName, mapName );

	if ( mapName.empty() )
	{
		if ( !findFirstMapInArchive( Archive, mapName ) )
		{
			return;
		}
	}

	Q3BSPFileParser fileParser( mapName, &Archive );
	Q3BSPModel *pBSPModel = fileParser.getModel();
	if ( NULL != pBSPModel )
	{
	}
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::separateMapName( const std::string &rImportName, std::string &rArchiveName, 
										std::string &rMapName )
{
	rArchiveName = "";
	rMapName = "";
	if ( rImportName.empty() )
		return;

	std::string::size_type pos = rImportName.rfind( "," );
	if ( std::string::npos == pos )
	{
		rArchiveName = rImportName;
		return;
	}

	rArchiveName = rImportName.substr( 0, pos );
	rMapName = rImportName.substr( pos, rImportName.size() - pos - 1 );
}

// ------------------------------------------------------------------------------------------------
bool Q3BSPFileImporter::findFirstMapInArchive( Q3BSPZipArchive &rArchive, std::string &rMapName )
{
	rMapName = "";
	std::vector<std::string> fileList;
	rArchive.getFileList( fileList );
	if ( fileList.empty() )  
		return false;

	for ( std::vector<std::string>::iterator it = fileList.begin(); it != fileList.end();
		++it )
	{
		std::string::size_type pos = (*it).find( "maps/" );
		if ( std::string::npos != pos )
		{
			std::string::size_type extPos = (*it).find( ".bsp" );
			if ( std::string::npos != extPos )
			{
				rMapName = *it;
				return true;
			}
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------

} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_Q3BSP_IMPORTER
