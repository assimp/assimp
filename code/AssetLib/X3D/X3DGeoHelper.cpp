#include "X3DGeoHelper.h"
#include "X3DImporter.hpp"

#include <assimp/vector3.h>
#include <assimp/Exceptional.h>
#include <assimp/StringUtils.h>

#include <vector>

namespace Assimp {

aiVector3D X3DGeoHelper::make_point2D(float angle, float radius) {
    return aiVector3D(radius * std::cos(angle), radius * std::sin(angle), 0);
}

void X3DGeoHelper::make_arc2D(float pStartAngle, float pEndAngle, float pRadius, size_t numSegments, std::list<aiVector3D> &pVertices) {
    // check argument values ranges.
    if ((pStartAngle < -AI_MATH_TWO_PI_F) || (pStartAngle > AI_MATH_TWO_PI_F)) {
        throw DeadlyImportError("GeometryHelper_Make_Arc2D.pStartAngle");
    }
    if ((pEndAngle < -AI_MATH_TWO_PI_F) || (pEndAngle > AI_MATH_TWO_PI_F)) {
        throw DeadlyImportError("GeometryHelper_Make_Arc2D.pEndAngle");
    }
    if (pRadius <= 0) {
        throw DeadlyImportError("GeometryHelper_Make_Arc2D.pRadius");
    }

    // calculate arc angle and check type of arc
    float angle_full = std::fabs(pEndAngle - pStartAngle);
    if ((angle_full > AI_MATH_TWO_PI_F) || (angle_full == 0.0f)) {
        angle_full = AI_MATH_TWO_PI_F;
    }

    // calculate angle for one step - angle to next point of line.
    float angle_step = angle_full / (float)numSegments;
    // make points
    for (size_t pi = 0; pi <= numSegments; pi++) {
        float tangle = pStartAngle + pi * angle_step;
        pVertices.emplace_back(make_point2D(tangle, pRadius));
    } // for(size_t pi = 0; pi <= pNumSegments; pi++)

    // if we making full circle then add last vertex equal to first vertex
    if (angle_full == AI_MATH_TWO_PI_F) pVertices.push_back(*pVertices.begin());
}

void X3DGeoHelper::extend_point_to_line(const std::list<aiVector3D> &pPoint, std::list<aiVector3D> &pLine) {
    std::list<aiVector3D>::const_iterator pit = pPoint.begin();
    std::list<aiVector3D>::const_iterator pit_last = pPoint.end();

    --pit_last;

    if (pPoint.size() < 2) {
        throw DeadlyImportError("GeometryHelper_Extend_PointToLine.pPoint.size() can not be less than 2.");
    }

    // add first point of first line.
    pLine.push_back(*pit++);
    // add internal points
    while (pit != pit_last) {
        pLine.push_back(*pit); // second point of previous line
        pLine.push_back(*pit); // first point of next line
        ++pit;
    }
    // add last point of last line
    pLine.push_back(*pit);
}

void X3DGeoHelper::polylineIdx_to_lineIdx(const std::list<int32_t> &pPolylineCoordIdx, std::list<int32_t> &pLineCoordIdx) {
    std::list<int32_t>::const_iterator plit = pPolylineCoordIdx.begin();

    while (plit != pPolylineCoordIdx.end()) {
        // add first point of polyline
        pLineCoordIdx.push_back(*plit++);
        while ((*plit != (-1)) && (plit != pPolylineCoordIdx.end())) {
            std::list<int32_t>::const_iterator plit_next;

            plit_next = plit, ++plit_next;
            pLineCoordIdx.push_back(*plit); // second point of previous line.
            pLineCoordIdx.push_back(-1); // delimiter
            if ((*plit_next == (-1)) || (plit_next == pPolylineCoordIdx.end())) break; // current polyline is finished

            pLineCoordIdx.push_back(*plit); // first point of next line.
            plit = plit_next;
        } // while((*plit != (-1)) && (plit != pPolylineCoordIdx.end()))
    } // while(plit != pPolylineCoordIdx.end())
}

#define MACRO_FACE_ADD_QUAD_FA(pCCW, pOut, pIn, pP1, pP2, pP3, pP4) \
    do {                                                            \
        if (pCCW) {                                                 \
            pOut.push_back(pIn[pP1]);                               \
            pOut.push_back(pIn[pP2]);                               \
            pOut.push_back(pIn[pP3]);                               \
            pOut.push_back(pIn[pP4]);                               \
        } else {                                                    \
            pOut.push_back(pIn[pP4]);                               \
            pOut.push_back(pIn[pP3]);                               \
            pOut.push_back(pIn[pP2]);                               \
            pOut.push_back(pIn[pP1]);                               \
        }                                                           \
    } while (false)

#define MESH_RectParallelepiped_CREATE_VERT \
    aiVector3D vert_set[8];                 \
    float x1, x2, y1, y2, z1, z2, hs;       \
                                            \
    hs = pSize.x / 2, x1 = -hs, x2 = hs;    \
    hs = pSize.y / 2, y1 = -hs, y2 = hs;    \
    hs = pSize.z / 2, z1 = -hs, z2 = hs;    \
    vert_set[0].Set(x2, y1, z2);            \
    vert_set[1].Set(x2, y2, z2);            \
    vert_set[2].Set(x2, y2, z1);            \
    vert_set[3].Set(x2, y1, z1);            \
    vert_set[4].Set(x1, y1, z2);            \
    vert_set[5].Set(x1, y2, z2);            \
    vert_set[6].Set(x1, y2, z1);            \
    vert_set[7].Set(x1, y1, z1)

void X3DGeoHelper::rect_parallel_epiped(const aiVector3D &pSize, std::list<aiVector3D> &pVertices) {
    MESH_RectParallelepiped_CREATE_VERT;
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 3, 2, 1, 0); // front
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 6, 7, 4, 5); // back
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 7, 3, 0, 4); // left
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 2, 6, 5, 1); // right
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 0, 1, 5, 4); // top
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 7, 6, 2, 3); // bottom
}

