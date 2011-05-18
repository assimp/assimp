/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

/** @file  IFC.h
 *  @brief Declaration of the Industry Foundation Classes (IFC) loader main class
 */
#ifndef INCLUDED_AI_IFC_LOADER_H
#define INCLUDED_AI_IFC_LOADER_H

#include "BaseImporter.h"
#include "LogAux.h"

namespace Assimp	{
	
	// TinyFormatter.h
	namespace Formatter {
		template <typename T,typename TR, typename A> class basic_formatter;
		typedef class basic_formatter< char, std::char_traits<char>, std::allocator<char> > format;
	}

	namespace STEP {
		class DB;
	}


// -------------------------------------------------------------------------------------------
/** Load the IFC format, which is an open specification to describe building and construction
    industry data.

 See http://en.wikipedia.org/wiki/Industry_Foundation_Classes
*/
// -------------------------------------------------------------------------------------------
class IFCImporter : public BaseImporter, public LogFunctions<IFCImporter>
{
	friend class Importer;

protected:

	/** Constructor to be privately used by Importer */
	IFCImporter();

	/** Destructor, private as well */
	~IFCImporter();

public:

	// --------------------
	bool CanRead( const std::string& pFile, 
		IOSystem* pIOHandler,
		bool checkSig
	) const;

protected:

	// --------------------
	void GetExtensionList(std::set<std::string>& app);

	// --------------------
	void SetupProperties(const Importer* pImp);

	// --------------------
	void InternReadFile( const std::string& pFile, 
		aiScene* pScene, 
		IOSystem* pIOHandler
	);

private:


public: 


	// loader settings, publicy accessible via their corresponding AI_CONFIG constants
	struct Settings 
	{
		Settings()
			: skipSpaceRepresentations()
			, skipCurveRepresentations()
			, useCustomTriangulation()
		{}


		bool skipSpaceRepresentations;
		bool skipCurveRepresentations;
		bool useCustomTriangulation;
	};
	
	
private:

	Settings settings;

}; // !class IFCImporter

} // end of namespace Assimp
#endif // !INCLUDED_AI_IFC_LOADER_H
