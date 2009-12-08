/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

 * Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

#pragma once

//managed includes
#include "Vector3D.h"
#include "Color3D.h"

//natvie includes
#include "aiLight.h"

using namespace System;

namespace AssimpNET
{
	public enum LightSourceType
	{
		aiLightSource_UNDEFINED = 0x0,
		aiLightSource_DIRECTIONAL = 0x1,
		aiLightSource_POINT = 0x2,
		aiLightSource_SPOT = 0x3,
		_aiLightSource_Force32Bit = 0x9fffffff
	};

	public ref class Light
	{
	public:
		Light(void);
		Light(aiLight* native);
		~Light(void);

		property float AngleInnerCone
		{
			float get()
			{
				return this->p_native->mAngleInnerCone;
			}
			void set(float value)
			{
				this->p_native->mAngleInnerCone = value;
			}
		}

		property float AngleOuterCone
		{
			float get()
			{
				return this->p_native->mAngleOuterCone;
			}
			void set(float value)
			{
				this->p_native->mAngleOuterCone = value;
			}
		}

		property float AttenuationConstant
		{
			float get()
			{
				return this->p_native->mAttenuationConstant;
			}
			void set(float value)
			{
				this->p_native->mAttenuationConstant = value;
			}
		}

		property float AttenuationLinear
		{
			float get()
			{
				return this->p_native->mAttenuationLinear;
			}
			void set(float value)
			{
				this->p_native->mAttenuationLinear = value;
			}
		}

		property float AttenuationQuadratic
		{
			float get()
			{
				return this->p_native->mAttenuationQuadratic;
			}
			void set(float value)
			{
				this->p_native->mAttenuationQuadratic = value;
			}
		}

		property Color3D^ ColorAmbient
		{
			Color3D^ get()
			{
				return gcnew Color3D(&this->p_native->mColorAmbient);
			}
			void set(Color3D^ value)
			{
				this->p_native->mColorAmbient = aiColor3D(*value->getNative());
			}
		}
		
		property Color3D^ ColorDiffuse
		{
			Color3D^ get()
			{
				return gcnew Color3D(&this->p_native->mColorDiffuse);
			}
			void set(Color3D^ value)
			{
				this->p_native->mColorDiffuse = aiColor3D(*value->getNative());
			}
		}
		
		property Color3D^ ColorSpecular
		{
			Color3D^ get()
			{
				return gcnew Color3D(&this->p_native->mColorSpecular);
			}
			void set(Color3D^ value)
			{
				this->p_native->mColorSpecular = aiColor3D(*value->getNative());
			}
		}
		
		property Vector3D^ Direction
		{
			Vector3D^ get()
			{
				return gcnew Vector3D(&this->p_native->mDirection);
			}
			void set(Vector3D^ value)
			{
				this->p_native->mDirection = aiVector3D(*value->getNative());
			}
		}
	
		property Vector3D^ Position
		{
			Vector3D^ get()
			{
				return gcnew Vector3D(&this->p_native->mPosition);
			}
			void set(Vector3D^ value)
			{
				this->p_native->mPosition = aiVector3D(*value->getNative());
			}
		}
		
		property String^ Name
		{
			String^ get()
			{
				return gcnew String(this->p_native->mName.data);	
			}
			void set(String^ value)
			{
				this->p_native->mName.Set((char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(value).ToPointer());
			}
		}
		
		property LightSourceType Type
		{
			LightSourceType get(){throw gcnew System::NotImplementedException();}
			void set(LightSourceType value){throw gcnew System::NotImplementedException();}
		}

		aiLight* getNative();
		private:
		aiLight *p_native;
	};
}//namespace