#undef MESH_RectParallelepiped_CREATE_VERT

void X3DGeoHelper::coordIdx_str2faces_arr(const std::vector<int32_t> &pCoordIdx, std::vector<aiFace> &pFaces, unsigned int &pPrimitiveTypes) {
    std::vector<int32_t> f_data(pCoordIdx);
    std::vector<unsigned int> inds;
    unsigned int prim_type = 0;

    if (f_data.back() != (-1)) {
        f_data.push_back(-1);
    }

    // reserve average size.
    pFaces.reserve(f_data.size() / 3);
    inds.reserve(4);
    //PrintVectorSet("build. ci", pCoordIdx);
    for (std::vector<int32_t>::iterator it = f_data.begin(); it != f_data.end(); ++it) {
        // when face is got count how many indices in it.
        if (*it == (-1)) {
            aiFace tface;
            size_t ts;

            ts = inds.size();
            switch (ts) {
                case 0:
                    goto mg_m_err;
                case 1:
                    prim_type |= aiPrimitiveType_POINT;
                    break;
                case 2:
                    prim_type |= aiPrimitiveType_LINE;
                    break;
                case 3:
                    prim_type |= aiPrimitiveType_TRIANGLE;
                    break;
                default:
                    prim_type |= aiPrimitiveType_POLYGON;
                    break;
            }

            tface.mNumIndices = static_cast<unsigned int>(ts);
            tface.mIndices = new unsigned int[ts];
            memcpy(tface.mIndices, inds.data(), ts * sizeof(unsigned int));
            pFaces.push_back(tface);
            inds.clear();
        } // if(*it == (-1))
        else {
            inds.push_back(*it);
        } // if(*it == (-1)) else
    } // for(std::list<int32_t>::iterator it = f_data.begin(); it != f_data.end(); it++)
    //PrintVectorSet("build. faces", pCoordIdx);

    pPrimitiveTypes = prim_type;

    return;

mg_m_err:
    for (size_t i = 0, i_e = pFaces.size(); i < i_e; i++)
        delete[] pFaces.at(i).mIndices;

    pFaces.clear();
}

