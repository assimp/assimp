/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team


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
#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <assimp/DefaultIOSystem.h>
#include "MMDCpp14.h"
#include "PmdHelper.h"

using namespace::Assimp;

namespace pmd
{
    class PmdHeader
    {
    public:
        std::string name;
        std::string name_english;
        std::string comment;
        std::string comment_english;

        bool Read(IOStream* stream)
        {
            name = PmdHelper::ReadString(stream, 20);
            comment = PmdHelper::ReadString(stream, 256);
            return true;
        }

        bool ReadExtension(IOStream* stream)
        {
            name_english = PmdHelper::ReadString(stream, 20);
            comment_english = PmdHelper::ReadString(stream, 256);
            return true;
        }
    };

    class PmdVertex
    {
    public:
        float position[3];

        float normal[3];

        float uv[2];

        uint16_t bone_index[2];

        uint8_t bone_weight;

        bool edge_invisible;

        bool Read(IOStream* stream)
        {
            stream->Read((char*)position, sizeof(float) * 3, 1);
            stream->Read((char*)normal, sizeof(float) * 3, 1);
            stream->Read((char*)uv, sizeof(float) * 2, 1);
            stream->Read((char*)bone_index, sizeof(uint16_t) * 2, 1);
            stream->Read((char*)&bone_weight, sizeof(uint8_t), 1);
            stream->Read((char*)&edge_invisible, sizeof(uint8_t), 1);
            return true;
        }
    };

    class PmdMaterial
    {
    public:
        float diffuse[4];
        float power;
        float specular[3];
        float ambient[3];
        uint8_t toon_index;
        uint8_t edge_flag;
        uint32_t index_count;
        std::string texture_filename;
        std::string sphere_filename;

        bool Read(IOStream* stream)
        {
            stream->Read((char*)&diffuse, sizeof(float) * 4, 1);
            stream->Read((char*)&power, sizeof(float), 1);
            stream->Read((char*)&specular, sizeof(float) * 3, 1);
            stream->Read((char*)&ambient, sizeof(float) * 3, 1);
            stream->Read((char*)&toon_index, sizeof(uint8_t), 1);
            stream->Read((char*)&edge_flag, sizeof(uint8_t), 1);
            stream->Read((char*)&index_count, sizeof(uint32_t), 1);
            std::string buffer = PmdHelper::ReadString(stream, 20);
            size_t pstar = buffer.find('*');
            if (pstar == std::string::npos)
            {
                texture_filename = buffer;
                sphere_filename.clear();
            }
            else {
                texture_filename = buffer;
                sphere_filename = buffer.substr(pstar + 1);
            }
            return true;
        }
    };

    enum class BoneType : uint8_t
    {
        Rotation,
        RotationAndMove,
        IkEffector,
        Unknown,
        IkEffectable,
        RotationEffectable,
        IkTarget,
        Invisible,
        Twist,
        RotationMovement
    };

    class PmdBone
    {
    public:
        std::string name;
        std::string name_english;
        uint16_t parent_bone_index;
        uint16_t tail_pos_bone_index;
        BoneType bone_type;
        uint16_t ik_parent_bone_index;
        float bone_head_pos[3];

        void Read(IOStream* stream)
        {
            name = PmdHelper::ReadString(stream, 20);
            stream->Read((char*)&parent_bone_index, sizeof(uint16_t), 1);
            stream->Read((char*)&tail_pos_bone_index, sizeof(uint16_t), 1);
            stream->Read((char*)&bone_type, sizeof(uint8_t), 1);
            stream->Read((char*)&ik_parent_bone_index, sizeof(uint16_t), 1);
            stream->Read((char*)&bone_head_pos, sizeof(float) * 3, 1);
        }

        void ReadExpantion(IOStream *stream)
        {
            name_english = PmdHelper::ReadString(stream, 20);
        }
    };

    class PmdIk
    {
    public:
        uint16_t ik_bone_index;
        uint16_t target_bone_index;
        uint16_t interations;
        float angle_limit;
        std::vector<uint16_t> ik_child_bone_index;

