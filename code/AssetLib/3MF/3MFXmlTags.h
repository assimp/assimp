/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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
    static const char *RootTag = "3MF";

    // Meta-data
    static const char *meta = "metadata";
    static const char *meta_name = "name";

    // Model-data specific tags
    static const char *model = "model";
    static const char *model_unit = "unit";
    static const char *metadata = "metadata";
    static const char *resources = "resources";
    static const char *object = "object";
    static const char *mesh = "mesh";
    static const char *components = "components";
    static const char *component = "component";
    static const char *vertices = "vertices";
    static const char *vertex = "vertex";
    static const char *triangles = "triangles";
    static const char *triangle = "triangle";
    static const char *x = "x";
    static const char *y = "y";
    static const char *z = "z";
    static const char *v1 = "v1";
    static const char *v2 = "v2";
    static const char *v3 = "v3";
    static const char *id = "id";
    static const char *pid = "pid";
    static const char *pindex = "pindex";
    static const char *p1 = "p1";
    static const char *name = "name";
    static const char *type = "type";
    static const char *build = "build";
    static const char *item = "item";
    static const char *objectid = "objectid";
    static const char *transform = "transform";

    // Material definitions
    static const char *basematerials = "basematerials";
    static const char *basematerials_id = "id";
    static const char *basematerials_base = "base";
    static const char *basematerials_name = "name";
    static const char *basematerials_displaycolor = "displaycolor";

    // Meta info tags
    static const char *CONTENT_TYPES_ARCHIVE = "[Content_Types].xml";
    static const char *ROOT_RELATIONSHIPS_ARCHIVE = "_rels/.rels";
    static const char *SCHEMA_CONTENTTYPES = "http://schemas.openxmlformats.org/package/2006/content-types";
    static const char *SCHEMA_RELATIONSHIPS = "http://schemas.openxmlformats.org/package/2006/relationships";
    static const char *RELS_RELATIONSHIP_CONTAINER = "Relationships";
    static const char *RELS_RELATIONSHIP_NODE = "Relationship";
    static const char *RELS_ATTRIB_TARGET = "Target";
    static const char *RELS_ATTRIB_TYPE = "Type";
    static const char *RELS_ATTRIB_ID = "Id";
    static const char *PACKAGE_START_PART_RELATIONSHIP_TYPE = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel";
    static const char *PACKAGE_PRINT_TICKET_RELATIONSHIP_TYPE = "http://schemas.microsoft.com/3dmanufacturing/2013/01/printticket";
    static const char *PACKAGE_TEXTURE_RELATIONSHIP_TYPE = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dtexture";
    static const char *PACKAGE_CORE_PROPERTIES_RELATIONSHIP_TYPE = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties";
    static const char *PACKAGE_THUMBNAIL_RELATIONSHIP_TYPE = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail";
}

} // Namespace D3MF
} // Namespace Assimp
