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
/// \file   X3DImporter_Postprocess.cpp
/// \brief  Convert built scenegraph and objects to Assimp scenegraph.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DGeoHelper.h"
#include "X3DImporter.hpp"

// Header files, Assimp.
#include <assimp/StandardShapes.h>
#include <assimp/StringUtils.h>
#include <assimp/ai_assert.h>

// Header files, stdlib.
#include <algorithm>
#include <iterator>
#include <string>

namespace Assimp {

aiMatrix4x4 X3DImporter::PostprocessHelper_Matrix_GlobalToCurrent() const {
    X3DNodeElementBase *cur_node;
    std::list<aiMatrix4x4> matr;
    aiMatrix4x4 out_matr;

    // starting walk from current element to root
    cur_node = mNodeElementCur;
    if (cur_node != nullptr) {
        do {
            // if cur_node is group then store group transformation matrix in list.
            if (cur_node->Type == X3DElemType::ENET_Group) matr.push_back(((X3DNodeElementGroup *)cur_node)->Transformation);

            cur_node = cur_node->Parent;
        } while (cur_node != nullptr);
    }

    // multiplicate all matrices in reverse order
    for (std::list<aiMatrix4x4>::reverse_iterator rit = matr.rbegin(); rit != matr.rend(); ++rit)
        out_matr = out_matr * (*rit);

    return out_matr;
}

void X3DImporter::PostprocessHelper_CollectMetadata(const X3DNodeElementBase &pNodeElement, std::list<X3DNodeElementBase *> &pList) const {
    // walk through childs and find for metadata.
    for (std::list<X3DNodeElementBase *>::const_iterator el_it = pNodeElement.Children.begin(); el_it != pNodeElement.Children.end(); ++el_it) {
        if (((*el_it)->Type == X3DElemType::ENET_MetaBoolean) || ((*el_it)->Type == X3DElemType::ENET_MetaDouble) ||
                ((*el_it)->Type == X3DElemType::ENET_MetaFloat) || ((*el_it)->Type == X3DElemType::ENET_MetaInteger) ||
                ((*el_it)->Type == X3DElemType::ENET_MetaString)) {
            pList.push_back(*el_it);
        } else if ((*el_it)->Type == X3DElemType::ENET_MetaSet) {
            PostprocessHelper_CollectMetadata(**el_it, pList);
        }
    } // for(std::list<X3DNodeElementBase*>::const_iterator el_it = pNodeElement.Children.begin(); el_it != pNodeElement.Children.end(); el_it++)
}

bool X3DImporter::PostprocessHelper_ElementIsMetadata(const X3DElemType pType) const {
    if ((pType == X3DElemType::ENET_MetaBoolean) || (pType == X3DElemType::ENET_MetaDouble) ||
            (pType == X3DElemType::ENET_MetaFloat) || (pType == X3DElemType::ENET_MetaInteger) ||
            (pType == X3DElemType::ENET_MetaString) || (pType == X3DElemType::ENET_MetaSet)) {
        return true;
    } else {
        return false;
    }
}

bool X3DImporter::PostprocessHelper_ElementIsMesh(const X3DElemType pType) const {
    if ((pType == X3DElemType::ENET_Arc2D) || (pType == X3DElemType::ENET_ArcClose2D) ||
            (pType == X3DElemType::ENET_Box) || (pType == X3DElemType::ENET_Circle2D) ||
            (pType == X3DElemType::ENET_Cone) || (pType == X3DElemType::ENET_Cylinder) ||
            (pType == X3DElemType::ENET_Disk2D) || (pType == X3DElemType::ENET_ElevationGrid) ||
            (pType == X3DElemType::ENET_Extrusion) || (pType == X3DElemType::ENET_IndexedFaceSet) ||
            (pType == X3DElemType::ENET_IndexedLineSet) || (pType == X3DElemType::ENET_IndexedTriangleFanSet) ||
            (pType == X3DElemType::ENET_IndexedTriangleSet) || (pType == X3DElemType::ENET_IndexedTriangleStripSet) ||
            (pType == X3DElemType::ENET_PointSet) || (pType == X3DElemType::ENET_LineSet) ||
            (pType == X3DElemType::ENET_Polyline2D) || (pType == X3DElemType::ENET_Polypoint2D) ||
            (pType == X3DElemType::ENET_Rectangle2D) || (pType == X3DElemType::ENET_Sphere) ||
            (pType == X3DElemType::ENET_TriangleFanSet) || (pType == X3DElemType::ENET_TriangleSet) ||
            (pType == X3DElemType::ENET_TriangleSet2D) || (pType == X3DElemType::ENET_TriangleStripSet)) {
        return true;
    } else {
        return false;
    }
}

void X3DImporter::Postprocess_BuildLight(const X3DNodeElementBase &pNodeElement, std::list<aiLight *> &pSceneLightList) const {
    const X3DNodeElementLight &ne = *((X3DNodeElementLight *)&pNodeElement);
    aiMatrix4x4 transform_matr = PostprocessHelper_Matrix_GlobalToCurrent();
    aiLight *new_light = new aiLight;

    new_light->mName = ne.ID;
    new_light->mColorAmbient = ne.Color * ne.AmbientIntensity;
    new_light->mColorDiffuse = ne.Color * ne.Intensity;
    new_light->mColorSpecular = ne.Color * ne.Intensity;
    switch (pNodeElement.Type) {
    case X3DElemType::ENET_DirectionalLight:
        new_light->mType = aiLightSource_DIRECTIONAL;
        new_light->mDirection = ne.Direction, new_light->mDirection *= transform_matr;

        break;
    case X3DElemType::ENET_PointLight:
        new_light->mType = aiLightSource_POINT;
        new_light->mPosition = ne.Location, new_light->mPosition *= transform_matr;
        new_light->mAttenuationConstant = ne.Attenuation.x;
        new_light->mAttenuationLinear = ne.Attenuation.y;
        new_light->mAttenuationQuadratic = ne.Attenuation.z;

        break;
    case X3DElemType::ENET_SpotLight:
        new_light->mType = aiLightSource_SPOT;
        new_light->mPosition = ne.Location, new_light->mPosition *= transform_matr;
        new_light->mDirection = ne.Direction, new_light->mDirection *= transform_matr;
        new_light->mAttenuationConstant = ne.Attenuation.x;
        new_light->mAttenuationLinear = ne.Attenuation.y;
        new_light->mAttenuationQuadratic = ne.Attenuation.z;
        new_light->mAngleInnerCone = ne.BeamWidth;
        new_light->mAngleOuterCone = ne.CutOffAngle;

        break;
    default:
        throw DeadlyImportError("Postprocess_BuildLight. Unknown type of light: " + ai_to_string(pNodeElement.Type) + ".");
    }

    pSceneLightList.push_back(new_light);
}

void X3DImporter::Postprocess_BuildMaterial(const X3DNodeElementBase &pNodeElement, aiMaterial **pMaterial) const {
    // check argument
    if (pMaterial == nullptr) throw DeadlyImportError("Postprocess_BuildMaterial. pMaterial is nullptr.");
    if (*pMaterial != nullptr) throw DeadlyImportError("Postprocess_BuildMaterial. *pMaterial must be nullptr.");

    *pMaterial = new aiMaterial;
    aiMaterial &taimat = **pMaterial; // creating alias for convenience.

    // at this point pNodeElement point to <Appearance> node. Walk through childs and add all stored data.
    for (std::list<X3DNodeElementBase *>::const_iterator el_it = pNodeElement.Children.begin(); el_it != pNodeElement.Children.end(); ++el_it) {
        if ((*el_it)->Type == X3DElemType::ENET_Material) {
            aiColor3D tcol3;
            float tvalf;
            X3DNodeElementMaterial &tnemat = *((X3DNodeElementMaterial *)*el_it);

            tcol3.r = tnemat.AmbientIntensity, tcol3.g = tnemat.AmbientIntensity, tcol3.b = tnemat.AmbientIntensity;
            taimat.AddProperty(&tcol3, 1, AI_MATKEY_COLOR_AMBIENT);
            taimat.AddProperty(&tnemat.DiffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);
            taimat.AddProperty(&tnemat.EmissiveColor, 1, AI_MATKEY_COLOR_EMISSIVE);
            taimat.AddProperty(&tnemat.SpecularColor, 1, AI_MATKEY_COLOR_SPECULAR);
            tvalf = 1;
            taimat.AddProperty(&tvalf, 1, AI_MATKEY_SHININESS_STRENGTH);
            taimat.AddProperty(&tnemat.Shininess, 1, AI_MATKEY_SHININESS);
            tvalf = 1.0f - tnemat.Transparency;
            taimat.AddProperty(&tvalf, 1, AI_MATKEY_OPACITY);
        } // if((*el_it)->Type == X3DElemType::ENET_Material)
        else if ((*el_it)->Type == X3DElemType::ENET_ImageTexture) {
            X3DNodeElementImageTexture &tnetex = *((X3DNodeElementImageTexture *)*el_it);
            aiString url_str(tnetex.URL.c_str());
            int mode = aiTextureOp_Multiply;

            taimat.AddProperty(&url_str, AI_MATKEY_TEXTURE_DIFFUSE(0));
            taimat.AddProperty(&tnetex.RepeatS, 1, AI_MATKEY_MAPPINGMODE_U_DIFFUSE(0));
            taimat.AddProperty(&tnetex.RepeatT, 1, AI_MATKEY_MAPPINGMODE_V_DIFFUSE(0));
            taimat.AddProperty(&mode, 1, AI_MATKEY_TEXOP_DIFFUSE(0));
        } // else if((*el_it)->Type == X3DElemType::ENET_ImageTexture)
        else if ((*el_it)->Type == X3DElemType::ENET_TextureTransform) {
            aiUVTransform trans;
            X3DNodeElementTextureTransform &tnetextr = *((X3DNodeElementTextureTransform *)*el_it);

            trans.mTranslation = tnetextr.Translation - tnetextr.Center;
            trans.mScaling = tnetextr.Scale;
            trans.mRotation = tnetextr.Rotation;
            taimat.AddProperty(&trans, 1, AI_MATKEY_UVTRANSFORM_DIFFUSE(0));
        } // else if((*el_it)->Type == X3DElemType::ENET_TextureTransform)
    } // for(std::list<X3DNodeElementBase*>::const_iterator el_it = pNodeElement.Children.begin(); el_it != pNodeElement.Children.end(); el_it++)
}

void X3DImporter::Postprocess_BuildMesh(const X3DNodeElementBase &pNodeElement, aiMesh **pMesh) const {
    // check argument
    if (pMesh == nullptr) throw DeadlyImportError("Postprocess_BuildMesh. pMesh is nullptr.");
    if (*pMesh != nullptr) throw DeadlyImportError("Postprocess_BuildMesh. *pMesh must be nullptr.");

    /************************************************************************************************************************************/
    /************************************************************ Geometry2D ************************************************************/
    /************************************************************************************************************************************/
    if ((pNodeElement.Type == X3DElemType::ENET_Arc2D) || (pNodeElement.Type == X3DElemType::ENET_ArcClose2D) ||
            (pNodeElement.Type == X3DElemType::ENET_Circle2D) || (pNodeElement.Type == X3DElemType::ENET_Disk2D) ||
            (pNodeElement.Type == X3DElemType::ENET_Polyline2D) || (pNodeElement.Type == X3DElemType::ENET_Polypoint2D) ||
            (pNodeElement.Type == X3DElemType::ENET_Rectangle2D) || (pNodeElement.Type == X3DElemType::ENET_TriangleSet2D)) {
        X3DNodeElementGeometry2D &tnemesh = *((X3DNodeElementGeometry2D *)&pNodeElement); // create alias for convenience
        std::vector<aiVector3D> tarr;

        tarr.reserve(tnemesh.Vertices.size());
        for (std::list<aiVector3D>::iterator it = tnemesh.Vertices.begin(); it != tnemesh.Vertices.end(); ++it)
            tarr.push_back(*it);
        *pMesh = StandardShapes::MakeMesh(tarr, static_cast<unsigned int>(tnemesh.NumIndices)); // create mesh from vertices using Assimp help.

        return; // mesh is build, nothing to do anymore.
    }
    /************************************************************************************************************************************/
    /************************************************************ Geometry3D ************************************************************/
    /************************************************************************************************************************************/
    //
    // Predefined figures
    //
    if ((pNodeElement.Type == X3DElemType::ENET_Box) || (pNodeElement.Type == X3DElemType::ENET_Cone) ||
            (pNodeElement.Type == X3DElemType::ENET_Cylinder) || (pNodeElement.Type == X3DElemType::ENET_Sphere)) {
        X3DNodeElementGeometry3D &tnemesh = *((X3DNodeElementGeometry3D *)&pNodeElement); // create alias for convenience
        std::vector<aiVector3D> tarr;

        tarr.reserve(tnemesh.Vertices.size());
        for (std::list<aiVector3D>::iterator it = tnemesh.Vertices.begin(); it != tnemesh.Vertices.end(); ++it)
            tarr.push_back(*it);

        *pMesh = StandardShapes::MakeMesh(tarr, static_cast<unsigned int>(tnemesh.NumIndices)); // create mesh from vertices using Assimp help.

        return; // mesh is build, nothing to do anymore.
    }
    //
    // Parametric figures
    //
    if (pNodeElement.Type == X3DElemType::ENET_ElevationGrid) {
        X3DNodeElementElevationGrid &tnemesh = *((X3DNodeElementElevationGrid *)&pNodeElement); // create alias for convenience

        // at first create mesh from existing vertices.
        *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIdx, tnemesh.Vertices);
        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Normal)
                X3DGeoHelper::add_normal(**pMesh, ((X3DNodeElementNormal *)*ch_it)->Value, tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_TextureCoordinate)
                X3DGeoHelper::add_tex_coord(**pMesh, ((X3DNodeElementTextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of ElevationGrid: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_ElevationGrid)
    //
    // Indexed primitives sets
    //
    if (pNodeElement.Type == X3DElemType::ENET_IndexedFaceSet) {
        X3DNodeElementIndexedSet &tnemesh = *((X3DNodeElementIndexedSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, ((X3DNodeElementCoordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((X3DNodeElementColorRGBA *)*ch_it)->Value,
                        tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == X3DElemType::ENET_Normal)
                X3DGeoHelper::add_normal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((X3DNodeElementNormal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_TextureCoordinate)
                X3DGeoHelper::add_tex_coord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((X3DNodeElementTextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of IndexedFaceSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_IndexedFaceSet)

    if (pNodeElement.Type == X3DElemType::ENET_IndexedLineSet) {
        X3DNodeElementIndexedSet &tnemesh = *((X3DNodeElementIndexedSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, ((X3DNodeElementCoordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((X3DNodeElementColorRGBA *)*ch_it)->Value,
                        tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of IndexedLineSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_IndexedLineSet)

    if ((pNodeElement.Type == X3DElemType::ENET_IndexedTriangleSet) ||
            (pNodeElement.Type == X3DElemType::ENET_IndexedTriangleFanSet) ||
            (pNodeElement.Type == X3DElemType::ENET_IndexedTriangleStripSet)) {
        X3DNodeElementIndexedSet &tnemesh = *((X3DNodeElementIndexedSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, ((X3DNodeElementCoordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((X3DNodeElementColorRGBA *)*ch_it)->Value,
                        tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == X3DElemType::ENET_Normal)
                X3DGeoHelper::add_normal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((X3DNodeElementNormal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_TextureCoordinate)
                X3DGeoHelper::add_tex_coord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((X3DNodeElementTextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of IndexedTriangleSet or IndexedTriangleFanSet, or \
                                                                    IndexedTriangleStripSet: " +
                                        ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if((pNodeElement.Type == X3DElemType::ENET_IndexedTriangleFanSet) || (pNodeElement.Type == X3DElemType::ENET_IndexedTriangleStripSet))

    if (pNodeElement.Type == X3DElemType::ENET_Extrusion) {
        X3DNodeElementIndexedSet &tnemesh = *((X3DNodeElementIndexedSet *)&pNodeElement); // create alias for convenience

        *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, tnemesh.Vertices);

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_Extrusion)

    //
    // Primitives sets
    //
    if (pNodeElement.Type == X3DElemType::ENET_PointSet) {
        X3DNodeElementSet &tnemesh = *((X3DNodeElementSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                std::vector<aiVector3D> vec_copy;

                vec_copy.reserve(((X3DNodeElementCoordinate *)*ch_it)->Value.size());
                for (std::list<aiVector3D>::const_iterator it = ((X3DNodeElementCoordinate *)*ch_it)->Value.begin();
                        it != ((X3DNodeElementCoordinate *)*ch_it)->Value.end(); ++it) {
                    vec_copy.push_back(*it);
                }

                *pMesh = StandardShapes::MakeMesh(vec_copy, 1);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColor *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColorRGBA *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of PointSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_PointSet)

    if (pNodeElement.Type == X3DElemType::ENET_LineSet) {
        X3DNodeElementSet &tnemesh = *((X3DNodeElementSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, ((X3DNodeElementCoordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColor *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColorRGBA *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of LineSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_LineSet)

    if (pNodeElement.Type == X3DElemType::ENET_TriangleFanSet) {
        X3DNodeElementSet &tnemesh = *((X3DNodeElementSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, ((X3DNodeElementCoordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if (nullptr == *pMesh) {
                break;
            }
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == X3DElemType::ENET_Normal)
                X3DGeoHelper::add_normal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((X3DNodeElementNormal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_TextureCoordinate)
                X3DGeoHelper::add_tex_coord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((X3DNodeElementTextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of TrianlgeFanSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_TriangleFanSet)

    if (pNodeElement.Type == X3DElemType::ENET_TriangleSet) {
        X3DNodeElementSet &tnemesh = *((X3DNodeElementSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                std::vector<aiVector3D> vec_copy;

                vec_copy.reserve(((X3DNodeElementCoordinate *)*ch_it)->Value.size());
                for (std::list<aiVector3D>::const_iterator it = ((X3DNodeElementCoordinate *)*ch_it)->Value.begin();
                        it != ((X3DNodeElementCoordinate *)*ch_it)->Value.end(); ++it) {
                    vec_copy.push_back(*it);
                }

                *pMesh = StandardShapes::MakeMesh(vec_copy, 3);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == X3DElemType::ENET_Normal)
                X3DGeoHelper::add_normal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((X3DNodeElementNormal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_TextureCoordinate)
                X3DGeoHelper::add_tex_coord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((X3DNodeElementTextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of TrianlgeSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_TriangleSet)

    if (pNodeElement.Type == X3DElemType::ENET_TriangleStripSet) {
        X3DNodeElementSet &tnemesh = *((X3DNodeElementSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
                *pMesh = X3DGeoHelper::make_mesh(tnemesh.CoordIndex, ((X3DNodeElementCoordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<X3DNodeElementBase *>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == X3DElemType::ENET_Color)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColor *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_ColorRGBA)
                X3DGeoHelper::add_color(**pMesh, ((X3DNodeElementColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == X3DElemType::ENET_Normal)
                X3DGeoHelper::add_normal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((X3DNodeElementNormal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == X3DElemType::ENET_TextureCoordinate)
                X3DGeoHelper::add_tex_coord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((X3DNodeElementTextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of TriangleStripSet: " + ai_to_string((*ch_it)->Type) + ".");
        } // for(std::list<X3DNodeElementBase*>::iterator ch_it = tnemesh.Children.begin(); ch_it != tnemesh.Children.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == X3DElemType::ENET_TriangleStripSet)

    throw DeadlyImportError("Postprocess_BuildMesh. Unknown mesh type: " + ai_to_string(pNodeElement.Type) + ".");
}

void X3DImporter::Postprocess_BuildNode(const X3DNodeElementBase &pNodeElement, aiNode &pSceneNode, std::list<aiMesh *> &pSceneMeshList,
        std::list<aiMaterial *> &pSceneMaterialList, std::list<aiLight *> &pSceneLightList) const {
    std::list<X3DNodeElementBase *>::const_iterator chit_begin = pNodeElement.Children.begin();
    std::list<X3DNodeElementBase *>::const_iterator chit_end = pNodeElement.Children.end();
    std::list<aiNode *> SceneNode_Child;
    std::list<unsigned int> SceneNode_Mesh;

    // At first read all metadata
    Postprocess_CollectMetadata(pNodeElement, pSceneNode);
    // check if we have deal with grouping node. Which can contain transformation or switch
    if (pNodeElement.Type == X3DElemType::ENET_Group) {
        const X3DNodeElementGroup &tne_group = *((X3DNodeElementGroup *)&pNodeElement); // create alias for convenience

        pSceneNode.mTransformation = tne_group.Transformation;
        if (tne_group.UseChoice) {
            // If Choice is less than zero or greater than the number of nodes in the children field, nothing is chosen.
            if ((tne_group.Choice < 0) || ((size_t)tne_group.Choice >= pNodeElement.Children.size())) {
                chit_begin = pNodeElement.Children.end();
                chit_end = pNodeElement.Children.end();
            } else {
                for (size_t i = 0; i < (size_t)tne_group.Choice; i++)
                    ++chit_begin; // forward iterator to chosen node.

                chit_end = chit_begin;
                ++chit_end; // point end iterator to next element after chosen node.
            }
        } // if(tne_group.UseChoice)
    } // if(pNodeElement.Type == X3DElemType::ENET_Group)

    // Reserve memory for fast access and check children.
    for (std::list<X3DNodeElementBase *>::const_iterator it = chit_begin; it != chit_end; ++it) { // in this loop we do not read metadata because it's already read at begin.
        if ((*it)->Type == X3DElemType::ENET_Group) {
            // if child is group then create new node and do recursive call.
            aiNode *new_node = new aiNode;

            new_node->mName = (*it)->ID;
            new_node->mParent = &pSceneNode;
            SceneNode_Child.push_back(new_node);
            Postprocess_BuildNode(**it, *new_node, pSceneMeshList, pSceneMaterialList, pSceneLightList);
        } else if ((*it)->Type == X3DElemType::ENET_Shape) {
            // shape can contain only one geometry and one appearance nodes.
            Postprocess_BuildShape(*((X3DNodeElementShape *)*it), SceneNode_Mesh, pSceneMeshList, pSceneMaterialList);
        } else if (((*it)->Type == X3DElemType::ENET_DirectionalLight) || ((*it)->Type == X3DElemType::ENET_PointLight) ||
                   ((*it)->Type == X3DElemType::ENET_SpotLight)) {
            Postprocess_BuildLight(*((X3DNodeElementLight *)*it), pSceneLightList);
        } else if (!PostprocessHelper_ElementIsMetadata((*it)->Type)) // skip metadata
        {
            throw DeadlyImportError("Postprocess_BuildNode. Unknown type: " + ai_to_string((*it)->Type) + ".");
        }
    } // for(std::list<X3DNodeElementBase*>::const_iterator it = chit_begin; it != chit_end; it++)

    // copy data about children and meshes to aiNode.
    if (!SceneNode_Child.empty()) {
        std::list<aiNode *>::const_iterator it = SceneNode_Child.begin();

        pSceneNode.mNumChildren = static_cast<unsigned int>(SceneNode_Child.size());
        pSceneNode.mChildren = new aiNode *[pSceneNode.mNumChildren];
        for (size_t i = 0; i < pSceneNode.mNumChildren; i++)
            pSceneNode.mChildren[i] = *it++;
    }

    if (!SceneNode_Mesh.empty()) {
        std::list<unsigned int>::const_iterator it = SceneNode_Mesh.begin();

        pSceneNode.mNumMeshes = static_cast<unsigned int>(SceneNode_Mesh.size());
        pSceneNode.mMeshes = new unsigned int[pSceneNode.mNumMeshes];
        for (size_t i = 0; i < pSceneNode.mNumMeshes; i++)
            pSceneNode.mMeshes[i] = *it++;
    }

    // that's all. return to previous deals
}

void X3DImporter::Postprocess_BuildShape(const X3DNodeElementShape &pShapeNodeElement, std::list<unsigned int> &pNodeMeshInd,
        std::list<aiMesh *> &pSceneMeshList, std::list<aiMaterial *> &pSceneMaterialList) const {
    aiMaterial *tmat = nullptr;
    aiMesh *tmesh = nullptr;
    X3DElemType mesh_type = X3DElemType::ENET_Invalid;
    unsigned int mat_ind = 0;

    for (std::list<X3DNodeElementBase *>::const_iterator it = pShapeNodeElement.Children.begin(); it != pShapeNodeElement.Children.end(); ++it) {
        if (PostprocessHelper_ElementIsMesh((*it)->Type)) {
            Postprocess_BuildMesh(**it, &tmesh);
            if (tmesh != nullptr) {
                // if mesh successfully built then add data about it to arrays
                pNodeMeshInd.push_back(static_cast<unsigned int>(pSceneMeshList.size()));
                pSceneMeshList.push_back(tmesh);
                // keep mesh type. Need above for texture coordinate generation.
                mesh_type = (*it)->Type;
            }
        } else if ((*it)->Type == X3DElemType::ENET_Appearance) {
            Postprocess_BuildMaterial(**it, &tmat);
            if (tmat != nullptr) {
                // if material successfully built then add data about it to array
                mat_ind = static_cast<unsigned int>(pSceneMaterialList.size());
                pSceneMaterialList.push_back(tmat);
            }
        }
    } // for(std::list<X3DNodeElementBase*>::const_iterator it = pShapeNodeElement.Children.begin(); it != pShapeNodeElement.Children.end(); it++)

    // associate read material with read mesh.
    if ((tmesh != nullptr) && (tmat != nullptr)) {
        tmesh->mMaterialIndex = mat_ind;
        // Check texture mapping. If material has texture but mesh has no texture coordinate then try to ask Assimp to generate texture coordinates.
        if ((tmat->GetTextureCount(aiTextureType_DIFFUSE) != 0) && !tmesh->HasTextureCoords(0)) {
            int32_t tm;
            aiVector3D tvec3;

            switch (mesh_type) {
            case X3DElemType::ENET_Box:
                tm = aiTextureMapping_BOX;
                break;
            case X3DElemType::ENET_Cone:
            case X3DElemType::ENET_Cylinder:
                tm = aiTextureMapping_CYLINDER;
                break;
            case X3DElemType::ENET_Sphere:
                tm = aiTextureMapping_SPHERE;
                break;
            default:
                tm = aiTextureMapping_PLANE;
                break;
            } // switch(mesh_type)

            tmat->AddProperty(&tm, 1, AI_MATKEY_MAPPING_DIFFUSE(0));
        } // if((tmat->GetTextureCount(aiTextureType_DIFFUSE) != 0) && !tmesh->HasTextureCoords(0))
    } // if((tmesh != nullptr) && (tmat != nullptr))
}

void X3DImporter::Postprocess_CollectMetadata(const X3DNodeElementBase &pNodeElement, aiNode &pSceneNode) const {
    std::list<X3DNodeElementBase *> meta_list;
    size_t meta_idx;

    PostprocessHelper_CollectMetadata(pNodeElement, meta_list); // find metadata in current node element.
    if (!meta_list.empty()) {
        if (pSceneNode.mMetaData != nullptr) {
            throw DeadlyImportError("Postprocess. MetaData member in node are not nullptr. Something went wrong.");
        }

        // copy collected metadata to output node.
        pSceneNode.mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(meta_list.size()));
        meta_idx = 0;
        for (std::list<X3DNodeElementBase *>::const_iterator it = meta_list.begin(); it != meta_list.end(); ++it, ++meta_idx) {
            X3DNodeElementMeta *cur_meta = (X3DNodeElementMeta *)*it;

            // due to limitations we can add only first element of value list.
            // Add an element according to its type.
            if ((*it)->Type == X3DElemType::ENET_MetaBoolean) {
                if (((X3DNodeElementMetaBoolean *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, *(((X3DNodeElementMetaBoolean *)cur_meta)->Value.begin()) == true);
            } else if ((*it)->Type == X3DElemType::ENET_MetaDouble) {
                if (((X3DNodeElementMetaDouble *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, (float)*(((X3DNodeElementMetaDouble *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == X3DElemType::ENET_MetaFloat) {
                if (((X3DNodeElementMetaFloat *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, *(((X3DNodeElementMetaFloat *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == X3DElemType::ENET_MetaInteger) {
                if (((X3DNodeElementMetaInt *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, *(((X3DNodeElementMetaInt *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == X3DElemType::ENET_MetaString) {
                if (((X3DNodeElementMetaString *)cur_meta)->Value.size() > 0) {
                    aiString tstr(((X3DNodeElementMetaString *)cur_meta)->Value.begin()->data());

                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, tstr);
                }
            } else {
                throw DeadlyImportError("Postprocess. Unknown metadata type.");
            } // if((*it)->Type == X3DElemType::ENET_Meta*) else
        } // for(std::list<X3DNodeElementBase*>::const_iterator it = meta_list.begin(); it != meta_list.end(); it++)
    } // if( !meta_list.empty() )
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
