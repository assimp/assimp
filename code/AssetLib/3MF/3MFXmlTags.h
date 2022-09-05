/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

namespace Assimp {
namespace D3MF {

namespace XmlTag {
    // Root tag
    const char* const RootTag = "3MF";

    // Meta-data
    const char* const meta = "metadata";
    const char* const meta_name = "name";

    // Model-data specific tags
    const char* const model = "model";
    const char* const model_unit = "unit";
    const char* const metadata = "metadata";
    const char* const resources = "resources";
    const char* const object = "object";
    const char* const mesh = "mesh";
    const char* const components = "components";
    const char* const component = "component";
    const char* const vertices = "vertices";
    const char* const vertex = "vertex";
    const char* const triangles = "triangles";
    const char* const triangle = "triangle";
    const char* const x = "x";
    const char* const y = "y";
    const char* const z = "z";
    const char* const v1 = "v1";
    const char* const v2 = "v2";
    const char* const v3 = "v3";
    const char* const id = "id";
    const char* const pid = "pid";
    const char* const pindex = "pindex";
    const char* const p1 = "p1";
    const char *const p2 = "p2";
    const char *const p3 = "p3";
    const char* const name = "name";
    const char* const type = "type";
    const char* const build = "build";
    const char* const item = "item";
    const char* const objectid = "objectid";
    const char* const transform = "transform";
    const char *const path = "path";

    // Material definitions
    const char* const basematerials = "basematerials";
    const char* const basematerials_base = "base";
    const char* const basematerials_name = "name";
    const char* const basematerials_displaycolor = "displaycolor";
    const char* const texture_2d = "m:texture2d";
    const char *const texture_group = "m:texture2dgroup";
    const char *const texture_content_type = "contenttype";
    const char *const texture_tilestyleu = "tilestyleu";
    const char *const texture_tilestylev = "tilestylev";
    const char *const texture_2d_coord = "m:tex2coord";
    const char *const texture_cuurd_u = "u";
    const char *const texture_cuurd_v = "v";

    // Meta info tags
    const char* const CONTENT_TYPES_ARCHIVE = "[Content_Types].xml";
    const char* const ROOT_RELATIONSHIPS_ARCHIVE = "_rels/.rels";
    const char* const SCHEMA_CONTENTTYPES = "http://schemas.openxmlformats.org/package/2006/content-types";
    const char* const SCHEMA_RELATIONSHIPS = "http://schemas.openxmlformats.org/package/2006/relationships";
    const char* const RELS_RELATIONSHIP_CONTAINER = "Relationships";
    const char* const RELS_RELATIONSHIP_NODE = "Relationship";
    const char* const RELS_ATTRIB_TARGET = "Target";
    const char* const RELS_ATTRIB_TYPE = "Type";
    const char* const RELS_ATTRIB_ID = "Id";
    const char* const PACKAGE_START_PART_RELATIONSHIP_TYPE = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel";
    const char* const PACKAGE_PRINT_TICKET_RELATIONSHIP_TYPE = "http://schemas.microsoft.com/3dmanufacturing/2013/01/printticket";
    const char* const PACKAGE_TEXTURE_RELATIONSHIP_TYPE = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dtexture";
    const char* const PACKAGE_CORE_PROPERTIES_RELATIONSHIP_TYPE = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties";
    const char* const PACKAGE_THUMBNAIL_RELATIONSHIP_TYPE = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail";
}

} // Namespace D3MF
} // Namespace Assimp
