/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team

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

//#include <rapidjson/stringbuffer.h>
//#include <rapidjson/writer.h>
//#include <rapidjson/prettywriter.h>
#include <fstream>

namespace glTF {
    using json = nlohmann::json;

/*    using rapidjson::StringBuffer;
    using rapidjson::PrettyWriter;
    using rapidjson::Writer;
    using rapidjson::StringRef;
    using rapidjson::StringRef;*/

    namespace {

        template<size_t N>
        inline 
        json& MakeValue(json& val, float(&r)[N] ) {
            val = json::array();
            for (decltype(N) i = 0; i < N; ++i) {
                val.push_back(r[i] );
            }
            return val;
        }

        inline 
        json& MakeValue(json& val, const std::vector<float> & r) {
            val = json::array();
            for (unsigned int i = 0; i < r.size(); ++i) {
                val.push_back(r[i]);
            }
            return val;
        }

        template<class T>
        inline 
        void AddRefsVector(json& obj, const char* fieldId, std::vector< Ref<T> >& v ) {
            if ( v.empty() ) {
                return;
            }
            json lst = json::array();
            for (size_t i = 0; i < v.size(); ++i) {
                lst.push_back( v[i]->id);
            }
            obj[ fieldId ] = lst;
        }

    }

    inline 
    void Write(json& obj, Accessor& a, AssetWriter& w) {
        obj["bufferView"] = a.bufferView->id;
        obj["byteOffset"] = a.byteOffset;
        obj["byteStride"] = a.byteStride;
        obj["componentType"] = a.componentType;
        obj["count"] = a.count;
        obj["type"] = a.type;

        json vTmpMax, vTmpMin;
        obj[ "max" ] = MakeValue( vTmpMax, a.max );
        obj[ "min" ] = MakeValue( vTmpMin, a.min );
    }

    inline
    void Write( json& obj, Animation& a, AssetWriter& w) {
        /****************** Channels *******************/
        json channels = json::array();

        for (size_t i = 0; i < unsigned(a.Channels.size()); ++i) {
            Animation::AnimChannel& c = a.Channels[i];
            json valChannel = json::object();
            {
                valChannel["sampler"] = c.sampler;

                json valTarget = json::object();
                {
                    valTarget["id"] = c.target.id->id;
                    valTarget["path"] = c.target.path;
                }
                valChannel["target"] =  valTarget;
            }
            channels.push_back( valChannel );
        }
        obj["channels"] = channels;

        /****************** Parameters *******************/
        json valParameters = json::object();
        {
            if (a.Parameters.TIME) {
                valParameters["TIME"] = a.Parameters.TIME->id;
            }
            if (a.Parameters.rotation) {
                valParameters["rotation"] = a.Parameters.rotation->id;
            }
            if (a.Parameters.scale) {
                valParameters[ "scale"] = a.Parameters.scale->id;
            }
            if (a.Parameters.translation) {
                valParameters["translation"] = a.Parameters.translation->id;
            }
        }
        obj["parameters"] = valParameters;

        /****************** Samplers *******************/
        json valSamplers = json::object();

        for (size_t i = 0; i < unsigned(a.Samplers.size()); ++i) {
            Animation::AnimSampler& s = a.Samplers[i];
            json valSampler = json::object();
            {
                valSampler["input"] = s.input;
                valSampler["interpolation"] = s.interpolation;
                valSampler["output"] = s.output;
            }
            valSamplers[s.id] = valSampler;
        }
        obj["samplers"] = valSamplers;
    }

    inline 
    void Write(json& obj, Buffer& b, AssetWriter& w) {
        const char* type;
        switch (b.type) {
            case Buffer::Type_text:
                type = "text"; 
                break;
            default:
                type = "arraybuffer";
                break;
        }

        obj["byteLength"] = static_cast<uint64_t>(b.byteLength);
        obj["type"] = type;
        obj["uri"] = b.GetURI();
    }

    inline 
    void Write(json& obj, BufferView& bv, AssetWriter& w) {
        obj[ "buffer"] = bv.buffer->id;
        obj[ "byteOffset"] = static_cast<uint64_t>(bv.byteOffset);
        obj[ "byteLength"] = static_cast<uint64_t>(bv.byteLength);
        obj[ "target" ] = int(bv.target);
    }

    inline void Write( json& obj, Camera& c, AssetWriter& w)
    {

    }