void X3DGeoHelper::coordIdx_str2lines_arr(const std::vector<int32_t> &pCoordIdx, std::vector<aiFace> &pFaces) {
    std::vector<int32_t> f_data(pCoordIdx);

    if (f_data.back() != (-1)) {
        f_data.push_back(-1);
    }

    // reserve average size.
    pFaces.reserve(f_data.size() / 2);
    for (std::vector<int32_t>::const_iterator startIt = f_data.cbegin(), endIt = f_data.cbegin(); endIt != f_data.cend(); ++endIt) {
        // check for end of current polyline
        if (*endIt != -1)
            continue;

        // found end of polyline, check if this is a valid polyline
        std::size_t numIndices = std::distance(startIt, endIt);
        if (numIndices <= 1)
            goto mg_m_err;

        // create line faces out of polyline indices
        for (int32_t idx0 = *startIt++; startIt != endIt; ++startIt) {
            int32_t idx1 = *startIt;

            aiFace tface;
            tface.mNumIndices = 2;
            tface.mIndices = new unsigned int[2];
            tface.mIndices[0] = idx0;
            tface.mIndices[1] = idx1;
            pFaces.push_back(tface);

            idx0 = idx1;
        }

        ++startIt;
    }

    return;

mg_m_err:
    for (size_t i = 0, i_e = pFaces.size(); i < i_e; i++)
        delete[] pFaces[i].mIndices;

    pFaces.clear();
}

void X3DGeoHelper::add_color(aiMesh &pMesh, const std::list<aiColor3D> &pColors, const bool pColorPerVertex) {
    std::list<aiColor4D> tcol;

    // create RGBA array from RGB.
    for (std::list<aiColor3D>::const_iterator it = pColors.begin(); it != pColors.end(); ++it)
        tcol.emplace_back((*it).r, (*it).g, (*it).b, static_cast<ai_real>(1));

    // call existing function for adding RGBA colors
    add_color(pMesh, tcol, pColorPerVertex);
}

void X3DGeoHelper::add_color(aiMesh &pMesh, const std::list<aiColor4D> &pColors, const bool pColorPerVertex) {
    std::list<aiColor4D>::const_iterator col_it = pColors.begin();

    if (pColorPerVertex) {
        if (pColors.size() < pMesh.mNumVertices) {
            throw DeadlyImportError("MeshGeometry_AddColor1. Colors count(" + ai_to_string(pColors.size()) + ") can not be less than Vertices count(" +
                                    ai_to_string(pMesh.mNumVertices) + ").");
        }

        // copy colors to mesh
        pMesh.mColors[0] = new aiColor4D[pMesh.mNumVertices];
        for (size_t i = 0; i < pMesh.mNumVertices; i++)
            pMesh.mColors[0][i] = *col_it++;
    } // if(pColorPerVertex)
    else {
        if (pColors.size() < pMesh.mNumFaces) {
            throw DeadlyImportError("MeshGeometry_AddColor1. Colors count(" + ai_to_string(pColors.size()) + ") can not be less than Faces count(" +
                                    ai_to_string(pMesh.mNumFaces) + ").");
        }

        // copy colors to mesh
        pMesh.mColors[0] = new aiColor4D[pMesh.mNumVertices];
        for (size_t fi = 0; fi < pMesh.mNumFaces; fi++) {
            // apply color to all vertices of face
            for (size_t vi = 0, vi_e = pMesh.mFaces[fi].mNumIndices; vi < vi_e; vi++) {
                pMesh.mColors[0][pMesh.mFaces[fi].mIndices[vi]] = *col_it;
            }

            ++col_it;
        }
    } // if(pColorPerVertex) else
}

