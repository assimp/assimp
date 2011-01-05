/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2011, ASSIMP Development Team

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

/** @file  export.hpp
*  @brief Defines the CPP-API for the Assimp export interface
*/
#ifndef AI_EXPORT_HPP_INC
#define AI_EXPORT_HPP_INC

#ifndef ASSIMP_BUILD_NO_EXPORT

#include "export.h"

namespace Assimp	{

class ASSIMP_API Exporter
#ifdef __cplusplus
	: public boost::noncopyable
#endif // __cplusplus
{
public:

	
	Exporter() : blob() {
	}


	~Exporter() {
		if (blob) {
			::aiReleaseExportData(blob);
		}
	}

public:

	// -------------------------------------------------------------------
	/** Exports the given scene to a chosen file format. Returns the exported 
	* data as a binary blob which you can write into a file or something.
	* When you're done with the data, simply let the #Exporter instance go 
	* out of scope to have it released automatically.
	* @param pScene The scene to export. Stays in possession of the caller,
	*   is not changed by the function.
	* @param pFormatId ID string to specify to which format you want to 
	*   export to. Use 
	* #GetExportFormatCount / #GetExportFormatDescription to learn which 
	*   export formats are available.
	* @return the exported data or NULL in case of error.
	* @note If the Exporter instance did already hold a blob from
	*   a previous call to #ExportToBlob, it will be disposed. */
	const aiExportDataBlob* ExportToBlob(  const aiScene* pScene, const char* pFormatId ) {
		if (blob) {
			::aiReleaseExportData(blob);
		}

		return blob = ::aiExportScene(pScene,pFormatId);
	}


	// -------------------------------------------------------------------
	/** Convenience function to export directly to a file.
	 * @param pBlob A data blob obtained from a previous call to #aiExportScene. Must not be NULL.
	 * @param pPath Full target file name. Target must be accessible.
	 * @return AI_SUCCESS if everything was fine. */
	aiReturn ExportToFile( const aiScene* pScene, const char* pFormatId, const char* pPath ) {
		
		if(!ExportToBlob(pScene,pFormatId)) {
			return AI_FAILURE;
		}


		return WriteBlobToFile(pPath);
	}


	// -------------------------------------------------------------------
	/** Convenience function to write a blob to a file. 
	 * @param pBlob A data blob obtained from a previous call to #aiExportScene. Must not be NULL.
	 * @param pPath Full target file name. Target must be accessible.
	 * @return AI_SUCCESS if everything was fine. */
	aiReturn WriteBlobToFile( const char* pPath ) const {
		if (!blob) {
			return AI_FAILURE;
		}

		// TODO
		return AI_FAILURE; // ::aiWriteBlobToFile(blob,pPath,mIOSystem);
	}


	// -------------------------------------------------------------------
	aiReturn WriteBlobToFile( const std::string& pPath ) const {
		return WriteBlobToFile(pPath.c_str());
	}


	// -------------------------------------------------------------------
	/** Return the blob obtained from the last call to #ExportToBlob */
	const aiExportDataBlob* GetBlob() const {
		return blob;
	}


	// -------------------------------------------------------------------
	/** Orphan the blob from the last call to #ExportToBlob. That means
	 *  the caller takes ownership and is thus responsible for calling
	 *  #aiReleaseExportData to free the data again. */
	const aiExportDataBlob* GetOrphanedBlob() const {
		const aiExportDataBlob* tmp = blob;
		blob = NULL;
		return tmp;
	}


	// -------------------------------------------------------------------
	/** Returns the number of export file formats available in the current
	 *  Assimp build. Use #Exporter::GetExportFormatDescription to
	 *  retrieve infos of a specific export format */
	size_t aiGetExportFormatCount() const {
		return ::aiGetExportFormatCount();
	}


	// -------------------------------------------------------------------
	/** Returns a description of the nth export file format. Use #
	 *  #Exporter::GetExportFormatCount to learn how many export 
	 *  formats are supported. 
	 * @param pIndex Index of the export format to retrieve information 
	 *  for. Valid range is 0 to #Exporter::GetExportFormatCount
	 * @return A description of that specific export format. 
	 *  NULL if pIndex is out of range. */
	const aiExportFormatDesc* aiGetExportFormatDescription( size_t pIndex ) const {
		return ::aiGetExportFormatDescription(pIndex);
	}


private:

	const aiExportDataBlob* blob;
	Assimp::IOSystem* mIOSystem;
};

} // namespace Assimp
#endif // ASSIMP_BUILD_NO_EXPORT
#endif // AI_EXPORT_HPP_INC

