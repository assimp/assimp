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

/** @file FileSystemFilter.h
 *  Implements a filter system to filter calls to Exists() and Open()
 *  in order to improve the sucess rate of file opening ...
 */
#ifndef AI_FILESYSTEMFILTER_H_INC
#define AI_FILESYSTEMFILTER_H_INC

#include "../include/IOSystem.h"
#include "fast_atof.h"
#include "ParsingUtils.h"
namespace Assimp	{

// ---------------------------------------------------------------------------
/** File system filter  
 */
class FileSystemFilter : public IOSystem
{
public:
	/** Constructor. */
	FileSystemFilter(const std::string& file, IOSystem* old)
		: wrapped  (old)
		, src_file (file)
	{
		ai_assert(NULL != wrapped);

		// Determine base directory
		base = src_file;
		std::string::size_type ss2;
		if (std::string::npos != (ss2 = base.find_last_of("\\/")))	{
			base.erase(ss2,base.length()-ss2);
		}
		else {
			base = "";
		//	return;
		}

		// make sure the directory is terminated properly
		char s;

		if (base.length() == 0) {
			base = ".";
			base += getOsSeparator();
		}
		else if ((s = *(base.end()-1)) != '\\' && s != '/')
			base += getOsSeparator();

		DefaultLogger::get()->info("Import root directory is \'" + base + "\'");
	}

	/** Destructor. */
	~FileSystemFilter()
	{
		// haha
	}

	// -------------------------------------------------------------------
	/** Tests for the existence of a file at the given path. */
	bool Exists( const char* pFile) const
	{
		std::string tmp = pFile;

		// Currently this IOSystem is also used to open THE ONE FILE.
		if (tmp != src_file)	{
			BuildPath(tmp);
			Cleanup(tmp);
		}

		return wrapped->Exists(tmp);
	}

	// -------------------------------------------------------------------
	/** Returns the directory separator. */
	char getOsSeparator() const
	{
		return wrapped->getOsSeparator();
	}

	// -------------------------------------------------------------------
	/** Open a new file with a given path. */
	IOStream* Open( const char* pFile, const char* pMode = "rb")
	{
		ai_assert(pFile);
		ai_assert(pMode);

		// First try the unchanged path
		IOStream* s = wrapped->Open(pFile,pMode);

		if (!s)	{
			std::string tmp = pFile;

			// Try to convert between absolute and relative paths
			BuildPath(tmp);
			s = wrapped->Open(tmp,pMode);

			if (!s) {
				// Finally, look for typical issues with paths
				// and try to correct them. This is our last
				// resort.
				Cleanup(tmp);
				s = wrapped->Open(tmp,pMode);
			}
		}
		
		return s;
	}

	// -------------------------------------------------------------------
	/** Closes the given file and releases all resources associated with it. */
	void Close( IOStream* pFile)
	{
		return wrapped->Close(pFile);
	}

	// -------------------------------------------------------------------
	/** Compare two paths */
	bool ComparePaths (const char* one, const char* second) const
	{
		return wrapped->ComparePaths (one,second);
	}

private:
	IOSystem* wrapped;
	std::string src_file, base;

	// -------------------------------------------------------------------
	/** Build a valid path from a given relative or absolute path.
	 */
	void BuildPath (std::string& in) const
	{
		// if we can already access the file, great.
		if (in.length() < 3 || wrapped->Exists(in.c_str())) {
			return;
		}

		// Determine whether this is a relative path (Windows-specific - most assets are packaged on Windows). 
		if (in[1] != ':') {
		
			// append base path and try 
			in = base + in;
			if (wrapped->Exists(in.c_str())) {
				return;
			}
		}

		// hopefully the underyling file system has another few tricks to access this file ...
	}

	// -------------------------------------------------------------------
	/** Cleanup the given path
	 */
	void Cleanup (std::string& in) const
	{
		char last = 0;

		// Remove a very common issue when we're parsing file names: spaces at the
		// beginning of the path. 
		std::string::iterator it = in.begin();
		while (IsSpaceOrNewLine( *it ))++it;
		if (it != in.begin())
			in.erase(in.begin(),it+1);

		const char sep = getOsSeparator();
		for (it = in.begin(); it != in.end(); ++it) {
			// Exclude :// and \\, which remain untouched.
			// https://sourceforge.net/tracker/?func=detail&aid=3031725&group_id=226462&atid=1067632
			if ( !strncmp(&*it, "://", 3 )) {
				it += 3;
				continue;
			}
			if (it == in.begin() && !strncmp(&*it, "\\\\", 2)) {
				it += 2;
				continue;
			}

			// Cleanup path delimiters
			if (*it == '/' || (*it) == '\\') {
				*it = sep;

				// And we're removing double delimiters, frequent issue with
				// incorrectly composited paths ...
				if (last == *it) {
					it = in.erase(it);
					--it;
				}
			}
			else if (*it == '%' && in.end() - it > 2) {
			
				// Hex sequence in URIs
				uint32_t tmp;
				if( 0xffffffff != (tmp = HexOctetToDecimal(&*it))) {
					*it = (char)tmp;
					it = in.erase(it+1,it+2);
					--it;
				}
			}

			last = *it;
		}
	}
};

} //!ns Assimp

#endif //AI_DEFAULTIOSYSTEM_H_INC
