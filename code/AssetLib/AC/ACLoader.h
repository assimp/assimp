/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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

/** @file  ACLoader.h
 *  @brief Declaration of the .ac importer class.
 */
#ifndef AI_AC3DLOADER_H_INCLUDED
#define AI_AC3DLOADER_H_INCLUDED

#include <vector>

#include <assimp/BaseImporter.h>
#include <assimp/types.h>

struct aiNode;
struct aiMesh;
struct aiMaterial;
struct aiLight;

namespace Assimp {

// ---------------------------------------------------------------------------
/** AC3D (*.ac) importer class
*/
class AC3DImporter final : public BaseImporter {
public:
    /// @brief Default constructor
    AC3DImporter() = default;

    /// @brief Destructor.
    ~AC3DImporter() override = default;

    /// Represents an AC3D material
    struct Material {
        Material() :
                rgb(0.6f, 0.6f, 0.6f),
                spec(1.f, 1.f, 1.f),
                shin(0.f),
                trans(0.f) {}

        /// base color of the material
        aiColor3D rgb;

        /// ambient color of the material
        aiColor3D amb;

        /// emissive color of the material
        aiColor3D emis;

        /// specular color of the material
        aiColor3D spec;

        /// shininess exponent
        float shin;

        /// transparency. 0 == opaque
        float trans;

        /// name of the material. optional.
        std::string name;
    };

    /// Represents an AC3D surface
    struct Surface {
        Surface() = default;

        unsigned int mat{0};
        unsigned int flags{0};
        using SurfaceEntry = std::pair<unsigned int, aiVector2D>;
        std::vector<SurfaceEntry> entries{};

        // Type is low nibble of flags
        enum Type : uint8_t {
            Polygon = 0x0,
            ClosedLine = 0x1,
            OpenLine = 0x2,
            TriangleStrip = 0x4, // ACC extension (TORCS and Speed Dreams)

            Mask = 0xf,
        };

        uint8_t GetType() const { return (flags & Mask); }
    };

    /// Represents an AC3D object
    struct Object {
        Object() = default;

        /// Type description
        enum Type {
            World = 0x0,
            Poly = 0x1,
            Group = 0x2,
            Light = 0x4
        } type{};

        /// name of the object
        std::string name{};

        /// object children
        std::vector<Object> children{};

        /// texture to be assigned to all surfaces of the object
        /// the .acc format supports up to 4 textures
        std::vector<std::string> textures{};

        /// texture repat factor repeat
        aiVector2D texRepeat{1, 1};
        /// texture repat factor offset
        aiVector2D texOffset{0, 0};

        /// rotation matrix
        aiMatrix3x3 rotation{};

        /// translation vector
        aiVector3D translation{};

        /// vertices
        std::vector<aiVector3D> vertices{};

        /// surfaces
        std::vector<Surface> surfaces{};

        /// number of indices (= num vert's in verbose format)
        unsigned int numRefs{0};

        /// number of subdivisions to be performed on the imported data
        unsigned int subDiv{0};

        /// max angle limit for smoothing
        float crease{0.0f};
    };

public:
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler,
            bool checkSig) const override;

protected:
    // -------------------------------------------------------------------
    const aiImporterDesc *GetInfo() const override;

    // -------------------------------------------------------------------
    void InternReadFile(const std::string &pFile, aiScene *pScene,
            IOSystem *pIOHandler) override;

    // -------------------------------------------------------------------
    void SetupProperties(const Importer *pImp) override;

private:
    // -------------------------------------------------------------------
    /**
     * Get the next line from the file.
     * @return false if the end of the file was reached
     */
    bool GetNextLine();

    // -------------------------------------------------------------------
    /**
     * Load the object section. This method is called recursively to
     * load subobjects, the method returns after a 'kids 0' was
     * encountered.
     * @objects List of output objects
     */
    bool LoadObjectSection(std::vector<Object> &objects);

    // -------------------------------------------------------------------
    /**
     * Convert all objects into meshes and nodes.
     * @param [in] object Current object to work on
     * @param [in] meshes Pointer to the list of output meshes
     * @param [in] outMaterials List of output materials
     * @param [in] materials Material list
     * @param [in] Scenegraph node for the object
     * @return Scenegraph node for the object
     */
    aiNode *ConvertObjectSection(Object &object,
            std::vector<aiMesh *> &meshes,
            std::vector<aiMaterial *> &outMaterials,
            const std::vector<Material> &materials,
            aiNode *parent = nullptr);

    // -------------------------------------------------------------------
    /**
     * Convert a material
     * @param [in] object Current object
     * @param [in] matSrc Source material description
     * @param [in] matDest Destination material to be filled */
    void ConvertMaterial(const Object &object,
            const Material &matSrc,
            aiMaterial &matDest);

private:
    // points to the next data line
    aiBuffer mBuffer{};

    // Configuration option: if enabled, up to two meshes
    // are generated per material: those faces who have
    // their bf cull flags set are separated.
    bool configSplitBFCull{false};

    // Configuration switch: subdivision surfaces are only
    // evaluated if the value is true.
    bool configEvalSubdivision{false};

    // counts how many objects we have in the tree.
    // basing on this information we can find a
    // good estimate how many meshes we'll have in the final scene.
    unsigned int mNumMeshes{0};

    // current list of light sources
    std::vector<aiLight *> *mLights{};

    // name counters
    unsigned int mLightsCounter{0};
    unsigned int mGroupsCounter{0};
    unsigned int mPolysCounter{0};
    unsigned int mWorldsCounter{0};
};

} // end of namespace Assimp

#endif // AI_AC3DIMPORTER_H_INC
