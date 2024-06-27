/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team
Copyright (c) 2019 bzt

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

----------------------------------------------------------------------
*/

/** @file M3DImporter.h
*   @brief Declares the importer class to read a scene from a Model 3D file
*/
#ifndef AI_M3DIMPORTER_H_INC
#define AI_M3DIMPORTER_H_INC

#ifndef ASSIMP_BUILD_NO_M3D_IMPORTER

#include <assimp/BaseImporter.h>
#include <assimp/material.h>
#include <vector>

struct aiMesh;
struct aiNode;
struct aiMaterial;
struct aiFace;

namespace Assimp {

class M3DWrapper;

class M3DImporter : public BaseImporter {
public:
	/// \brief  Default constructor
	M3DImporter();
    ~M3DImporter() override = default;

	/// \brief  Returns whether the class can handle the format of the given file.
	/// \remark See BaseImporter::CanRead() for details.
	bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

protected:
    //! \brief  Appends the supported extension.
    const aiImporterDesc *GetInfo() const override;

    //! \brief  File import implementation.
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;

private:
	void importMaterials(const M3DWrapper &m3d);
	void importTextures(const M3DWrapper &m3d);
	void importMeshes(const M3DWrapper &m3d);
	void importBones(const M3DWrapper &m3d, unsigned int parentid, aiNode *pParent);
	void importAnimations(const M3DWrapper &m3d);

	// helper functions
	aiColor4D mkColor(uint32_t c);
	void convertPose(const M3DWrapper &m3d, aiMatrix4x4 *m, unsigned int posid, unsigned int orientid);
    aiNode *findNode(aiNode *pNode, const aiString &name);
    void calculateOffsetMatrix(aiNode *pNode, aiMatrix4x4 *m);
	void populateMesh(const M3DWrapper &m3d, aiMesh *pMesh, std::vector<aiFace> *faces, std::vector<aiVector3D> *verteces,
			std::vector<aiVector3D> *normals, std::vector<aiVector3D> *texcoords, std::vector<aiColor4D> *colors,
			std::vector<unsigned int> *vertexids);

private:
    aiScene *mScene = nullptr; // the scene to import to
};

} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_M3D_IMPORTER

#endif // AI_M3DIMPORTER_H_INC
