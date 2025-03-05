/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_STEP_EXPORTER

#include "AssetLib/Step/StepExporter.h"
#include "PostProcessing/ConvertToLHProcess.h"

#include <assimp/Bitmap.h>
#include <assimp/BaseImporter.h>
#include <assimp/fast_atof.h>
#include <assimp/SceneCombiner.h>
#include <assimp/Exceptional.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/light.h>

#include <iostream>
#include <ctime>
#include <set>
#include <map>
#include <list>
#include <memory>

//
#if _MSC_VER > 1500 || (defined __GNUC___)
#   define ASSIMP_STEP_USE_UNORDERED_MULTIMAP
#   else
#   define step_unordered_map map
#   define step_unordered_multimap multimap
#endif

#ifdef ASSIMP_STEP_USE_UNORDERED_MULTIMAP
#   include <unordered_map>
#   if defined(_MSC_VER) && _MSC_VER <= 1600
#       define step_unordered_map tr1::unordered_map
#       define step_unordered_multimap tr1::unordered_multimap
#   else
#       define step_unordered_map unordered_map
#       define step_unordered_multimap unordered_multimap
#   endif
#endif

typedef std::step_unordered_map<aiVector3D*, int> VectorIndexUMap;

/* Tested with Step viewer v4 from www.ida-step.net */

using namespace Assimp;

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Collada. Prototyped and registered in Exporter.cpp
void ExportSceneStep(const char* pFile,IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* pProperties)
{
    std::string path = DefaultIOSystem::absolutePath(std::string(pFile));
    std::string file = DefaultIOSystem::completeBaseName(std::string(pFile));

    // create/copy Properties
    ExportProperties props(*pProperties);

    // invoke the exporter
    StepExporter iDoTheExportThing( pScene, pIOSystem, path, file, &props);

    // we're still here - export successfully completed. Write result to the given IOSYstem
    std::unique_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wt"));
    if (outfile == nullptr) {
        throw DeadlyExportError("could not open output .stp file: " + std::string(pFile));
    }

    // XXX maybe use a small wrapper around IOStream that behaves like std::stringstream in order to avoid the extra copy.
    outfile->Write( iDoTheExportThing.mOutput.str().c_str(), static_cast<size_t>(iDoTheExportThing.mOutput.tellp()),1);
}

} // end of namespace Assimp


namespace {
    // Collect world transformations for each node
    void CollectTrafos(const aiNode* node, std::map<const aiNode*, aiMatrix4x4>& trafos) {
        const aiMatrix4x4& parent = node->mParent ? trafos[node->mParent] : aiMatrix4x4();
        trafos[node] = parent * node->mTransformation;
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            CollectTrafos(node->mChildren[i], trafos);
        }
    }

    // Generate a flat list of the meshes (by index) assigned to each node
    void CollectMeshes(const aiNode* node, std::multimap<const aiNode*, unsigned int>& meshes) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            meshes.insert(std::make_pair(node, node->mMeshes[i]));
        }
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            CollectMeshes(node->mChildren[i], meshes);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Constructor for a specific scene to export
StepExporter::StepExporter(const aiScene* pScene, IOSystem* pIOSystem, const std::string& path,
    const std::string& file, const ExportProperties* pProperties) :
    mProperties(pProperties), mIOSystem(pIOSystem), mFile(file), mPath(path),
    mScene(pScene), endstr(";\n") {
    CollectTrafos(pScene->mRootNode, trafos);
    CollectMeshes(pScene->mRootNode, meshes);

    // make sure that all formatting happens using the standard, C locale and not the user's current locale
    mOutput.imbue(std::locale("C"));
    mOutput.precision(ASSIMP_AI_REAL_TEXT_PRECISION);

    // start writing
    WriteFile();
}

