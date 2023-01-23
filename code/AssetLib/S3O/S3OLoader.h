/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2023, assimp team


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

/** @file  S3OLoader.h
 *  @brief S3O File format loader
 */
#ifndef AI_S3OIMPORTER_H_INC
#define AI_S3OIMPORTER_H_INC
#ifndef ASSIMP_BUILD_NO_S3O_IMPORTER

#include <assimp/BaseImporter.h>
#include <assimp/types.h>
#include <assimp/scene.h>


namespace Assimp    {

// ---------------------------------------------------------------------------------
/** Loader class Spring RTS S3O Files
 */
class S3OImporter : public BaseImporter {
public:

    S3OImporter();
    ~S3OImporter();

    // -------------------------------------------------------------------
    /** Returns whether the class can handle the format of the given file.
     *
     * The implementation is expected to perform a full check of the file
     * structure, possibly searching the first bytes of the file for magic
     * identifiers or keywords.
     *
     * @param pFile Path and file name of the file to be examined.
     * @param pIOHandler The IO handler to use for accessing any file.
     * @param checkSig Legacy; do not use.
     * @return true if the class can read this file, false if not or if
     * unsure.
     */
    bool CanRead( const std::string& pFile, IOSystem* pIOHandler,
        bool checkSig) const override;

    // -------------------------------------------------------------------
    /** Called prior to ReadFile().
     * The function is a request to the importer to update its configuration
     * basing on the Importer's configuration property list.
     * @param pImp Importer instance
     */
    void SetupProperties(const Importer* pImp) override;

protected:

    // -------------------------------------------------------------------
    /** Return importer meta information.
     * See #BaseImporter::GetInfo for the details
     */
    const aiImporterDesc* GetInfo () const override;


    /** Imports the given file into the given scene structure. The
     * function is expected to throw an ImportErrorException if there is
     * an error. If it terminates normally, the data in aiScene is
     * expected to be correct. Override this function to implement the
     * actual importing.
     * <br>
     *  The output scene must meet the following requirements:<br>
     * <ul>
     * <li>At least a root node must be there, even if its only purpose
     *     is to reference one mesh.</li>
     * <li>aiMesh::mPrimitiveTypes may be 0. The types of primitives
     *   in the mesh are determined automatically in this case.</li>
     * <li>the vertex data is stored in a pseudo-indexed "verbose" format.
     *   In fact this means that every vertex that is referenced by
     *   a face is unique. Or the other way round: a vertex index may
     *   not occur twice in a single aiMesh.</li>
     * <li>aiAnimation::mDuration may be -1. Assimp determines the length
     *   of the animation automatically in this case as the length of
     *   the longest animation channel.</li>
     * <li>aiMesh::mBitangents may be nullptr if tangents and normals are
     *   given. In this case bitangents are computed as the cross product
     *   between normal and tangent.</li>
     * <li>There needn't be a material. If none is there a default material
     *   is generated. However, it is recommended practice for loaders
     *   to generate a default material for yourself that matches the
     *   default material setting for the file format better than Assimp's
     *   generic default material. Note that default materials *should*
     *   be named AI_DEFAULT_MATERIAL_NAME if they're just color-shaded
     *   or AI_DEFAULT_TEXTURED_MATERIAL_NAME if they define a (dummy)
     *   texture. </li>
     * </ul>
     * If the AI_SCENE_FLAGS_INCOMPLETE-Flag is <b>not</b> set:<ul>
     * <li> at least one mesh must be there</li>
     * <li> there may be no meshes with 0 vertices or faces</li>
     * </ul>
     * This won't be checked (except by the validation step): Assimp will
     * crash if one of the conditions is not met!
     *
     * @param pFile Path of the file to be imported.
     * @param pScene The scene object to hold the imported data.
     * nullptr is not a valid parameter.
     * @param pIOHandler The IO handler to use for any file access.
     * nullptr is not a valid parameter. */
    void InternReadFile( const std::string& pFile, aiScene* pScene,
        IOSystem* pIOHandler) override;
};

} // end of namespace Assimp

#endif // !! ASSIMP_BUILD_NO_S3O_IMPORTER

#endif // AI_S3OIMPORTER_H_INC