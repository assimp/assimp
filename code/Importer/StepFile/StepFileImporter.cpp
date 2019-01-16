/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team

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

#ifndef ASSIMP_BUILD_NO_STEPFILE_IMPORTER

#include "StepFileImporter.h"
#include "../../Importer/IFC/STEPFileReader.h"
#include "StepReaderGen.h"
#include <assimp/importerdesc.h>
#include <assimp/DefaultIOSystem.h>

namespace Assimp {
namespace StepFile {

using namespace STEP;

static const aiImporterDesc desc = { "StepFile Importer",
                                "",
                                "",
                                "",
                                0,
                                0,
                                0,
                                0,
                                0,
                                "stp" };

StepFileImporter::StepFileImporter()
: BaseImporter()
, mCarthesianPoints() {
    // empty
}

StepFileImporter::~StepFileImporter() {
    // empty
}

bool StepFileImporter::CanRead(const std::string& file, IOSystem* pIOHandler, bool checkSig) const {
    const std::string &extension = GetExtension(file);
    if (extension == "stp" || extension == "step") {
        return true;
    }
    else if ((!extension.length() || checkSig) && pIOHandler) {
        const char* tokens[] = { "ISO-10303-21" };
        const bool found(SearchFileHeaderForToken(pIOHandler, file, tokens, 1));
        return found;
    }

    return false;
}

const aiImporterDesc *StepFileImporter::GetInfo() const {
    return &desc;
}

static const std::string mode = "rb";
static const std::string StepFileSchema = "CONFIG_CONTROL_DESIGN";

void StepFileImporter::InternReadFile(const std::string &file, aiScene* pScene, IOSystem* pIOHandler) {
    // Read file into memory
    std::shared_ptr<IOStream> fileStream(pIOHandler->Open(file, mode));
    if (!fileStream.get()) {
        throw DeadlyImportError("Failed to open file " + file + ".");
    }

    std::unique_ptr<STEP::DB> db(STEP::ReadFileHeader(fileStream));
    const STEP::HeaderInfo& head = static_cast<const STEP::DB&>(*db).GetHeader();
    if (!head.fileSchema.size() || head.fileSchema != StepFileSchema) {
        DeadlyImportError("Unrecognized file schema: " + head.fileSchema);
    }
    ::Assimp::STEP::EXPRESS::ConversionSchema schema;
    GetSchema(schema);

    // tell the reader which entity types to track with special care
    static const char* const types_to_track[] = {
        "product",
        "vertex_point",
        "line",
        "face_outer_bound",
        "edge_loop",
        "edge_courve",
        "b_spline_surface_with_knots",
        "cartesian_point"
    };

    // tell the reader for which types we need to simulate STEPs reverse indices
    static const char* const inverse_indices_to_track[] = {
        "PRODUCT_DEFINITION"
    };

    // feed the IFC schema into the reader and pre-parse all lines
    STEP::ReadFile(*db, schema, types_to_track, 8, nullptr, 0);

    //STEP::ReadFile(*db, schema, types_to_track, nullptr, 0);
    const STEP::LazyObject *proj = db->GetObject("product");
    if (!proj) {
        DeadlyImportError("missing IfcProject entity");
    }

    ReadSpatialData( db );
}

void StepFileImporter::ReadCarthesianData(const cartesian_point *pt ) {
    ai_assert(nullptr != pt);

    Point3D point;
    point.x = static_cast<ai_real>(pt->coordinates.at(0));
    point.y = static_cast<ai_real>(pt->coordinates.at(1));
    point.z = static_cast<ai_real>(pt->coordinates.at(2));
    mCarthesianPoints.push_back(point);
}

void StepFileImporter::ReadVertexPointData( const vertex_point *vp ) {
    ai_assert(nullptr != vp);


}

enum TokenType {
    CarthesianType =0,
    VertexPointType,
    LineType,
    FaceOuterBoundType,
    EdgeLoopType,
    EdgeCurveType,
    BSplineSurfaceWithKnotsType,

    NoneType
};

TokenType translate(const std::string &key) {
    if (key == "cartesian_point") {
        return CarthesianType;
    } else if (key == "vertex_point") {
        return VertexPointType;
    } else if (key == "line") {
        return LineType;
    } else if (key == "face_outer_bound") {
        return FaceOuterBoundType;
    } else if (key == "edge_loop") {
        return EdgeLoopType;
    } else if (key == "edge_courve") {
        return EdgeCurveType;
    } else if (key == "b_spline_surface_with_knots") {
        return BSplineSurfaceWithKnotsType;
    }

    return NoneType;
}

void StepFileImporter::ReadSpatialData(std::unique_ptr<STEP::DB> &db) {
    const STEP::DB::ObjectMapByType &map = db->GetObjectsByType();
    if (map.empty()) {
        return;
    }
    STEP::DB::ObjectMapByType::const_iterator it( map.begin() );
    for (it; it != map.end(); ++it) {
        const STEP::DB::ObjectSet* data(nullptr);
        const std::string &key(it->first);
        TokenType type = translate(key);
        switch (type) {
            case CarthesianType:
                data = &it->second;
                for (const STEP::LazyObject *lz : *data) {
                    const cartesian_point* const pt = lz->ToPtr<cartesian_point>();
                    ReadCarthesianData(pt);
                }
                break;

            case VertexPointType:
                data = &it->second;
                for (const STEP::LazyObject *lz : *data) {
                    const vertex_point* const vp = lz->ToPtr<vertex_point>();
                    ReadVertexPointData( vp );
                }
                break;

            default:
                break;
        }
    }
}

} // Namespace StepFile
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_STEPFILE_IMPORTER
