/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team


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
#include <utility>
#include "MMDPmxParser.h"
#include <assimp/StringUtils.h>
#include "../contrib/utf8cpp/source/utf8.h"
#include <assimp/Exceptional.h>
#include <assimp/DefaultIOSystem.h>

using namespace::Assimp;

namespace pmx
{
	int ReadIndex(IOStream *stream, int size)
	{
		switch (size)
		{
		case 1:
			uint8_t tmp8;
			stream->Read((char*) &tmp8, sizeof(uint8_t), 1);
			if (255 == tmp8)
			{
				return -1;
			}
			else {
				return (int) tmp8;
			}
		case 2:
			uint16_t tmp16;
			stream->Read((char*) &tmp16, sizeof(uint16_t), 1);
			if (65535 == tmp16)
			{
				return -1;
			}
			else {
				return (int) tmp16;
			}
		case 4:
			int tmp32;
			stream->Read((char*) &tmp32, sizeof(int), 1);
			return tmp32;
		default:
			return -1;
		}
	}

	std::string ReadString(IOStream *stream, uint8_t encoding)
	{
		int size;
		stream->Read((char*) &size, sizeof(int), 1);
		std::vector<char> buffer;
		if (size == 0)
		{
			return std::string("");
		}
		buffer.reserve(size);
		stream->Read((char*) buffer.data(), size, 1);
		if (encoding == 0)
		{
			// UTF16 to UTF8
			const uint16_t* sourceStart = (uint16_t*)buffer.data();
			const unsigned int targetSize = size * 3; // enough to encode
			char *targetStart = new char[targetSize];
            std::memset(targetStart, 0, targetSize * sizeof(char));
            
            utf8::utf16to8( sourceStart, sourceStart + size/2, targetStart );

			std::string result(targetStart);
            delete [] targetStart;
			return result;
		}
		else
		{
			// the name is already UTF8
			return std::string((const char*)buffer.data(), size);
		}
	}

	void PmxSetting::Read(IOStream *stream)
	{
		uint8_t count;
		stream->Read((char*) &count, sizeof(uint8_t), 1);
		if (count < 8)
		{
			throw DeadlyImportError("MMD: invalid size");
		}
		stream->Read((char*) &encoding, sizeof(uint8_t), 1);
		stream->Read((char*) &uv, sizeof(uint8_t), 1);
		stream->Read((char*) &vertex_index_size, sizeof(uint8_t), 1);
		stream->Read((char*) &texture_index_size, sizeof(uint8_t), 1);
		stream->Read((char*) &material_index_size, sizeof(uint8_t), 1);
		stream->Read((char*) &bone_index_size, sizeof(uint8_t), 1);
		stream->Read((char*) &morph_index_size, sizeof(uint8_t), 1);
		stream->Read((char*) &rigidbody_index_size, sizeof(uint8_t), 1);
		uint8_t temp;
		for (int i = 8; i < count; i++)
		{
			stream->Read((char*)&temp, sizeof(uint8_t), 1);
		}
	}

