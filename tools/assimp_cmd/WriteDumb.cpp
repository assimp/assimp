/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team



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

/** @file  WriteTextDumb.cpp
 *  @brief Implementation of the 'assimp dump' utility
 */

#include "Main.h"
#include "PostProcessing/ProcessHelper.h"

const char* AICMD_MSG_DUMP_HELP = 
"assimp dump <model> [<out>] [-b] [-s] [-z] [common parameters]\n"
"\t -b Binary output \n"
"\t -s Shortened  \n"
"\t -z Compressed  \n"
"\t[See the assimp_cmd docs for a full list of all common parameters]  \n"
"\t -cfast    Fast post processing preset, runs just a few important steps \n"
"\t -cdefault Default post processing: runs all recommended steps\n"
"\t -cfull    Fires almost all post processing steps \n"
;

#include "Common/assbin_chunks.h"
#include <assimp/DefaultIOSystem.h>
#include <code/Assbin/AssbinFileWriter.h>

#include <memory>

FILE* out = NULL;
bool shortened = false;

// -----------------------------------------------------------------------------------
// Convert a name to standard XML format
void ConvertName(aiString& out, const aiString& in)
{
	out.length = 0;
	for (unsigned int i = 0; i < in.length; ++i)  {
		switch (in.data[i]) {
			case '<':
				out.Append("&lt;");break;
			case '>':
				out.Append("&gt;");break;
			case '&':
				out.Append("&amp;");break;
			case '\"':
				out.Append("&quot;");break;
			case '\'':
				out.Append("&apos;");break;
			default:
				out.data[out.length++] = in.data[i];
		}
	}
	out.data[out.length] = 0;
}

// -----------------------------------------------------------------------------------
// Write a single node as text dump
void WriteNode(const aiNode* node, FILE* out, unsigned int depth)
{
	char prefix[512];
	for (unsigned int i = 0; i < depth;++i)
		prefix[i] = '\t';
	prefix[depth] = '\0';

	const aiMatrix4x4& m = node->mTransformation;

	aiString name;
	ConvertName(name,node->mName);
	fprintf(out,"%s<Node name=\"%s\"> \n"
		"%s\t<Matrix4> \n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t</Matrix4> \n",
		prefix,name.data,prefix,
		prefix,m.a1,m.a2,m.a3,m.a4,
		prefix,m.b1,m.b2,m.b3,m.b4,
		prefix,m.c1,m.c2,m.c3,m.c4,
		prefix,m.d1,m.d2,m.d3,m.d4,prefix);

	if (node->mNumMeshes) {
		fprintf(out, "%s\t<MeshRefs num=\"%u\">\n%s\t",
			prefix,node->mNumMeshes,prefix);

		for (unsigned int i = 0; i < node->mNumMeshes;++i) {
			fprintf(out,"%u ",node->mMeshes[i]);
		}
		fprintf(out,"\n%s\t</MeshRefs>\n",prefix);
	}

	if (node->mNumChildren) {
		fprintf(out,"%s\t<NodeList num=\"%u\">\n",
			prefix,node->mNumChildren);

		for (unsigned int i = 0; i < node->mNumChildren;++i) {
			WriteNode(node->mChildren[i],out,depth+2);
		}
		fprintf(out,"%s\t</NodeList>\n",prefix);
	}
	fprintf(out,"%s</Node>\n",prefix);
}


// -------------------------------------------------------------------------------
const char* TextureTypeToString(aiTextureType in)
{
	switch (in)
	{
	case aiTextureType_NONE:
		return "n/a";
	case aiTextureType_DIFFUSE:
		return "Diffuse";
	case aiTextureType_SPECULAR:
		return "Specular";
	case aiTextureType_AMBIENT:
		return "Ambient";
	case aiTextureType_EMISSIVE:
		return "Emissive";
	case aiTextureType_OPACITY:
		return "Opacity";
	case aiTextureType_NORMALS:
		return "Normals";
	case aiTextureType_HEIGHT:
		return "Height";
	case aiTextureType_SHININESS:
		return "Shininess";
	case aiTextureType_DISPLACEMENT:
		return "Displacement";
	case aiTextureType_LIGHTMAP:
		return "Lightmap";
	case aiTextureType_REFLECTION:
		return "Reflection";
	case aiTextureType_UNKNOWN:
		return "Unknown";
	default:
		break;
	}
	ai_assert(false); 
	return  "BUG";    
}