    inline void Write( json& obj, Image& img, AssetWriter& w)
    {
        std::string uri;
        if (w.mAsset.extensionsUsed.KHR_binary_glTF && img.bufferView) {
            json exts = json::object(), ext = json::object();

            ext["bufferView"] = img.bufferView->id;

            if (!img.mimeType.empty())
                ext["mimeType"] = img.mimeType;

            exts["KHR_binary_glTF"] = ext;
            obj["extensions"] = exts;
            return;
        }
        else if (img.HasData()) {
            uri = "data:" + (img.mimeType.empty() ? "application/octet-stream" : img.mimeType);
            uri += ";base64,";
            Util::EncodeBase64(img.GetData(), img.GetDataLength(), uri);
        }
        else {
            uri = img.uri;
        }

        obj["uri"] = uri;
    }

    namespace {
        inline 
        void WriteColorOrTex(json& obj, TexProperty& prop, const char* propName ) {
            if (prop.texture)
                obj[ propName] = prop.texture->id;
            else {
                json col;
                obj[ propName ] = MakeValue(col, prop.color );
            }
        }
    }

    inline 
    void Write(json& obj, Material& m, AssetWriter& w) {
        json v = json::object();
        {
            WriteColorOrTex(v, m.ambient, "ambient" );
            WriteColorOrTex(v, m.diffuse, "diffuse" );
            WriteColorOrTex(v, m.specular, "specular" );
            WriteColorOrTex(v, m.emission, "emission" );

            if ( m.transparent ) {
                v[ "transparency" ] = m.transparency;
            }

            v[ "shininess" ] = m.shininess;
        }
        obj["values" ] = v;
    }

    namespace {
        inline void WriteAttrs(AssetWriter& w, json& attrs, Mesh::AccessorList& lst,
            const char* semantic, bool forceNumber = false)
        {
            if ( lst.empty() ) {
                return;
            }
            if (lst.size() == 1 && !forceNumber) {
                attrs[ semantic ] = lst[0]->id;
            } else {
                for (size_t i = 0; i < lst.size(); ++i) {
                    char buffer[32];
                    ai_snprintf(buffer, 32, "%s_%d", semantic, int(i));
                    attrs[buffer] = lst[i]->id;
                }
            }
        }
    }

    inline 
    void Write(json& obj, Mesh& m, AssetWriter& w)  {
		/********************* Name **********************/
		obj["name"]= m.name;

		/**************** Mesh extensions ****************/
		if(m.Extension.size() > 0)
		{
			json json_extensions = json::object();
			for(Mesh::SExtension* ptr_ext : m.Extension)
			{
				switch(ptr_ext->Type)
				{
#ifdef ASSIMP_IMPORTER_GLTF_USE_OPEN3DGC
					case Mesh::SExtension::EType::Compression_Open3DGC:
						{
							json json_comp_data = json::object();
							Mesh::SCompression_Open3DGC* ptr_ext_comp = (Mesh::SCompression_Open3DGC*)ptr_ext;

							// filling object "compressedData"
							json_comp_data["buffer"] = ptr_ext_comp->Buffer;
							json_comp_data["byteOffset"] = ptr_ext_comp->Offset;
							json_comp_data["componentType"] =  5121;
							json_comp_data["type"] = "SCALAR";
							json_comp_data["count"] = ptr_ext_comp->Count;
							if(ptr_ext_comp->Binary)
								json_comp_data["mode"] = "binary";
							else
								json_comp_data["mode"] = "ascii";

							json_comp_data["indicesCount"] = ptr_ext_comp->IndicesCount;
							json_comp_data["verticesCount"] = ptr_ext_comp->VerticesCount;
							// filling object "Open3DGC-compression"
                            json json_o3dgc = json::object();

							json_o3dgc["compressedData"] = json_comp_data;
							
                            // add member to object "extensions"
							json_extensions["Open3DGC-compression"] = json_o3dgc;
						}

						break;
#endif
					default:
						throw DeadlyImportError("GLTF: Can not write mesh: unknown mesh extension, only Open3DGC is supported.");
				}// switch(ptr_ext->Type)
			}// for(Mesh::SExtension* ptr_ext : m.Extension)

			// Add extensions to mesh
			obj["extensions"] = json_extensions;
		}// if(m.Extension.size() > 0)

		/****************** Primitives *******************/
        json primitives = json::array();

        for (size_t i = 0; i < m.primitives.size(); ++i) {
            Mesh::Primitive& p = m.primitives[i];
            json prim = json::object();
            {
                prim["mode"] = int(p.mode);

                if (p.material)
                    prim["material"] = p.material->id;

                if (p.indices)
                    prim["indices"] = p.indices->id;

                json attrs = json::object();
                {
                    WriteAttrs(w, attrs, p.attributes.position, "POSITION");
                    WriteAttrs(w, attrs, p.attributes.normal, "NORMAL");
                    WriteAttrs(w, attrs, p.attributes.texcoord, "TEXCOORD", true);
                    WriteAttrs(w, attrs, p.attributes.color, "COLOR");
                    WriteAttrs(w, attrs, p.attributes.joint, "JOINT");
                    WriteAttrs(w, attrs, p.attributes.jointmatrix, "JOINTMATRIX");
                    WriteAttrs(w, attrs, p.attributes.weight, "WEIGHT");
                }
                prim["attributes"] =attrs;
            }
            primitives.push_back(prim);
        }

        obj["primitives"] = primitives;
    }