void X3DGeoHelper::add_color(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pColorIdx,
        const std::list<aiColor3D> &pColors, const bool pColorPerVertex) {
    std::list<aiColor4D> tcol;

    // create RGBA array from RGB.
    for (std::list<aiColor3D>::const_iterator it = pColors.begin(); it != pColors.end(); ++it) {
        tcol.emplace_back((*it).r, (*it).g, (*it).b, static_cast<ai_real>(1));
    }

    // call existing function for adding RGBA colors
    add_color(pMesh, pCoordIdx, pColorIdx, tcol, pColorPerVertex);
}

void X3DGeoHelper::add_color(aiMesh &pMesh, const std::vector<int32_t> &coordIdx, const std::vector<int32_t> &colorIdx,
        const std::list<aiColor4D> &colors, bool pColorPerVertex) {
    std::vector<aiColor4D> col_tgt_arr;
    std::list<aiColor4D> col_tgt_list;
    std::vector<aiColor4D> col_arr_copy;

    if (coordIdx.size() == 0) {
        throw DeadlyImportError("MeshGeometry_AddColor2. pCoordIdx can not be empty.");
    }

    // copy list to array because we are need indexed access to colors.
    col_arr_copy.reserve(colors.size());
    for (std::list<aiColor4D>::const_iterator it = colors.begin(); it != colors.end(); ++it) {
        col_arr_copy.push_back(*it);
    }

    if (pColorPerVertex) {
        if (colorIdx.size() > 0) {
            // check indices array count.
            if (colorIdx.size() < coordIdx.size()) {
                throw DeadlyImportError("MeshGeometry_AddColor2. Colors indices count(" + ai_to_string(colorIdx.size()) +
                                        ") can not be less than Coords indices count(" + ai_to_string(coordIdx.size()) + ").");
            }
            // create list with colors for every vertex.
            col_tgt_arr.resize(pMesh.mNumVertices);
            for (std::vector<int32_t>::const_iterator colidx_it = colorIdx.begin(), coordidx_it = coordIdx.begin(); colidx_it != colorIdx.end(); ++colidx_it, ++coordidx_it) {
                if (*colidx_it == (-1)) {
                    continue; // skip faces delimiter
                }
                if ((unsigned int)(*coordidx_it) > pMesh.mNumVertices) {
                    throw DeadlyImportError("MeshGeometry_AddColor2. Coordinate idx is out of range.");
                }
                if ((unsigned int)*colidx_it > pMesh.mNumVertices) {
                    throw DeadlyImportError("MeshGeometry_AddColor2. Color idx is out of range.");
                }

                col_tgt_arr[*coordidx_it] = col_arr_copy[*colidx_it];
            }
        } // if(pColorIdx.size() > 0)
        else {
            // when color indices list is absent use CoordIdx.
            // check indices array count.
            if (colors.size() < pMesh.mNumVertices) {
                throw DeadlyImportError("MeshGeometry_AddColor2. Colors count(" + ai_to_string(colors.size()) + ") can not be less than Vertices count(" +
                                        ai_to_string(pMesh.mNumVertices) + ").");
            }
            // create list with colors for every vertex.
            col_tgt_arr.resize(pMesh.mNumVertices);
            for (size_t i = 0; i < pMesh.mNumVertices; i++) {
                col_tgt_arr[i] = col_arr_copy[i];
            }
        } // if(pColorIdx.size() > 0) else
    } // if(pColorPerVertex)
    else {
        if (colorIdx.size() > 0) {
            // check indices array count.
            if (colorIdx.size() < pMesh.mNumFaces) {
                throw DeadlyImportError("MeshGeometry_AddColor2. Colors indices count(" + ai_to_string(colorIdx.size()) +
                                        ") can not be less than Faces count(" + ai_to_string(pMesh.mNumFaces) + ").");
            }
            // create list with colors for every vertex using faces indices.
            col_tgt_arr.resize(pMesh.mNumFaces);

            std::vector<int32_t>::const_iterator colidx_it = colorIdx.begin();
            for (size_t fi = 0; fi < pMesh.mNumFaces; fi++) {
                if ((unsigned int)*colidx_it > pMesh.mNumFaces) throw DeadlyImportError("MeshGeometry_AddColor2. Face idx is out of range.");

                col_tgt_arr[fi] = col_arr_copy[*colidx_it++];
            }
        } // if(pColorIdx.size() > 0)
        else {
            // when color indices list is absent use CoordIdx.
            // check indices array count.
            if (colors.size() < pMesh.mNumFaces) {
                throw DeadlyImportError("MeshGeometry_AddColor2. Colors count(" + ai_to_string(colors.size()) + ") can not be less than Faces count(" +
                                        ai_to_string(pMesh.mNumFaces) + ").");
            }
            // create list with colors for every vertex using faces indices.
            col_tgt_arr.resize(pMesh.mNumFaces);
            for (size_t fi = 0; fi < pMesh.mNumFaces; fi++)
                col_tgt_arr[fi] = col_arr_copy[fi];

        } // if(pColorIdx.size() > 0) else
    } // if(pColorPerVertex) else

    // copy array to list for calling function that add colors.
    for (std::vector<aiColor4D>::const_iterator it = col_tgt_arr.begin(); it != col_tgt_arr.end(); ++it)
        col_tgt_list.push_back(*it);
    // add prepared colors list to mesh.
    add_color(pMesh, col_tgt_list, pColorPerVertex);
}

