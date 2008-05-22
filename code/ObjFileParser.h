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


#ifndef OBJ_FILEPARSER_H_INC
#define OBJ_FILEPARSER_H_INC

#include <vector>
#include <string>
#include "../include/aiTypes.h"

/*struct aiVector2D_t;
struct aiVector3D_t;*/

namespace Assimp
{

namespace ObjFile
{
struct Model;
struct Object;
struct Material;
struct Point3;
struct Point2;
}
class ObjFileImporter;

class ObjFileParser
{
public:
	static const size_t BUFFERSIZE = 1024;
	typedef std::vector<char> DataArray;
	typedef std::vector<char>::iterator DataArrayIt;
	typedef std::vector<char>::const_iterator ConstDataArrayIt;

public:
	ObjFileParser(std::vector<char> &Data, const std::string &strAbsPath, const std::string &strModelName);
	~ObjFileParser();
	ObjFile::Model *GetModel() const;

private:
	void parseFile();
	void copyNextWord(char *pBuffer, size_t length);
	void copyNextLine(char *pBuffer, size_t length);
	void getVector3(std::vector<aiVector3D*> &point3d_array);
	void getVector2(std::vector<aiVector2D*> &point2d_array);
	void getFace();
	void getMaterialDesc();
	void getComment();
	void getMaterialLib();
	void getNewMaterial();
	void getGroupName();
	void getGroupNumber();
	void getObjectName();
	void createObject(const std::string &strObjectName);
	void reportErrorTokenInFace();
	void extractExtension(const std::string strFile, std::string &strExt);

private:
	std::string m_strAbsPath;
	DataArrayIt m_DataIt;
	DataArrayIt m_DataItEnd;
	ObjFile::Model *m_pModel;
	unsigned int m_uiLine;
	char m_buffer[BUFFERSIZE];

};

}	// Namespace Assimp

#endif
