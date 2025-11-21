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
#pragma once

namespace Assimp::D3MF::XmlTag {
    // Root tag
    constexpr char RootTag[] = "3MF";

    // Meta-data
    constexpr char meta[] = "metadata";
    constexpr char meta_name[] = "name";

    // Model-data specific tags
    constexpr char model[] = "model";
    constexpr char model_unit[] = "unit";
    constexpr char metadata[] = "metadata";
    constexpr char resources[] = "resources";
    constexpr char object[] = "object";
    constexpr char mesh[] = "mesh";
    constexpr char components[] = "components";
    constexpr char component[] = "component";
    constexpr char vertices[] = "vertices";
    constexpr char vertex[] = "vertex";
    constexpr char triangles[] = "triangles";
    constexpr char triangle[] = "triangle";
    constexpr char x[] = "x";
    constexpr char y[] = "y";
    constexpr char z[] = "z";
    constexpr char v1[] = "v1";
    constexpr char v2[] = "v2";
    constexpr char v3[] = "v3";
    constexpr char id[] = "id";
    constexpr char pid[] = "pid";
    constexpr char pindex[] = "pindex";
    constexpr char p1[] = "p1";
    constexpr char p2[] = "p2";
    constexpr char p3[] = "p3";
    constexpr char name[] = "name";
    constexpr char type[] = "type";
    constexpr char build[] = "build";
    constexpr char item[] = "item";
    constexpr char objectid[] = "objectid";
    constexpr char transform[] = "transform";
    constexpr char path[] = "path";

    // Material definitions
    constexpr char basematerials[] = "basematerials";
    constexpr char basematerials_base[] = "base";
    constexpr char basematerials_name[] = "name";
    constexpr char basematerials_displaycolor[] = "displaycolor";
    constexpr char texture_2d[] = "m:texture2d";
    constexpr char texture_group[] = "m:texture2dgroup";
    constexpr char texture_content_type[] = "contenttype";
    constexpr char texture_tilestyleu[] = "tilestyleu";
    constexpr char texture_tilestylev[] = "tilestylev";
    constexpr char texture_2d_coord[] = "m:tex2coord";
    constexpr char texture_cuurd_u[] = "u";
    constexpr char texture_cuurd_v[] = "v";

    // vertex color definitions
    constexpr char colorgroup[] = "m:colorgroup";
    constexpr char color_item[] = "m:color";
    constexpr char color_value[] = "color";

    // Meta info tags
    constexpr char CONTENT_TYPES_ARCHIVE[] = "[Content_Types].xml";
    constexpr char ROOT_RELATIONSHIPS_ARCHIVE[] = "_rels/.rels";
    constexpr char SCHEMA_CONTENTTYPES[] = "http://schemas.openxmlformats.org/package/2006/content-types";
    constexpr char SCHEMA_RELATIONSHIPS[] = "http://schemas.openxmlformats.org/package/2006/relationships";
    constexpr char RELS_RELATIONSHIP_CONTAINER[] = "Relationships";
    constexpr char RELS_RELATIONSHIP_NODE[] = "Relationship";
    constexpr char RELS_ATTRIB_TARGET[] = "Target";
    constexpr char RELS_ATTRIB_TYPE[] = "Type";
    constexpr char RELS_ATTRIB_ID[] = "Id";
    constexpr char PACKAGE_START_PART_RELATIONSHIP_TYPE[] = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel";
    constexpr char PACKAGE_PRINT_TICKET_RELATIONSHIP_TYPE[] = "http://schemas.microsoft.com/3dmanufacturing/2013/01/printticket";
    constexpr char PACKAGE_TEXTURE_RELATIONSHIP_TYPE[] = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dtexture";
    constexpr char PACKAGE_CORE_PROPERTIES_RELATIONSHIP_TYPE[] = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties";
    constexpr char PACKAGE_THUMBNAIL_RELATIONSHIP_TYPE[] = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail";

} // namespace Assimp::D3MF