void X3DGeoHelper::add_normal(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pNormalIdx,
        const std::list<aiVector3D> &pNormals, const bool pNormalPerVertex) {
    std::vector<size_t> tind;
    std::vector<aiVector3D> norm_arr_copy;

    // copy list to array because we are need indexed access to normals.
    norm_arr_copy.reserve(pNormals.size());
    for (std::list<aiVector3D>::const_iterator it = pNormals.begin(); it != pNormals.end(); ++it) {
        norm_arr_copy.push_back(*it);
    }

    if (pNormalPerVertex) {
        if (pNormalIdx.size() > 0) {
            // check indices array count.
            if (pNormalIdx.size() != pCoordIdx.size()) throw DeadlyImportError("Normals and Coords inidces count must be equal.");

            tind.reserve(pNormalIdx.size());
            for (std::vector<int32_t>::const_iterator it = pNormalIdx.begin(); it != pNormalIdx.end(); ++it) {
                if (*it != (-1)) tind.push_back(*it);
            }

            // copy normals to mesh
            pMesh.mNormals = new aiVector3D[pMesh.mNumVertices];
            for (size_t i = 0; (i < pMesh.mNumVertices) && (i < tind.size()); i++) {
                if (tind[i] >= norm_arr_copy.size())
                    throw DeadlyImportError("MeshGeometry_AddNormal. Normal index(" + ai_to_string(tind[i]) +
                                            ") is out of range. Normals count: " + ai_to_string(norm_arr_copy.size()) + ".");

                pMesh.mNormals[i] = norm_arr_copy[tind[i]];
            }
        } else {
            if (pNormals.size() != pMesh.mNumVertices) throw DeadlyImportError("MeshGeometry_AddNormal. Normals and vertices count must be equal.");

            // copy normals to mesh
            pMesh.mNormals = new aiVector3D[pMesh.mNumVertices];
            std::list<aiVector3D>::const_iterator norm_it = pNormals.begin();
            for (size_t i = 0; i < pMesh.mNumVertices; i++)
                pMesh.mNormals[i] = *norm_it++;
        }
    } // if(pNormalPerVertex)
    else {
        if (pNormalIdx.size() > 0) {
            if (pMesh.mNumFaces != pNormalIdx.size()) throw DeadlyImportError("Normals faces count must be equal to mesh faces count.");

            std::vector<int32_t>::const_iterator normidx_it = pNormalIdx.begin();

            tind.reserve(pNormalIdx.size());
            for (size_t i = 0, i_e = pNormalIdx.size(); i < i_e; i++)
                tind.push_back(*normidx_it++);

        } else {
            tind.reserve(pMesh.mNumFaces);
            for (size_t i = 0; i < pMesh.mNumFaces; i++)
                tind.push_back(i);
        }

        // copy normals to mesh
        pMesh.mNormals = new aiVector3D[pMesh.mNumVertices];
        for (size_t fi = 0; fi < pMesh.mNumFaces; fi++) {
            aiVector3D tnorm;

            tnorm = norm_arr_copy[tind[fi]];
            for (size_t vi = 0, vi_e = pMesh.mFaces[fi].mNumIndices; vi < vi_e; vi++)
                pMesh.mNormals[pMesh.mFaces[fi].mIndices[vi]] = tnorm;
        }
    } // if(pNormalPerVertex) else
}