    inline void Write(json& obj, Node& n, AssetWriter& w)
    {

        if (n.matrix.isPresent) {
            json val;
            obj["matrix"] = MakeValue(val, n.matrix.value);
        }

        if (n.translation.isPresent) {
            json val;
            obj["translation"] = MakeValue(val, n.translation.value);
        }

        if (n.scale.isPresent) {
            json val;
            obj["scale"] = MakeValue(val, n.scale.value);
        }
        if (n.rotation.isPresent) {
            json val;
            obj["rotation"] = MakeValue(val, n.rotation.value);
        }

        AddRefsVector(obj, "children", n.children );

        AddRefsVector(obj, "meshes", n.meshes);

        AddRefsVector(obj, "skeletons", n.skeletons);

        if (n.skin) {
            obj["skin"] = n.skin->id;
        }

        if (!n.jointName.empty()) {
            obj["jointName"] = n.jointName;
        }
    }

    inline void Write(json& obj, Program& b, AssetWriter& w)
    {

    }

    inline void Write(json& obj, Sampler& b, AssetWriter& w)
    {
        if (b.wrapS) {
            obj["wrapS"] = b.wrapS;
        }
        if (b.wrapT) {
            obj["wrapT"] = b.wrapT;
        }
        if (b.magFilter) {
            obj["magFilter" ] = b.magFilter;
        }
        if (b.minFilter) {
            obj["minFilter"] = b.minFilter;
        }
    }

    inline void Write(json& scene, Scene& s, AssetWriter& w)
    {
        AddRefsVector(scene, "nodes", s.nodes);
    }

    inline void Write(json& obj, Shader& b, AssetWriter& w)
    {

    }

    inline void Write(json& obj, Skin& b, AssetWriter& w)
    {
        /****************** jointNames *******************/
        json vJointNames = json::array();

        for (size_t i = 0; i < unsigned(b.jointNames.size()); ++i) {
            vJointNames.push_back( b.jointNames[i]->jointName );
        }
        obj["jointNames"] = vJointNames;

        if (b.bindShapeMatrix.isPresent) {
            json val;
            obj["bindShapeMatrix"] = MakeValue(val, b.bindShapeMatrix.value);
        }

        if (b.inverseBindMatrices) {
            obj["inverseBindMatrices"] = b.inverseBindMatrices->id;
        }

    }

    inline void Write(json& obj, Technique& b, AssetWriter& w)
    {

    }

    inline void Write(json& obj, Texture& tex, AssetWriter& w)
    {
        if (tex.source) {
            obj["source"] = tex.source->id;
        }
        if (tex.sampler) {
            obj["sampler"] = tex.sampler->id;
        }
    }

    inline void Write(json& obj, Light& b, AssetWriter& w)
    {

    }


    inline AssetWriter::AssetWriter(Asset& a)
        : mDoc()
        , mAsset(a)
    {
        mDoc = json::object();

        WriteMetadata();
        WriteExtensionsUsed();

        // Dump the contents of the dictionaries
        for (size_t i = 0; i < a.mDicts.size(); ++i) {
            a.mDicts[i]->WriteObjects(*this);
        }

        // Add the target scene field
        if (mAsset.scene) {
            mDoc["scene"]= mAsset.scene->id;
        }
    }

    inline void AssetWriter::WriteFile(const char* path)
    {
        std::unique_ptr<IOStream> jsonOutFile(mAsset.OpenFile(path, "wt", true));

        if (jsonOutFile == 0) {
            throw DeadlyExportError("Could not open output file: " + std::string(path));
        }

        //StringBuffer docBuffer;

        //PrettyWriter<StringBuffer> writer(docBuffer);
        //mDoc.Accept(writer);

        std::ofstream o( path );
        o << std::setw( 4 ) << mDoc << std::endl;
        /*if (jsonOutFile->Write(docBuffer.GetString(), docBuffer.GetSize(), 1) != 1) {
            throw DeadlyExportError("Failed to write scene data!");
        }*/

        // Write buffer data to separate .bin files
        for (unsigned int i = 0; i < mAsset.buffers.Size(); ++i) {
            Ref<Buffer> b = mAsset.buffers.Get(i);

            std::string binPath = b->GetURI();

            std::unique_ptr<IOStream> binOutFile(mAsset.OpenFile(binPath, "wb", true));

            if (binOutFile == 0) {
                throw DeadlyExportError("Could not open output file: " + binPath);
            }

            if (b->byteLength > 0) {
                if (binOutFile->Write(b->GetPointer(), b->byteLength, 1) != 1) {
                    throw DeadlyExportError("Failed to write binary file: " + binPath);
                }
            }
        }
    }