        void Read(IOStream *stream)
        {
            stream->Read((char *)&ik_bone_index, sizeof(uint16_t), 1);
            stream->Read((char *)&target_bone_index, sizeof(uint16_t), 1);
            uint8_t ik_chain_length;
            stream->Read((char*)&ik_chain_length, sizeof(uint8_t), 1);
            stream->Read((char *)&interations, sizeof(uint16_t), 1);
            stream->Read((char *)&angle_limit, sizeof(float), 1);
            ik_child_bone_index.resize(ik_chain_length);
            for (int i = 0; i < ik_chain_length; i++)
            {
                stream->Read((char *)&ik_child_bone_index[i], sizeof(uint16_t), 1);
            }
        }
    };

    class PmdFaceVertex
    {
    public:
        int vertex_index;
        float position[3];

        void Read(IOStream *stream)
        {
            stream->Read((char *)&vertex_index, sizeof(int), 1);
            stream->Read((char *)position, sizeof(float) * 3, 1);
        }
    };

    enum class FaceCategory : uint8_t
    {
        Base,
        Eyebrow,
        Eye,
        Mouth,
        Other
    };

    class PmdFace
    {
    public:
        std::string name;
        FaceCategory type;
        std::vector<PmdFaceVertex> vertices;
        std::string name_english;

        void Read(IOStream *stream)
        {
            name = PmdHelper::ReadString(stream, 20);
            int vertex_count;
            stream->Read((char*)&vertex_count, sizeof(int), 1);
            stream->Read((char*)&type, sizeof(uint8_t), 1);
            vertices.resize(vertex_count);
            for (int i = 0; i < vertex_count; i++)
            {
                vertices[i].Read(stream);
            }
        }

        void ReadExpantion(IOStream *stream)
        {
            name_english = PmdHelper::ReadString(stream, 20);
        }
    };

    class PmdBoneDispName
    {
    public:
        std::string bone_disp_name;
        std::string bone_disp_name_english;

        void Read(IOStream *stream)
        {
            bone_disp_name = PmdHelper::ReadString(stream, 50);
            bone_disp_name_english.clear();
        }
        void ReadExpantion(IOStream *stream)
        {
            bone_disp_name_english = PmdHelper::ReadString(stream, 50);
        }
    };

    class PmdBoneDisp
    {
    public:
        uint16_t bone_index;
        uint8_t bone_disp_index;

        void Read(IOStream *stream)
        {
            stream->Read((char*)&bone_index, sizeof(uint16_t), 1);
            stream->Read((char*)&bone_disp_index, sizeof(uint8_t), 1);
        }
    };

    enum class RigidBodyShape : uint8_t
    {
        Sphere = 0,
        Box = 1,
        Cpusel = 2
    };

    enum class RigidBodyType : uint8_t
    {
        BoneConnected = 0,
        Physics = 1,
        ConnectedPhysics = 2
    };

    class PmdRigidBody
    {
    public:
        std::string name;
        uint16_t related_bone_index;
        uint8_t group_index;
        uint16_t mask;
        RigidBodyShape shape;
        float size[3];
        float position[3];
        float orientation[3];
        float weight;
        float linear_damping;
        float anglar_damping;
        float restitution;
        float friction;
        RigidBodyType rigid_type;

        void Read(IOStream *stream)
        {
            name = PmdHelper::ReadString(stream, 20);
            stream->Read((char*)&related_bone_index, sizeof(uint16_t), 1);
            stream->Read((char*)&group_index, sizeof(uint8_t), 1);
            stream->Read((char*)&mask, sizeof(uint16_t), 1);
            stream->Read((char*)&shape, sizeof(uint8_t), 1);
            stream->Read((char*)size, sizeof(float) * 3, 1);
            stream->Read((char*)position, sizeof(float) * 3, 1);
            stream->Read((char*)orientation, sizeof(float) * 3, 1);
            stream->Read((char*)&weight, sizeof(float), 1);
            stream->Read((char*)&linear_damping, sizeof(float), 1);
            stream->Read((char*)&anglar_damping, sizeof(float), 1);
            stream->Read((char*)&restitution, sizeof(float), 1);
            stream->Read((char*)&friction, sizeof(float), 1);
            stream->Read((char*)&rigid_type, sizeof(char), 1);
        }
    };

