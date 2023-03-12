/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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
---------------------------------------------------------------------------
*/

/** @file HL1MDLLoader.h
 *  @brief Declaration of the Half-Life 1 MDL loader.
 */

#ifndef AI_HL1MDLLOADER_INCLUDED
#define AI_HL1MDLLOADER_INCLUDED

#include "HL1FileData.h"
#include "HL1ImportSettings.h"
#include "UniqueNameGenerator.h"

#include <memory>
#include <string>

#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/IOSystem.hpp>
#include <assimp/DefaultIOSystem.h>
#include <assimp/Exceptional.h>

namespace Assimp {
namespace MDL {
namespace HalfLife {

class HL1MDLLoader {
public:
    HL1MDLLoader() = delete;
    HL1MDLLoader(const HL1MDLLoader &) = delete;

    /** See variables descriptions at the end for more details. */
    HL1MDLLoader(
        aiScene *scene,
        IOSystem *io,
        const unsigned char *buffer,
        const std::string &file_path,
        const HL1ImportSettings &import_settings);

    ~HL1MDLLoader();

    void load_file();

protected:
    /** \brief Validate the header data structure of a Half-Life 1 MDL file.
     * \param[in] header Input header to be validated.
     * \param[in] is_texture_header Whether or not we are reading an MDL
     *   texture file.
     */
    void validate_header(const Header_HL1 *header, bool is_texture_header);

    void load_texture_file();
    void load_sequence_groups_files();
    void read_textures();
    void read_skins();
    void read_bones();
    void read_meshes();
    void read_animations();
    void read_sequence_groups_info();
    void read_sequence_infos();
    void read_sequence_transitions();
    void read_attachments();
    void read_hitboxes();
    void read_bone_controllers();
    void read_global_info();

private:
    void release_resources();

    /** \brief Load a file and copy it's content to a buffer.
     * \param file_path The path to the file to be loaded.
     * \param buffer A pointer to a buffer to receive the data.
     */
    template <typename MDLFileHeader>
    void load_file_into_buffer(const std::string &file_path, unsigned char *&buffer);

    /** \brief Read an MDL texture.
     * \param[in] ptexture A pointer to an MDL texture.
     * \param[in] data A pointer to the data from \p ptexture.
     * \param[in] pal A pointer to the texture palette from \p ptexture.
     * \param[in,out] pResult A pointer to the output resulting Assimp texture.
     * \param[in,out] last_palette_color The last color from the image palette.
     */
    void read_texture(const Texture_HL1 *ptexture,
            uint8_t *data, uint8_t *pal, aiTexture *pResult,
            aiColor3D &last_palette_color);

    /** \brief This method reads a compressed anim value.
    * \param[in] panimvalue A pointer to the animation data.
    * \param[in] frame The frame to look for.
    * \param[in] bone_scale The current bone scale to apply to the compressed value.
    * \param[in,out] value The decompressed anim value at \p frame.
    */
    void extract_anim_value(const AnimValue_HL1 *panimvalue,
            int frame, float bone_scale, ai_real &value);

    /**
     *  \brief Given the number of blend animations, determine the number of blend controllers.
     *
     * \param[in] num_blend_animations The number of blend animations.
     * \param[out] num_blend_controllers The number of blend controllers.
     * \return True if the number of blend controllers was determined. False otherwise.
     */
    static bool get_num_blend_controllers(const int num_blend_animations, int &num_blend_controllers);

    /**
     *  \brief Build a bone's node children hierarchy.
     *
     * \param[in] bone The bone for which we must build all children hierarchy.
     */
    struct TempBone;
    void build_bone_children_hierarchy(const TempBone& bone);

    /** Output scene to be filled */
    aiScene *scene_;

    /** Output I/O handler. Required for additional IO operations. */
    IOSystem *io_;

    /** Buffer from MDLLoader class. */
    const unsigned char *buffer_;

    /** The full file path to the MDL file we are trying to load.
     * Used to locate other MDL files since MDL may store resources
     * in external MDL files. */
    const std::string &file_path_;

    /** Configuration for HL1 MDL */
    const HL1ImportSettings &import_settings_;

    /** Main MDL header. */
    const Header_HL1 *header_;

    /** External MDL texture header. */
    const Header_HL1 *texture_header_;

    /** External MDL animation headers.
     * One for each loaded animation file. */
    SequenceHeader_HL1 **anim_headers_;

    /** Texture file data. */
    unsigned char *texture_buffer_;

    /** Animation files data. */
    unsigned char **anim_buffers_;

    /** The number of sequence groups. */
    int num_sequence_groups_;

    /** The list of children to be appended to the scene's root node. */
    std::vector<aiNode *> rootnode_children_;

    /** A unique name generator. Used to generate names for MDL values
     * that may have empty/duplicate names. */
    UniqueNameGenerator unique_name_generator_;

    /** The list of unique sequence names. */
    std::vector<std::string> unique_sequence_names_;

    /** The list of unique sequence groups names. */
    std::vector<std::string> unique_sequence_groups_names_;

    /** Structure to store temporary bone information. */
    struct TempBone {

        TempBone() :
            node(nullptr),
            absolute_transform(),
            offset_matrix(),
            children() {}

        aiNode *node;
        aiMatrix4x4 absolute_transform;
        aiMatrix4x4 offset_matrix;
        std::vector<int> children; // Bone children
    };

    std::vector<TempBone> temp_bones_;

    /** The number of available bone controllers in the model. */
    int num_blend_controllers_;

    /** Self explanatory. */
    int total_models_;
};

// ------------------------------------------------------------------------------------------------
template <typename MDLFileHeader>
void HL1MDLLoader::load_file_into_buffer(const std::string &file_path, unsigned char *&buffer) {
    if (!io_->Exists(file_path))
        throw DeadlyImportError("Missing file ", DefaultIOSystem::fileName(file_path), ".");

    std::unique_ptr<IOStream> file(io_->Open(file_path));

    if (file == nullptr) {
        throw DeadlyImportError("Failed to open MDL file ", DefaultIOSystem::fileName(file_path), ".");
    }

    const size_t file_size = file->FileSize();
    if (file_size < sizeof(MDLFileHeader)) {
        throw DeadlyImportError("MDL file is too small.");
    }

    buffer = new unsigned char[1 + file_size];
    file->Read((void *)buffer, 1, file_size);
    buffer[file_size] = '\0';
}

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_HL1MDLLOADER_INCLUDED