    inline void AssetWriter::WriteGLBFile(const char* path)
    {
        std::unique_ptr<IOStream> outfile(mAsset.OpenFile(path, "wb", true));

        if (outfile == 0) {
            throw DeadlyExportError("Could not open output file: " + std::string(path));
        }

        // we will write the header later, skip its size
        outfile->Seek(sizeof(GLB_Header), aiOrigin_SET);

/*        StringBuffer docBuffer;
        Writer<StringBuffer> writer(docBuffer);
        mDoc.Accept(writer);*/
        std::string s = mDoc.dump( 4 );
        if ( outfile->Write( s.c_str(), s.size(), 1 ) != 1 ) {
        //if (outfile->Write(docBuffer.GetString(), docBuffer.GetSize(), 1) != 1) {
            throw DeadlyExportError("Failed to write scene data!");
        }

        WriteBinaryData(outfile.get(), s.size());
    }

    inline void AssetWriter::WriteBinaryData(IOStream* outfile, size_t sceneLength)
    {
        //
        // write the body data
        //

        size_t bodyLength = 0;
        if (Ref<Buffer> b = mAsset.GetBodyBuffer()) {
            bodyLength = b->byteLength;

            if (bodyLength > 0) {
                size_t bodyOffset = sizeof(GLB_Header) + sceneLength;
                bodyOffset = (bodyOffset + 3) & ~3; // Round up to next multiple of 4

                outfile->Seek(bodyOffset, aiOrigin_SET);

                if (outfile->Write(b->GetPointer(), b->byteLength, 1) != 1) {
                    throw DeadlyExportError("Failed to write body data!");
                }
            }
        }

        //
        // write the header
        //

        GLB_Header header;
        memcpy(header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic));

        header.version = 1;
        AI_SWAP4(header.version);

        header.length = uint32_t(sizeof(header) + sceneLength + bodyLength);
        AI_SWAP4(header.length);

        header.sceneLength = uint32_t(sceneLength);
        AI_SWAP4(header.sceneLength);

        header.sceneFormat = SceneFormat_JSON;
        AI_SWAP4(header.sceneFormat);

        outfile->Seek(0, aiOrigin_SET);

        if (outfile->Write(&header, 1, sizeof(header)) != sizeof(header)) {
            throw DeadlyExportError("Failed to write the header!");
        }
    }


    inline void AssetWriter::WriteMetadata()
    {
        json asset = json::object();
        {
            char versionChar[10];
            ai_snprintf(versionChar, sizeof(versionChar), "%d", mAsset.asset.version);
            asset["version"] = versionChar;

            asset["generator"] = mAsset.asset.generator;
        }
        mDoc["asset"] = asset;
    }

    inline void AssetWriter::WriteExtensionsUsed()
    {
        json exts = json::array();
        {
            if (false)
                exts.push_back("KHR_binary_glTF");

            if (false)
                exts.push_back("KHR_materials_common");
        }

        if (!exts.empty())
            mDoc["extensionsUsed"] = exts;
    }

    template<class T>
    void AssetWriter::WriteObjects(LazyDict<T>& d)
    {
        if (d.mObjs.empty()) return;

        json* container = &mDoc;

        if (d.mExtId) {
            json* exts = FindObject(mDoc, "extensions");
            if (!exts) {
                mDoc["extensions"] = json::object();
                exts = FindObject(mDoc, "extensions");
            }

            if (!(container = FindObject(*exts, d.mExtId))) {
                //exts[ d.mExtId ] = json::object();
                container = FindObject(*exts, d.mExtId);
            }
        }

        json* dict;
        if (!(dict = FindObject(*container, d.mDictId))) {
            //container[d.mDictId] = json::object();
            dict = FindObject(*container, d.mDictId);
        }

        for (size_t i = 0; i < d.mObjs.size(); ++i) {
            if (d.mObjs[i]->IsSpecial()) continue;

            json obj;

            if (!d.mObjs[i]->name.empty()) {
                obj["name"] = d.mObjs[i]->name.c_str();
            }

            Write(obj, *d.mObjs[i], *this);

            std::string id = d.mObjs[ i ]->id;
            (*dict)[id] = obj;
        }
    }

    template<class T>
    void WriteLazyDict(LazyDict<T>& d, AssetWriter& w)
    {
        w.WriteObjects(d);
    }

}