    class PmdConstraint
    {
    public:
        std::string name;
        uint32_t rigid_body_index_a;
        uint32_t rigid_body_index_b;
        float position[3];
        float orientation[3];
        float linear_lower_limit[3];
        float linear_upper_limit[3];
        float angular_lower_limit[3];
        float angular_upper_limit[3];
        float linear_stiffness[3];
        float angular_stiffness[3];

        void Read(IOStream *stream)
        {
            name = PmdHelper::ReadString(stream, 20);
            stream->Read((char *)&rigid_body_index_a, sizeof(uint32_t), 1);
            stream->Read((char *)&rigid_body_index_b, sizeof(uint32_t), 1);
            stream->Read((char *)position, sizeof(float) * 3, 1);
            stream->Read((char *)orientation, sizeof(float) * 3, 1);
            stream->Read((char *)linear_lower_limit, sizeof(float) * 3, 1);
            stream->Read((char *)linear_upper_limit, sizeof(float) * 3, 1);
            stream->Read((char *)angular_lower_limit, sizeof(float) * 3, 1);
            stream->Read((char *)angular_upper_limit, sizeof(float) * 3, 1);
            stream->Read((char *)linear_stiffness, sizeof(float) * 3, 1);
            stream->Read((char *)angular_stiffness, sizeof(float) * 3, 1);
        }
    };
    class PmdModel
    {
    public:
        float version;
        PmdHeader header;
        std::vector<PmdVertex> vertices;
        std::vector<uint16_t> indices;
        std::vector<PmdMaterial> materials;
        std::vector<PmdBone> bones;
        std::vector<PmdIk> iks;
        std::vector<PmdFace> faces;
        std::vector<uint16_t> faces_indices;
        std::vector<PmdBoneDispName> bone_disp_name;
        std::vector<PmdBoneDisp> bone_disp;
        std::vector<std::string> toon_filenames;
        std::vector<PmdRigidBody> rigid_bodies;
        std::vector<PmdConstraint> constraints;

        static std::unique_ptr<PmdModel> LoadFromFile(const char *file, IOSystem * pIOHandler)
        {
            std::unique_ptr<IOStream> fileStreamPtr(pIOHandler->Open(file, "rb"));
            IOStream* stream = fileStreamPtr.get();
            if (!stream)
            {
                std::cerr << "could not open \"" << file << "\"" << std::endl;
                return nullptr;
            }
            auto result = LoadFromStream(stream);
            //stream.Flush();
            return result;
        }

