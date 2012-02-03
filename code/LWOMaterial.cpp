/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

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

/** @file Implementation of the material oart of the LWO importer class */


#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_LWO_IMPORTER

// internal headers
#include "LWOLoader.h"
#include "ByteSwap.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
template <class T>
T lerp(const T& one, const T& two, float val)
{
	return one + (two-one)*val;
}

// ------------------------------------------------------------------------------------------------
// Convert a lightwave mapping mode to our's
inline aiTextureMapMode GetMapMode(LWO::Texture::Wrap in)
{
	switch (in)
	{	
		case LWO::Texture::REPEAT:
			return aiTextureMapMode_Wrap;

		case LWO::Texture::MIRROR:
			return aiTextureMapMode_Mirror;

		case LWO::Texture::RESET:
			DefaultLogger::get()->warn("LWO2: Unsupported texture map mode: RESET");

			// fall though here
		case LWO::Texture::EDGE:
			return aiTextureMapMode_Clamp;
	}
	return (aiTextureMapMode)0;
}

// ------------------------------------------------------------------------------------------------
bool LWOImporter::HandleTextures(aiMaterial* pcMat, const TextureList& in, aiTextureType type)
{
	ai_assert(NULL != pcMat);

	unsigned int cur = 0, temp = 0;
	aiString s;
	bool ret = false;

	for (TextureList::const_iterator it = in.begin(), end = in.end();it != end;++it)	{
		if (!(*it).enabled || !(*it).bCanUse)
			continue;
		ret = true;

		// Convert lightwave's mapping modes to ours. We let them
		// as they are, the GenUVcoords step will compute UV 
		// channels if they're not there.

		aiTextureMapping mapping;
		switch ((*it).mapMode)
		{
			case LWO::Texture::Planar:
				mapping = aiTextureMapping_PLANE;
				break;
			case LWO::Texture::Cylindrical:
				mapping = aiTextureMapping_CYLINDER;
				break;
			case LWO::Texture::Spherical:
				mapping = aiTextureMapping_SPHERE;
				break;
			case LWO::Texture::Cubic:
				mapping = aiTextureMapping_BOX;
				break;
			case LWO::Texture::FrontProjection:
				DefaultLogger::get()->error("LWO2: Unsupported texture mapping: FrontProjection");
				mapping = aiTextureMapping_OTHER;
				break;
			case LWO::Texture::UV:
				{
					if( UINT_MAX == (*it).mRealUVIndex )	{
						// We have no UV index for this texture, so we can't display it
						continue;
					}

					// add the UV source index
					temp = (*it).mRealUVIndex;
					pcMat->AddProperty<int>((int*)&temp,1,AI_MATKEY_UVWSRC(type,cur));

					mapping = aiTextureMapping_UV;
				}
				break;
			default:
				ai_assert(false);
		};

		if (mapping != aiTextureMapping_UV)	{
			// Setup the main axis 
			aiVector3D v;
			switch ((*it).majorAxis)	{
				case Texture::AXIS_X:
					v = aiVector3D(1.f,0.f,0.f);
					break;
				case Texture::AXIS_Y:
					v = aiVector3D(0.f,1.f,0.f);
					break;
				default: // case Texture::AXIS_Z:
					v = aiVector3D(0.f,0.f,1.f);
					break;
			}

			pcMat->AddProperty(&v,1,AI_MATKEY_TEXMAP_AXIS(type,cur));

			// Setup UV scalings for cylindric and spherical projections
			if (mapping == aiTextureMapping_CYLINDER || mapping == aiTextureMapping_SPHERE)	{
				aiUVTransform trafo;
				trafo.mScaling.x = (*it).wrapAmountW;
				trafo.mScaling.y = (*it).wrapAmountH;

				BOOST_STATIC_ASSERT(sizeof(aiUVTransform)/sizeof(float) == 5);
				pcMat->AddProperty(&trafo,1,AI_MATKEY_UVTRANSFORM(type,cur));
			}
			DefaultLogger::get()->debug("LWO2: Setting up non-UV mapping");
		}

		// The older LWOB format does not use indirect references to clips.
		// The file name of a texture is directly specified in the tex chunk.
		if (mIsLWO2)	{
			// find the corresponding clip
			ClipList::iterator clip = mClips.begin();
			temp = (*it).mClipIdx;
			for (ClipList::iterator end = mClips.end(); clip != end; ++clip)	{
				if ((*clip).idx == temp)
					break;
				
			}
			if (mClips.end() == clip)	{
				DefaultLogger::get()->error("LWO2: Clip index is out of bounds");
				temp = 0;

				// fixme: appearently some LWO files shipping with Doom3 don't
				// have clips at all ... check whether that's true or whether
				// it's a bug in the loader.

				s.Set("$texture.png");

				//continue;
			}
			else {
				if (Clip::UNSUPPORTED == (*clip).type)	{
					DefaultLogger::get()->error("LWO2: Clip type is not supported");
					continue;
				}
				AdjustTexturePath((*clip).path);
				s.Set((*clip).path);

				// Additional image settings
				int flags = 0;
				if ((*clip).negate) {
					flags |= aiTextureFlags_Invert;
				}
				pcMat->AddProperty(&flags,1,AI_MATKEY_TEXFLAGS(type,cur));
			}
		}
		else 
		{
			std::string ss = (*it).mFileName;
			if (!ss.length()) {
				DefaultLogger::get()->error("LWOB: Empty file name");
				continue;
			}
			AdjustTexturePath(ss);
			s.Set(ss);
		}
		pcMat->AddProperty(&s,AI_MATKEY_TEXTURE(type,cur));

		// add the blend factor
		pcMat->AddProperty<float>(&(*it).mStrength,1,AI_MATKEY_TEXBLEND(type,cur));

		// add the blend operation
		switch ((*it).blendType)
		{
			case LWO::Texture::Normal:
			case LWO::Texture::Multiply:
				temp = (unsigned int)aiTextureOp_Multiply;
				break;

			case LWO::Texture::Subtractive:
			case LWO::Texture::Difference:
				temp = (unsigned int)aiTextureOp_Subtract;
				break;

			case LWO::Texture::Divide:
				temp = (unsigned int)aiTextureOp_Divide;
				break;

			case LWO::Texture::Additive:
				temp = (unsigned int)aiTextureOp_Add;
				break;

			default:
				temp = (unsigned int)aiTextureOp_Multiply;
				DefaultLogger::get()->warn("LWO2: Unsupported texture blend mode: alpha or displacement");

		}
		// Setup texture operation
		pcMat->AddProperty<int>((int*)&temp,1,AI_MATKEY_TEXOP(type,cur));

		// setup the mapping mode
		pcMat->AddProperty<int>((int*)&mapping,1,AI_MATKEY_MAPPING(type,cur));

		// add the u-wrapping
		temp = (unsigned int)GetMapMode((*it).wrapModeWidth);
		pcMat->AddProperty<int>((int*)&temp,1,AI_MATKEY_MAPPINGMODE_U(type,cur));

		// add the v-wrapping
		temp = (unsigned int)GetMapMode((*it).wrapModeHeight);
		pcMat->AddProperty<int>((int*)&temp,1,AI_MATKEY_MAPPINGMODE_V(type,cur));

		++cur;
	}
	return ret;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::ConvertMaterial(const LWO::Surface& surf,aiMaterial* pcMat)
{
	// copy the name of the surface
	aiString st;
	st.Set(surf.mName);
	pcMat->AddProperty(&st,AI_MATKEY_NAME);

	const int i = surf.bDoubleSided ? 1 : 0;
	pcMat->AddProperty(&i,1,AI_MATKEY_TWOSIDED);

	// add the refraction index and the bump intensity
	pcMat->AddProperty(&surf.mIOR,1,AI_MATKEY_REFRACTI);
	pcMat->AddProperty(&surf.mBumpIntensity,1,AI_MATKEY_BUMPSCALING);
	
	aiShadingMode m;
	if (surf.mSpecularValue && surf.mGlossiness)
	{
		float fGloss;
		if (mIsLWO2)	{
			fGloss = pow( surf.mGlossiness*10.0f+2.0f, 2.0f);
		}
		else
		{
			if (16.0f >= surf.mGlossiness)
				fGloss = 6.0f;
			else if (64.0f >= surf.mGlossiness)
				fGloss = 20.0f;
			else if (256.0f >= surf.mGlossiness)
				fGloss = 50.0f;
			else fGloss = 80.0f;
		}

		pcMat->AddProperty(&surf.mSpecularValue,1,AI_MATKEY_SHININESS_STRENGTH);
		pcMat->AddProperty(&fGloss,1,AI_MATKEY_SHININESS);
		m = aiShadingMode_Phong;
	}
	else m = aiShadingMode_Gouraud;

	// specular color
	aiColor3D clr = lerp( aiColor3D(1.f,1.f,1.f), surf.mColor, surf.mColorHighlights );
	pcMat->AddProperty(&clr,1,AI_MATKEY_COLOR_SPECULAR);
	pcMat->AddProperty(&surf.mSpecularValue,1,AI_MATKEY_SHININESS_STRENGTH);

	// emissive color
	// luminosity is not really the same but it affects the surface in a similar way. Some scaling looks good.
	clr.g = clr.b = clr.r = surf.mLuminosity*0.8f;
	pcMat->AddProperty<aiColor3D>(&clr,1,AI_MATKEY_COLOR_EMISSIVE);

	// opacity ... either additive or default-blended, please
	if (0.f != surf.mAdditiveTransparency)	{

		const int add = aiBlendMode_Additive;
		pcMat->AddProperty(&surf.mAdditiveTransparency,1,AI_MATKEY_OPACITY);
		pcMat->AddProperty(&add,1,AI_MATKEY_BLEND_FUNC);
	}

	else if (10e10f != surf.mTransparency)	{
		const int def = aiBlendMode_Default;
		const float f = 1.0f-surf.mTransparency;
		pcMat->AddProperty(&f,1,AI_MATKEY_OPACITY);
		pcMat->AddProperty(&def,1,AI_MATKEY_BLEND_FUNC);
	}
	

	// ADD TEXTURES to the material
	// TODO: find out how we can handle COLOR textures correctly...
	bool b = HandleTextures(pcMat,surf.mColorTextures,aiTextureType_DIFFUSE);
	b = (b || HandleTextures(pcMat,surf.mDiffuseTextures,aiTextureType_DIFFUSE));
	HandleTextures(pcMat,surf.mSpecularTextures,aiTextureType_SPECULAR);
	HandleTextures(pcMat,surf.mGlossinessTextures,aiTextureType_SHININESS);
	HandleTextures(pcMat,surf.mBumpTextures,aiTextureType_HEIGHT);
	HandleTextures(pcMat,surf.mOpacityTextures,aiTextureType_OPACITY);
	HandleTextures(pcMat,surf.mReflectionTextures,aiTextureType_REFLECTION);

	// Now we need to know which shader to use .. iterate through the shader list of
	// the surface and  search for a name which we know ... 
	for (ShaderList::const_iterator it = surf.mShaders.begin(), end = surf.mShaders.end();it != end;++it)	{
		//if (!(*it).enabled)continue;

		if ((*it).functionName == "LW_SuperCelShader" || (*it).functionName == "AH_CelShader")	{
			DefaultLogger::get()->info("LWO2: Mapping LW_SuperCelShader/AH_CelShader to aiShadingMode_Toon");

			m = aiShadingMode_Toon;
			break;
		}
		else if ((*it).functionName == "LW_RealFresnel" || (*it).functionName == "LW_FastFresnel")	{
			DefaultLogger::get()->info("LWO2: Mapping LW_RealFresnel/LW_FastFresnel to aiShadingMode_Fresnel");

			m = aiShadingMode_Fresnel;
			break;
		}
		else
		{
			DefaultLogger::get()->warn("LWO2: Unknown surface shader: " + (*it).functionName);
		}
	}
	if (surf.mMaximumSmoothAngle <= 0.0f)
		m = aiShadingMode_Flat;
	pcMat->AddProperty((int*)&m,1,AI_MATKEY_SHADING_MODEL);

	// (the diffuse value is just a scaling factor)
	// If a diffuse texture is set, we set this value to 1.0
	clr = (b && false ? aiColor3D(1.f,1.f,1.f) : surf.mColor);
	clr.r *= surf.mDiffuseValue;
	clr.g *= surf.mDiffuseValue;
	clr.b *= surf.mDiffuseValue;
	pcMat->AddProperty<aiColor3D>(&clr,1,AI_MATKEY_COLOR_DIFFUSE);
}

// ------------------------------------------------------------------------------------------------
char LWOImporter::FindUVChannels(LWO::TextureList& list,
	LWO::Layer& /*layer*/,LWO::UVChannel& uv, unsigned int next)
{
	char ret = 0;
	for (TextureList::iterator it = list.begin(), end = list.end();it != end;++it)	{

		// Ignore textures with non-UV mappings for the moment.
		if (!(*it).enabled || !(*it).bCanUse || (*it).mapMode != LWO::Texture::UV)	{
			continue;
		}
		
		if ((*it).mUVChannelIndex == uv.name) {
			ret = 1;
		
			// got it.
			if ((*it).mRealUVIndex == UINT_MAX || (*it).mRealUVIndex == next)
			{
				(*it).mRealUVIndex = next;
			}
			else {
				// channel mismatch. need to duplicate the material.
				DefaultLogger::get()->warn("LWO: Channel mismatch, would need to duplicate surface [design bug]");

				// TODO
			}
		}
	}
	return ret;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::FindUVChannels(LWO::Surface& surf, 
	LWO::SortedRep& sorted,LWO::Layer& layer,
	unsigned int out[AI_MAX_NUMBER_OF_TEXTURECOORDS])
{
	unsigned int next = 0, extra = 0, num_extra = 0;

	// Check whether we have an UV entry != 0 for one of the faces in 'sorted'
	for (unsigned int i = 0; i < layer.mUVChannels.size();++i)	{
		LWO::UVChannel& uv = layer.mUVChannels[i];

		for (LWO::SortedRep::const_iterator it = sorted.begin(); it != sorted.end(); ++it)	{
			
			LWO::Face& face = layer.mFaces[*it];

			for (unsigned int n = 0; n < face.mNumIndices; ++n) {
				unsigned int idx = face.mIndices[n];

				if (uv.abAssigned[idx] && ((aiVector2D*)&uv.rawData[0])[idx] != aiVector2D()) {

					if (extra >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {

						DefaultLogger::get()->error("LWO: Maximum number of UV channels for "
							"this mesh reached. Skipping channel \'" + uv.name + "\'");

					}
					else {
						// Search through all textures assigned to 'surf' and look for this UV channel
						char had = 0;
						had |= FindUVChannels(surf.mColorTextures,layer,uv,next);
						had |= FindUVChannels(surf.mDiffuseTextures,layer,uv,next);
						had |= FindUVChannels(surf.mSpecularTextures,layer,uv,next);
						had |= FindUVChannels(surf.mGlossinessTextures,layer,uv,next);
						had |= FindUVChannels(surf.mOpacityTextures,layer,uv,next);
						had |= FindUVChannels(surf.mBumpTextures,layer,uv,next);
						had |= FindUVChannels(surf.mReflectionTextures,layer,uv,next);

						// We have a texture referencing this UV channel so we have to take special care
						// and are willing to drop unreferenced channels in favour of it.
						if (had != 0) {
							if (num_extra) {
							
								for (unsigned int a = next; a < std::min( extra, AI_MAX_NUMBER_OF_TEXTURECOORDS-1u ); ++a) {								
									out[a+1] = out[a];
								}
							}
							++extra;
							out[next++] = i;
						}
						// Bäh ... seems not to be used at all. Push to end if enough space is available.
						else {
							out[extra++] = i;
							++num_extra;
						}
					}
					it = sorted.end()-1;
					break;
				}
			}
		}
	}
	if (extra < AI_MAX_NUMBER_OF_TEXTURECOORDS) {
		out[extra] = UINT_MAX;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::FindVCChannels(const LWO::Surface& surf, LWO::SortedRep& sorted, const LWO::Layer& layer,
	unsigned int out[AI_MAX_NUMBER_OF_COLOR_SETS])
{
	unsigned int next = 0;

	// Check whether we have an vc entry != 0 for one of the faces in 'sorted'
	for (unsigned int i = 0; i < layer.mVColorChannels.size();++i)	{
		const LWO::VColorChannel& vc = layer.mVColorChannels[i];

		if (surf.mVCMap == vc.name) {
			// The vertex color map is explicitely requested by the surface so we need to take special care of it
			for (unsigned int a = 0; a < std::min(next,AI_MAX_NUMBER_OF_COLOR_SETS-1u); ++a) {
				out[a+1] = out[a];
			}
			out[0] = i;
			++next;
		}
		else {

			for (LWO::SortedRep::iterator it = sorted.begin(); it != sorted.end(); ++it)	{
				const LWO::Face& face = layer.mFaces[*it];

				for (unsigned int n = 0; n < face.mNumIndices; ++n) {
					unsigned int idx = face.mIndices[n];

					if (vc.abAssigned[idx] && ((aiColor4D*)&vc.rawData[0])[idx] != aiColor4D(0.f,0.f,0.f,1.f)) {
						if (next >= AI_MAX_NUMBER_OF_COLOR_SETS) {

							DefaultLogger::get()->error("LWO: Maximum number of vertex color channels for "
								"this mesh reached. Skipping channel \'" + vc.name + "\'");

						}
						else {
							out[next++] = i;
						}
						it = sorted.end()-1;
						break;
					}
				}
			}
		}
	}
	if (next != AI_MAX_NUMBER_OF_COLOR_SETS) {
		out[next] = UINT_MAX;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2ImageMap(unsigned int size, LWO::Texture& tex )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;
	while (true)
	{
		if (mFileBuffer + 6 >= end)break;
		LE_NCONST IFF::SubChunkHeader* const head = IFF::LoadSubChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
			throw DeadlyImportError("LWO2: Invalid SURF.BLOCK chunk length");

		uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
		case AI_LWO_PROJ:
			tex.mapMode = (Texture::MappingMode)GetU2();
			break;
		case AI_LWO_WRAP:
			tex.wrapModeWidth  = (Texture::Wrap)GetU2();
			tex.wrapModeHeight = (Texture::Wrap)GetU2();
			break;
		case AI_LWO_AXIS:
			tex.majorAxis = (Texture::Axes)GetU2();
			break;
		case AI_LWO_IMAG:
			tex.mClipIdx = GetU2();
			break;
		case AI_LWO_VMAP:
			GetS0(tex.mUVChannelIndex,head->length);
			break;
		case AI_LWO_WRPH:
			tex.wrapAmountH = GetF4();
			break;
		case AI_LWO_WRPW:
			tex.wrapAmountW = GetF4();
			break;
		}
		mFileBuffer = next;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Procedural(unsigned int /*size*/, LWO::Texture& tex )
{
	// --- not supported at the moment
	DefaultLogger::get()->error("LWO2: Found procedural texture, this is not supported");
	tex.bCanUse = false;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Gradient(unsigned int /*size*/, LWO::Texture& tex  )
{
	// --- not supported at the moment
	DefaultLogger::get()->error("LWO2: Found gradient texture, this is not supported");
	tex.bCanUse = false;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2TextureHeader(unsigned int size, LWO::Texture& tex )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	// get the ordinal string
	GetS0( tex.ordinal, size);

	// we could crash later if this is an empty string ...
	if (!tex.ordinal.length())
	{
		DefaultLogger::get()->error("LWO2: Ill-formed SURF.BLOK ordinal string");
		tex.ordinal = "\x00";
	}
	while (true)
	{
		if (mFileBuffer + 6 >= end)break;
		LE_NCONST IFF::SubChunkHeader* const head = IFF::LoadSubChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
			throw DeadlyImportError("LWO2: Invalid texture header chunk length");

		uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
		case AI_LWO_CHAN:
			tex.type = GetU4();
			break;
		case AI_LWO_ENAB:
			tex.enabled = GetU2() ? true : false;
			break;
		case AI_LWO_OPAC:
			tex.blendType = (Texture::BlendType)GetU2();
			tex.mStrength = GetF4();
			break;
		}
		mFileBuffer = next;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2TextureBlock(LE_NCONST IFF::SubChunkHeader* head, unsigned int size )
{
	ai_assert(!mSurfaces->empty());
	LWO::Surface& surf = mSurfaces->back();
	LWO::Texture tex;

	// load the texture header
	LoadLWO2TextureHeader(head->length,tex);
	size -= head->length + 6;

	// now get the exact type of the texture
	switch (head->type)
	{
	case AI_LWO_PROC:
		LoadLWO2Procedural(size,tex);
		break;
	case AI_LWO_GRAD:
		LoadLWO2Gradient(size,tex); 
		break;
	case AI_LWO_IMAP:
		LoadLWO2ImageMap(size,tex);
	}

	// get the destination channel
	TextureList* listRef = NULL;
	switch (tex.type)
	{
	case AI_LWO_COLR:
		listRef = &surf.mColorTextures;break;
	case AI_LWO_DIFF:
		listRef = &surf.mDiffuseTextures;break;
	case AI_LWO_SPEC:
		listRef = &surf.mSpecularTextures;break;
	case AI_LWO_GLOS:
		listRef = &surf.mGlossinessTextures;break;
	case AI_LWO_BUMP:
		listRef = &surf.mBumpTextures;break;
	case AI_LWO_TRAN:
		listRef = &surf.mOpacityTextures;break;
	case AI_LWO_REFL:
		listRef = &surf.mReflectionTextures;break;
	default:
		DefaultLogger::get()->warn("LWO2: Encountered unknown texture type");
		return;
	}

	// now attach the texture to the parent surface - sort by ordinal string
	for (TextureList::iterator it = listRef->begin();it != listRef->end(); ++it)	{
		if (::strcmp(tex.ordinal.c_str(),(*it).ordinal.c_str()) < 0)	{
			listRef->insert(it,tex);
			return;
		}
	}
	listRef->push_back(tex);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2ShaderBlock(LE_NCONST IFF::SubChunkHeader* /*head*/, unsigned int size )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	ai_assert(!mSurfaces->empty());
	LWO::Surface& surf = mSurfaces->back();
	LWO::Shader shader;

	// get the ordinal string
	GetS0( shader.ordinal, size);

	// we could crash later if this is an empty string ...
	if (!shader.ordinal.length())
	{
		DefaultLogger::get()->error("LWO2: Ill-formed SURF.BLOK ordinal string");
		shader.ordinal = "\x00";
	}

	// read the header
	while (true)
	{
		if (mFileBuffer + 6 >= end)break;
		LE_NCONST IFF::SubChunkHeader* const head = IFF::LoadSubChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
			throw DeadlyImportError("LWO2: Invalid shader header chunk length");

		uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
		case AI_LWO_ENAB:
			shader.enabled = GetU2() ? true : false;
			break;

		case AI_LWO_FUNC:
			GetS0( shader.functionName, head->length );
		}
		mFileBuffer = next;
	}

	// now attach the shader to the parent surface - sort by ordinal string
	for (ShaderList::iterator it = surf.mShaders.begin();it != surf.mShaders.end(); ++it)	{
		if (::strcmp(shader.ordinal.c_str(),(*it).ordinal.c_str()) < 0)	{
			surf.mShaders.insert(it,shader);
			return;
		}
	}
	surf.mShaders.push_back(shader);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Surface(unsigned int size)
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	mSurfaces->push_back( LWO::Surface () );
	LWO::Surface& surf = mSurfaces->back();

	GetS0(surf.mName,size);

	// check whether this surface was derived from any other surface
	std::string derived;
	GetS0(derived,(unsigned int)(end - mFileBuffer));
	if (derived.length())	{
		// yes, find this surface
		for (SurfaceList::iterator it = mSurfaces->begin(), end = mSurfaces->end()-1; it != end; ++it)	{
			if ((*it).mName == derived)	{
				// we have it ...
				surf = *it;
				derived.clear();break;
			}
		}
		if (derived.size())
			DefaultLogger::get()->warn("LWO2: Unable to find source surface: " + derived);
	}

	while (true)
	{
		if (mFileBuffer + 6 >= end)
			break;
		LE_NCONST IFF::SubChunkHeader* const head = IFF::LoadSubChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
			throw DeadlyImportError("LWO2: Invalid surface chunk length");

		uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
			// diffuse color
		case AI_LWO_COLR:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,COLR,12);
				surf.mColor.r = GetF4();
				surf.mColor.g = GetF4();
				surf.mColor.b = GetF4();
				break;
			}
			// diffuse strength ... hopefully
		case AI_LWO_DIFF:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,DIFF,4);
				surf.mDiffuseValue = GetF4();
				break;
			}
			// specular strength ... hopefully
		case AI_LWO_SPEC:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,SPEC,4);
				surf.mSpecularValue = GetF4();
				break;
			}
			// transparency
		case AI_LWO_TRAN:
			{
				// transparency explicitly disabled?
				if (surf.mTransparency == 10e10f)
					break;

				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,TRAN,4);
				surf.mTransparency = GetF4();
				break;
			}
			// additive transparency
		case AI_LWO_ADTR:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,ADTR,4);
				surf.mAdditiveTransparency = GetF4();
				break;
			}
			// wireframe mode
		case AI_LWO_LINE:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,LINE,2);
				if (GetU2() & 0x1)
					surf.mWireframe = true;
				break;
			}
			// glossiness
		case AI_LWO_GLOS:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,GLOS,4);
				surf.mGlossiness = GetF4();
				break;
			}
			// bump intensity
		case AI_LWO_BUMP:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,BUMP,4);
				surf.mBumpIntensity = GetF4();
				break;
			}
			// color highlights
		case AI_LWO_CLRH:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,CLRH,4);
				surf.mColorHighlights = GetF4();
				break;
			}
			// index of refraction
		case AI_LWO_RIND:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,RIND,4);
				surf.mIOR = GetF4();
				break;
			}
			// polygon sidedness
		case AI_LWO_SIDE:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,SIDE,2);
				surf.bDoubleSided = (3 == GetU2());
				break;
			}
			// maximum smoothing angle
		case AI_LWO_SMAN:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,SMAN,4);
				surf.mMaximumSmoothAngle = fabs( GetF4() );
				break;
			}
			// vertex color channel to be applied to the surface
		case AI_LWO_VCOL:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,VCOL,12);
				surf.mDiffuseValue *= GetF4();				// strength
				ReadVSizedIntLWO2(mFileBuffer);             // skip envelope
				surf.mVCMapType = GetU4();					// type of the channel

				// name of the channel
				GetS0(surf.mVCMap, (unsigned int) (next - mFileBuffer ));
				break;
			}
			// surface bock entry
		case AI_LWO_BLOK:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,BLOK,4);
				LE_NCONST IFF::SubChunkHeader* head2 = IFF::LoadSubChunk(mFileBuffer);

				switch (head2->type)
				{
				case AI_LWO_PROC:
				case AI_LWO_GRAD:
				case AI_LWO_IMAP:
					LoadLWO2TextureBlock(head2, head->length);
					break;
				case AI_LWO_SHDR:
					LoadLWO2ShaderBlock(head2, head->length);
					break;

				default:
					DefaultLogger::get()->warn("LWO2: Found an unsupported surface BLOK");
				};

				break;
			}
		}
		mFileBuffer = next;
	}
}

#endif // !! ASSIMP_BUILD_NO_X_IMPORTER