// -----------------------------------------------------------------------------------
// Some chuncks of text will need to be encoded for XML
// http://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string#5665377
static std::string encodeXML(const std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    return buffer;
}



// -----------------------------------------------------------------------------------
// Write a text model dump
void WriteDump(const aiScene* scene, FILE* out, const char* src, const char* cmd, bool shortened)
{
	time_t tt = ::time(NULL);
#if _WIN32
    tm* p = gmtime(&tt);
#else
    struct tm now;
    tm* p = gmtime_r(&tt, &now);
#endif
    ai_assert(nullptr != p);

	std::string c = cmd;
	std::string::size_type s; 

	// https://sourceforge.net/tracker/?func=detail&aid=3167364&group_id=226462&atid=1067632
	// -- not allowed in XML comments
	while((s = c.find("--")) != std::string::npos) {
		c[s] = '?';
	}
	aiString name;

	// write header
	fprintf(out,
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<ASSIMP format_id=\"1\">\n\n"

		"<!-- XML Model dump produced by assimp dump\n"
		"  Library version: %u.%u.%u\n"
		"  Source: %s\n"
		"  Command line: %s\n"
		"  %s\n"
		"-->"
		" \n\n"
		"<Scene flags=\"%u\" postprocessing=\"%i\">\n",
		
		aiGetVersionMajor(),aiGetVersionMinor(),aiGetVersionRevision(),src,c.c_str(),asctime(p),
		scene->mFlags,
		0 /*globalImporter->GetEffectivePostProcessing()*/);

	// write the node graph
	WriteNode(scene->mRootNode, out, 0);

#if 0
		// write cameras
	for (unsigned int i = 0; i < scene->mNumCameras;++i) {
		aiCamera* cam  = scene->mCameras[i];
		ConvertName(name,cam->mName);

		// camera header
		fprintf(out,"\t<Camera parent=\"%s\">\n"
			"\t\t<Vector3 name=\"up\"        > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"lookat\"    > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"pos\"       > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Float   name=\"fov\"       > %f </Float>\n"
			"\t\t<Float   name=\"aspect\"    > %f </Float>\n"
			"\t\t<Float   name=\"near_clip\" > %f </Float>\n"
			"\t\t<Float   name=\"far_clip\"  > %f </Float>\n"
			"\t</Camera>\n",
			name.data,
			cam->mUp.x,cam->mUp.y,cam->mUp.z,
			cam->mLookAt.x,cam->mLookAt.y,cam->mLookAt.z,
			cam->mPosition.x,cam->mPosition.y,cam->mPosition.z,
			cam->mHorizontalFOV,cam->mAspect,cam->mClipPlaneNear,cam->mClipPlaneFar,i);
	}

	// write lights
	for (unsigned int i = 0; i < scene->mNumLights;++i) {
		aiLight* l  = scene->mLights[i];
		ConvertName(name,l->mName);

		// light header
		fprintf(out,"\t<Light parent=\"%s\"> type=\"%s\"\n"
			"\t\t<Vector3 name=\"diffuse\"   > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"specular\"  > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"ambient\"   > %0 8f %0 8f %0 8f </Vector3>\n",
			name.data,
			(l->mType == aiLightSource_DIRECTIONAL ? "directional" :
			(l->mType == aiLightSource_POINT ? "point" : "spot" )),
			l->mColorDiffuse.r, l->mColorDiffuse.g, l->mColorDiffuse.b,
			l->mColorSpecular.r,l->mColorSpecular.g,l->mColorSpecular.b,
			l->mColorAmbient.r, l->mColorAmbient.g, l->mColorAmbient.b);

		if (l->mType != aiLightSource_DIRECTIONAL) {
			fprintf(out,
				"\t\t<Vector3 name=\"pos\"       > %0 8f %0 8f %0 8f </Vector3>\n"
				"\t\t<Float   name=\"atten_cst\" > %f </Float>\n"
				"\t\t<Float   name=\"atten_lin\" > %f </Float>\n"
				"\t\t<Float   name=\"atten_sqr\" > %f </Float>\n",
				l->mPosition.x,l->mPosition.y,l->mPosition.z,
				l->mAttenuationConstant,l->mAttenuationLinear,l->mAttenuationQuadratic);
		}

		if (l->mType != aiLightSource_POINT) {
			fprintf(out,
				"\t\t<Vector3 name=\"lookat\"    > %0 8f %0 8f %0 8f </Vector3>\n",
				l->mDirection.x,l->mDirection.y,l->mDirection.z);
		}

		if (l->mType == aiLightSource_SPOT) {
			fprintf(out,
				"\t\t<Float   name=\"cone_out\" > %f </Float>\n"
				"\t\t<Float   name=\"cone_inn\" > %f </Float>\n",
				l->mAngleOuterCone,l->mAngleInnerCone);
		}
		fprintf(out,"\t</Light>\n");
	}
#endif

	// write textures
	if (scene->mNumTextures) {
		fprintf(out,"<TextureList num=\"%u\">\n",scene->mNumTextures);
		for (unsigned int i = 0; i < scene->mNumTextures;++i) {
			aiTexture* tex  = scene->mTextures[i];
			bool compressed = (tex->mHeight == 0);

			// mesh header
			fprintf(out,"\t<Texture width=\"%i\" height=\"%i\" compressed=\"%s\"> \n",
				(compressed ? -1 : tex->mWidth),(compressed ? -1 : tex->mHeight),
				(compressed ? "true" : "false"));

			if (compressed) {
				fprintf(out,"\t\t<Data length=\"%u\"> \n",tex->mWidth);

				if (!shortened) {
					for (unsigned int n = 0; n < tex->mWidth;++n) {
						fprintf(out,"\t\t\t%2x",reinterpret_cast<uint8_t*>(tex->pcData)[n]);
						if (n && !(n % 50)) {
							fprintf(out,"\n");
						}
					}
				}
			}
			else if (!shortened){
				fprintf(out,"\t\t<Data length=\"%i\"> \n",tex->mWidth*tex->mHeight*4);

				// const unsigned int width = (unsigned int)log10((double)std::max(tex->mHeight,tex->mWidth))+1;
				for (unsigned int y = 0; y < tex->mHeight;++y) {
					for (unsigned int x = 0; x < tex->mWidth;++x) {
						aiTexel* tx = tex->pcData + y*tex->mWidth+x;
						unsigned int r = tx->r,g=tx->g,b=tx->b,a=tx->a;
						fprintf(out,"\t\t\t%2x %2x %2x %2x",r,g,b,a);

						// group by four for readibility
						if (0 == (x+y*tex->mWidth) % 4)
							fprintf(out,"\n");
					}
				}
			}
			fprintf(out,"\t\t</Data>\n\t</Texture>\n");
		}
		fprintf(out,"</TextureList>\n");
	}

	// write materials
	if (scene->mNumMaterials) {
		fprintf(out,"<MaterialList num=\"%u\">\n",scene->mNumMaterials);
		for (unsigned int i = 0; i< scene->mNumMaterials; ++i) {
			const aiMaterial* mat = scene->mMaterials[i];

			fprintf(out,"\t<Material>\n");
			fprintf(out,"\t\t<MatPropertyList  num=\"%u\">\n",mat->mNumProperties);
			for (unsigned int n = 0; n < mat->mNumProperties;++n) {

				const aiMaterialProperty* prop = mat->mProperties[n];
				const char* sz = "";
				if (prop->mType == aiPTI_Float) {
					sz = "float";
				}
				else if (prop->mType == aiPTI_Integer) {
					sz = "integer";
				}
				else if (prop->mType == aiPTI_String) {
					sz = "string";
				}
				else if (prop->mType == aiPTI_Buffer) {
					sz = "binary_buffer";
				}

				fprintf(out,"\t\t\t<MatProperty key=\"%s\" \n\t\t\ttype=\"%s\" tex_usage=\"%s\" tex_index=\"%u\"",
					prop->mKey.data, sz,
					::TextureTypeToString((aiTextureType)prop->mSemantic),prop->mIndex);

				if (prop->mType == aiPTI_Float) {
					fprintf(out," size=\"%i\">\n\t\t\t\t",
						static_cast<int>(prop->mDataLength/sizeof(float)));

					for (unsigned int p = 0; p < prop->mDataLength/sizeof(float);++p) {
						fprintf(out,"%f ",*((float*)(prop->mData+p*sizeof(float))));
					}
				}
				else if (prop->mType == aiPTI_Integer) {
					fprintf(out," size=\"%i\">\n\t\t\t\t",
						static_cast<int>(prop->mDataLength/sizeof(int)));

					for (unsigned int p = 0; p < prop->mDataLength/sizeof(int);++p) {
						fprintf(out,"%i ",*((int*)(prop->mData+p*sizeof(int))));
					}
				}
				else if (prop->mType == aiPTI_Buffer) {
					fprintf(out," size=\"%i\">\n\t\t\t\t",
						static_cast<int>(prop->mDataLength));

					for (unsigned int p = 0; p < prop->mDataLength;++p) {
						fprintf(out,"%2x ",prop->mData[p]);
						if (p && 0 == p%30) {
							fprintf(out,"\n\t\t\t\t");
						}
					}
				}
				else if (prop->mType == aiPTI_String) {
					fprintf(out,">\n\t\t\t\t\"%s\"",encodeXML(prop->mData+4).c_str() /* skip length */);
				}
				fprintf(out,"\n\t\t\t</MatProperty>\n");
			}
			fprintf(out,"\t\t</MatPropertyList>\n");
			fprintf(out,"\t</Material>\n");
		}
		fprintf(out,"</MaterialList>\n");
	}

	// write animations
	if (scene->mNumAnimations) {
		fprintf(out,"<AnimationList num=\"%u\">\n",scene->mNumAnimations);
		for (unsigned int i = 0; i < scene->mNumAnimations;++i) {
			aiAnimation* anim = scene->mAnimations[i];

			// anim header
			ConvertName(name,anim->mName);
			fprintf(out,"\t<Animation name=\"%s\" duration=\"%e\" tick_cnt=\"%e\">\n",
				name.data, anim->mDuration, anim->mTicksPerSecond);

			// write bone animation channels
			if (anim->mNumChannels) {
				fprintf(out,"\t\t<NodeAnimList num=\"%u\">\n",anim->mNumChannels);
				for (unsigned int n = 0; n < anim->mNumChannels;++n) {
					aiNodeAnim* nd = anim->mChannels[n];

					// node anim header
					ConvertName(name,nd->mNodeName);
					fprintf(out,"\t\t\t<NodeAnim node=\"%s\">\n",name.data);

					if (!shortened) {
						// write position keys
						if (nd->mNumPositionKeys) {
							fprintf(out,"\t\t\t\t<PositionKeyList num=\"%u\">\n",nd->mNumPositionKeys);
							for (unsigned int a = 0; a < nd->mNumPositionKeys;++a) {
								aiVectorKey* vc = nd->mPositionKeys+a;
								fprintf(out,"\t\t\t\t\t<PositionKey time=\"%e\">\n"
									"\t\t\t\t\t\t%0 8f %0 8f %0 8f\n\t\t\t\t\t</PositionKey>\n",
									vc->mTime,vc->mValue.x,vc->mValue.y,vc->mValue.z);
							}
							fprintf(out,"\t\t\t\t</PositionKeyList>\n");
						}

						// write scaling keys
						if (nd->mNumScalingKeys) {
							fprintf(out,"\t\t\t\t<ScalingKeyList num=\"%u\">\n",nd->mNumScalingKeys);
							for (unsigned int a = 0; a < nd->mNumScalingKeys;++a) {
								aiVectorKey* vc = nd->mScalingKeys+a;
								fprintf(out,"\t\t\t\t\t<ScalingKey time=\"%e\">\n"
									"\t\t\t\t\t\t%0 8f %0 8f %0 8f\n\t\t\t\t\t</ScalingKey>\n",
									vc->mTime,vc->mValue.x,vc->mValue.y,vc->mValue.z);
							}
							fprintf(out,"\t\t\t\t</ScalingKeyList>\n");
						}

						// write rotation keys
						if (nd->mNumRotationKeys) {
							fprintf(out,"\t\t\t\t<RotationKeyList num=\"%u\">\n",nd->mNumRotationKeys);
							for (unsigned int a = 0; a < nd->mNumRotationKeys;++a) {
								aiQuatKey* vc = nd->mRotationKeys+a;
								fprintf(out,"\t\t\t\t\t<RotationKey time=\"%e\">\n"
									"\t\t\t\t\t\t%0 8f %0 8f %0 8f %0 8f\n\t\t\t\t\t</RotationKey>\n",
									vc->mTime,vc->mValue.x,vc->mValue.y,vc->mValue.z,vc->mValue.w);
							}
							fprintf(out,"\t\t\t\t</RotationKeyList>\n");
						}
					}
					fprintf(out,"\t\t\t</NodeAnim>\n");
				}
				fprintf(out,"\t\t</NodeAnimList>\n");
			}
			fprintf(out,"\t</Animation>\n");
		}
		fprintf(out,"</AnimationList>\n");
	}

	// write meshes
	if (scene->mNumMeshes) {
		fprintf(out,"<MeshList num=\"%u\">\n",scene->mNumMeshes);
		for (unsigned int i = 0; i < scene->mNumMeshes;++i) {
			aiMesh* mesh = scene->mMeshes[i];
			// const unsigned int width = (unsigned int)log10((double)mesh->mNumVertices)+1;

			// mesh header
			fprintf(out,"\t<Mesh types=\"%s %s %s %s\" material_index=\"%u\">\n",
				(mesh->mPrimitiveTypes & aiPrimitiveType_POINT    ? "points"    : ""),
				(mesh->mPrimitiveTypes & aiPrimitiveType_LINE     ? "lines"     : ""),
				(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE ? "triangles" : ""),
				(mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON  ? "polygons"  : ""),
				mesh->mMaterialIndex);

			// bones
			if (mesh->mNumBones) {
				fprintf(out,"\t\t<BoneList num=\"%u\">\n",mesh->mNumBones);

				for (unsigned int n = 0; n < mesh->mNumBones;++n) {
					aiBone* bone = mesh->mBones[n];

					ConvertName(name,bone->mName);
					// bone header
					fprintf(out,"\t\t\t<Bone name=\"%s\">\n"
						"\t\t\t\t<Matrix4> \n"
						"\t\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
						"\t\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
						"\t\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
						"\t\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
						"\t\t\t\t</Matrix4> \n",
						name.data,
						bone->mOffsetMatrix.a1,bone->mOffsetMatrix.a2,bone->mOffsetMatrix.a3,bone->mOffsetMatrix.a4,
						bone->mOffsetMatrix.b1,bone->mOffsetMatrix.b2,bone->mOffsetMatrix.b3,bone->mOffsetMatrix.b4,
						bone->mOffsetMatrix.c1,bone->mOffsetMatrix.c2,bone->mOffsetMatrix.c3,bone->mOffsetMatrix.c4,
						bone->mOffsetMatrix.d1,bone->mOffsetMatrix.d2,bone->mOffsetMatrix.d3,bone->mOffsetMatrix.d4);

					if (!shortened && bone->mNumWeights) {
						fprintf(out,"\t\t\t\t<WeightList num=\"%u\">\n",bone->mNumWeights);

						// bone weights
						for (unsigned int a = 0; a < bone->mNumWeights;++a) {
							aiVertexWeight* wght = bone->mWeights+a;

							fprintf(out,"\t\t\t\t\t<Weight index=\"%u\">\n\t\t\t\t\t\t%f\n\t\t\t\t\t</Weight>\n",
								wght->mVertexId,wght->mWeight);
						}
						fprintf(out,"\t\t\t\t</WeightList>\n");
					}
					fprintf(out,"\t\t\t</Bone>\n");
				}
				fprintf(out,"\t\t</BoneList>\n");
			}

			// faces
			if (!shortened && mesh->mNumFaces) {
				fprintf(out,"\t\t<FaceList num=\"%u\">\n",mesh->mNumFaces);
				for (unsigned int n = 0; n < mesh->mNumFaces; ++n) {
					aiFace& f = mesh->mFaces[n];
					fprintf(out,"\t\t\t<Face num=\"%u\">\n"
						"\t\t\t\t",f.mNumIndices);

					for (unsigned int j = 0; j < f.mNumIndices;++j)
						fprintf(out,"%u ",f.mIndices[j]);

					fprintf(out,"\n\t\t\t</Face>\n");
				}
				fprintf(out,"\t\t</FaceList>\n");
			}

			// vertex positions
			if (mesh->HasPositions()) {
				fprintf(out,"\t\t<Positions num=\"%u\" set=\"0\" num_components=\"3\"> \n",mesh->mNumVertices);
				if (!shortened) {
					for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
						fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
							mesh->mVertices[n].x,
							mesh->mVertices[n].y,
							mesh->mVertices[n].z);
					}
				}
				fprintf(out,"\t\t</Positions>\n");
			}

			// vertex normals
			if (mesh->HasNormals()) {
				fprintf(out,"\t\t<Normals num=\"%u\" set=\"0\" num_components=\"3\"> \n",mesh->mNumVertices);
				if (!shortened) {
					for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
						fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
							mesh->mNormals[n].x,
							mesh->mNormals[n].y,
							mesh->mNormals[n].z);
					}
				}
				else {
				}
				fprintf(out,"\t\t</Normals>\n");
			}

			// vertex tangents and bitangents
			if (mesh->HasTangentsAndBitangents()) {
				fprintf(out,"\t\t<Tangents num=\"%u\" set=\"0\" num_components=\"3\"> \n",mesh->mNumVertices);
				if (!shortened) {
					for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
						fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
							mesh->mTangents[n].x,
							mesh->mTangents[n].y,
							mesh->mTangents[n].z);
					}
				}
				fprintf(out,"\t\t</Tangents>\n");

				fprintf(out,"\t\t<Bitangents num=\"%u\" set=\"0\" num_components=\"3\"> \n",mesh->mNumVertices);
				if (!shortened) {
					for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
						fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
							mesh->mBitangents[n].x,
							mesh->mBitangents[n].y,
							mesh->mBitangents[n].z);
					}
				}
				fprintf(out,"\t\t</Bitangents>\n");
			}

			// texture coordinates
			for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
				if (!mesh->mTextureCoords[a])
					break;

				fprintf(out,"\t\t<TextureCoords num=\"%u\" set=\"%u\" num_components=\"%u\"> \n",mesh->mNumVertices,
					a,mesh->mNumUVComponents[a]);
				
				if (!shortened) {
					if (mesh->mNumUVComponents[a] == 3) {
						for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
							fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
								mesh->mTextureCoords[a][n].x,
								mesh->mTextureCoords[a][n].y,
								mesh->mTextureCoords[a][n].z);
						}
					}
					else {
						for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
							fprintf(out,"\t\t%0 8f %0 8f\n",
								mesh->mTextureCoords[a][n].x,
								mesh->mTextureCoords[a][n].y);
						}
					}
				}
				fprintf(out,"\t\t</TextureCoords>\n");
			}

			// vertex colors
			for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a) {
				if (!mesh->mColors[a])
					break;
				fprintf(out,"\t\t<Colors num=\"%u\" set=\"%u\" num_components=\"4\"> \n",mesh->mNumVertices,a);
				if (!shortened) {
					for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
						fprintf(out,"\t\t%0 8f %0 8f %0 8f %0 8f\n",
							mesh->mColors[a][n].r,
							mesh->mColors[a][n].g,
							mesh->mColors[a][n].b,
							mesh->mColors[a][n].a);
					}
				}
				fprintf(out,"\t\t</Colors>\n");
			}
			fprintf(out,"\t</Mesh>\n");
		}
		fprintf(out,"</MeshList>\n");
	}
	fprintf(out,"</Scene>\n</ASSIMP>");
}


