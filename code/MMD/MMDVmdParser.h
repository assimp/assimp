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

#include <vector>
#include <string>
#include <memory>
#include "MMDCpp14.h"
#include <assimp/DefaultIOSystem.h>

using namespace::Assimp;

namespace vmd
{
	class VmdBoneFrame
	{
	public:
		std::string name;
		int frame;
		float position[3];
		float orientation[4];
		char interpolation[4][4][4];

		void Read(IOStream* stream)
		{
			char buffer[15];
			stream->Read((char*) buffer, sizeof(char)*15, 1);
			name = std::string(buffer);
			stream->Read((char*) &frame, sizeof(int), 1);
			stream->Read((char*) position, sizeof(float)*3, 1);
			stream->Read((char*) orientation, sizeof(float)*4, 1);
			stream->Read((char*) interpolation, sizeof(char) * 4 * 4 * 4, 1);
		}

		void Write(IOStream* stream)
		{
			stream->Write((char*)name.c_str(), sizeof(char) * 15, 1);
			stream->Write((char*)&frame, sizeof(int), 1);
			stream->Write((char*)position, sizeof(float) * 3, 1);
			stream->Write((char*)orientation, sizeof(float) * 4, 1);
			stream->Write((char*)interpolation, sizeof(char) * 4 * 4 * 4, 1);
		}
	};

	class VmdFaceFrame
	{
	public:
		std::string face_name;
		float weight;
		uint32_t frame;

		void Read(IOStream* stream)
		{
			char buffer[15];
			stream->Read((char*) &buffer, sizeof(char) * 15, 1);
			face_name = std::string(buffer);
			stream->Read((char*) &frame, sizeof(int), 1);
			stream->Read((char*) &weight, sizeof(float), 1);
		}

		void Write(IOStream* stream)
		{
			stream->Write((char*)face_name.c_str(), sizeof(char) * 15, 1);
			stream->Write((char*)&frame, sizeof(int), 1);
			stream->Write((char*)&weight, sizeof(float), 1);
		}
	};

	class VmdCameraFrame
	{
	public:
		int frame;
		float distance;
		float position[3];
		float orientation[3];
		char interpolation[6][4];
		float angle;
		char unknown[3];

		void Read(IOStream *stream)
		{
			stream->Read((char*) &frame, sizeof(int),1);
			stream->Read((char*) &distance, sizeof(float), 1);
			stream->Read((char*) position, sizeof(float) * 3, 1);
			stream->Read((char*) orientation, sizeof(float) * 3, 1);
			stream->Read((char*) interpolation, sizeof(char) * 24, 1);
			stream->Read((char*) &angle, sizeof(float), 1);
			stream->Read((char*) unknown, sizeof(char) * 3, 1);
		}

		void Write(IOStream *stream)
		{
			stream->Write((char*)&frame, sizeof(int), 1);
			stream->Write((char*)&distance, sizeof(float), 1);
			stream->Write((char*)position, sizeof(float) * 3, 1);
			stream->Write((char*)orientation, sizeof(float) * 3, 1);
			stream->Write((char*)interpolation, sizeof(char) * 24, 1);
			stream->Write((char*)&angle, sizeof(float), 1);
			stream->Write((char*)unknown, sizeof(char) * 3, 1);
		}
	};

	class VmdLightFrame
	{
	public:
		int frame;
		float color[3];
		float position[3];

		void Read(IOStream* stream)
		{
			stream->Read((char*) &frame, sizeof(int), 1);
			stream->Read((char*) color, sizeof(float) * 3, 1);
			stream->Read((char*) position, sizeof(float) * 3, 1);
		}

		void Write(IOStream* stream)
		{
			stream->Write((char*)&frame, sizeof(int), 1);
			stream->Write((char*)color, sizeof(float) * 3, 1);
			stream->Write((char*)position, sizeof(float) * 3, 1);
		}
	};

	class VmdIkEnable
	{
	public:
		std::string ik_name;
		bool enable;
	};

	class VmdIkFrame
	{
	public:
		int frame;
		bool display;
		std::vector<VmdIkEnable> ik_enable;

		void Read(IOStream *stream)
		{
			char buffer[20];
			stream->Read((char*) &frame, sizeof(int), 1);
			stream->Read((char*) &display, sizeof(uint8_t), 1);
			int ik_count;
			stream->Read((char*) &ik_count, sizeof(int), 1);
			ik_enable.resize(ik_count);
			for (int i = 0; i < ik_count; i++)
			{
				stream->Read(buffer, 20, 1);
				ik_enable[i].ik_name = std::string(buffer);
				stream->Read((char*) &ik_enable[i].enable, sizeof(uint8_t), 1);
			}
		}

		void Write(IOStream *stream)
		{
			stream->Write((char*)&frame, sizeof(int), 1);
			stream->Write((char*)&display, sizeof(uint8_t), 1);
			int ik_count = static_cast<int>(ik_enable.size());
			stream->Write((char*)&ik_count, sizeof(int), 1);
			for (int i = 0; i < ik_count; i++)
			{
				const VmdIkEnable& ik_enable = this->ik_enable.at(i);
				stream->Write(ik_enable.ik_name.c_str(), 20, 1);
				stream->Write((char*)&ik_enable.enable, sizeof(uint8_t), 1);
			}
		}
	};

