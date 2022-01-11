/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team


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

#ifndef AI_OGREXMLSERIALIZER_H_INC
#define AI_OGREXMLSERIALIZER_H_INC

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "OgreStructs.h"
#include <assimp/XmlParser.h>

namespace Assimp {

namespace Ogre {

using XmlParserPtr = std::shared_ptr<::Assimp::XmlParser> ;

class OgreXmlSerializer {
public:
    /// Imports mesh and returns the result.
    /// @note Fatal unrecoverable errors will throw a DeadlyImportError.
    static MeshXml *ImportMesh(XmlParser *parser);

    /// Imports skeleton to @c mesh.
    /// If mesh does not have a skeleton reference or the skeleton file
    /// cannot be found it is not a fatal DeadlyImportError.
    /// @return If skeleton import was successful.
    static bool ImportSkeleton(IOSystem *pIOHandler, MeshXml *mesh);
    static bool ImportSkeleton(IOSystem *pIOHandler, Mesh *mesh);

private:
    explicit OgreXmlSerializer(XmlParser *xmlParser);

    static XmlParserPtr OpenXmlParser(Assimp::IOSystem *pIOHandler, const std::string &filename);

    // Mesh
    void ReadMesh(MeshXml *mesh);
    void ReadSubMesh(XmlNode &node, MeshXml *mesh);
    void ReadGeometry(XmlNode &node, VertexDataXml *dest);
    void ReadGeometryVertexBuffer(XmlNode &node, VertexDataXml *dest);
    void ReadBoneAssignments(XmlNode &node, VertexDataXml *dest);

    // Skeleton
    void ReadSkeleton(XmlNode &node, Skeleton *skeleton);
    void ReadBones(XmlNode &node, Skeleton *skeleton);
    void ReadBoneHierarchy(XmlNode &node, Skeleton *skeleton);
    void ReadAnimations(XmlNode &node, Skeleton *skeleton);
    void ReadAnimationTracks(XmlNode &node, Animation *dest);
    void ReadAnimationKeyFrames(XmlNode &node, Animation *anim, VertexAnimationTrack *dest);

    template <typename T>
    T ReadAttribute(XmlNode &xmlNode, const char *name) const;

private:
    XmlParser *mParser;
};


} // namespace Ogre
} // namespace Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
#endif // AI_OGREXMLSERIALIZER_H_INC
