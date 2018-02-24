/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team

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
#ifndef ASSIMP_BUILD_NO_FBX_EXPORTER

#include "FBXExportNode.h"
#include "FBXCommon.h"

#include <assimp/StreamWriter.h> // StreamWriterLE
#include <assimp/ai_assert.h>

#include <string>
#include <memory> // shared_ptr

// AddP70<type> helpers... there's no usable pattern here,
// so all are defined as separate functions.
// Even "animatable" properties are often completely different
// from the standard (nonanimated) property definition,
// so they are specified with an 'A' suffix.

void FBX::Node::AddP70int(
    const std::string& name, int32_t value
) {
    FBX::Node n("P");
    n.AddProperties(name, "int", "Integer", "", value);
    AddChild(n);
}

void FBX::Node::AddP70bool(
    const std::string& name, bool value
) {
    FBX::Node n("P");
    n.AddProperties(name, "bool", "", "", int32_t(value));
    AddChild(n);
}

void FBX::Node::AddP70double(
    const std::string& name, double value
) {
    FBX::Node n("P");
    n.AddProperties(name, "double", "Number", "", value);
    AddChild(n);
}

void FBX::Node::AddP70numberA(
    const std::string& name, double value
) {
    FBX::Node n("P");
    n.AddProperties(name, "Number", "", "A", value);
    AddChild(n);
}

void FBX::Node::AddP70color(
    const std::string& name, double r, double g, double b
) {
    FBX::Node n("P");
    n.AddProperties(name, "ColorRGB", "Color", "", r, g, b);
    AddChild(n);
}

void FBX::Node::AddP70colorA(
    const std::string& name, double r, double g, double b
) {
    FBX::Node n("P");
    n.AddProperties(name, "Color", "", "A", r, g, b);
    AddChild(n);
}

void FBX::Node::AddP70vector(
    const std::string& name, double x, double y, double z
) {
    FBX::Node n("P");
    n.AddProperties(name, "Vector3D", "Vector", "", x, y, z);
    AddChild(n);
}

void FBX::Node::AddP70vectorA(
    const std::string& name, double x, double y, double z
) {
    FBX::Node n("P");
    n.AddProperties(name, "Vector", "", "A", x, y, z);
    AddChild(n);
}

void FBX::Node::AddP70string(
    const std::string& name, const std::string& value
) {
    FBX::Node n("P");
    n.AddProperties(name, "KString", "", "", value);
    AddChild(n);
}

void FBX::Node::AddP70enum(
    const std::string& name, int32_t value
) {
    FBX::Node n("P");
    n.AddProperties(name, "enum", "", "", value);
    AddChild(n);
}

void FBX::Node::AddP70time(
    const std::string& name, int64_t value
) {
    FBX::Node n("P");
    n.AddProperties(name, "KTime", "Time", "", value);
    AddChild(n);
}


// public member functions for writing to binary fbx

void FBX::Node::Dump(std::shared_ptr<Assimp::IOStream> outfile)
{
    Assimp::StreamWriterLE outstream(outfile);
    Dump(outstream);
}

void FBX::Node::Dump(Assimp::StreamWriterLE &s)
{
    // write header section (with placeholders for some things)
    Begin(s);

    // write properties
    DumpProperties(s);

    // go back and fill in property related placeholders
    EndProperties(s, properties.size());

    // write children
    DumpChildren(s);

    // finish, filling in end offset placeholder
    End(s, !children.empty());
}

void FBX::Node::Begin(Assimp::StreamWriterLE &s)
{
    // remember start pos so we can come back and write the end pos
    this->start_pos = s.Tell();

    // placeholders for end pos and property section info
    s.PutU4(0); // end pos
    s.PutU4(0); // number of properties
    s.PutU4(0); // total property section length

    // node name
    s.PutU1(name.size()); // length of node name
    s.PutString(name); // node name as raw bytes

    // property data comes after here
    this->property_start = s.Tell();
}

void FBX::Node::DumpProperties(Assimp::StreamWriterLE& s)
{
    for (auto &p : properties) {
        p.Dump(s);
    }
}

void FBX::Node::DumpChildren(Assimp::StreamWriterLE& s)
{
    for (FBX::Node& child : children) {
        child.Dump(s);
    }
}

void FBX::Node::EndProperties(Assimp::StreamWriterLE &s)
{
    EndProperties(s, properties.size());
}

void FBX::Node::EndProperties(
    Assimp::StreamWriterLE &s,
    size_t num_properties
) {
    if (num_properties == 0) { return; }
    size_t pos = s.Tell();
    ai_assert(pos > property_start);
    size_t property_section_size = pos - property_start;
    s.Seek(start_pos + 4);
    s.PutU4(num_properties);
    s.PutU4(property_section_size);
    s.Seek(pos);
}

void FBX::Node::End(
    Assimp::StreamWriterLE &s,
    bool has_children
) {
    // if there were children, add a null record
    if (has_children) { s.PutString(FBX::NULL_RECORD); }

    // now go back and write initial pos
    this->end_pos = s.Tell();
    s.Seek(start_pos);
    s.PutU4(end_pos);
    s.Seek(end_pos);
}


// static member functions

// convenience function to create and write a property node,
// holding a single property which is an array of values.
// does not copy the data, so is efficient for large arrays.
// TODO: optional zip compression!
void FBX::Node::WritePropertyNode(
    const std::string& name,
    const std::vector<double>& v,
    Assimp::StreamWriterLE& s
){
    Node node(name);
    node.Begin(s);
    s.PutU1('d');
    s.PutU4(v.size()); // number of elements
    s.PutU4(0); // no encoding (1 would be zip-compressed)
    s.PutU4(v.size() * 8); // data size
    for (auto it = v.begin(); it != v.end(); ++it) { s.PutF8(*it); }
    node.EndProperties(s, 1);
    node.End(s, false);
}

// convenience function to create and write a property node,
// holding a single property which is an array of values.
// does not copy the data, so is efficient for large arrays.
// TODO: optional zip compression!
void FBX::Node::WritePropertyNode(
    const std::string& name,
    const std::vector<int32_t>& v,
    Assimp::StreamWriterLE& s
){
    Node node(name);
    node.Begin(s);
    s.PutU1('i');
    s.PutU4(v.size()); // number of elements
    s.PutU4(0); // no encoding (1 would be zip-compressed)
    s.PutU4(v.size() * 4); // data size
    for (auto it = v.begin(); it != v.end(); ++it) { s.PutI4(*it); }
    node.EndProperties(s, 1);
    node.End(s, false);
}


#endif // ASSIMP_BUILD_NO_FBX_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