void X3DGeoHelper::add_normal(aiMesh &pMesh, const std::list<aiVector3D> &pNormals, const bool pNormalPerVertex) {
    std::list<aiVector3D>::const_iterator norm_it = pNormals.begin();

    if (pNormalPerVertex) {
        if (pNormals.size() != pMesh.mNumVertices) throw DeadlyImportError("MeshGeometry_AddNormal. Normals and vertices count must be equal.");

        // copy normals to mesh
        pMesh.mNormals = new aiVector3D[pMesh.mNumVertices];
        for (size_t i = 0; i < pMesh.mNumVertices; i++)
            pMesh.mNormals[i] = *norm_it++;
    } // if(pNormalPerVertex)
    else {
        if (pNormals.size() != pMesh.mNumFaces) throw DeadlyImportError("MeshGeometry_AddNormal. Normals and faces count must be equal.");

        // copy normals to mesh
        pMesh.mNormals = new aiVector3D[pMesh.mNumVertices];
        for (size_t fi = 0; fi < pMesh.mNumFaces; fi++) {
            // apply color to all vertices of face
            for (size_t vi = 0, vi_e = pMesh.mFaces[fi].mNumIndices; vi < vi_e; vi++)
                pMesh.mNormals[pMesh.mFaces[fi].mIndices[vi]] = *norm_it;

            ++norm_it;
        }
    } // if(pNormalPerVertex) else
}

void X3DGeoHelper::add_tex_coord(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pTexCoordIdx,
        const std::list<aiVector2D> &pTexCoords) {
    std::vector<aiVector3D> texcoord_arr_copy;
    std::vector<aiFace> faces;
    unsigned int prim_type;

    // copy list to array because we are need indexed access to normals.
    texcoord_arr_copy.reserve(pTexCoords.size());
    for (std::list<aiVector2D>::const_iterator it = pTexCoords.begin(); it != pTexCoords.end(); ++it) {
        texcoord_arr_copy.emplace_back((*it).x, (*it).y, static_cast<ai_real>(0));
    }

    if (pTexCoordIdx.size() > 0) {
        coordIdx_str2faces_arr(pTexCoordIdx, faces, prim_type);
        if (faces.empty()) {
            throw DeadlyImportError("Failed to add texture coordinates to mesh, faces list is empty.");
        }
        if (faces.size() != pMesh.mNumFaces) {
            throw DeadlyImportError("Texture coordinates faces count must be equal to mesh faces count.");
        }
    } else {
        coordIdx_str2faces_arr(pCoordIdx, faces, prim_type);
    }

    pMesh.mTextureCoords[0] = new aiVector3D[pMesh.mNumVertices];
    pMesh.mNumUVComponents[0] = 2;
    for (size_t fi = 0, fi_e = faces.size(); fi < fi_e; fi++) {
        if (pMesh.mFaces[fi].mNumIndices != faces.at(fi).mNumIndices)
            throw DeadlyImportError("Number of indices in texture face and mesh face must be equal. Invalid face index: " + ai_to_string(fi) + ".");

        for (size_t ii = 0; ii < pMesh.mFaces[fi].mNumIndices; ii++) {
            size_t vert_idx = pMesh.mFaces[fi].mIndices[ii];
            size_t tc_idx = faces.at(fi).mIndices[ii];

            pMesh.mTextureCoords[0][vert_idx] = texcoord_arr_copy.at(tc_idx);
        }
    } // for(size_t fi = 0, fi_e = faces.size(); fi < fi_e; fi++)
}

