/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------
Typed BLEND animation emit helpers (Object TRS, pose, morph, mesh-cache).
----------------------------------------------------------------------
*/
#pragma once
#ifndef INCLUDED_AI_BLEND_ANIM_EMIT_H
#define INCLUDED_AI_BLEND_ANIM_EMIT_H

#ifndef ASSIMP_BUILD_NO_BLEND_IMPORTER

#include "BlenderIntermediate.h"
#include <assimp/anim.h>
#include <assimp/mesh.h>
#include <vector>

namespace Assimp {
namespace Blender {

/** Collect Object + NLA Action TRS into typed aiNodeAnim (all Object types). */
void BlendConvertObjectAnimations(std::vector<aiAnimation *> &anims, ConversionData &conv_data);

/** Armature pose.bones Actions -> aiNodeAnim (existing path; NLA strip window aware). */
void BlendConvertArmatureAnimations(std::vector<aiAnimation *> &anims, ConversionData &conv_data);

/** Shape-key weight curves -> aiMeshMorphAnim (targets already on aiMesh::mAnimMeshes). */
void BlendConvertShapeKeyAnimations(std::vector<aiAnimation *> &anims, ConversionData &conv_data);

/** Skinning: MDeformVert + Armature VGroups -> aiBone on each submesh. */
void BlendAttachMeshBones(aiMesh *out, const Object &obj, const Mesh &mesh,
        ConversionData &conv_data, const std::vector<int> &outToSrcVert);

/** Shape key blocks -> aiAnimMesh targets (absolute positions from DNA). */
void BlendAttachShapeKeyTargets(aiMesh *out, const Object *obj, const Mesh &mesh,
        const std::vector<int> &outToSrcVert, ConversionData &conv_data);

/**
 * MeshCache MDD -> aiAnimMesh frames + aiMeshAnim when filepath loads and
 * vertex count matches DNA mesh. PC2 / unloadable paths: leave (no invent).
 */
void BlendAttachMeshCacheAnim(aiMesh *out, const Object &obj, const Mesh &mesh,
        ConversionData &conv_data, const std::vector<int> &outToSrcVert,
        std::vector<aiAnimation *> &anims);

} // namespace Blender
} // namespace Assimp

#endif // ASSIMP_BUILD_NO_BLEND_IMPORTER
#endif