        static std::unique_ptr<PmdModel> LoadFromStream(IOStream *stream)
        {
            auto result = mmd::make_unique<PmdModel>();

            // magic
            char magic[3];
            stream->Read(magic, 3, 1);
            if (magic[0] != 'P' || magic[1] != 'm' || magic[2] != 'd')
            {
                std::cerr << "invalid file" << std::endl;
                return nullptr;
            }

            // version
            stream->Read((char*) &(result->version), sizeof(float), 1);
            if (result->version != 1.0f)
            {
                std::cerr << "invalid version" << std::endl;
                return nullptr;
            }

            // header
            result->header.Read(stream);

            // vertices
            uint32_t vertex_num;
            stream->Read((char*)&vertex_num, sizeof(uint32_t), 1);
            result->vertices.resize(vertex_num);
            for (uint32_t i = 0; i < vertex_num; i++)
            {
                result->vertices[i].Read(stream);
            }

            // indices
            uint32_t index_num;
            stream->Read((char*)&index_num, sizeof(uint32_t), 1);
            result->indices.resize(index_num);
            for (uint32_t i = 0; i < index_num; i++)
            {
                stream->Read((char*)&result->indices[i], sizeof(uint16_t), 1);
            }

            // materials
            uint32_t material_num;
            stream->Read((char*)&material_num, sizeof(uint32_t), 1);
            result->materials.resize(material_num);
            for (uint32_t i = 0; i < material_num; i++)
            {
                result->materials[i].Read(stream);
            }

            // bones
            uint16_t bone_num;
            stream->Read((char*)&bone_num, sizeof(uint16_t), 1);
            result->bones.resize(bone_num);
            for (uint32_t i = 0; i < bone_num; i++)
            {
                result->bones[i].Read(stream);
            }

            // iks
            uint16_t ik_num;
            stream->Read((char*)&ik_num, sizeof(uint16_t), 1);
            result->iks.resize(ik_num);
            for (uint32_t i = 0; i < ik_num; i++)
            {
                result->iks[i].Read(stream);
            }

            // faces
            uint16_t face_num;
            stream->Read((char*)&face_num, sizeof(uint16_t), 1);
            result->faces.resize(face_num);
            for (uint32_t i = 0; i < face_num; i++)
            {
                result->faces[i].Read(stream);
            }

            // face frames
            uint8_t face_frame_num;
            stream->Read((char*)&face_frame_num, sizeof(uint8_t), 1);
            result->faces_indices.resize(face_frame_num);
            for (uint32_t i = 0; i < face_frame_num; i++)
            {
                stream->Read((char*)&result->faces_indices[i], sizeof(uint16_t), 1);
            }

            // bone names
            uint8_t bone_disp_num;
            stream->Read((char*)&bone_disp_num, sizeof(uint8_t), 1);
            result->bone_disp_name.resize(bone_disp_num);
            for (uint32_t i = 0; i < bone_disp_num; i++)
            {
                result->bone_disp_name[i].Read(stream);
            }

            // bone frame
            uint32_t bone_frame_num;
            stream->Read((char*)&bone_frame_num, sizeof(uint32_t), 1);
            result->bone_disp.resize(bone_frame_num);
            for (uint32_t i = 0; i < bone_frame_num; i++)
            {
                result->bone_disp[i].Read(stream);
            }

            // english name
            bool english;
            stream->Read((char*)&english, sizeof(char), 1);
            if (english)
            {
                result->header.ReadExtension(stream);
                for (uint32_t i = 0; i < bone_num; i++)
                {
                    result->bones[i].ReadExpantion(stream);
                }
                for (uint32_t i = 0; i < face_num; i++)
                {
                    if (result->faces[i].type == pmd::FaceCategory::Base)
                    {
                        continue;
                    }
                    result->faces[i].ReadExpantion(stream);
                }
                for (uint32_t i = 0; i < result->bone_disp_name.size(); i++)
                {
                    result->bone_disp_name[i].ReadExpantion(stream);
                }
            }

            // toon textures
            if (stream->Tell() >= stream->FileSize())
            {
                result->toon_filenames.clear();
            }
            else {
                result->toon_filenames.resize(10);
                for (uint32_t i = 0; i < 10; i++)
                {
                    result->toon_filenames[i] = PmdHelper::ReadString(stream, 100);
                }
            }

            // physics
            if (stream->Tell() >= stream->FileSize())
            {
                result->rigid_bodies.clear();
                result->constraints.clear();
            }
            else {
                uint32_t rigid_body_num;
                stream->Read((char*)&rigid_body_num, sizeof(uint32_t), 1);
                result->rigid_bodies.resize(rigid_body_num);
                for (uint32_t i = 0; i < rigid_body_num; i++)
                {
                    result->rigid_bodies[i].Read(stream);
                }
                uint32_t constraint_num;
                stream->Read((char*)&constraint_num, sizeof(uint32_t), 1);
                result->constraints.resize(constraint_num);
                for (uint32_t i = 0; i < constraint_num; i++)
                {
                    result->constraints[i].Read(stream);
                }
            }

            if (stream->Tell() < stream->FileSize())
            {
                std::cerr << "there is unknown data" << std::endl;
            }

            return result;
        }
    };
}