// ------------------------------------------------------------------------------------------------
// Starts writing the contents
void StepExporter::WriteFile()
{
    // see http://shodhganga.inflibnet.ac.in:8080/jspui/bitstream/10603/14116/11/11_chapter%203.pdf
    // note, that all realnumber values must be comma separated in x files
    mOutput.setf(std::ios::fixed);
    // precision for double
    // see http://stackoverflow.com/questions/554063/how-do-i-print-a-double-value-with-full-precision-using-cout
    mOutput.precision(ASSIMP_AI_REAL_TEXT_PRECISION);

    // standard color
    aiColor4D fColor;
    fColor.r = 0.8f;
    fColor.g = 0.8f;
    fColor.b = 0.8f;

    int ind = 100; // the start index to be used
    std::vector<int> faceEntryLen; // numbers of entries for a triangle/face
    // prepare unique (count triangles and vertices)

    VectorIndexUMap uniqueVerts; // use a map to reduce find complexity to log(n)
    VectorIndexUMap::iterator it;

    for (unsigned int i=0; i<mScene->mNumMeshes; ++i)
    {
        aiMesh* mesh = mScene->mMeshes[i];
        for (unsigned int j=0; j<mesh->mNumFaces; ++j)
        {
            aiFace* face = &(mesh->mFaces[j]);

            if (face->mNumIndices >= 3) faceEntryLen.push_back(15 + 5 * face->mNumIndices);
        }
        for (unsigned int j=0; j<mesh->mNumVertices; ++j)
        {
            aiVector3D* v = &(mesh->mVertices[j]);
            it =uniqueVerts.find(v);
            if (it == uniqueVerts.end())
            {
                uniqueVerts[v] = -1; // first mark the vector as not transformed
            }
        }
    }

    static const unsigned int date_nb_chars = 20;
    char date_str[date_nb_chars];
    std::time_t date = std::time(nullptr);
    std::strftime(date_str, date_nb_chars, "%Y-%m-%dT%H:%M:%S", std::localtime(&date));

    // write the header
    mOutput << "ISO-10303-21" << endstr;
    mOutput << "HEADER" << endstr;
    mOutput << "FILE_DESCRIPTION(('STEP AP214'),'1')" << endstr;
    mOutput << "FILE_NAME('" << mFile << ".stp','" << date_str << "',(' '),(' '),'Spatial InterOp 3D',' ',' ')" << endstr;
    mOutput << "FILE_SCHEMA(('automotive_design'))" << endstr;
    mOutput << "ENDSEC" << endstr;

    // write the top of data
    mOutput << "DATA" << endstr;
    mOutput << "#1=MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION(' ',(";
    size_t countFace = faceEntryLen.size();
    size_t faceLenIndex = ind + 2 * uniqueVerts.size();
    for (size_t i=0; i<countFace; ++i)
    {
        mOutput << "#" << faceLenIndex;
        if (i!=countFace-1) mOutput << ",";
        faceLenIndex += faceEntryLen[i];
    }
    mOutput << "),#6)" << endstr;

    mOutput << "#2=PRODUCT_DEFINITION_CONTEXT('',#7,'design')" << endstr;
    mOutput << "#3=APPLICATION_PROTOCOL_DEFINITION('INTERNATIONAL STANDARD','automotive_design',1994,#7)" << endstr;
    mOutput << "#4=PRODUCT_CATEGORY_RELATIONSHIP('NONE','NONE',#8,#9)" << endstr;
    mOutput << "#5=SHAPE_DEFINITION_REPRESENTATION(#10,#11)" << endstr;
    mOutput << "#6= (GEOMETRIC_REPRESENTATION_CONTEXT(3)GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#12))GLOBAL_UNIT_ASSIGNED_CONTEXT((#13,#14,#15))REPRESENTATION_CONTEXT('NONE','WORKSPACE'))" << endstr;
    mOutput << "#7=APPLICATION_CONTEXT(' ')" << endstr;
    mOutput << "#8=PRODUCT_CATEGORY('part','NONE')" << endstr;
    mOutput << "#9=PRODUCT_RELATED_PRODUCT_CATEGORY('detail',' ',(#17))" << endstr;
    mOutput << "#10=PRODUCT_DEFINITION_SHAPE('NONE','NONE',#18)" << endstr;
    mOutput << "#11=MANIFOLD_SURFACE_SHAPE_REPRESENTATION('Root',(#16,#19),#6)" << endstr;
    mOutput << "#12=UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.0E-006),#13,'','')" << endstr;
    mOutput << "#13=(CONVERSION_BASED_UNIT('METRE',#20)LENGTH_UNIT()NAMED_UNIT(#21))" << endstr;
    mOutput << "#14=(NAMED_UNIT(#22)PLANE_ANGLE_UNIT()SI_UNIT($,.RADIAN.))" << endstr;
    mOutput << "#15=(NAMED_UNIT(#22)SOLID_ANGLE_UNIT()SI_UNIT($,.STERADIAN.))" << endstr;
    mOutput << "#16=SHELL_BASED_SURFACE_MODEL('Root',(#29))" << endstr;
    mOutput << "#17=PRODUCT('Root','Root','Root',(#23))" << endstr;
    mOutput << "#18=PRODUCT_DEFINITION('NONE','NONE',#24,#2)" << endstr;
    mOutput << "#19=AXIS2_PLACEMENT_3D('',#25,#26,#27)" << endstr;
    mOutput << "#20=LENGTH_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.0),#28)" << endstr;
    mOutput << "#21=DIMENSIONAL_EXPONENTS(1.0,0.0,0.0,0.0,0.0,0.0,0.0)" << endstr;
    mOutput << "#22=DIMENSIONAL_EXPONENTS(0.0,0.0,0.0,0.0,0.0,0.0,0.0)" << endstr;
    mOutput << "#23=PRODUCT_CONTEXT('',#7,'mechanical')" << endstr;
    mOutput << "#24=PRODUCT_DEFINITION_FORMATION_WITH_SPECIFIED_SOURCE(' ','NONE',#17,.NOT_KNOWN.)" << endstr;
    mOutput << "#25=CARTESIAN_POINT('',(0.0,0.0,0.0))" << endstr;
    mOutput << "#26=DIRECTION('',(0.0,0.0,1.0))" << endstr;
    mOutput << "#27=DIRECTION('',(1.0,0.0,0.0))" << endstr;
    mOutput << "#28= (NAMED_UNIT(#21)LENGTH_UNIT()SI_UNIT(.MILLI.,.METRE.))" << endstr;
    mOutput << "#29=CLOSED_SHELL('',(";
    faceLenIndex = ind + 2 * uniqueVerts.size() + 8;
    for (size_t i=0; i<countFace; ++i)
    {
        mOutput << "#" << faceLenIndex;
        if (i!=countFace-1) mOutput << ",";
        faceLenIndex += faceEntryLen[i];
    }
    mOutput << "))" << endstr;

    // write all the unique transformed CARTESIAN and VERTEX
    for (MeshesByNodeMap::const_iterator it2 = meshes.begin(); it2 != meshes.end(); ++it2)
    {
        const aiNode& node = *(*it2).first;
        unsigned int mesh_idx = (*it2).second;

        const aiMesh* mesh = mScene->mMeshes[mesh_idx];
        aiMatrix4x4& trafo = trafos[&node];
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            aiVector3D* v = &(mesh->mVertices[i]);
            it = uniqueVerts.find(v);
            if (it->second >=0 ) continue;
            it->second = ind; // this one is new, so set the index (ind)
            aiVector3D vt = trafo * (*v); // transform the coordinate
            mOutput << "#" << it->second << "=CARTESIAN_POINT('',(" << vt.x << "," << vt.y << "," << vt.z << "))" << endstr;
            mOutput << "#" << it->second+1 << "=VERTEX_POINT('',#" << it->second << ")" << endstr;
            ind += 2;
        }
    }

    // write the triangles
    for (unsigned int i=0; i<mScene->mNumMeshes; ++i)
    {
        aiMesh* mesh = mScene->mMeshes[i];
        for (unsigned int j=0; j<mesh->mNumFaces; ++j)
        {
            aiFace* face = &(mesh->mFaces[j]);

            const int numIndices = face->mNumIndices;
            if (numIndices < 3) continue;

            std::vector<int> pidArray(numIndices, -1); // vertex id
            std::vector<aiVector3D> dvArray(numIndices); // edge dir
            for (int k = 0; k < numIndices; ++k)
            {
                aiVector3D *v1 = &(mesh->mVertices[face->mIndices[k]]);
                pidArray[k] = uniqueVerts.find(v1)->second;

                aiVector3D *v2 = nullptr;
                if (k + 1 == numIndices)
                    v2 = &(mesh->mVertices[face->mIndices[0]]);
                else
                    v2 = &(mesh->mVertices[face->mIndices[k + 1]]);
                dvArray[k] = *v2 - *v1;
                dvArray[k].Normalize();
            }

            aiVector3D dvY = dvArray[1];
            aiVector3D dvX = dvY ^ dvArray[0];
            dvX.Normalize();

            // mean vertex color for the face if available
            if (mesh->HasVertexColors(0))
            {
                fColor.r = 0.0;
                fColor.g = 0.0;
                fColor.b = 0.0;
                fColor += mesh->mColors[0][face->mIndices[0]];
                fColor += mesh->mColors[0][face->mIndices[1]];
                fColor += mesh->mColors[0][face->mIndices[2]];
                fColor /= 3.0f;
            }

            int sid = ind; // the sub index
            mOutput << "#" << sid << "=STYLED_ITEM('',(#" << sid+1 << "),#" << sid+8 << ")" << endstr; /* the item that must be referenced in #1 */
            /* This is the color information of the Triangle */
            mOutput << "#" << sid+1 << "=PRESENTATION_STYLE_ASSIGNMENT((#" << sid+2 << "))" << endstr;
            mOutput << "#" << sid+2 << "=SURFACE_STYLE_USAGE(.BOTH.,#" << sid+3 << ")" << endstr;
            mOutput << "#" << sid+3 << "=SURFACE_SIDE_STYLE('',(#" << sid+4 << "))" << endstr;
            mOutput << "#" << sid+4 << "=SURFACE_STYLE_FILL_AREA(#" << sid+5 << ")" << endstr;
            mOutput << "#" << sid+5 << "=FILL_AREA_STYLE('',(#" << sid+6 << "))" << endstr;
            mOutput << "#" << sid+6 << "=FILL_AREA_STYLE_COLOUR('',#" << sid+7 << ")" << endstr;
            mOutput << "#" << sid+7 << "=COLOUR_RGB(''," << fColor.r << "," << fColor.g << "," << fColor.b << ")" << endstr;

            /* this is the geometry */
            mOutput << "#" << sid+8 << "=FACE_SURFACE('',(#" << sid+13 << "),#" << sid+9<< ",.T.)" << endstr; /* the face that must be referenced in 29 */

            /* 2 directions of the plane */
            mOutput << "#" << sid+9 << "=PLANE('',#" << sid+10 << ")" << endstr;
            mOutput << "#" << sid+10 << "=AXIS2_PLACEMENT_3D('',#" << pidArray[0] << ",#" << sid+11 << ",#" << sid+12 << ")" << endstr;

            mOutput << "#" << sid + 11 << "=DIRECTION('',(" << dvX.x << "," << dvX.y << "," << dvX.z << "))" << endstr;
            mOutput << "#" << sid + 12 << "=DIRECTION('',(" << dvY.x << "," << dvY.y << "," << dvY.z << "))" << endstr;

            mOutput << "#" << sid+13 << "=FACE_BOUND('',#" << sid+14 << ",.T.)" << endstr;
            mOutput << "#" << sid+14 << "=EDGE_LOOP('',(";
            int edgeLoopStart = sid + 15;
            for (int k = 0; k < numIndices; ++k)
            {
                if (k == 0)
                    mOutput << "#";
                else
                    mOutput << ",#";
                mOutput << edgeLoopStart + k;
            }
            mOutput << "))" << endstr;

            /* edge loop  */
            int orientedEdgesStart = edgeLoopStart + numIndices;
            for (int k=0; k < numIndices; k++)
            {
                mOutput << "#" << edgeLoopStart+k << "=ORIENTED_EDGE('',*,*,#" << orientedEdgesStart + k << ",.T.)" << endstr;
            }

            /* oriented edges */
            int lineStart = orientedEdgesStart + numIndices;
            for (int k=0; k < numIndices; ++k)
            {
                if (k == 0)
                    mOutput << "#" << orientedEdgesStart+k << "=EDGE_CURVE('',#" << pidArray[k]+1 << ",#" << pidArray[k+1]+1 << ",#" << lineStart+k << ",.F.)" << endstr;
                else if (k+1 == numIndices)
                    mOutput << "#" << orientedEdgesStart+k << "=EDGE_CURVE('',#" << pidArray[k]+1 << ",#" << pidArray[0]+1 << ",#" << lineStart+k << ",.T.)" << endstr;
                else
                    mOutput << "#" << orientedEdgesStart+k << "=EDGE_CURVE('',#" << pidArray[k]+1 << ",#" << pidArray[k+1]+1 << ",#" << lineStart+k << ",.T.)" << endstr;
            }

            /* n lines and n vectors for the lines for the n edge curves */
            int vectorStart = lineStart + numIndices;
            for (int k=0; k < numIndices; ++k)
            {
                mOutput << "#" << lineStart+k << "=LINE('',#" << pidArray[k] << ",#" << vectorStart+k << ")" << endstr;
            }

            int directionStart = vectorStart + numIndices;
            for (int k=0; k < numIndices; ++k)
            {
                mOutput << "#" << vectorStart+k << "=VECTOR('',#" << directionStart+k << ",1.0)" << endstr;
            }

            for (int k=0; k < numIndices; ++k)
            {
                const aiVector3D &dv = dvArray[k];
                mOutput << "#" << directionStart + k << "=DIRECTION('',(" << dv.x << "," << dv.y << "," << dv.z << "))" << endstr;
            }
            ind += 15 + 5*numIndices; // increase counter
        }
    }

    mOutput << "ENDSEC" << endstr; // end of data section
    mOutput << "END-ISO-10303-21" << endstr; // end of file
}

#endif
#endif
