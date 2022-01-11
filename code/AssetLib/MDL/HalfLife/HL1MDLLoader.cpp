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

/** @file HL1MDLLoader.cpp
 *  @brief Implementation for the Half-Life 1 MDL loader.
 */

#include "HL1MDLLoader.h"
#include "HL1ImportDefinitions.h"
#include "HL1MeshTrivert.h"
#include "UniqueNameGenerator.h"

#include <assimp/BaseImporter.h>
#include <assimp/StringUtils.h>
#include <assimp/ai_assert.h>
#include <assimp/qnan.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>

#include <iomanip>
#include <sstream>
#include <map>

#ifdef MDL_HALFLIFE_LOG_WARN_HEADER
#undef MDL_HALFLIFE_LOG_WARN_HEADER
#endif
#define MDL_HALFLIFE_LOG_HEADER "[Half-Life 1 MDL] "
#include "LogFunctions.h"

namespace Assimp {
namespace MDL {
namespace HalfLife {

#ifdef _MSC_VER
#    pragma warning(disable : 4706)
#endif // _MSC_VER

// ------------------------------------------------------------------------------------------------
HL1MDLLoader::HL1MDLLoader(
    aiScene *scene,
    IOSystem *io,
    const unsigned char *buffer,
    const std::string &file_path,
    const HL1ImportSettings &import_settings) :
    scene_(scene),
    io_(io),
    buffer_(buffer),
    file_path_(file_path),
    import_settings_(import_settings),
    header_(nullptr),
    texture_header_(nullptr),
    anim_headers_(nullptr),
    texture_buffer_(nullptr),
    anim_buffers_(nullptr),
    num_sequence_groups_(0),
    rootnode_children_(),
    unique_name_generator_(),
    unique_sequence_names_(),
    unique_sequence_groups_names_(),
    temp_bones_(),
    num_blend_controllers_(0),
    total_models_(0) {
    load_file();
}

// ------------------------------------------------------------------------------------------------
HL1MDLLoader::~HL1MDLLoader() {
    release_resources();
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::release_resources() {
    if (buffer_ != texture_buffer_) {
        delete[] texture_buffer_;
        texture_buffer_ = nullptr;
    }

    if (num_sequence_groups_ && anim_buffers_) {
        for (int i = 1; i < num_sequence_groups_; ++i) {
            if (anim_buffers_[i]) {
                delete[] anim_buffers_[i];
                anim_buffers_[i] = nullptr;
            }
        }

        delete[] anim_buffers_;
        anim_buffers_ = nullptr;
    }

    if (anim_headers_) {
        delete[] anim_headers_;
        anim_headers_ = nullptr;
    }

    // Root has some children nodes. so let's proceed them
    if (!rootnode_children_.empty()) {
        // Here, it means that the nodes were not added to the
        // scene root node. We still have to delete them.
        for (auto it = rootnode_children_.begin(); it != rootnode_children_.end(); ++it) {
            if (*it) {
                delete *it;
            }
        }
        // Ensure this happens only once.
        rootnode_children_.clear();
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::load_file() {
    try {
        header_ = (const Header_HL1 *)buffer_;
        validate_header(header_, false);

        // Create the root scene node.
        scene_->mRootNode = new aiNode(AI_MDL_HL1_NODE_ROOT);

        load_texture_file();

        if (import_settings_.read_animations) {
            load_sequence_groups_files();
        }

        read_textures();
        read_skins();

        read_bones();
        read_meshes();

        if (import_settings_.read_animations) {
            read_sequence_groups_info();
            read_animations();
            read_sequence_infos();
            if (import_settings_.read_sequence_transitions)
                read_sequence_transitions();
        }

        if (import_settings_.read_attachments) {
            read_attachments();
        }

        if (import_settings_.read_hitboxes) {
            read_hitboxes();
        }

        if (import_settings_.read_bone_controllers) {
            read_bone_controllers();
        }

        read_global_info();

        if (!header_->numbodyparts) {
            // This could be an MDL external texture file. In this case,
            // add this flag to allow the scene to be loaded even if it
            // has no meshes.
            scene_->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
        }

        // Append children to root node.
        if (rootnode_children_.size()) {
            scene_->mRootNode->addChildren(
                    static_cast<unsigned int>(rootnode_children_.size()),
                    rootnode_children_.data());

            // Clear the list of nodes so they will not be destroyed
            // when resources are released.
            rootnode_children_.clear();
        }

        release_resources();

    } catch (...) {
        release_resources();
        throw;
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::validate_header(const Header_HL1 *header, bool is_texture_header) {
    if (is_texture_header) {
        // Every single Half-Life model is assumed to have at least one texture.
        if (!header->numtextures) {
            throw DeadlyImportError(MDL_HALFLIFE_LOG_HEADER "There are no textures in the file");
        }

        if (header->numtextures > AI_MDL_HL1_MAX_TEXTURES) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_TEXTURES>(header->numtextures, "textures");
        }

        if (header->numskinfamilies > AI_MDL_HL1_MAX_SKIN_FAMILIES) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_SKIN_FAMILIES>(header->numskinfamilies, "skin families");
        }

    } else {

        if (header->numbodyparts > AI_MDL_HL1_MAX_BODYPARTS) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_BODYPARTS>(header->numbodyparts, "bodyparts");
        }

        if (header->numbones > AI_MDL_HL1_MAX_BONES) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_BONES>(header->numbones, "bones");
        }

        if (header->numbonecontrollers > AI_MDL_HL1_MAX_BONE_CONTROLLERS) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_BONE_CONTROLLERS>(header->numbonecontrollers, "bone controllers");
        }

        if (header->numseq > AI_MDL_HL1_MAX_SEQUENCES) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_SEQUENCES>(header->numseq, "sequences");
        }

        if (header->numseqgroups > AI_MDL_HL1_MAX_SEQUENCE_GROUPS) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_SEQUENCE_GROUPS>(header->numseqgroups, "sequence groups");
        }