	void PmxVertexSkinningBDEF1::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_index = ReadIndex(stream, setting->bone_index_size);
	}

	void PmxVertexSkinningBDEF2::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->bone_weight, sizeof(float), 1);
	}

	void PmxVertexSkinningBDEF4::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index3 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index4 = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->bone_weight1, sizeof(float), 1);
		stream->Read((char*) &this->bone_weight2, sizeof(float), 1);
		stream->Read((char*) &this->bone_weight3, sizeof(float), 1);
		stream->Read((char*) &this->bone_weight4, sizeof(float), 1);
	}

	void PmxVertexSkinningSDEF::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->bone_weight, sizeof(float), 1);
		stream->Read((char*) this->sdef_c, sizeof(float) * 3, 1);
		stream->Read((char*) this->sdef_r0, sizeof(float) * 3, 1);
		stream->Read((char*) this->sdef_r1, sizeof(float) * 3, 1);
	}

	void PmxVertexSkinningQDEF::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index3 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index4 = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->bone_weight1, sizeof(float), 1);
		stream->Read((char*) &this->bone_weight2, sizeof(float), 1);
		stream->Read((char*) &this->bone_weight3, sizeof(float), 1);
		stream->Read((char*) &this->bone_weight4, sizeof(float), 1);
	}

	void PmxVertex::Read(IOStream *stream, PmxSetting *setting)
	{
		stream->Read((char*) this->position, sizeof(float) * 3, 1);
		stream->Read((char*) this->normal, sizeof(float) * 3, 1);
		stream->Read((char*) this->uv, sizeof(float) * 2, 1);
		for (int i = 0; i < setting->uv; ++i)
		{
			stream->Read((char*) this->uva[i], sizeof(float) * 4, 1);
		}
		stream->Read((char*) &this->skinning_type, sizeof(PmxVertexSkinningType), 1);
		switch (this->skinning_type)
		{
		case PmxVertexSkinningType::BDEF1:
			this->skinning = mmd::make_unique<PmxVertexSkinningBDEF1>();
			break;
		case PmxVertexSkinningType::BDEF2:
			this->skinning = mmd::make_unique<PmxVertexSkinningBDEF2>();
			break;
		case PmxVertexSkinningType::BDEF4:
			this->skinning = mmd::make_unique<PmxVertexSkinningBDEF4>();
			break;
		case PmxVertexSkinningType::SDEF:
			this->skinning = mmd::make_unique<PmxVertexSkinningSDEF>();
			break;
		case PmxVertexSkinningType::QDEF:
			this->skinning = mmd::make_unique<PmxVertexSkinningQDEF>();
			break;
		default:
			throw "invalid skinning type";
		}
		this->skinning->Read(stream, setting);
		stream->Read((char*) &this->edge, sizeof(float), 1);
	}

	void PmxMaterial::Read(IOStream *stream, PmxSetting *setting)
	{
		this->material_name = ReadString(stream, setting->encoding);
		this->material_english_name = ReadString(stream, setting->encoding);
		stream->Read((char*) this->diffuse, sizeof(float) * 4, 1);
		stream->Read((char*) this->specular, sizeof(float) * 3, 1);
		stream->Read((char*) &this->specularlity, sizeof(float), 1);
		stream->Read((char*) this->ambient, sizeof(float) * 3, 1);
		stream->Read((char*) &this->flag, sizeof(uint8_t), 1);
		stream->Read((char*) this->edge_color, sizeof(float) * 4, 1);
		stream->Read((char*) &this->edge_size, sizeof(float), 1);
		this->diffuse_texture_index = ReadIndex(stream, setting->texture_index_size);
		this->sphere_texture_index = ReadIndex(stream, setting->texture_index_size);
		stream->Read((char*) &this->sphere_op_mode, sizeof(uint8_t), 1);
		stream->Read((char*) &this->common_toon_flag, sizeof(uint8_t), 1);
		if (this->common_toon_flag)
		{
			stream->Read((char*) &this->toon_texture_index, sizeof(uint8_t), 1);
		}
		else {
			this->toon_texture_index = ReadIndex(stream, setting->texture_index_size);
		}
		this->memo = ReadString(stream, setting->encoding);
		stream->Read((char*) &this->index_count, sizeof(int), 1);
	}

	void PmxIkLink::Read(IOStream *stream, PmxSetting *setting)
	{
		this->link_target = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->angle_lock, sizeof(uint8_t), 1);
		if (angle_lock == 1)
		{
			stream->Read((char*) this->max_radian, sizeof(float) * 3, 1);
			stream->Read((char*) this->min_radian, sizeof(float) * 3, 1);
		}
	}

	void PmxBone::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_name = ReadString(stream, setting->encoding);
		this->bone_english_name = ReadString(stream, setting->encoding);
		stream->Read((char*) this->position, sizeof(float) * 3, 1);
		this->parent_index = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->level, sizeof(int), 1);
		stream->Read((char*) &this->bone_flag, sizeof(uint16_t), 1);
		if (this->bone_flag & 0x0001) {
			this->target_index = ReadIndex(stream, setting->bone_index_size);
		}
		else {
			stream->Read((char*)this->offset, sizeof(float) * 3, 1);
		}
		if (this->bone_flag & (0x0100 | 0x0200)) {
			this->grant_parent_index = ReadIndex(stream, setting->bone_index_size);
			stream->Read((char*) &this->grant_weight, sizeof(float), 1);
		}
		if (this->bone_flag & 0x0400) {
			stream->Read((char*)this->lock_axis_orientation, sizeof(float) * 3, 1);
		}
		if (this->bone_flag & 0x0800) {
			stream->Read((char*)this->local_axis_x_orientation, sizeof(float) * 3, 1);
			stream->Read((char*)this->local_axis_y_orientation, sizeof(float) * 3, 1);
		}
		if (this->bone_flag & 0x2000) {
			stream->Read((char*) &this->key, sizeof(int), 1);
		}
		if (this->bone_flag & 0x0020) {
			this->ik_target_bone_index = ReadIndex(stream, setting->bone_index_size);
			stream->Read((char*) &ik_loop, sizeof(int), 1);
			stream->Read((char*) &ik_loop_angle_limit, sizeof(float), 1);
			stream->Read((char*) &ik_link_count, sizeof(int), 1);
			this->ik_links = mmd::make_unique<PmxIkLink []>(ik_link_count);
			for (int i = 0; i < ik_link_count; i++) {
				ik_links[i].Read(stream, setting);
			}
		}
	}

	void PmxMorphVertexOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->vertex_index = ReadIndex(stream, setting->vertex_index_size);
		stream->Read((char*)this->position_offset, sizeof(float) * 3, 1);
	}

	void PmxMorphUVOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->vertex_index = ReadIndex(stream, setting->vertex_index_size);
		stream->Read((char*)this->uv_offset, sizeof(float) * 4, 1);
	}

	void PmxMorphBoneOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->bone_index = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*)this->translation, sizeof(float) * 3, 1);
		stream->Read((char*)this->rotation, sizeof(float) * 4, 1);
	}

	void PmxMorphMaterialOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->material_index = ReadIndex(stream, setting->material_index_size);
		stream->Read((char*) &this->offset_operation, sizeof(uint8_t), 1);
		stream->Read((char*)this->diffuse, sizeof(float) * 4, 1);
		stream->Read((char*)this->specular, sizeof(float) * 3, 1);
		stream->Read((char*) &this->specularity, sizeof(float), 1);
		stream->Read((char*)this->ambient, sizeof(float) * 3, 1);
		stream->Read((char*)this->edge_color, sizeof(float) * 4, 1);
		stream->Read((char*) &this->edge_size, sizeof(float), 1);
		stream->Read((char*)this->texture_argb, sizeof(float) * 4, 1);
		stream->Read((char*)this->sphere_texture_argb, sizeof(float) * 4, 1);
		stream->Read((char*)this->toon_texture_argb, sizeof(float) * 4, 1);
	}

	void PmxMorphGroupOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->morph_index = ReadIndex(stream, setting->morph_index_size);
		stream->Read((char*) &this->morph_weight, sizeof(float), 1);
	}

	void PmxMorphFlipOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->morph_index = ReadIndex(stream, setting->morph_index_size);
		stream->Read((char*) &this->morph_value, sizeof(float), 1);
	}

	void PmxMorphImplusOffset::Read(IOStream *stream, PmxSetting *setting)
	{
		this->rigid_body_index = ReadIndex(stream, setting->rigidbody_index_size);
		stream->Read((char*) &this->is_local, sizeof(uint8_t), 1);
		stream->Read((char*)this->velocity, sizeof(float) * 3, 1);
		stream->Read((char*)this->angular_torque, sizeof(float) * 3, 1);
	}

	void PmxMorph::Read(IOStream *stream, PmxSetting *setting)
	{
		this->morph_name = ReadString(stream, setting->encoding);
		this->morph_english_name = ReadString(stream, setting->encoding);
		stream->Read((char*) &category, sizeof(MorphCategory), 1);
		stream->Read((char*) &morph_type, sizeof(MorphType), 1);
		stream->Read((char*) &this->offset_count, sizeof(int), 1);
		switch (this->morph_type)
		{
		case MorphType::Group:
			group_offsets = mmd::make_unique<PmxMorphGroupOffset []>(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				group_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::Vertex:
			vertex_offsets = mmd::make_unique<PmxMorphVertexOffset []>(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				vertex_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::Bone:
			bone_offsets = mmd::make_unique<PmxMorphBoneOffset []>(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				bone_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::Matrial:
			material_offsets = mmd::make_unique<PmxMorphMaterialOffset []>(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				material_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::UV:
		case MorphType::AdditionalUV1:
		case MorphType::AdditionalUV2:
		case MorphType::AdditionalUV3:
		case MorphType::AdditionalUV4:
			uv_offsets = mmd::make_unique<PmxMorphUVOffset []>(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				uv_offsets[i].Read(stream, setting);
			}
			break;
		default:
            throw DeadlyImportError("MMD: unknown morth type");
		}
	}

	void PmxFrameElement::Read(IOStream *stream, PmxSetting *setting)
	{
		stream->Read((char*) &this->element_target, sizeof(uint8_t), 1);
		if (this->element_target == 0x00)
		{
			this->index = ReadIndex(stream, setting->bone_index_size);
		}
		else {
			this->index = ReadIndex(stream, setting->morph_index_size);
		}
	}

	void PmxFrame::Read(IOStream *stream, PmxSetting *setting)
	{
		this->frame_name = ReadString(stream, setting->encoding);
		this->frame_english_name = ReadString(stream, setting->encoding);
		stream->Read((char*) &this->frame_flag, sizeof(uint8_t), 1);
		stream->Read((char*) &this->element_count, sizeof(int), 1);
		this->elements = mmd::make_unique<PmxFrameElement []>(this->element_count);
		for (int i = 0; i < this->element_count; i++)
		{
			this->elements[i].Read(stream, setting);
		}
	}

	void PmxRigidBody::Read(IOStream *stream, PmxSetting *setting)
	{
		this->girid_body_name = ReadString(stream, setting->encoding);
		this->girid_body_english_name = ReadString(stream, setting->encoding);
		this->target_bone = ReadIndex(stream, setting->bone_index_size);
		stream->Read((char*) &this->group, sizeof(uint8_t), 1);
		stream->Read((char*) &this->mask, sizeof(uint16_t), 1);
		stream->Read((char*) &this->shape, sizeof(uint8_t), 1);
		stream->Read((char*) this->size, sizeof(float) * 3, 1);
		stream->Read((char*) this->position, sizeof(float) * 3, 1);
		stream->Read((char*) this->orientation, sizeof(float) * 3, 1);
		stream->Read((char*) &this->mass, sizeof(float), 1);
		stream->Read((char*) &this->move_attenuation, sizeof(float), 1);
		stream->Read((char*) &this->rotation_attenuation, sizeof(float), 1);
		stream->Read((char*) &this->repulsion, sizeof(float), 1);
		stream->Read((char*) &this->friction, sizeof(float), 1);
		stream->Read((char*) &this->physics_calc_type, sizeof(uint8_t), 1);
	}

	void PmxJointParam::Read(IOStream *stream, PmxSetting *setting)
	{
		this->rigid_body1 = ReadIndex(stream, setting->rigidbody_index_size);
		this->rigid_body2 = ReadIndex(stream, setting->rigidbody_index_size);
		stream->Read((char*) this->position, sizeof(float) * 3, 1);
		stream->Read((char*) this->orientaiton, sizeof(float) * 3, 1);
		stream->Read((char*) this->move_limitation_min, sizeof(float) * 3, 1);
		stream->Read((char*) this->move_limitation_max, sizeof(float) * 3, 1);
		stream->Read((char*) this->rotation_limitation_min, sizeof(float) * 3, 1);
		stream->Read((char*) this->rotation_limitation_max, sizeof(float) * 3, 1);
		stream->Read((char*) this->spring_move_coefficient, sizeof(float) * 3, 1);
		stream->Read((char*) this->spring_rotation_coefficient, sizeof(float) * 3, 1);
	}

	void PmxJoint::Read(IOStream *stream, PmxSetting *setting)
	{
		this->joint_name = ReadString(stream, setting->encoding);
		this->joint_english_name = ReadString(stream, setting->encoding);
		stream->Read((char*) &this->joint_type, sizeof(uint8_t), 1);
		this->param.Read(stream, setting);
	}

	void PmxAncherRigidBody::Read(IOStream *stream, PmxSetting *setting)
	{
		this->related_rigid_body = ReadIndex(stream, setting->rigidbody_index_size);
		this->related_vertex = ReadIndex(stream, setting->vertex_index_size);
		stream->Read((char*) &this->is_near, sizeof(uint8_t), 1);
	}

    void PmxSoftBody::Read(IOStream * /*stream*/, PmxSetting * /*setting*/)
	{
		std::cerr << "Not Implemented Exception" << std::endl;
        throw DeadlyImportError("MMD: Not Implemented Exception");
    }

	void PmxModel::Init()
	{
		this->version = 0.0f;
		this->model_name.clear();
		this->model_english_name.clear();
		this->model_comment.clear();
		this->model_english_comment.clear();
		this->vertex_count = 0;
		this->vertices = nullptr;
		this->index_count = 0;
		this->indices = nullptr;
		this->texture_count = 0;
		this->textures = nullptr;
		this->material_count = 0;
		this->materials = nullptr;
		this->bone_count = 0;
		this->bones = nullptr;
		this->morph_count = 0;
		this->morphs = nullptr;
		this->frame_count = 0;
		this->frames = nullptr;
		this->rigid_body_count = 0;
		this->rigid_bodies = nullptr;
		this->joint_count = 0;
		this->joints = nullptr;
		this->soft_body_count = 0;
		this->soft_bodies = nullptr;
	}

	void PmxModel::Read(IOStream *stream)
	{
		char magic[4];
		stream->Read((char*) magic, sizeof(char) * 4, 1);
		if (magic[0] != 0x50 || magic[1] != 0x4d || magic[2] != 0x58 || magic[3] != 0x20)
		{
			std::cerr << "invalid magic number." << std::endl;
      throw DeadlyImportError("MMD: invalid magic number.");
    }
		stream->Read((char*) &version, sizeof(float), 1);
		if (version != 2.0f && version != 2.1f)
		{
			std::cerr << "this is not ver2.0 or ver2.1 but " << version << "." << std::endl;
            throw DeadlyImportError("MMD: this is not ver2.0 or ver2.1 but " + to_string(version));
    }
		this->setting.Read(stream);

		this->model_name = ReadString(stream, setting.encoding);
		this->model_english_name = ReadString(stream, setting.encoding);
		this->model_comment = ReadString(stream, setting.encoding);
		this->model_english_comment = ReadString(stream, setting.encoding);

		// read vertices
		stream->Read((char*) &vertex_count, sizeof(int), 1);
		this->vertices = mmd::make_unique<PmxVertex []>(vertex_count);
		for (int i = 0; i < vertex_count; i++)
		{
			vertices[i].Read(stream, &setting);
		}

		// read indices
		stream->Read((char*) &index_count, sizeof(int), 1);
		this->indices = mmd::make_unique<int []>(index_count);
		for (int i = 0; i < index_count; i++)
		{
			this->indices[i] = ReadIndex(stream, setting.vertex_index_size);
		}

		// read texture names
		stream->Read((char*) &texture_count, sizeof(int), 1);
		this->textures = mmd::make_unique<std::string []>(texture_count);
		for (int i = 0; i < texture_count; i++)
		{
			this->textures[i] = ReadString(stream, setting.encoding);
		}

		// read materials
		stream->Read((char*) &material_count, sizeof(int), 1);
		this->materials = mmd::make_unique<PmxMaterial []>(material_count);
		for (int i = 0; i < material_count; i++)
		{
			this->materials[i].Read(stream, &setting);
		}

		// read bones
		stream->Read((char*) &this->bone_count, sizeof(int), 1);
		this->bones = mmd::make_unique<PmxBone []>(this->bone_count);
		for (int i = 0; i < this->bone_count; i++)
		{
			this->bones[i].Read(stream, &setting);
		}

		// read morphs
		stream->Read((char*) &this->morph_count, sizeof(int), 1);
		this->morphs = mmd::make_unique<PmxMorph []>(this->morph_count);
		for (int i = 0; i < this->morph_count; i++)
		{
			this->morphs[i].Read(stream, &setting);
		}

		// read display frames
		stream->Read((char*) &this->frame_count, sizeof(int), 1);
		this->frames = mmd::make_unique<PmxFrame []>(this->frame_count);
		for (int i = 0; i < this->frame_count; i++)
		{
			this->frames[i].Read(stream, &setting);
		}

		// read rigid bodies
		stream->Read((char*) &this->rigid_body_count, sizeof(int), 1);
		this->rigid_bodies = mmd::make_unique<PmxRigidBody []>(this->rigid_body_count);
		for (int i = 0; i < this->rigid_body_count; i++)
		{
			this->rigid_bodies[i].Read(stream, &setting);
		}

		// read joints
		stream->Read((char*) &this->joint_count, sizeof(int), 1);
		this->joints = mmd::make_unique<PmxJoint []>(this->joint_count);
		for (int i = 0; i < this->joint_count; i++)
		{
			this->joints[i].Read(stream, &setting);
		}
	}
}