// -----------------------------------------------------------------------------------
int Assimp_Dump (const char* const* params, unsigned int num)
{
	const char* fail = "assimp dump: Invalid number of arguments. "
			"See \'assimp dump --help\'\r\n";

	// --help
	if (!strcmp( params[0], "-h") || !strcmp( params[0], "--help") || !strcmp( params[0], "-?") ) {
		printf("%s",AICMD_MSG_DUMP_HELP);
		return AssimpCmdError::Success;
	}

	// asssimp dump in out [options]
	if (num < 1) {
		printf("%s", fail);
		return AssimpCmdError::InvalidNumberOfArguments;
	}

	std::string in  = std::string(params[0]);
	std::string out = (num > 1 ? std::string(params[1]) : std::string("-"));

	// store full command line
	std::string cmd;
	for (unsigned int i = (out[0] == '-' ? 1 : 2); i < num;++i)	{
		if (!params[i])continue;
		cmd.append(params[i]);
		cmd.append(" ");
	}

	// get import flags
	ImportData import;
	ProcessStandardArguments(import,params+1,num-1);

	bool binary = false, shortened = false,compressed=false;
	
	// process other flags
	for (unsigned int i = 1; i < num;++i)		{
		if (!params[i])continue;
		if (!strcmp( params[i], "-b") || !strcmp( params[i], "--binary")) {
			binary = true;
		}
		else if (!strcmp( params[i], "-s") || !strcmp( params[i], "--short")) {
			shortened = true;
		}
		else if (!strcmp( params[i], "-z") || !strcmp( params[i], "--compressed")) {
			compressed = true;
		}
#if 0
		else if (i > 2 || params[i][0] == '-') {
			::printf("Unknown parameter: %s\n",params[i]);
			return 10;
		}
#endif
	}

	if (out[0] == '-') {
		// take file name from input file
		std::string::size_type s = in.find_last_of('.');
		if (s == std::string::npos) {
			s = in.length();
		}

		out = in.substr(0,s);
		out.append((binary ? ".assbin" : ".assxml"));
		if (shortened && binary) {
			out.append(".regress");
		}
	}

	// import the main model
	const aiScene* scene = ImportModel(import,in);
	if (!scene) {
		printf("assimp dump: Unable to load input file %s\n",in.c_str());
		return AssimpCmdError::FailedToLoadInputFile;
	}

	if (binary) {
		try {
			std::unique_ptr<IOSystem> pIOSystem(new DefaultIOSystem());
			DumpSceneToAssbin(out.c_str(), cmd.c_str(), pIOSystem.get(),
				scene, shortened, compressed);
		}
		catch (const std::exception& e) {
			printf("%s", ("assimp dump: " + std::string(e.what())).c_str());
			return AssimpCmdError::ExceptionWasRaised;
		}
		catch (...) {
			printf("assimp dump: An unknown exception occured.\n");
			return AssimpCmdError::ExceptionWasRaised;
		}
	}
	else {
		FILE* o = ::fopen(out.c_str(), "wt");
		if (!o) {
			printf("assimp dump: Unable to open output file %s\n",out.c_str());
			return AssimpCmdError::FailedToOpenOutputFile;
		}
		WriteDump (scene,o,in.c_str(),cmd.c_str(),shortened);
		fclose(o);
	}

	printf("assimp dump: Wrote output dump %s\n",out.c_str());
	return AssimpCmdError::Success;
}

