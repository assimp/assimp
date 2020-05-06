/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team
Copyright (c) 2019 bzt

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

/** @file M3DMaterials.h
*   @brief Declares the Assimp and Model 3D file material type relations
*/
#ifndef AI_M3DMATERIALS_H_INC
#define AI_M3DMATERIALS_H_INC

/*
 * In the m3d.h header, there's a static array which defines the material
 * properties, called m3d_propertytypes. These must have the same size, and
 * list the matching Assimp materials for those properties. Used by both the
 * M3DImporter and the M3DExporter, so you have to define these relations
 * only once. D.R.Y. and K.I.S.S.
 */
typedef struct {
    const char *pKey;
    unsigned int type;
    unsigned int index;
} aiMatProp;

/* --- Scalar Properties ---        !!!!! must match m3d_propertytypes !!!!! */
static const aiMatProp aiProps[] = {
    { AI_MATKEY_COLOR_DIFFUSE },                                /* m3dp_Kd */
    { AI_MATKEY_COLOR_AMBIENT },                                /* m3dp_Ka */
    { AI_MATKEY_COLOR_SPECULAR },                               /* m3dp_Ks */
    { AI_MATKEY_SHININESS },                                    /* m3dp_Ns */
    { AI_MATKEY_COLOR_EMISSIVE },                               /* m3dp_Ke */
    { AI_MATKEY_COLOR_REFLECTIVE },                             /* m3dp_Tf */
    { AI_MATKEY_BUMPSCALING },                                  /* m3dp_Km */
    { AI_MATKEY_OPACITY },                                      /* m3dp_d */
    { AI_MATKEY_SHADING_MODEL },                                /* m3dp_il */

    { NULL, 0, 0 },                                             /* m3dp_Pr */
    { AI_MATKEY_REFLECTIVITY },                                 /* m3dp_Pm */
    { NULL, 0, 0 },                                             /* m3dp_Ps */
    { AI_MATKEY_REFRACTI },                                     /* m3dp_Ni */
    { NULL, 0, 0 },                                             /* m3dp_Nt */
    { NULL, 0, 0 },
    { NULL, 0, 0 },
    { NULL, 0, 0 }
};

/* --- Texture Map Properties ---   !!!!! must match m3d_propertytypes !!!!! */
static const aiMatProp aiTxProps[] = {
    { AI_MATKEY_TEXTURE_DIFFUSE(0) },                        /* m3dp_map_Kd */
    { AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION,0) },/* m3dp_map_Ka */
    { AI_MATKEY_TEXTURE_SPECULAR(0) },                       /* m3dp_map_Ks */
    { AI_MATKEY_TEXTURE_SHININESS(0) },                      /* m3dp_map_Ns */
    { AI_MATKEY_TEXTURE_EMISSIVE(0) },                       /* m3dp_map_Ke */
    { NULL, 0, 0 },                                          /* m3dp_map_Tf */
    { AI_MATKEY_TEXTURE_HEIGHT(0) },                         /* m3dp_bump */
    { AI_MATKEY_TEXTURE_OPACITY(0) },                        /* m3dp_map_d */
    { AI_MATKEY_TEXTURE_NORMALS(0) },                        /* m3dp_map_N */

    { AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS,0) },/* m3dp_map_Pr */
    { AI_MATKEY_TEXTURE(aiTextureType_METALNESS,0) },        /* m3dp_map_Pm */
    { NULL, 0, 0 },                                          /* m3dp_map_Ps */
    { AI_MATKEY_TEXTURE(aiTextureType_REFLECTION,0) },       /* m3dp_map_Ni */
    { NULL, 0, 0 },                                          /* m3dp_map_Nt */
    { NULL, 0, 0 },
    { NULL, 0, 0 },
    { NULL, 0, 0 }
};

#endif // AI_M3DMATERIALS_H_INC