        if (header->numattachments > AI_MDL_HL1_MAX_ATTACHMENTS) {
            log_warning_limit_exceeded<AI_MDL_HL1_MAX_ATTACHMENTS>(header->numattachments, "attachments");
        }
    }
}

// ------------------------------------------------------------------------------------------------
/*
    Load textures.

    There are two ways for textures to be stored in a Half-Life model:

    1. Directly in the MDL file (filePath) or
    2. In an external MDL file.

    Due to the way StudioMDL works (tool used to compile SMDs into MDLs),
    it is assumed that an external texture file follows the naming
    convention: <YourModelName>T.mdl. Note the extra (T) at the end of the
    model name.

    .e.g For a given model named MyModel.mdl

    The external texture file name would be MyModelT.mdl
*/
void HL1MDLLoader::load_texture_file() {
    if (header_->numtextures == 0) {
        // Load an external MDL texture file.
        std::string texture_file_path =
                DefaultIOSystem::absolutePath(file_path_) + io_->getOsSeparator() +
                DefaultIOSystem::completeBaseName(file_path_) + "T." +
                BaseImporter::GetExtension(file_path_);

        load_file_into_buffer<Header_HL1>(texture_file_path, texture_buffer_);
    } else {
        // Model has no external texture file. This means the texture is stored inside the main MDL file.
        texture_buffer_ = const_cast<unsigned char *>(buffer_);
    }

    texture_header_ = (const Header_HL1 *)texture_buffer_;

    // Validate texture header.
    validate_header(texture_header_, true);
}

// ------------------------------------------------------------------------------------------------
/*
    Load sequence group files if any.

    Due to the way StudioMDL works (tool used to compile SMDs into MDLs),
    it is assumed that a sequence group file follows the naming
    convention: <YourModelName>0X.mdl. Note the extra (0X) at the end of
    the model name, where (X) is the sequence group.

    .e.g For a given model named MyModel.mdl

    Sequence group 1 => MyModel01.mdl
    Sequence group 2 => MyModel02.mdl
    Sequence group X => MyModel0X.mdl

*/
void HL1MDLLoader::load_sequence_groups_files() {
    if (header_->numseqgroups <= 1) {
        return;
    }

    num_sequence_groups_ = header_->numseqgroups;

    anim_buffers_ = new unsigned char *[num_sequence_groups_];
    anim_headers_ = new SequenceHeader_HL1 *[num_sequence_groups_];
    for (int i = 0; i < num_sequence_groups_; ++i) {
        anim_buffers_[i] = nullptr;
        anim_headers_[i] = nullptr;
    }

    std::string file_path_without_extension =
            DefaultIOSystem::absolutePath(file_path_) +
            io_->getOsSeparator() +
            DefaultIOSystem::completeBaseName(file_path_);

    for (int i = 1; i < num_sequence_groups_; ++i) {
        std::stringstream ss;
        ss << file_path_without_extension;
        ss << std::setw(2) << std::setfill('0') << i;
        ss << '.' << BaseImporter::GetExtension(file_path_);

        std::string sequence_file_path = ss.str();

        load_file_into_buffer<SequenceHeader_HL1>(sequence_file_path, anim_buffers_[i]);

        anim_headers_[i] = (SequenceHeader_HL1 *)anim_buffers_[i];
    }
}

// ------------------------------------------------------------------------------------------------
// Read an MDL texture.
void HL1MDLLoader::read_texture(const Texture_HL1 *ptexture,
        uint8_t *data, uint8_t *pal, aiTexture *pResult,
        aiColor3D &last_palette_color) {
    pResult->mFilename = ptexture->name;
    pResult->mWidth = static_cast<unsigned int>(ptexture->width);
    pResult->mHeight = static_cast<unsigned int>(ptexture->height);
    pResult->achFormatHint[0] = 'r';
    pResult->achFormatHint[1] = 'g';
    pResult->achFormatHint[2] = 'b';
    pResult->achFormatHint[3] = 'a';
    pResult->achFormatHint[4] = '8';
    pResult->achFormatHint[5] = '8';
    pResult->achFormatHint[6] = '8';
    pResult->achFormatHint[7] = '8';
    pResult->achFormatHint[8] = '\0';

    const size_t num_pixels = pResult->mWidth * pResult->mHeight;
    aiTexel *out = pResult->pcData = new aiTexel[num_pixels];

    // Convert indexed 8 bit to 32 bit RGBA.
    for (size_t i = 0; i < num_pixels; ++i, ++out) {
        out->r = pal[data[i] * 3];
        out->g = pal[data[i] * 3 + 1];
        out->b = pal[data[i] * 3 + 2];
        out->a = 255;
    }

    // Get the last palette color.
    last_palette_color.r = pal[255 * 3];
    last_palette_color.g = pal[255 * 3 + 1];
    last_palette_color.b = pal[255 * 3 + 2];
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_textures() {
    const Texture_HL1 *ptexture = (const Texture_HL1 *)((uint8_t *)texture_header_ + texture_header_->textureindex);
    unsigned char *pin = texture_buffer_;

    scene_->mNumTextures = scene_->mNumMaterials = texture_header_->numtextures;
    scene_->mTextures = new aiTexture *[scene_->mNumTextures];
    scene_->mMaterials = new aiMaterial *[scene_->mNumMaterials];

    for (int i = 0; i < texture_header_->numtextures; ++i) {
        scene_->mTextures[i] = new aiTexture();

        aiColor3D last_palette_color;
        read_texture(&ptexture[i],
                pin + ptexture[i].index,
                pin + ptexture[i].width * ptexture[i].height + ptexture[i].index,
                scene_->mTextures[i],
                last_palette_color);

        aiMaterial *scene_material = scene_->mMaterials[i] = new aiMaterial();

        const aiTextureType texture_type = aiTextureType_DIFFUSE;
        aiString texture_name(ptexture[i].name);
        scene_material->AddProperty(&texture_name, AI_MATKEY_TEXTURE(texture_type, 0));

        // Is this a chrome texture?
        int chrome = ptexture[i].flags & AI_MDL_HL1_STUDIO_NF_CHROME ? 1 : 0;
        scene_material->AddProperty(&chrome, 1, AI_MDL_HL1_MATKEY_CHROME(texture_type, 0));

        if (ptexture[i].flags & AI_MDL_HL1_STUDIO_NF_FLATSHADE) {
            // Flat shading.
            const aiShadingMode shading_mode = aiShadingMode_Flat;
            scene_material->AddProperty(&shading_mode, 1, AI_MATKEY_SHADING_MODEL);
        }

        if (ptexture[i].flags & AI_MDL_HL1_STUDIO_NF_ADDITIVE) {
            // Additive texture.
            const aiBlendMode blend_mode = aiBlendMode_Additive;
            scene_material->AddProperty(&blend_mode, 1, AI_MATKEY_BLEND_FUNC);
        } else if (ptexture[i].flags & AI_MDL_HL1_STUDIO_NF_MASKED) {
            // Texture with 1 bit alpha test.
            const aiTextureFlags use_alpha = aiTextureFlags_UseAlpha;
            scene_material->AddProperty(&use_alpha, 1, AI_MATKEY_TEXFLAGS(texture_type, 0));
            scene_material->AddProperty(&last_palette_color, 1, AI_MATKEY_COLOR_TRANSPARENT);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_skins() {
    // Read skins, if any.
    if (texture_header_->numskinfamilies <= 1) {
        return;
    }

    // Pointer to base texture index.
    short *default_skin_ptr = (short *)((uint8_t *)texture_header_ + texture_header_->skinindex);

    // Start at first replacement skin.
    short *replacement_skin_ptr = default_skin_ptr + texture_header_->numskinref;

    for (int i = 1; i < texture_header_->numskinfamilies; ++i, replacement_skin_ptr += texture_header_->numskinref) {
        for (int j = 0; j < texture_header_->numskinref; ++j) {
            if (default_skin_ptr[j] != replacement_skin_ptr[j]) {
                // Save replacement textures.
                aiString skinMaterialId(scene_->mTextures[replacement_skin_ptr[j]]->mFilename);
                scene_->mMaterials[default_skin_ptr[j]]->AddProperty(&skinMaterialId, AI_MATKEY_TEXTURE_DIFFUSE(i));
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_bones() {
    if (!header_->numbones) {
        return;
    }

    const Bone_HL1 *pbone = (const Bone_HL1 *)((uint8_t *)header_ + header_->boneindex);

    std::vector<std::string> unique_bones_names(header_->numbones);
    for (int i = 0; i < header_->numbones; ++i) {
        unique_bones_names[i] = pbone[i].name;
    }

    // Ensure bones have unique names.
    unique_name_generator_.set_template_name("Bone");
    unique_name_generator_.make_unique(unique_bones_names);

    temp_bones_.resize(header_->numbones);

    aiNode *bones_node = new aiNode(AI_MDL_HL1_NODE_BONES);
    rootnode_children_.push_back(bones_node);
    bones_node->mNumChildren = static_cast<unsigned int>(header_->numbones);
    bones_node->mChildren = new aiNode *[bones_node->mNumChildren];

    // Create bone matrices in local space.
    for (int i = 0; i < header_->numbones; ++i) {
        aiNode *bone_node = temp_bones_[i].node = bones_node->mChildren[i] = new aiNode(unique_bones_names[i]);

        aiVector3D angles(pbone[i].value[3], pbone[i].value[4], pbone[i].value[5]);
        temp_bones_[i].absolute_transform = bone_node->mTransformation =
                aiMatrix4x4(aiVector3D(1), aiQuaternion(angles.y, angles.z, angles.x),
                        aiVector3D(pbone[i].value[0], pbone[i].value[1], pbone[i].value[2]));

        if (pbone[i].parent == -1) {
            bone_node->mParent = scene_->mRootNode;
        } else {
            bone_node->mParent = bones_node->mChildren[pbone[i].parent];

            temp_bones_[i].absolute_transform =
                    temp_bones_[pbone[i].parent].absolute_transform * bone_node->mTransformation;
        }

        temp_bones_[i].offset_matrix = temp_bones_[i].absolute_transform;
        temp_bones_[i].offset_matrix.Inverse();
    }
}

// ------------------------------------------------------------------------------------------------
/*
    Read meshes.

    Half-Life MDLs are structured such that each MDL
    contains one or more 'bodypart(s)', which contain one
    or more 'model(s)', which contains one or more mesh(es).

    * Bodyparts are used to group models that may be replaced
    in the game .e.g a character could have a 'heads' group,
    'torso' group, 'shoes' group, with each group containing
    different 'model(s)'.

    * Models, also called 'sub models', contain vertices as
    well as a reference to each mesh used by the sub model.

    * Meshes contain a list of tris, also known as 'triverts'.
    Each tris contains the following information:

        1. The index of the position to use for the vertex.
        2. The index of the normal to use for the vertex.
        3. The S coordinate to use for the vertex UV.
        4. The T coordinate ^

    These tris represent the way to represent the triangles
    for each mesh. Depending on how the tool compiled the MDL,
    those triangles were saved as strips and or fans.

    NOTE: Each tris is NOT unique. This means that you
    might encounter the same vertex index but with a different
    normal index, S coordinate, T coordinate.

    In addition, each mesh contains the texture's index.

    ------------------------------------------------------
    With the details above, there are several things to
    take into consideration.

    * The Half-Life models store the vertices by sub model
    rather than by mesh. Due to Assimp's structure, it
    is necessary to remap each model vertex to be used
    per mesh. Unfortunately, this has the consequence
    to duplicate vertices.

    * Because the mesh triangles are comprised of strips and
    fans, it is necessary to convert each primitive to
    triangles, respectively (3 indices per face).
*/
void HL1MDLLoader::read_meshes() {
    if (!header_->numbodyparts) {
        return;
    }

    int total_verts = 0;
    int total_triangles = 0;
    total_models_ = 0;

    const Bodypart_HL1 *pbodypart = (const Bodypart_HL1 *)((uint8_t *)header_ + header_->bodypartindex);
    const Model_HL1 *pmodel = nullptr;
    const Mesh_HL1 *pmesh = nullptr;

    const Texture_HL1 *ptexture = (const Texture_HL1 *)((uint8_t *)texture_header_ + texture_header_->textureindex);
    short *pskinref = (short *)((uint8_t *)texture_header_ + texture_header_->skinindex);

    scene_->mNumMeshes = 0;

    std::vector<std::string> unique_bodyparts_names;
    unique_bodyparts_names.resize(header_->numbodyparts);

    // Count the number of meshes.

    for (int i = 0; i < header_->numbodyparts; ++i, ++pbodypart) {
        unique_bodyparts_names[i] = pbodypart->name;

        pmodel = (Model_HL1 *)((uint8_t *)header_ + pbodypart->modelindex);
        for (int j = 0; j < pbodypart->nummodels; ++j, ++pmodel) {
            scene_->mNumMeshes += pmodel->nummesh;
            total_verts += pmodel->numverts;
        }

        total_models_ += pbodypart->nummodels;
    }

    // Display limit infos.
    if (total_verts > AI_MDL_HL1_MAX_VERTICES) {
        log_warning_limit_exceeded<AI_MDL_HL1_MAX_VERTICES>(total_verts, "vertices");
    }

    if (scene_->mNumMeshes > AI_MDL_HL1_MAX_MESHES) {
        log_warning_limit_exceeded<AI_MDL_HL1_MAX_MESHES>(scene_->mNumMeshes, "meshes");
    }

    if (total_models_ > AI_MDL_HL1_MAX_MODELS) {
        log_warning_limit_exceeded<AI_MDL_HL1_MAX_MODELS>(total_models_, "models");
    }

    // Ensure bodyparts have unique names.
    unique_name_generator_.set_template_name("Bodypart");
    unique_name_generator_.make_unique(unique_bodyparts_names);

    // Now do the same for each model.
    pbodypart = (const Bodypart_HL1 *)((uint8_t *)header_ + header_->bodypartindex);

    // Prepare template name for bodypart models.
    std::vector<std::string> unique_models_names;
    unique_models_names.resize(total_models_);

    unsigned int model_index = 0;

    for (int i = 0; i < header_->numbodyparts; ++i, ++pbodypart) {
        pmodel = (Model_HL1 *)((uint8_t *)header_ + pbodypart->modelindex);
        for (int j = 0; j < pbodypart->nummodels; ++j, ++pmodel, ++model_index)
            unique_models_names[model_index] = pmodel->name;
    }

    unique_name_generator_.set_template_name("Model");
    unique_name_generator_.make_unique(unique_models_names);

    unsigned int mesh_index = 0;

    scene_->mMeshes = new aiMesh *[scene_->mNumMeshes];

    pbodypart = (const Bodypart_HL1 *)((uint8_t *)header_ + header_->bodypartindex);

    /* Create a node that will represent the mesh hierarchy.

        <MDL_bodyparts>
            |
            +-- bodypart --+-- model -- [mesh index, mesh index, ...]
            |              |
            |              +-- model -- [mesh index, mesh index, ...]
            |              |
            |              ...
            |
            |-- bodypart -- ...
            |
            ...
     */
    aiNode *bodyparts_node = new aiNode(AI_MDL_HL1_NODE_BODYPARTS);
    rootnode_children_.push_back(bodyparts_node);
    bodyparts_node->mNumChildren = static_cast<unsigned int>(header_->numbodyparts);
    bodyparts_node->mChildren = new aiNode *[bodyparts_node->mNumChildren];
    aiNode **bodyparts_node_ptr = bodyparts_node->mChildren;

    // The following variables are defined here so they don't have
    // to be recreated every iteration.

    // Model_HL1 vertices, in bind pose space.
    std::vector<aiVector3D> bind_pose_vertices;

    // Model_HL1 normals, in bind pose space.
    std::vector<aiVector3D> bind_pose_normals;

    // Used to contain temporary information for building a mesh.
    std::vector<HL1MeshTrivert> triverts;

    std::vector<short> tricmds;

    // Which triverts to use for the mesh.
    std::vector<short> mesh_triverts_indices;

    std::vector<HL1MeshFace> mesh_faces;

    /* triverts that have the same vertindex, but have different normindex,s,t values.
       Similar triverts are mapped from vertindex to a list of similar triverts. */
    std::map<short, std::set<short>> triverts_similars;

    // triverts per bone.
    std::map<int, std::set<short>> bone_triverts;

    /** This function adds a trivert index to the list of triverts per bone.
     * \param[in] bone The bone that affects the trivert at index \p trivert_index.
     * \param[in] trivert_index The trivert index.
     */
    auto AddTrivertToBone = [&](int bone, short trivert_index) {
        if (bone_triverts.count(bone) == 0)
            bone_triverts.insert({ bone, std::set<short>{ trivert_index }});
        else
            bone_triverts[bone].insert(trivert_index);
    };

    /** This function creates and appends a new trivert to the list of triverts.
     * \param[in] trivert The trivert to use as a prototype.
     * \param[in] bone The bone that affects \p trivert.
     */
    auto AddSimilarTrivert = [&](const Trivert &trivert, const int bone) {
        HL1MeshTrivert new_trivert(trivert);
        new_trivert.localindex = static_cast<short>(mesh_triverts_indices.size());

        short new_trivert_index = static_cast<short>(triverts.size());

        if (triverts_similars.count(trivert.vertindex) == 0)
            triverts_similars.insert({ trivert.vertindex, std::set<short>{ new_trivert_index }});
        else
            triverts_similars[trivert.vertindex].insert(new_trivert_index);

        triverts.push_back(new_trivert);

        mesh_triverts_indices.push_back(new_trivert_index);
        tricmds.push_back(new_trivert.localindex);
        AddTrivertToBone(bone, new_trivert.localindex);
    };

    model_index = 0;

    for (int i = 0; i < header_->numbodyparts; ++i, ++pbodypart, ++bodyparts_node_ptr) {
        pmodel = (const Model_HL1 *)((uint8_t *)header_ + pbodypart->modelindex);

        // Create bodypart node for the mesh tree hierarchy.
        aiNode *bodypart_node = (*bodyparts_node_ptr) = new aiNode(unique_bodyparts_names[i]);
        bodypart_node->mParent = bodyparts_node;
        bodypart_node->mMetaData = aiMetadata::Alloc(1);
        bodypart_node->mMetaData->Set(0, "Base", pbodypart->base);

        bodypart_node->mNumChildren = static_cast<unsigned int>(pbodypart->nummodels);
        bodypart_node->mChildren = new aiNode *[bodypart_node->mNumChildren];
        aiNode **bodypart_models_ptr = bodypart_node->mChildren;

        for (int j = 0; j < pbodypart->nummodels;
                ++j, ++pmodel, ++bodypart_models_ptr, ++model_index) {

            pmesh = (const Mesh_HL1 *)((uint8_t *)header_ + pmodel->meshindex);

            uint8_t *pvertbone = ((uint8_t *)header_ + pmodel->vertinfoindex);
            uint8_t *pnormbone = ((uint8_t *)header_ + pmodel->norminfoindex);
            vec3_t *pstudioverts = (vec3_t *)((uint8_t *)header_ + pmodel->vertindex);
            vec3_t *pstudionorms = (vec3_t *)((uint8_t *)header_ + pmodel->normindex);

            // Each vertex and normal is in local space, so transform
            // each of them to bring them in bind pose.
            bind_pose_vertices.resize(pmodel->numverts);
            bind_pose_normals.resize(pmodel->numnorms);
            for (size_t k = 0; k < bind_pose_vertices.size(); ++k) {
                const vec3_t &vert = pstudioverts[k];
                bind_pose_vertices[k] = temp_bones_[pvertbone[k]].absolute_transform * aiVector3D(vert[0], vert[1], vert[2]);
            }
            for (size_t k = 0; k < bind_pose_normals.size(); ++k) {
                const vec3_t &norm = pstudionorms[k];
                // Compute the normal matrix to transform the normal into bind pose,
                // without affecting its length.
                const aiMatrix4x4 normal_matrix = aiMatrix4x4(temp_bones_[pnormbone[k]].absolute_transform).Inverse().Transpose();
                bind_pose_normals[k] = normal_matrix * aiVector3D(norm[0], norm[1], norm[2]);
            }

            // Create model node for the mesh tree hierarchy.
            aiNode *model_node = (*bodypart_models_ptr) = new aiNode(unique_models_names[model_index]);
            model_node->mParent = bodypart_node;
            model_node->mNumMeshes = static_cast<unsigned int>(pmodel->nummesh);
            model_node->mMeshes = new unsigned int[model_node->mNumMeshes];
            unsigned int *model_meshes_ptr = model_node->mMeshes;

            for (int k = 0; k < pmodel->nummesh; ++k, ++pmesh, ++mesh_index, ++model_meshes_ptr) {
                *model_meshes_ptr = mesh_index;

                // Read triverts.
                short *ptricmds = (short *)((uint8_t *)header_ + pmesh->triindex);
                float texcoords_s_scale = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
                float texcoords_t_scale = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

                // Reset the data for the upcoming mesh.
                triverts.clear();
                triverts.resize(pmodel->numverts);
                mesh_triverts_indices.clear();
                mesh_faces.clear();
                triverts_similars.clear();
                bone_triverts.clear();

                int l;
                while ((l = *(ptricmds++))) {
                    bool is_triangle_fan = false;

                    if (l < 0) {
                        l = -l;
                        is_triangle_fan = true;
                    }

                    // Clear the list of tris for the upcoming tris.
                    tricmds.clear();

                    for (; l > 0; l--, ptricmds += 4) {
                        const Trivert *input_trivert = reinterpret_cast<const Trivert *>(ptricmds);
                        const int bone = pvertbone[input_trivert->vertindex];

                        HL1MeshTrivert *private_trivert = &triverts[input_trivert->vertindex];
                        if (private_trivert->localindex == -1) {
                            // First time referenced.
                            *private_trivert = *input_trivert;
                            private_trivert->localindex = static_cast<short>(mesh_triverts_indices.size());
                            mesh_triverts_indices.push_back(input_trivert->vertindex);
                            tricmds.push_back(private_trivert->localindex);
                            AddTrivertToBone(bone, private_trivert->localindex);
                        } else if (*private_trivert == *input_trivert) {
                            // Exists and is the same.
                            tricmds.push_back(private_trivert->localindex);
                        } else {
                            // No similar trivert associated to the trivert currently processed.
                            if (triverts_similars.count(input_trivert->vertindex) == 0)
                                AddSimilarTrivert(*input_trivert, bone);
                            else {
                                // Search in the list of similar triverts to see if the
                                // trivert in process is already registered.
                                short similar_index = -1;
                                for (auto it = triverts_similars[input_trivert->vertindex].cbegin();
                                        similar_index == -1 && it != triverts_similars[input_trivert->vertindex].cend();
                                        ++it) {
                                    if (triverts[*it] == *input_trivert)
                                        similar_index = *it;
                                }

                                // If a similar trivert has been found, reuse it.
                                // Otherwise, add it.
                                if (similar_index == -1)
                                    AddSimilarTrivert(*input_trivert, bone);
                                else
                                    tricmds.push_back(triverts[similar_index].localindex);
                            }
                        }
                    }

                    // Build mesh faces.
                    const int num_faces = static_cast<int>(tricmds.size() - 2);
                    mesh_faces.reserve(num_faces);

                    if (is_triangle_fan) {
                        for (int faceIdx = 0; faceIdx < num_faces; ++faceIdx) {
                            mesh_faces.push_back(HL1MeshFace{
                                    tricmds[0],
                                    tricmds[faceIdx + 1],
                                    tricmds[faceIdx + 2] });
                        }
                    } else {
                        for (int faceIdx = 0; faceIdx < num_faces; ++faceIdx) {
                            if (faceIdx & 1) {
                                // Preserve winding order.
                                mesh_faces.push_back(HL1MeshFace{
                                        tricmds[faceIdx + 1],
                                        tricmds[faceIdx],
                                        tricmds[faceIdx + 2] });
                            } else {
                                mesh_faces.push_back(HL1MeshFace{
                                        tricmds[faceIdx],
                                        tricmds[faceIdx + 1],
                                        tricmds[faceIdx + 2] });
                            }
                        }
                    }

                    total_triangles += num_faces;
                }

                // Create the scene mesh.
                aiMesh *scene_mesh = scene_->mMeshes[mesh_index] = new aiMesh();
                scene_mesh->mPrimitiveTypes = aiPrimitiveType::aiPrimitiveType_TRIANGLE;
                scene_mesh->mMaterialIndex = pskinref[pmesh->skinref];

                scene_mesh->mNumVertices = static_cast<unsigned int>(mesh_triverts_indices.size());

                if (scene_mesh->mNumVertices) {
                    scene_mesh->mVertices = new aiVector3D[scene_mesh->mNumVertices];
                    scene_mesh->mNormals = new aiVector3D[scene_mesh->mNumVertices];

                    scene_mesh->mNumUVComponents[0] = 2;
                    scene_mesh->mTextureCoords[0] = new aiVector3D[scene_mesh->mNumVertices];

                    // Add vertices.
                    for (unsigned int v = 0; v < scene_mesh->mNumVertices; ++v) {
                        const HL1MeshTrivert *pTrivert = &triverts[mesh_triverts_indices[v]];
                        scene_mesh->mVertices[v] = bind_pose_vertices[pTrivert->vertindex];
                        scene_mesh->mNormals[v] = bind_pose_normals[pTrivert->normindex];
                        scene_mesh->mTextureCoords[0][v] = aiVector3D(
                                pTrivert->s * texcoords_s_scale,
                                pTrivert->t * -texcoords_t_scale, 0);
                    }

                    // Add face and indices.
                    scene_mesh->mNumFaces = static_cast<unsigned int>(mesh_faces.size());
                    scene_mesh->mFaces = new aiFace[scene_mesh->mNumFaces];

                    for (unsigned int f = 0; f < scene_mesh->mNumFaces; ++f) {
                        aiFace *face = &scene_mesh->mFaces[f];
                        face->mNumIndices = 3;
                        face->mIndices = new unsigned int[3];
                        face->mIndices[0] = mesh_faces[f].v2;
                        face->mIndices[1] = mesh_faces[f].v1;
                        face->mIndices[2] = mesh_faces[f].v0;
                    }

                    // Add mesh bones.
                    scene_mesh->mNumBones = static_cast<unsigned int>(bone_triverts.size());
                    scene_mesh->mBones = new aiBone *[scene_mesh->mNumBones];

                    aiBone **scene_bone_ptr = scene_mesh->mBones;

                    for (auto bone_it = bone_triverts.cbegin();
                            bone_it != bone_triverts.cend();
                            ++bone_it, ++scene_bone_ptr) {
                        const int bone_index = bone_it->first;

                        aiBone *scene_bone = (*scene_bone_ptr) = new aiBone();
                        scene_bone->mName = temp_bones_[bone_index].node->mName;

                        scene_bone->mOffsetMatrix = temp_bones_[bone_index].offset_matrix;

                        auto vertex_ids = bone_triverts.at(bone_index);

                        // Add vertex weight per bone.
                        scene_bone->mNumWeights = static_cast<unsigned int>(vertex_ids.size());
                        aiVertexWeight *vertex_weight_ptr = scene_bone->mWeights = new aiVertexWeight[scene_bone->mNumWeights];

                        for (auto vertex_it = vertex_ids.begin();
                                vertex_it != vertex_ids.end();
                                ++vertex_it, ++vertex_weight_ptr) {
                            vertex_weight_ptr->mVertexId = *vertex_it;
                            vertex_weight_ptr->mWeight = 1.0f;
                        }
                    }
                }
            }
        }
    }

    if (total_triangles > AI_MDL_HL1_MAX_TRIANGLES) {
        log_warning_limit_exceeded<AI_MDL_HL1_MAX_TRIANGLES>(total_triangles, "triangles");
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_animations() {
    if (!header_->numseq) {
        return;
    }

    const SequenceDesc_HL1 *pseqdesc = (const SequenceDesc_HL1 *)((uint8_t *)header_ + header_->seqindex);
    const SequenceGroup_HL1 *pseqgroup = nullptr;
    const AnimValueOffset_HL1 *panim = nullptr;
    const AnimValue_HL1 *panimvalue = nullptr;

    unique_sequence_names_.resize(header_->numseq);
    for (int i = 0; i < header_->numseq; ++i)
        unique_sequence_names_[i] = pseqdesc[i].label;

    // Ensure sequences have unique names.
    unique_name_generator_.set_template_name("Sequence");
    unique_name_generator_.make_unique(unique_sequence_names_);

    scene_->mNumAnimations = 0;

    int highest_num_blend_animations = SequenceBlendMode_HL1::NoBlend;

    // Count the total number of animations.
    for (int i = 0; i < header_->numseq; ++i, ++pseqdesc) {
        scene_->mNumAnimations += pseqdesc->numblends;
        highest_num_blend_animations = std::max(pseqdesc->numblends, highest_num_blend_animations);
    }

    // Get the number of available blend controllers for global info.
    get_num_blend_controllers(highest_num_blend_animations, num_blend_controllers_);

    pseqdesc = (const SequenceDesc_HL1 *)((uint8_t *)header_ + header_->seqindex);

    aiAnimation **scene_animations_ptr = scene_->mAnimations = new aiAnimation *[scene_->mNumAnimations];

    for (int sequence = 0; sequence < header_->numseq; ++sequence, ++pseqdesc) {
        pseqgroup = (const SequenceGroup_HL1 *)((uint8_t *)header_ + header_->seqgroupindex) + pseqdesc->seqgroup;

        if (pseqdesc->seqgroup == 0) {
            panim = (const AnimValueOffset_HL1 *)((uint8_t *)header_ + pseqgroup->unused2 + pseqdesc->animindex);
        } else {
            panim = (const AnimValueOffset_HL1 *)((uint8_t *)anim_headers_[pseqdesc->seqgroup] + pseqdesc->animindex);
        }

        for (int blend = 0; blend < pseqdesc->numblends; ++blend, ++scene_animations_ptr) {

            const Bone_HL1 *pbone = (const Bone_HL1 *)((uint8_t *)header_ + header_->boneindex);

            aiAnimation *scene_animation = (*scene_animations_ptr) = new aiAnimation();

            scene_animation->mName = unique_sequence_names_[sequence];
            scene_animation->mTicksPerSecond = pseqdesc->fps;
            scene_animation->mDuration = static_cast<double>(pseqdesc->fps) * pseqdesc->numframes;
            scene_animation->mNumChannels = static_cast<unsigned int>(header_->numbones);
            scene_animation->mChannels = new aiNodeAnim *[scene_animation->mNumChannels];

            for (int bone = 0; bone < header_->numbones; bone++, ++pbone, ++panim) {
                aiNodeAnim *node_anim = scene_animation->mChannels[bone] = new aiNodeAnim();
                node_anim->mNodeName = temp_bones_[bone].node->mName;

                node_anim->mNumPositionKeys = pseqdesc->numframes;
                node_anim->mNumRotationKeys = node_anim->mNumPositionKeys;
                node_anim->mNumScalingKeys = 0;

                node_anim->mPositionKeys = new aiVectorKey[node_anim->mNumPositionKeys];
                node_anim->mRotationKeys = new aiQuatKey[node_anim->mNumRotationKeys];

                for (int frame = 0; frame < pseqdesc->numframes; ++frame) {
                    aiVectorKey *position_key = &node_anim->mPositionKeys[frame];
                    aiQuatKey *rotation_key = &node_anim->mRotationKeys[frame];

                    aiVector3D angle1;
                    for (int j = 0; j < 3; ++j) {
                        if (panim->offset[j + 3] != 0) {
                            // Read compressed rotation delta.
                            panimvalue = (const AnimValue_HL1 *)((uint8_t *)panim + panim->offset[j + 3]);
                            extract_anim_value(panimvalue, frame, pbone->scale[j + 3], angle1[j]);
                        }

                        // Add the default rotation value.
                        angle1[j] += pbone->value[j + 3];

                        if (panim->offset[j] != 0) {
                            // Read compressed position delta.
                            panimvalue = (const AnimValue_HL1 *)((uint8_t *)panim + panim->offset[j]);
                            extract_anim_value(panimvalue, frame, pbone->scale[j], position_key->mValue[j]);
                        }

                        // Add the default position value.
                        position_key->mValue[j] += pbone->value[j];
                    }

                    position_key->mTime = rotation_key->mTime = static_cast<double>(frame);
                    /* The Half-Life engine uses X as forward, Y as left, Z as up. Therefore,
                       pitch,yaw,roll is represented as (YZX). */
                    rotation_key->mValue = aiQuaternion(angle1.y, angle1.z, angle1.x);
                    rotation_key->mValue.Normalize();
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_sequence_groups_info() {
    if (!header_->numseqgroups) {
        return;
    }

    aiNode *sequence_groups_node = new aiNode(AI_MDL_HL1_NODE_SEQUENCE_GROUPS);
    rootnode_children_.push_back(sequence_groups_node);

    sequence_groups_node->mNumChildren = static_cast<unsigned int>(header_->numseqgroups);
    sequence_groups_node->mChildren = new aiNode *[sequence_groups_node->mNumChildren];

    const SequenceGroup_HL1 *pseqgroup = (const SequenceGroup_HL1 *)((uint8_t *)header_ + header_->seqgroupindex);

    unique_sequence_groups_names_.resize(header_->numseqgroups);
    for (int i = 0; i < header_->numseqgroups; ++i) {
        unique_sequence_groups_names_[i] = pseqgroup[i].label;
    }

    // Ensure sequence groups have unique names.
    unique_name_generator_.set_template_name("SequenceGroup");
    unique_name_generator_.make_unique(unique_sequence_groups_names_);

    for (int i = 0; i < header_->numseqgroups; ++i, ++pseqgroup) {
        aiNode *sequence_group_node = sequence_groups_node->mChildren[i] = new aiNode(unique_sequence_groups_names_[i]);
        sequence_group_node->mParent = sequence_groups_node;

        aiMetadata *md = sequence_group_node->mMetaData = aiMetadata::Alloc(1);
        if (i == 0) {
            /* StudioMDL does not write the file name for the default sequence group,
               so we will write it. */
            md->Set(0, "File", aiString(file_path_));
        } else {
            md->Set(0, "File", aiString(pseqgroup->name));
        }
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_sequence_infos() {
    if (!header_->numseq) {
        return;
    }

    const SequenceDesc_HL1 *pseqdesc = (const SequenceDesc_HL1 *)((uint8_t *)header_ + header_->seqindex);

    aiNode *sequence_infos_node = new aiNode(AI_MDL_HL1_NODE_SEQUENCE_INFOS);
    rootnode_children_.push_back(sequence_infos_node);

    sequence_infos_node->mNumChildren = static_cast<unsigned int>(header_->numseq);
    sequence_infos_node->mChildren = new aiNode *[sequence_infos_node->mNumChildren];

    std::vector<aiNode *> sequence_info_node_children;

    int animation_index = 0;
    for (int i = 0; i < header_->numseq; ++i, ++pseqdesc) {
        // Clear the list of children for the upcoming sequence info node.
        sequence_info_node_children.clear();

        aiNode *sequence_info_node = sequence_infos_node->mChildren[i] = new aiNode(unique_sequence_names_[i]);
        sequence_info_node->mParent = sequence_infos_node;

        // Setup sequence info node Metadata.
        aiMetadata *md = sequence_info_node->mMetaData = aiMetadata::Alloc(16);
        md->Set(0, "AnimationIndex", animation_index);
        animation_index += pseqdesc->numblends;

        // Reference the sequence group by name. This allows us to search a particular
        // sequence group by name using aiNode(s).
        md->Set(1, "SequenceGroup", aiString(unique_sequence_groups_names_[pseqdesc->seqgroup]));
        md->Set(2, "FramesPerSecond", pseqdesc->fps);
        md->Set(3, "NumFrames", pseqdesc->numframes);
        md->Set(4, "NumBlends", pseqdesc->numblends);
        md->Set(5, "Activity", pseqdesc->activity);
        md->Set(6, "ActivityWeight", pseqdesc->actweight);
        md->Set(7, "MotionFlags", pseqdesc->motiontype);
        md->Set(8, "MotionBone", temp_bones_[pseqdesc->motionbone].node->mName);
        md->Set(9, "LinearMovement", aiVector3D(pseqdesc->linearmovement[0], pseqdesc->linearmovement[1], pseqdesc->linearmovement[2]));
        md->Set(10, "BBMin", aiVector3D(pseqdesc->bbmin[0], pseqdesc->bbmin[1], pseqdesc->bbmin[2]));
        md->Set(11, "BBMax", aiVector3D(pseqdesc->bbmax[0], pseqdesc->bbmax[1], pseqdesc->bbmax[2]));
        md->Set(12, "EntryNode", pseqdesc->entrynode);
        md->Set(13, "ExitNode", pseqdesc->exitnode);
        md->Set(14, "NodeFlags", pseqdesc->nodeflags);
        md->Set(15, "Flags", pseqdesc->flags);

        if (import_settings_.read_blend_controllers) {
            int num_blend_controllers;
            if (get_num_blend_controllers(pseqdesc->numblends, num_blend_controllers) && num_blend_controllers) {
                // Read blend controllers info.
                aiNode *blend_controllers_node = new aiNode(AI_MDL_HL1_NODE_BLEND_CONTROLLERS);
                sequence_info_node_children.push_back(blend_controllers_node);
                blend_controllers_node->mParent = sequence_info_node;
                blend_controllers_node->mNumChildren = static_cast<unsigned int>(num_blend_controllers);
                blend_controllers_node->mChildren = new aiNode *[blend_controllers_node->mNumChildren];

                for (unsigned int j = 0; j < blend_controllers_node->mNumChildren; ++j) {
                    aiNode *blend_controller_node = blend_controllers_node->mChildren[j] = new aiNode();
                    blend_controller_node->mParent = blend_controllers_node;

                    aiMetadata *metaData = blend_controller_node->mMetaData = aiMetadata::Alloc(3);
                    metaData->Set(0, "Start", pseqdesc->blendstart[j]);
                    metaData->Set(1, "End", pseqdesc->blendend[j]);
                    metaData->Set(2, "MotionFlags", pseqdesc->blendtype[j]);
                }
            }
        }

        if (import_settings_.read_animation_events && pseqdesc->numevents) {
            // Read animation events.

            if (pseqdesc->numevents > AI_MDL_HL1_MAX_EVENTS) {
                log_warning_limit_exceeded<AI_MDL_HL1_MAX_EVENTS>(
                        "Sequence " + std::string(pseqdesc->label),
                        pseqdesc->numevents, "animation events");
            }

            const AnimEvent_HL1 *pevent = (const AnimEvent_HL1 *)((uint8_t *)header_ + pseqdesc->eventindex);

            aiNode *pEventsNode = new aiNode(AI_MDL_HL1_NODE_ANIMATION_EVENTS);
            sequence_info_node_children.push_back(pEventsNode);
            pEventsNode->mParent = sequence_info_node;
            pEventsNode->mNumChildren = static_cast<unsigned int>(pseqdesc->numevents);
            pEventsNode->mChildren = new aiNode *[pEventsNode->mNumChildren];

            for (unsigned int j = 0; j < pEventsNode->mNumChildren; ++j, ++pevent) {
                aiNode *pEvent = pEventsNode->mChildren[j] = new aiNode();
                pEvent->mParent = pEventsNode;

                aiMetadata *metaData = pEvent->mMetaData = aiMetadata::Alloc(3);
                metaData->Set(0, "Frame", pevent->frame);
                metaData->Set(1, "ScriptEvent", pevent->event);
                metaData->Set(2, "Options", aiString(pevent->options));
            }
        }

        if (sequence_info_node_children.size()) {
            sequence_info_node->addChildren(
                    static_cast<unsigned int>(sequence_info_node_children.size()),
                    sequence_info_node_children.data());
        }
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_sequence_transitions() {
    if (!header_->numtransitions) {
        return;
    }

    // Read sequence transition graph.
    aiNode *transition_graph_node = new aiNode(AI_MDL_HL1_NODE_SEQUENCE_TRANSITION_GRAPH);
    rootnode_children_.push_back(transition_graph_node);

    uint8_t *ptransitions = ((uint8_t *)header_ + header_->transitionindex);
    aiMetadata *md = transition_graph_node->mMetaData = aiMetadata::Alloc(header_->numtransitions * header_->numtransitions);
    for (unsigned int i = 0; i < md->mNumProperties; ++i)
        md->Set(i, std::to_string(i), static_cast<int>(ptransitions[i]));
}

void HL1MDLLoader::read_attachments() {
    if (!header_->numattachments) {
        return;
    }

    const Attachment_HL1 *pattach = (const Attachment_HL1 *)((uint8_t *)header_ + header_->attachmentindex);

    aiNode *attachments_node = new aiNode(AI_MDL_HL1_NODE_ATTACHMENTS);
    rootnode_children_.push_back(attachments_node);
    attachments_node->mNumChildren = static_cast<unsigned int>(header_->numattachments);
    attachments_node->mChildren = new aiNode *[attachments_node->mNumChildren];

    for (int i = 0; i < header_->numattachments; ++i, ++pattach) {
        aiNode *attachment_node = attachments_node->mChildren[i] = new aiNode();
        attachment_node->mParent = attachments_node;
        attachment_node->mMetaData = aiMetadata::Alloc(2);
        attachment_node->mMetaData->Set(0, "Position", aiVector3D(pattach->org[0], pattach->org[1], pattach->org[2]));
        // Reference the bone by name. This allows us to search a particular
        // bone by name using aiNode(s).
        attachment_node->mMetaData->Set(1, "Bone", temp_bones_[pattach->bone].node->mName);
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_hitboxes() {
    if (!header_->numhitboxes) {
        return;
    }

    const Hitbox_HL1 *phitbox = (const Hitbox_HL1 *)((uint8_t *)header_ + header_->hitboxindex);

    aiNode *hitboxes_node = new aiNode(AI_MDL_HL1_NODE_HITBOXES);
    rootnode_children_.push_back(hitboxes_node);
    hitboxes_node->mNumChildren = static_cast<unsigned int>(header_->numhitboxes);
    hitboxes_node->mChildren = new aiNode *[hitboxes_node->mNumChildren];

    for (int i = 0; i < header_->numhitboxes; ++i, ++phitbox) {
        aiNode *hitbox_node = hitboxes_node->mChildren[i] = new aiNode();
        hitbox_node->mParent = hitboxes_node;

        aiMetadata *md = hitbox_node->mMetaData = aiMetadata::Alloc(4);
        // Reference the bone by name. This allows us to search a particular
        // bone by name using aiNode(s).
        md->Set(0, "Bone", temp_bones_[phitbox->bone].node->mName);
        md->Set(1, "HitGroup", phitbox->group);
        md->Set(2, "BBMin", aiVector3D(phitbox->bbmin[0], phitbox->bbmin[1], phitbox->bbmin[2]));
        md->Set(3, "BBMax", aiVector3D(phitbox->bbmax[0], phitbox->bbmax[1], phitbox->bbmax[2]));
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_bone_controllers() {
    if (!header_->numbonecontrollers) {
        return;
    }

    const BoneController_HL1 *pbonecontroller = (const BoneController_HL1 *)((uint8_t *)header_ + header_->bonecontrollerindex);

    aiNode *bones_controller_node = new aiNode(AI_MDL_HL1_NODE_BONE_CONTROLLERS);
    rootnode_children_.push_back(bones_controller_node);
    bones_controller_node->mNumChildren = static_cast<unsigned int>(header_->numbonecontrollers);
    bones_controller_node->mChildren = new aiNode *[bones_controller_node->mNumChildren];

    for (int i = 0; i < header_->numbonecontrollers; ++i, ++pbonecontroller) {
        aiNode *bone_controller_node = bones_controller_node->mChildren[i] = new aiNode();
        bone_controller_node->mParent = bones_controller_node;

        aiMetadata *md = bone_controller_node->mMetaData = aiMetadata::Alloc(5);
        // Reference the bone by name. This allows us to search a particular
        // bone by name using aiNode(s).
        md->Set(0, "Bone", temp_bones_[pbonecontroller->bone].node->mName);
        md->Set(1, "MotionFlags", pbonecontroller->type);
        md->Set(2, "Start", pbonecontroller->start);
        md->Set(3, "End", pbonecontroller->end);
        md->Set(4, "Channel", pbonecontroller->index);
    }
}

// ------------------------------------------------------------------------------------------------
void HL1MDLLoader::read_global_info() {
    aiNode *global_info_node = new aiNode(AI_MDL_HL1_NODE_GLOBAL_INFO);
    rootnode_children_.push_back(global_info_node);

    aiMetadata *md = global_info_node->mMetaData = aiMetadata::Alloc(import_settings_.read_misc_global_info ? 16 : 11);
    md->Set(0, "Version", AI_MDL_HL1_VERSION);
    md->Set(1, "NumBodyparts", header_->numbodyparts);
    md->Set(2, "NumModels", total_models_);
    md->Set(3, "NumBones", header_->numbones);
    md->Set(4, "NumAttachments", import_settings_.read_attachments ? header_->numattachments : 0);
    md->Set(5, "NumSkinFamilies", texture_header_->numskinfamilies);
    md->Set(6, "NumHitboxes", import_settings_.read_hitboxes ? header_->numhitboxes : 0);
    md->Set(7, "NumBoneControllers", import_settings_.read_bone_controllers ? header_->numbonecontrollers : 0);
    md->Set(8, "NumSequences", import_settings_.read_animations ? header_->numseq : 0);
    md->Set(9, "NumBlendControllers", import_settings_.read_blend_controllers ? num_blend_controllers_ : 0);
    md->Set(10, "NumTransitionNodes", import_settings_.read_sequence_transitions ? header_->numtransitions : 0);

    if (import_settings_.read_misc_global_info) {
        md->Set(11, "EyePosition", aiVector3D(header_->eyeposition[0], header_->eyeposition[1], header_->eyeposition[2]));
        md->Set(12, "HullMin", aiVector3D(header_->min[0], header_->min[1], header_->min[2]));
        md->Set(13, "HullMax", aiVector3D(header_->max[0], header_->max[1], header_->max[2]));
        md->Set(14, "CollisionMin", aiVector3D(header_->bbmin[0], header_->bbmin[1], header_->bbmin[2]));
        md->Set(15, "CollisionMax", aiVector3D(header_->bbmax[0], header_->bbmax[1], header_->bbmax[2]));
    }
}

// ------------------------------------------------------------------------------------------------
/** @brief This method reads a compressed anim value.
*
*   @note The structure of this method is taken from HL2 source code.
*   Although this is from HL2, it's implementation is almost identical
*   to code found in HL1 SDK. See HL1 and HL2 SDKs for more info.
*
*   source:
*       HL1 source code.
*           file: studio_render.cpp
*           function(s): CalcBoneQuaternion and CalcBonePosition
*
*       HL2 source code.
*           file: bone_setup.cpp
*           function(s): ExtractAnimValue
*/
void HL1MDLLoader::extract_anim_value(
        const AnimValue_HL1 *panimvalue,
        int frame, float bone_scale, ai_real &value) {
    int k = frame;

    // find span of values that includes the frame we want
    while (panimvalue->num.total <= k) {
        k -= panimvalue->num.total;
        panimvalue += panimvalue->num.valid + 1;
    }

    // Bah, missing blend!
    if (panimvalue->num.valid > k) {
        value = panimvalue[k + 1].value * bone_scale;
    } else {
        value = panimvalue[panimvalue->num.valid].value * bone_scale;
    }
}

// ------------------------------------------------------------------------------------------------
// Get the number of blend controllers.
bool HL1MDLLoader::get_num_blend_controllers(const int num_blend_animations, int &num_blend_controllers) {

    switch (num_blend_animations) {
        case SequenceBlendMode_HL1::NoBlend:
            num_blend_controllers = 0;
            return true;
        case SequenceBlendMode_HL1::TwoWayBlending:
            num_blend_controllers = 1;
            return true;
        case SequenceBlendMode_HL1::FourWayBlending:
            num_blend_controllers = 2;
            return true;
        default:
            num_blend_controllers = 0;
            ASSIMP_LOG_WARN(MDL_HALFLIFE_LOG_HEADER "Unsupported number of blend animations (", num_blend_animations, ")");
            return false;
    }
}

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp
