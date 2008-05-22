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

/** @file Definition of the base class for all importer worker classes. */
#ifndef AI_BASEIMPORTER_H_INC
#define AI_BASEIMPORTER_H_INC

#include <string>

struct aiScene;

namespace Assimp
{

class IOSystem;

// ---------------------------------------------------------------------------
/** Simple exception class to be thrown if an error occurs while importing. */
class ImportErrorException 
{
public:
	/** Constructor with arguments */
	ImportErrorException( const std::string& pErrorText)
	{
		mErrorText = pErrorText;
	}

	// -------------------------------------------------------------------
	/** Returns the error text provided when throwing the exception */
	inline const std::string& GetErrorText() const 
	{ return mErrorText; }

private:
	std::string mErrorText;
};

// ---------------------------------------------------------------------------
/** The BaseImporter defines a common interface for all importer worker 
 *  classes.
 *
 * The interface defines two functions: CanRead() is used to check if the 
 * importer can handle the format of the given file. If an implementation of 
 * this function returns true, the importer then calls ReadFile() which 
 * imports the given file. ReadFile is not overridable, it just calls 
 * InternReadFile() and catches any ImportErrorException that might occur.
 */
class BaseImporter
{
	friend class Importer;

protected:

	/** Constructor to be privately used by #Importer */
	BaseImporter();

	/** Destructor, private as well */
	virtual ~BaseImporter();

public:
	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file.
	* @param pFile Path and file name of the file to be examined.
	* @param pIOHandler The IO handler to use for accessing any file.
	* @return true if the class can read this file, false if not.
	*
	* @note Sometimes ASSIMP uses this method to determine whether a
	* a given file extension is generally supported. In this case the
	* file extension is passed in the pFile parameter, pIOHandler is NULL
	*/
	virtual bool CanRead( const std::string& pFile, 
		IOSystem* pIOHandler) const = 0;


	// -------------------------------------------------------------------
	/** Imports the given file and returns the imported data.
	* If the import succeeds, ownership of the data is transferred to 
	* the caller. If the import failes, NULL is returned. The function
	* takes care that any partially constructed data is destroyed
	* beforehand.
	*
	* @param pFile Path of the file to be imported. 
	* @param pIOHandler IO-Handler used to open this and possible other files.
	* @return The imported data or NULL if failed. If it failed a 
	* human-readable error description can be retrieved by calling 
	* GetErrorText()
	*
	* @note This function is not intended to be overridden. Implement 
	* InternReadFile() to do the import. If an exception is thrown somewhere 
	* in InternReadFile(), this function will catch it and transform it into
	*  a suitable response to the caller.
	*/
	aiScene* ReadFile( const std::string& pFile, IOSystem* pIOHandler);


	// -------------------------------------------------------------------
	/** Returns the error description of the last error that occured. 
	 * @return A description of the last error that occured. An empty
	 * string if there was no error.
	 */
	inline const std::string& GetErrorText() const 
		{ return mErrorText; }

protected:

	// -------------------------------------------------------------------
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 *  Importer implementations should append all file extensions
	 *  which they supported to the passed string.
	 *  Example: "*.blabb;*.quak;*.gug;*.foo" (no comma after the last!)
	 * @param append Output string
	 */
	virtual void GetExtensionList(std::string& append) = 0;

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. The 
	 * function is expected to throw an ImportErrorException if there is 
	 * an error. If it terminates normally, the data in aiScene is 
	 * expected to be correct. Override this function to implement the 
	 * actual importing.
	 * 
	 * @param pFile Path of the file to be imported.
	 * @param pScene The scene object to hold the imported data.
	 * NULL is not a valid parameter.
	 * @param pIOHandler The IO handler to use for any file access.
	 * NULL is not a valid parameter.
	 */
	virtual void InternReadFile( const std::string& pFile, 
		aiScene* pScene, IOSystem* pIOHandler) = 0;

protected:

	/** Error description in case there was one. */
	std::string mErrorText;
};

} // end of namespace Assimp

#endif // AI_BASEIMPORTER_H_INC