void X3DGeoHelper::add_tex_coord(aiMesh &pMesh, const std::list<aiVector2D> &pTexCoords) {
    std::vector<aiVector3D> tc_arr_copy;

    if (pTexCoords.size() != pMesh.mNumVertices) {
        throw DeadlyImportError("MeshGeometry_AddTexCoord. Texture coordinates and vertices count must be equal.");
    }

    // copy list to array because we are need convert aiVector2D to aiVector3D and also get indexed access as a bonus.
    tc_arr_copy.reserve(pTexCoords.size());
    for (std::list<aiVector2D>::const_iterator it = pTexCoords.begin(); it != pTexCoords.end(); ++it) {
        tc_arr_copy.emplace_back((*it).x, (*it).y, static_cast<ai_real>(0));
    }

    // copy texture coordinates to mesh
    pMesh.mTextureCoords[0] = new aiVector3D[pMesh.mNumVertices];
    pMesh.mNumUVComponents[0] = 2;
    for (size_t i = 0; i < pMesh.mNumVertices; i++) {
        pMesh.mTextureCoords[0][i] = tc_arr_copy[i];
    }
}

aiMesh *X3DGeoHelper::make_mesh(const std::vector<int32_t> &pCoordIdx, const std::list<aiVector3D> &pVertices) {
    std::vector<aiFace> faces;
    unsigned int prim_type = 0;

    // create faces array from input string with vertices indices.
    X3DGeoHelper::coordIdx_str2faces_arr(pCoordIdx, faces, prim_type);
    if (!faces.size()) {
        throw DeadlyImportError("Failed to create mesh, faces list is empty.");
    }

    //
    // Create new mesh and copy geometry data.
    //
    aiMesh *tmesh = new aiMesh;
    size_t ts = faces.size();
    // faces
    tmesh->mFaces = new aiFace[ts];
    tmesh->mNumFaces = static_cast<unsigned int>(ts);
    for (size_t i = 0; i < ts; i++)
        tmesh->mFaces[i] = faces.at(i);

    // vertices
    std::list<aiVector3D>::const_iterator vit = pVertices.begin();

    ts = pVertices.size();
    tmesh->mVertices = new aiVector3D[ts];
    tmesh->mNumVertices = static_cast<unsigned int>(ts);
    for (size_t i = 0; i < ts; i++) {
        tmesh->mVertices[i] = *vit++;
    }

    // set primitives type and return result.
    tmesh->mPrimitiveTypes = prim_type;

    return tmesh;
}

aiMesh *X3DGeoHelper::make_line_mesh(const std::vector<int32_t> &pCoordIdx, const std::list<aiVector3D> &pVertices) {
    std::vector<aiFace> faces;

    // create faces array from input string with vertices indices.
    X3DGeoHelper::coordIdx_str2lines_arr(pCoordIdx, faces);
    if (!faces.size()) {
        throw DeadlyImportError("Failed to create mesh, faces list is empty.");
    }

    //
    // Create new mesh and copy geometry data.
    //
    aiMesh *tmesh = new aiMesh;
    size_t ts = faces.size();
    // faces
    tmesh->mFaces = new aiFace[ts];
    tmesh->mNumFaces = static_cast<unsigned int>(ts);
    for (size_t i = 0; i < ts; i++)
        tmesh->mFaces[i] = faces[i];

    // vertices
    std::list<aiVector3D>::const_iterator vit = pVertices.begin();

    ts = pVertices.size();
    tmesh->mVertices = new aiVector3D[ts];
    tmesh->mNumVertices = static_cast<unsigned int>(ts);
    for (size_t i = 0; i < ts; i++) {
        tmesh->mVertices[i] = *vit++;
    }

    // set primitive type and return result.
    tmesh->mPrimitiveTypes = aiPrimitiveType_LINE;

    return tmesh;
}

} // namespace Assimp