	class VmdMotion
	{
	public:
		std::string model_name;
		int version;
		std::vector<VmdBoneFrame> bone_frames;
		std::vector<VmdFaceFrame> face_frames;
		std::vector<VmdCameraFrame> camera_frames;
		std::vector<VmdLightFrame> light_frames;
		std::vector<VmdIkFrame> ik_frames;

		static std::unique_ptr<VmdMotion> LoadFromFile(char const *file, IOSystem * pIOHandler)
		{
            std::unique_ptr<IOStream> fileStreamPtr(pIOHandler->Open(file, "rb"));
            IOStream* stream = fileStreamPtr.get();
			auto result = LoadFromStream(stream);
			return result;
		}

		static std::unique_ptr<VmdMotion> LoadFromStream(IOStream *stream)
		{

			char buffer[30];
			auto result = mmd::make_unique<VmdMotion>();

			// magic and version
			stream->Read((char*) buffer, 30, 1);
			if (strncmp(buffer, "Vocaloid Motion Data", 20))
			{
				std::cerr << "invalid vmd file." << std::endl;
				return nullptr;
			}
			result->version = std::atoi(buffer + 20);

			// name
			stream->Read(buffer, 20, 1);
			result->model_name = std::string(buffer);

			// bone frames
			int bone_frame_num;
			stream->Read((char*) &bone_frame_num, sizeof(int), 1);
			result->bone_frames.resize(bone_frame_num);
			for (int i = 0; i < bone_frame_num; i++)
			{
				result->bone_frames[i].Read(stream);
			}

			// face frames
			int face_frame_num;
			stream->Read((char*) &face_frame_num, sizeof(int), 1);
			result->face_frames.resize(face_frame_num);
			for (int i = 0; i < face_frame_num; i++)
			{
				result->face_frames[i].Read(stream);
			}

			// camera frames
			int camera_frame_num;
			stream->Read((char*) &camera_frame_num, sizeof(int), 1);
			result->camera_frames.resize(camera_frame_num);
			for (int i = 0; i < camera_frame_num; i++)
			{
				result->camera_frames[i].Read(stream);
			}

			// light frames
			int light_frame_num;
			stream->Read((char*) &light_frame_num, sizeof(int), 1);
			result->light_frames.resize(light_frame_num);
			for (int i = 0; i < light_frame_num; i++)
			{
				result->light_frames[i].Read(stream);
			}

			// unknown2
			stream->Read(buffer, 4, 1);

			// ik frames
			if (stream->Tell() < stream->FileSize())
			{
				int ik_num;
				stream->Read((char*) &ik_num, sizeof(int), 1);
				result->ik_frames.resize(ik_num);
				for (int i = 0; i < ik_num; i++)
				{
					result->ik_frames[i].Read(stream);
				}
			}

			if (stream->Tell() < stream->FileSize())
			{
				std::cerr << "vmd stream has unknown data." << std::endl;
			}

			return result;
		}

        bool SaveToFile(const std::u16string& /*filename*/)
		{
			// TODO: How to adapt u16string to string?
			/*
			IOStream stream(filename.c_str(), std::ios::binary);
			auto result = SaveToStream(&stream);
			stream.close();
			return result;
			*/
			return false;
		}

		bool SaveToStream(IOStream *stream)
		{
			std::string magic = "Vocaloid Motion Data 0002\0";
			magic.resize(30);

			// magic and version
			stream->Write(magic.c_str(), 30, 1);

			// name
			stream->Write(model_name.c_str(), 20, 1);

			// bone frames
			const int bone_frame_num = static_cast<int>(bone_frames.size());
			stream->Write(reinterpret_cast<const char*>(&bone_frame_num), sizeof(int), 1);
			for (int i = 0; i < bone_frame_num; i++)
			{
				bone_frames[i].Write(stream);
			}

			// face frames
			const int face_frame_num = static_cast<int>(face_frames.size());
			stream->Write(reinterpret_cast<const char*>(&face_frame_num), sizeof(int), 1);
			for (int i = 0; i < face_frame_num; i++)
			{
				face_frames[i].Write(stream);
			}

			// camera frames
			const int camera_frame_num = static_cast<int>(camera_frames.size());
			stream->Write(reinterpret_cast<const char*>(&camera_frame_num), sizeof(int), 1);
			for (int i = 0; i < camera_frame_num; i++)
			{
				camera_frames[i].Write(stream);
			}

			// light frames
			const int light_frame_num = static_cast<int>(light_frames.size());
			stream->Write(reinterpret_cast<const char*>(&light_frame_num), sizeof(int), 1);
			for (int i = 0; i < light_frame_num; i++)
			{
				light_frames[i].Write(stream);
			}

			// self shadow datas
			const int self_shadow_num = 0;
			stream->Write(reinterpret_cast<const char*>(&self_shadow_num), sizeof(int), 1);

			// ik frames
			const int ik_num = static_cast<int>(ik_frames.size());
			stream->Write(reinterpret_cast<const char*>(&ik_num), sizeof(int), 1);
			for (int i = 0; i < ik_num; i++)
			{
				ik_frames[i].Write(stream);
			}

			return true;
		}
	};
}
