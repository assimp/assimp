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
	class ExporterPimpl;

// ----------------------------------------------------------------------------------
/** CPP-API: The Exporter class forms an C++ interface to the export functionality 
 * of the Open Asset Import Library. Note that the export interface is available
 * only if Assimp has been built with ASSIMP_BUILD_NO_EXPORT not defined.
 * 
 * The interface is modelled after the importer interface and mostly
 * symmetric. The same rules for threading etc. apply.
 *
 * In a nutshell, there are two export interfaces: #Export, which writes the
 * output file(s) either to the regular file system or to a user-supplied 
 * #IOSystem, and #ExportToBlob which returns a linked list of memory
 * buffers (blob), each referring to one output file (in most cases
 * there will be only one output file of course, but this extra complexity is
 * needed since Assimp aims at supporting a wide range of file formats). 
 * 
 * #ExportToBlob is especially useful if you intend to work 
 * with the data in-memory. 
*/
class ASSIMP_API Exporter
	// TODO: causes good ol' base class has no dll interface warning
//#ifdef __cplusplus
//	: public boost::noncopyable
//#endif // __cplusplus
{
public:

	
	Exporter();
	~Exporter();

public:


	// -------------------------------------------------------------------
	/** Supplies a custom IO handler to the exporter to use to open and
	 * access files. 
	 * 
	 * If you need #Export to use custom IO logic to access the files, 
	 * you need to supply a custom implementation of IOSystem and 
	 * IOFile to the exporter. 
	 *
	 * #Exporter takes ownership of the object and will destroy it 
	 * afterwards. The previously assigned handler will be deleted.
	 * Pass NULL to take again ownership of your IOSystem and reset Assimp
	 * to use its default implementation, which uses plain file IO.
	 *
	 * @param pIOHandler The IO handler to be used in all file accesses 
	 *   of the Importer. 
	 */
	void SetIOHandler( IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Retrieves the IO handler that is currently set.
	 * You can use #IsDefaultIOHandler() to check whether the returned
	 * interface is the default IO handler provided by ASSIMP. The default
	 * handler is active as long the application doesn't supply its own
	 * custom IO handler via #SetIOHandler().
	 * @return A valid IOSystem interface, never NULL.
	 */
	IOSystem* GetIOHandler() const;

	// -------------------------------------------------------------------
	/** Checks whether a default IO handler is active 
	 * A default handler is active as long the application doesn't 
	 * supply its own custom IO handler via #SetIOHandler().
	 * @return true by default
	 */
	bool IsDefaultIOHandler() const;



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
	*   a previous call to #ExportToBlob, it will be disposed. 
	*   Any IO handlers set via #SetIOHandler are ignored here.*/
	const aiExportDataBlob* ExportToBlob(  const aiScene* pScene, const char* pFormatId );
	inline const aiExportDataBlob* ExportToBlob(  const aiScene* pScene, const std::string& pFormatId );


	// -------------------------------------------------------------------
	/** Convenience function to export directly to a file. Use
	 *  #SetIOSystem to supply a custom IOSystem to gain fine-grained control
	 *  about the output data flow of the export process.
	 * @param pBlob A data blob obtained from a previous call to #aiExportScene. Must not be NULL.
	 * @param pPath Full target file name. Target must be accessible.
	 * @return AI_SUCCESS if everything was fine. */
	aiReturn Export( const aiScene* pScene, const char* pFormatId, const char* pPath );
	inline aiReturn Export( const aiScene* pScene, const std::string& pFormatId, const std::string& pPath );



	// -------------------------------------------------------------------
	/** Return the blob obtained from the last call to #ExportToBlob */
	const aiExportDataBlob* GetBlob() const;


	// -------------------------------------------------------------------
	/** Orphan the blob from the last call to #ExportToBlob. This means
	 *  the caller takes ownership and is thus responsible for calling
	 *  #aiReleaseExportData to free the data again. */
	const aiExportDataBlob* GetOrphanedBlob() const;


	// -------------------------------------------------------------------
	/** Returns the number of export file formats available in the current
	 *  Assimp build. Use #Exporter::GetExportFormatDescription to
	 *  retrieve infos of a specific export format */
	size_t GetExportFormatCount() const;


	// -------------------------------------------------------------------
	/** Returns a description of the nth export file format. Use #
	 *  #Exporter::GetExportFormatCount to learn how many export 
	 *  formats are supported. 
	 * @param pIndex Index of the export format to retrieve information 
	 *  for. Valid range is 0 to #Exporter::GetExportFormatCount
	 * @return A description of that specific export format. 
	 *  NULL if pIndex is out of range. */
	const aiExportFormatDesc* GetExportFormatDescription( size_t pIndex ) const;


protected:

	// Just because we don't want you to know how we're hacking around.
	ExporterPimpl* pimpl;
};


// ----------------------------------------------------------------------------------
inline const aiExportDataBlob* Exporter :: ExportToBlob(  const aiScene* pScene, const std::string& pFormatId ) 
{
	return ExportToBlob(pScene,pFormatId.c_str());
}

// ----------------------------------------------------------------------------------
inline aiReturn Exporter :: Export( const aiScene* pScene, const std::string& pFormatId, const std::string& pPath )
{
	return Export(pScene,pFormatId.c_str(),pPath.c_str());
}

} // namespace Assimp
#endif // ASSIMP_BUILD_NO_EXPORT
#endif // AI_EXPORT_HPP_INC

