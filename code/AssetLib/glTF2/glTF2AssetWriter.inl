/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

#include <assimp/Base64.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

namespace glTF2 {

    using rapidjson::StringBuffer;
    using rapidjson::PrettyWriter;
    using rapidjson::Writer;
    using rapidjson::StringRef;
    using rapidjson::StringRef;

    namespace {

        template<typename T, size_t N>
        inline Value& MakeValue(Value& val, T(&r)[N], MemoryPoolAllocator<>& al) {
            val.SetArray();
            val.Reserve(N, al);
            for (decltype(N) i = 0; i < N; ++i) {
                val.PushBack(r[i], al);
            }
            return val;
        }

        template<typename T>
        inline Value& MakeValue(Value& val, const std::vector<T> & r, MemoryPoolAllocator<>& al) {
            val.SetArray();
            val.Reserve(static_cast<rapidjson::SizeType>(r.size()), al);
            for (unsigned int i = 0; i < r.size(); ++i) {
                val.PushBack(r[i], al);
            }
            return val;
        }

        template<typename C, typename T>
        inline Value& MakeValueCast(Value& val, const std::vector<T> & r, MemoryPoolAllocator<>& al) {
            val.SetArray();
            val.Reserve(static_cast<rapidjson::SizeType>(r.size()), al);
            for (unsigned int i = 0; i < r.size(); ++i) {
                val.PushBack(static_cast<C>(r[i]), al);
            }
            return val;
        }

        template<typename T>
        inline Value& MakeValue(Value& val, T r, MemoryPoolAllocator<>& /*al*/) {
            val.Set(r);

            return val;
        }

        template<class T>
        inline void AddRefsVector(Value& obj, const char* fieldId, std::vector< Ref<T> >& v, MemoryPoolAllocator<>& al) {
            if (v.empty()) return;
            Value lst;
            lst.SetArray();
            lst.Reserve(unsigned(v.size()), al);
            for (size_t i = 0; i < v.size(); ++i) {
                lst.PushBack(v[i]->index, al);
            }
            obj.AddMember(StringRef(fieldId), lst, al);
        }


    }

    inline void Write(Value& obj, Accessor& a, AssetWriter& w)
    {
        if (a.bufferView) {
            obj.AddMember("bufferView", a.bufferView->index, w.mAl);
            obj.AddMember("byteOffset", (unsigned int)a.byteOffset, w.mAl);
        }
        obj.AddMember("componentType", int(a.componentType), w.mAl);
        obj.AddMember("count", (unsigned int)a.count, w.mAl);
        obj.AddMember("type", StringRef(AttribType::ToString(a.type)), w.mAl);
        Value vTmpMax, vTmpMin;
        if (a.componentType == ComponentType_FLOAT) {
            obj.AddMember("max", MakeValue(vTmpMax, a.max, w.mAl), w.mAl);
            obj.AddMember("min", MakeValue(vTmpMin, a.min, w.mAl), w.mAl);
        } else {
            obj.AddMember("max", MakeValueCast<int64_t>(vTmpMax, a.max, w.mAl), w.mAl);
            obj.AddMember("min", MakeValueCast<int64_t>(vTmpMin, a.min, w.mAl), w.mAl);
        }

        if (a.sparse) {
            Value sparseValue;
            sparseValue.SetObject();

            //count
            sparseValue.AddMember("count", (unsigned int)a.sparse->count, w.mAl);

            //indices
            Value indices;
            indices.SetObject();
            indices.AddMember("bufferView", a.sparse->indices->index, w.mAl);
            indices.AddMember("byteOffset", (unsigned int)a.sparse->indicesByteOffset, w.mAl);
            indices.AddMember("componentType", int(a.sparse->indicesType), w.mAl);
            sparseValue.AddMember("indices", indices, w.mAl);

            //values
            Value values;
            values.SetObject();
            values.AddMember("bufferView", a.sparse->values->index, w.mAl);
            values.AddMember("byteOffset", (unsigned int)a.sparse->valuesByteOffset, w.mAl);
            sparseValue.AddMember("values", values, w.mAl);

            obj.AddMember("sparse", sparseValue, w.mAl);
        }
    }

    inline void Write(Value& obj, Animation& a, AssetWriter& w)
    {
        /****************** Channels *******************/
        Value channels;
        channels.SetArray();
        channels.Reserve(unsigned(a.channels.size()), w.mAl);

        for (size_t i = 0; i < unsigned(a.channels.size()); ++i) {
            Animation::Channel& c = a.channels[i];
            Value valChannel;
            valChannel.SetObject();
            {
                valChannel.AddMember("sampler", c.sampler, w.mAl);

                Value valTarget;
                valTarget.SetObject();
                {
                    valTarget.AddMember("node", c.target.node->index, w.mAl);
                    switch (c.target.path) {
                        case AnimationPath_TRANSLATION:
                            valTarget.AddMember("path", "translation", w.mAl);
                            break;
                        case AnimationPath_ROTATION:
                            valTarget.AddMember("path", "rotation", w.mAl);
                            break;
                        case AnimationPath_SCALE:
                            valTarget.AddMember("path", "scale", w.mAl);
                            break;
                        case AnimationPath_WEIGHTS:
                            valTarget.AddMember("path", "weights", w.mAl);
                            break;
                    }
                }
                valChannel.AddMember("target", valTarget, w.mAl);
            }
            channels.PushBack(valChannel, w.mAl);
        }
        obj.AddMember("channels", channels, w.mAl);

        /****************** Samplers *******************/
        Value valSamplers;
        valSamplers.SetArray();

        for (size_t i = 0; i < unsigned(a.samplers.size()); ++i) {
            Animation::Sampler& s = a.samplers[i];
            Value valSampler;
            valSampler.SetObject();
            {
                valSampler.AddMember("input", s.input->index, w.mAl);
                switch (s.interpolation) {
                    case Interpolation_LINEAR:
                        valSampler.AddMember("interpolation", "LINEAR", w.mAl);
                        break;
                    case Interpolation_STEP:
                        valSampler.AddMember("interpolation", "STEP", w.mAl);
                        break;
                    case Interpolation_CUBICSPLINE:
                        valSampler.AddMember("interpolation", "CUBICSPLINE", w.mAl);
                        break;
                }
                valSampler.AddMember("output", s.output->index, w.mAl);
            }
            valSamplers.PushBack(valSampler, w.mAl);
        }
        obj.AddMember("samplers", valSamplers, w.mAl);
    }

    inline void Write(Value& obj, Buffer& b, AssetWriter& w)
    {
        obj.AddMember("byteLength", static_cast<uint64_t>(b.byteLength), w.mAl);

        const auto uri = b.GetURI();
        const auto relativeUri = uri.substr(uri.find_last_of("/\\") + 1u);
        obj.AddMember("uri", Value(relativeUri, w.mAl).Move(), w.mAl);
    }

    inline void Write(Value& obj, BufferView& bv, AssetWriter& w)
    {
        obj.AddMember("buffer", bv.buffer->index, w.mAl);
        obj.AddMember("byteOffset", static_cast<uint64_t>(bv.byteOffset), w.mAl);
        obj.AddMember("byteLength", static_cast<uint64_t>(bv.byteLength), w.mAl);
        if (bv.byteStride != 0) {
            obj.AddMember("byteStride", bv.byteStride, w.mAl);
        }
        if (bv.target != BufferViewTarget_NONE) {
            obj.AddMember("target", int(bv.target), w.mAl);
        }
    }

    inline void Write(Value& /*obj*/, Camera& /*c*/, AssetWriter& /*w*/)
    {

    }

    inline void Write(Value& /*obj*/, Light& /*c*/, AssetWriter& /*w*/)
    {

    }

    inline void Write(Value& obj, Image& img, AssetWriter& w)
    {
        //basisu: no need to handle .ktx2, .basis, write as is
        if (img.bufferView) {
            obj.AddMember("bufferView", img.bufferView->index, w.mAl);
            obj.AddMember("mimeType", Value(img.mimeType, w.mAl).Move(), w.mAl);
        }
        else {
            std::string uri;
            if (img.HasData()) {
                uri = "data:" + (img.mimeType.empty() ? "application/octet-stream" : img.mimeType);
                uri += ";base64,";
                Base64::Encode(img.GetData(), img.GetDataLength(), uri);
            }
            else {
                uri = img.uri;
            }

            obj.AddMember("uri", Value(uri, w.mAl).Move(), w.mAl);
        }
    }

    namespace {
        inline void SetTexBasic(TextureInfo t, Value& tex, MemoryPoolAllocator<>& al)
        {
            tex.SetObject();
            tex.AddMember("index", t.texture->index, al);

            if (t.texCoord != 0) {
                tex.AddMember("texCoord", t.texCoord, al);
            }
        }

        inline void WriteTex(Value& obj, TextureInfo t, const char* propName, MemoryPoolAllocator<>& al)
        {

            if (t.texture) {
                Value tex;

                SetTexBasic(t, tex, al);

                obj.AddMember(StringRef(propName), tex, al);
            }
        }

        inline void WriteTex(Value& obj, NormalTextureInfo t, const char* propName, MemoryPoolAllocator<>& al)
        {

            if (t.texture) {
                Value tex;

                SetTexBasic(t, tex, al);

                if (t.scale != 1) {
                    tex.AddMember("scale", t.scale, al);
                }

                obj.AddMember(StringRef(propName), tex, al);
            }
        }

        inline void WriteTex(Value& obj, OcclusionTextureInfo t, const char* propName, MemoryPoolAllocator<>& al)
        {

            if (t.texture) {
                Value tex;

                SetTexBasic(t, tex, al);

                if (t.strength != 1) {
                    tex.AddMember("strength", t.strength, al);
                }

                obj.AddMember(StringRef(propName), tex, al);
            }
        }

        template<size_t N>
        inline void WriteVec(Value& obj, float(&prop)[N], const char* propName, MemoryPoolAllocator<>& al)
        {
            Value arr;
            obj.AddMember(StringRef(propName), MakeValue(arr, prop, al), al);
        }

        template<size_t N>
        inline void WriteVec(Value& obj, float(&prop)[N], const char* propName, const float(&defaultVal)[N], MemoryPoolAllocator<>& al)
        {
            if (!std::equal(std::begin(prop), std::end(prop), std::begin(defaultVal))) {
                WriteVec(obj, prop, propName, al);
            }
        }

        inline void WriteFloat(Value& obj, float prop, const char* propName, MemoryPoolAllocator<>& al)
        {
            Value num;
            obj.AddMember(StringRef(propName), MakeValue(num, prop, al), al);
        }
    }

    inline void Write(Value& obj, Material& m, AssetWriter& w)
    {
        Value pbrMetallicRoughness;
        pbrMetallicRoughness.SetObject();
        {
            WriteTex(pbrMetallicRoughness, m.pbrMetallicRoughness.baseColorTexture, "baseColorTexture", w.mAl);
            WriteTex(pbrMetallicRoughness, m.pbrMetallicRoughness.metallicRoughnessTexture, "metallicRoughnessTexture", w.mAl);
            WriteVec(pbrMetallicRoughness, m.pbrMetallicRoughness.baseColorFactor, "baseColorFactor", defaultBaseColor, w.mAl);

            if (m.pbrMetallicRoughness.metallicFactor != 1) {
                WriteFloat(pbrMetallicRoughness, m.pbrMetallicRoughness.metallicFactor, "metallicFactor", w.mAl);
            }

            if (m.pbrMetallicRoughness.roughnessFactor != 1) {
                WriteFloat(pbrMetallicRoughness, m.pbrMetallicRoughness.roughnessFactor, "roughnessFactor", w.mAl);
            }
        }

        if (!pbrMetallicRoughness.ObjectEmpty()) {
            obj.AddMember("pbrMetallicRoughness", pbrMetallicRoughness, w.mAl);
        }

        WriteTex(obj, m.normalTexture, "normalTexture", w.mAl);
        WriteTex(obj, m.emissiveTexture, "emissiveTexture", w.mAl);
        WriteTex(obj, m.occlusionTexture, "occlusionTexture", w.mAl);
        WriteVec(obj, m.emissiveFactor, "emissiveFactor", defaultEmissiveFactor, w.mAl);

        if (m.alphaCutoff != 0.5) {
            WriteFloat(obj, m.alphaCutoff, "alphaCutoff", w.mAl);
        }

        if (m.alphaMode != "OPAQUE") {
            obj.AddMember("alphaMode", Value(m.alphaMode, w.mAl).Move(), w.mAl);
        }

        if (m.doubleSided) {
            obj.AddMember("doubleSided", m.doubleSided, w.mAl);
        }

        Value exts;
        exts.SetObject();

        if (m.pbrSpecularGlossiness.isPresent) {
            Value pbrSpecularGlossiness;
            pbrSpecularGlossiness.SetObject();

            PbrSpecularGlossiness &pbrSG = m.pbrSpecularGlossiness.value;

            //pbrSpecularGlossiness
            WriteVec(pbrSpecularGlossiness, pbrSG.diffuseFactor, "diffuseFactor", defaultDiffuseFactor, w.mAl);
            WriteVec(pbrSpecularGlossiness, pbrSG.specularFactor, "specularFactor", defaultSpecularFactor, w.mAl);

            if (pbrSG.glossinessFactor != 1) {
                WriteFloat(pbrSpecularGlossiness, pbrSG.glossinessFactor, "glossinessFactor", w.mAl);
            }

            WriteTex(pbrSpecularGlossiness, pbrSG.diffuseTexture, "diffuseTexture", w.mAl);
            WriteTex(pbrSpecularGlossiness, pbrSG.specularGlossinessTexture, "specularGlossinessTexture", w.mAl);

            if (!pbrSpecularGlossiness.ObjectEmpty()) {
                exts.AddMember("KHR_materials_pbrSpecularGlossiness", pbrSpecularGlossiness, w.mAl);
            }
        }

        if (m.unlit) {
          Value unlit;
          unlit.SetObject();
          exts.AddMember("KHR_materials_unlit", unlit, w.mAl);
        }

        if (m.materialSpecular.isPresent) {
            Value materialSpecular(rapidjson::Type::kObjectType);
            materialSpecular.SetObject();

            MaterialSpecular &specular = m.materialSpecular.value;

            if (specular.specularFactor != 0.0f) {
                WriteFloat(materialSpecular, specular.specularFactor, "specularFactor", w.mAl);
            }
            if (specular.specularColorFactor[0] != defaultSpecularColorFactor[0] && specular.specularColorFactor[1] != defaultSpecularColorFactor[1] && specular.specularColorFactor[2] != defaultSpecularColorFactor[2]) {
                WriteVec(materialSpecular, specular.specularColorFactor, "specularColorFactor", w.mAl);
            }

            WriteTex(materialSpecular, specular.specularTexture, "specularTexture", w.mAl);
            WriteTex(materialSpecular, specular.specularColorTexture, "specularColorTexture", w.mAl);

            if (!materialSpecular.ObjectEmpty()) {
                exts.AddMember("KHR_materials_specular", materialSpecular, w.mAl);
            }
        }

        if (m.materialSheen.isPresent) {
            Value materialSheen(rapidjson::Type::kObjectType);

            MaterialSheen &sheen = m.materialSheen.value;

            WriteVec(materialSheen, sheen.sheenColorFactor, "sheenColorFactor", defaultSheenFactor, w.mAl);

            if (sheen.sheenRoughnessFactor != 0.f) {
                WriteFloat(materialSheen, sheen.sheenRoughnessFactor, "sheenRoughnessFactor", w.mAl);
            }

            WriteTex(materialSheen, sheen.sheenColorTexture, "sheenColorTexture", w.mAl);
            WriteTex(materialSheen, sheen.sheenRoughnessTexture, "sheenRoughnessTexture", w.mAl);

            if (!materialSheen.ObjectEmpty()) {
                exts.AddMember("KHR_materials_sheen", materialSheen, w.mAl);
            }
        }

        if (m.materialClearcoat.isPresent) {
            Value materialClearcoat(rapidjson::Type::kObjectType);

            MaterialClearcoat &clearcoat = m.materialClearcoat.value;

            if (clearcoat.clearcoatFactor != 0.f) {
                WriteFloat(materialClearcoat, clearcoat.clearcoatFactor, "clearcoatFactor", w.mAl);
            }

            if (clearcoat.clearcoatRoughnessFactor != 0.f) {
                WriteFloat(materialClearcoat, clearcoat.clearcoatRoughnessFactor, "clearcoatRoughnessFactor", w.mAl);
            }

            WriteTex(materialClearcoat, clearcoat.clearcoatTexture, "clearcoatTexture", w.mAl);
            WriteTex(materialClearcoat, clearcoat.clearcoatRoughnessTexture, "clearcoatRoughnessTexture", w.mAl);
            WriteTex(materialClearcoat, clearcoat.clearcoatNormalTexture, "clearcoatNormalTexture", w.mAl);

            if (!materialClearcoat.ObjectEmpty()) {
                exts.AddMember("KHR_materials_clearcoat", materialClearcoat, w.mAl);
            }
        }

        if (m.materialTransmission.isPresent) {
            Value materialTransmission(rapidjson::Type::kObjectType);

            MaterialTransmission &transmission = m.materialTransmission.value;

            if (transmission.transmissionFactor != 0.f) {
                WriteFloat(materialTransmission, transmission.transmissionFactor, "transmissionFactor", w.mAl);
            }

            WriteTex(materialTransmission, transmission.transmissionTexture, "transmissionTexture", w.mAl);

            if (!materialTransmission.ObjectEmpty()) {
                exts.AddMember("KHR_materials_transmission", materialTransmission, w.mAl);
            }
        }

        if (m.materialVolume.isPresent) {
            Value materialVolume(rapidjson::Type::kObjectType);

            MaterialVolume &volume = m.materialVolume.value;

            if (volume.thicknessFactor != 0.f) {
                WriteFloat(materialVolume, volume.thicknessFactor, "thicknessFactor", w.mAl);
            }

            WriteTex(materialVolume, volume.thicknessTexture, "thicknessTexture", w.mAl);

            if (volume.attenuationDistance != std::numeric_limits<float>::infinity()) {
                WriteFloat(materialVolume, volume.attenuationDistance, "attenuationDistance", w.mAl);
            }

            WriteVec(materialVolume, volume.attenuationColor, "attenuationColor", defaultAttenuationColor, w.mAl);

            if (!materialVolume.ObjectEmpty()) {
                exts.AddMember("KHR_materials_volume", materialVolume, w.mAl);
            }
        }

        if (m.materialIOR.isPresent) {
            Value materialIOR(rapidjson::Type::kObjectType);

            MaterialIOR &ior = m.materialIOR.value;

            if (ior.ior != 1.5f) {
                WriteFloat(materialIOR, ior.ior, "ior", w.mAl);
            }

            if (!materialIOR.ObjectEmpty()) {
                exts.AddMember("KHR_materials_ior", materialIOR, w.mAl);
            }
        }

        if (m.materialEmissiveStrength.isPresent) {
            Value materialEmissiveStrength(rapidjson::Type::kObjectType);

            MaterialEmissiveStrength &emissiveStrength = m.materialEmissiveStrength.value;

            if (emissiveStrength.emissiveStrength != 0.f) {
                WriteFloat(materialEmissiveStrength, emissiveStrength.emissiveStrength, "emissiveStrength", w.mAl);
            }

            if (!materialEmissiveStrength.ObjectEmpty()) {
                exts.AddMember("KHR_materials_emissive_strength", materialEmissiveStrength, w.mAl);
            }
        }

        if (m.materialAnisotropy.isPresent) {
            Value materialAnisotropy(rapidjson::Type::kObjectType);

            MaterialAnisotropy &anisotropy = m.materialAnisotropy.value;

            if (anisotropy.anisotropyStrength != 0.f) {
                WriteFloat(materialAnisotropy, anisotropy.anisotropyStrength, "anisotropyStrength", w.mAl);
            }

            if (anisotropy.anisotropyRotation != 0.f) {
                WriteFloat(materialAnisotropy, anisotropy.anisotropyRotation, "anisotropyRotation", w.mAl);
            }

            WriteTex(materialAnisotropy, anisotropy.anisotropyTexture, "anisotropyTexture", w.mAl);

            if (!materialAnisotropy.ObjectEmpty()) {
                exts.AddMember("KHR_materials_anisotropy", materialAnisotropy, w.mAl);
            }
        }

        if (!exts.ObjectEmpty()) {
            obj.AddMember("extensions", exts, w.mAl);
        }
    }

    namespace {
        inline void WriteAttrs(AssetWriter& w, Value& attrs, Mesh::AccessorList& lst,
            const char* semantic, bool forceNumber = false)
        {
            if (lst.empty()) return;
            if (lst.size() == 1 && !forceNumber) {
                attrs.AddMember(StringRef(semantic), lst[0]->index, w.mAl);
            }
            else {
                for (size_t i = 0; i < lst.size(); ++i) {
                    char buffer[32];
                    ai_snprintf(buffer, 32, "%s_%d", semantic, int(i));
                    attrs.AddMember(Value(buffer, w.mAl).Move(), lst[i]->index, w.mAl);
                }
            }
        }
    }

    inline void Write(Value& obj, Mesh& m, AssetWriter& w)
    {
        /****************** Primitives *******************/
        Value primitives;
        primitives.SetArray();
        primitives.Reserve(unsigned(m.primitives.size()), w.mAl);

        for (size_t i = 0; i < m.primitives.size(); ++i) {
            Mesh::Primitive& p = m.primitives[i];
            Value prim;
            prim.SetObject();

            // Extensions
            if (p.ngonEncoded)
            {
                Value exts;
                exts.SetObject();

                Value FB_ngon_encoding;
                FB_ngon_encoding.SetObject();

                exts.AddMember(StringRef("FB_ngon_encoding"), FB_ngon_encoding, w.mAl);
                prim.AddMember("extensions", exts, w.mAl);
            }

            {
                prim.AddMember("mode", Value(int(p.mode)).Move(), w.mAl);

                if (p.material)
                    prim.AddMember("material", p.material->index, w.mAl);

                if (p.indices)
                    prim.AddMember("indices", p.indices->index, w.mAl);

                Value attrs;
                attrs.SetObject();
                {
                    WriteAttrs(w, attrs, p.attributes.position, "POSITION");
                    WriteAttrs(w, attrs, p.attributes.normal, "NORMAL");
                    WriteAttrs(w, attrs, p.attributes.tangent, "TANGENT");
                    WriteAttrs(w, attrs, p.attributes.texcoord, "TEXCOORD", true);
                    WriteAttrs(w, attrs, p.attributes.color, "COLOR", true);
                    WriteAttrs(w, attrs, p.attributes.joint, "JOINTS", true);
                    WriteAttrs(w, attrs, p.attributes.weight, "WEIGHTS", true);
                }
                prim.AddMember("attributes", attrs, w.mAl);

                // targets for blendshapes
                if (p.targets.size() > 0) {
                    Value tjs;
                    tjs.SetArray();
                    tjs.Reserve(unsigned(p.targets.size()), w.mAl);
                    for (unsigned int t = 0; t < p.targets.size(); ++t) {
                        Value tj;
                        tj.SetObject();
                        {
                            WriteAttrs(w, tj, p.targets[t].position, "POSITION");
                            WriteAttrs(w, tj, p.targets[t].normal, "NORMAL");
                            WriteAttrs(w, tj, p.targets[t].tangent, "TANGENT");
                        }
                        tjs.PushBack(tj, w.mAl);
                    }
                    prim.AddMember("targets", tjs, w.mAl);
                }
            }
            primitives.PushBack(prim, w.mAl);
        }

        obj.AddMember("primitives", primitives, w.mAl);
        // targetNames
        if (m.targetNames.size() > 0) {
            Value extras;
            extras.SetObject();
            Value targetNames;
            targetNames.SetArray();
            targetNames.Reserve(unsigned(m.targetNames.size()), w.mAl);
            for (unsigned int n = 0; n < m.targetNames.size(); ++n) {
                std::string name = m.targetNames[n];
                Value tname;
                tname.SetString(name.c_str(), w.mAl);
                targetNames.PushBack(tname, w.mAl);
            }
            extras.AddMember("targetNames", targetNames, w.mAl);
            obj.AddMember("extras", extras, w.mAl);
        }
    }

    inline void WriteExtrasValue(Value &parent, const CustomExtension &value, AssetWriter &w) {
        Value valueNode;

        if (value.mStringValue.isPresent) {
            MakeValue(valueNode, value.mStringValue.value.c_str(), w.mAl);
        } else if (value.mDoubleValue.isPresent) {
            MakeValue(valueNode, value.mDoubleValue.value, w.mAl);
        } else if (value.mUint64Value.isPresent) {
            MakeValue(valueNode, value.mUint64Value.value, w.mAl);
        } else if (value.mInt64Value.isPresent) {
            MakeValue(valueNode, value.mInt64Value.value, w.mAl);
        } else if (value.mBoolValue.isPresent) {
            MakeValue(valueNode, value.mBoolValue.value, w.mAl);
        } else if (value.mValues.isPresent) {
            valueNode.SetObject();
            for (auto const &subvalue : value.mValues.value) {
                WriteExtrasValue(valueNode, subvalue, w);
            }
        }

        parent.AddMember(StringRef(value.name), valueNode, w.mAl);
    }

    inline void WriteExtras(Value &obj, const Extras &extras, AssetWriter &w) {
        if (!extras.HasExtras()) {
            return;
        }

        Value extrasNode;
        extrasNode.SetObject();

        for (auto const &value : extras.mValues) {
            WriteExtrasValue(extrasNode, value, w);
        }
        
        obj.AddMember("extras", extrasNode, w.mAl);
    }

    inline void Write(Value& obj, Node& n, AssetWriter& w)
    {
        if (n.matrix.isPresent) {
            Value val;
            obj.AddMember("matrix", MakeValue(val, n.matrix.value, w.mAl).Move(), w.mAl);
        }

        if (n.translation.isPresent) {
            Value val;
            obj.AddMember("translation", MakeValue(val, n.translation.value, w.mAl).Move(), w.mAl);
        }

        if (n.scale.isPresent) {
            Value val;
            obj.AddMember("scale", MakeValue(val, n.scale.value, w.mAl).Move(), w.mAl);
        }
        if (n.rotation.isPresent) {
            Value val;
            obj.AddMember("rotation", MakeValue(val, n.rotation.value, w.mAl).Move(), w.mAl);
        }

        AddRefsVector(obj, "children", n.children, w.mAl);

        if (!n.meshes.empty()) {
            obj.AddMember("mesh", n.meshes[0]->index, w.mAl);
        }

        if (n.skin) {
            obj.AddMember("skin", n.skin->index, w.mAl);
        }

        //gltf2 spec does not support "skeletons" under node
        if(n.skeletons.size()) {
            AddRefsVector(obj, "skeletons", n.skeletons, w.mAl);
        }

        WriteExtras(obj, n.extras, w);
    }

    inline void Write(Value& /*obj*/, Program& /*b*/, AssetWriter& /*w*/)
    {

    }

    inline void Write(Value& obj, Sampler& b, AssetWriter& w)
    {
        if (!b.name.empty()) {
            obj.AddMember("name", b.name, w.mAl);
        }

        if (b.wrapS != SamplerWrap::UNSET && b.wrapS != SamplerWrap::Repeat) {
            obj.AddMember("wrapS", static_cast<unsigned int>(b.wrapS), w.mAl);
        }

        if (b.wrapT != SamplerWrap::UNSET && b.wrapT != SamplerWrap::Repeat) {
            obj.AddMember("wrapT", static_cast<unsigned int>(b.wrapT), w.mAl);
        }

        if (b.magFilter != SamplerMagFilter::UNSET) {
            obj.AddMember("magFilter", static_cast<unsigned int>(b.magFilter), w.mAl);
        }

        if (b.minFilter != SamplerMinFilter::UNSET) {
            obj.AddMember("minFilter", static_cast<unsigned int>(b.minFilter), w.mAl);
        }
    }

    inline void Write(Value& scene, Scene& s, AssetWriter& w)
    {
        AddRefsVector(scene, "nodes", s.nodes, w.mAl);
    }

    inline void Write(Value& /*obj*/, Shader& /*b*/, AssetWriter& /*w*/)
    {

    }

    inline void Write(Value& obj, Skin& b, AssetWriter& w)
    {
        /****************** jointNames *******************/
        Value vJointNames;
        vJointNames.SetArray();
        vJointNames.Reserve(unsigned(b.jointNames.size()), w.mAl);

        for (size_t i = 0; i < unsigned(b.jointNames.size()); ++i) {
            vJointNames.PushBack(b.jointNames[i]->index, w.mAl);
        }
        obj.AddMember("joints", vJointNames, w.mAl);

        if (b.bindShapeMatrix.isPresent) {
            Value val;
            obj.AddMember("bindShapeMatrix", MakeValue(val, b.bindShapeMatrix.value, w.mAl).Move(), w.mAl);
        }

        if (b.inverseBindMatrices) {
            obj.AddMember("inverseBindMatrices", b.inverseBindMatrices->index, w.mAl);
        }

    }

    inline void Write(Value& obj, Texture& tex, AssetWriter& w)
    {
        if (tex.source) {
            obj.AddMember("source", tex.source->index, w.mAl);
        }
        if (tex.sampler) {
            obj.AddMember("sampler", tex.sampler->index, w.mAl);
        }
    }

    inline AssetWriter::AssetWriter(Asset& a)
        : mDoc()
        , mAsset(a)
        , mAl(mDoc.GetAllocator())
    {
        mDoc.SetObject();

        WriteMetadata();
        WriteExtensionsUsed();

        // Dump the contents of the dictionaries
        for (size_t i = 0; i < a.mDicts.size(); ++i) {
            a.mDicts[i]->WriteObjects(*this);
        }

        // Add the target scene field
        if (mAsset.scene) {
            mDoc.AddMember("scene", mAsset.scene->index, mAl);
        }

        if(mAsset.extras) {
            mDoc.AddMember("extras", *mAsset.extras, mAl);
        }
    }

    inline void AssetWriter::WriteFile(const char* path)
    {
        std::unique_ptr<IOStream> jsonOutFile(mAsset.OpenFile(path, "wt", true));

        if (jsonOutFile == nullptr) {
            throw DeadlyExportError("Could not open output file: " + std::string(path));
        }

        StringBuffer docBuffer;

        PrettyWriter<StringBuffer> writer(docBuffer);
        if (!mDoc.Accept(writer)) {
            throw DeadlyExportError("Failed to write scene data!");
        }

        if (jsonOutFile->Write(docBuffer.GetString(), docBuffer.GetSize(), 1) != 1) {
            throw DeadlyExportError("Failed to write scene data!");
        }

        // Write buffer data to separate .bin files
        for (unsigned int i = 0; i < mAsset.buffers.Size(); ++i) {
            Ref<Buffer> b = mAsset.buffers.Get(i);

            std::string binPath = b->GetURI();

            std::unique_ptr<IOStream> binOutFile(mAsset.OpenFile(binPath, "wb", true));

            if (binOutFile == nullptr) {
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

        if (outfile == nullptr) {
            throw DeadlyExportError("Could not open output file: " + std::string(path));
        }

        Ref<Buffer> bodyBuffer = mAsset.GetBodyBuffer();
        if (bodyBuffer->byteLength > 0) {
            rapidjson::Value glbBodyBuffer;
            glbBodyBuffer.SetObject();
            glbBodyBuffer.AddMember("byteLength", static_cast<uint64_t>(bodyBuffer->byteLength), mAl);
            mDoc["buffers"].PushBack(glbBodyBuffer, mAl);
        }

        // Padding with spaces as required by the spec
        uint32_t padding = 0x20202020;

        //
        // JSON chunk
        //

        StringBuffer docBuffer;
        Writer<StringBuffer> writer(docBuffer);
        if (!mDoc.Accept(writer)) {
            throw DeadlyExportError("Failed to write scene data!");
        }

        uint32_t jsonChunkLength = static_cast<uint32_t>((docBuffer.GetSize() + 3) & ~3); // Round up to next multiple of 4
        auto paddingLength = jsonChunkLength - docBuffer.GetSize();

        GLB_Chunk jsonChunk;
        jsonChunk.chunkLength = jsonChunkLength;
        jsonChunk.chunkType = ChunkType_JSON;
        AI_SWAP4(jsonChunk.chunkLength);

        outfile->Seek(sizeof(GLB_Header), aiOrigin_SET);
        if (outfile->Write(&jsonChunk, 1, sizeof(GLB_Chunk)) != sizeof(GLB_Chunk)) {
            throw DeadlyExportError("Failed to write scene data header!");
        }
        if (outfile->Write(docBuffer.GetString(), 1, docBuffer.GetSize()) != docBuffer.GetSize()) {
            throw DeadlyExportError("Failed to write scene data!");
        }
        if (paddingLength && outfile->Write(&padding, 1, paddingLength) != paddingLength) {
            throw DeadlyExportError("Failed to write scene data padding!");
        }

        //
        // Binary chunk
        //

        int GLB_Chunk_count = 1;
        uint32_t binaryChunkLength = 0;
        if (bodyBuffer->byteLength > 0) {
            binaryChunkLength = static_cast<uint32_t>((bodyBuffer->byteLength + 3) & ~3); // Round up to next multiple of 4

            auto curPaddingLength = binaryChunkLength - bodyBuffer->byteLength;
            ++GLB_Chunk_count;

            GLB_Chunk binaryChunk;
            binaryChunk.chunkLength = binaryChunkLength;
            binaryChunk.chunkType = ChunkType_BIN;
            AI_SWAP4(binaryChunk.chunkLength);

            size_t bodyOffset = sizeof(GLB_Header) + sizeof(GLB_Chunk) + jsonChunk.chunkLength;
            outfile->Seek(bodyOffset, aiOrigin_SET);
            if (outfile->Write(&binaryChunk, 1, sizeof(GLB_Chunk)) != sizeof(GLB_Chunk)) {
                throw DeadlyExportError("Failed to write body data header!");
            }
            if (outfile->Write(bodyBuffer->GetPointer(), 1, bodyBuffer->byteLength) != bodyBuffer->byteLength) {
                throw DeadlyExportError("Failed to write body data!");
            }
            if (curPaddingLength && outfile->Write(&padding, 1, curPaddingLength) != curPaddingLength) {
                throw DeadlyExportError("Failed to write body data padding!");
            }
        }

        //
        // Header
        //

        GLB_Header header;
        memcpy(header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic));

        header.version = 2;
        AI_SWAP4(header.version);

        header.length = uint32_t(sizeof(GLB_Header) + GLB_Chunk_count * sizeof(GLB_Chunk) + jsonChunkLength + binaryChunkLength);
        AI_SWAP4(header.length);

        outfile->Seek(0, aiOrigin_SET);
        if (outfile->Write(&header, 1, sizeof(GLB_Header)) != sizeof(GLB_Header)) {
            throw DeadlyExportError("Failed to write the header!");
        }
    }

    inline void AssetWriter::WriteMetadata()
    {
        Value asset;
        asset.SetObject();
        asset.AddMember("version", Value(mAsset.asset.version, mAl).Move(), mAl);
        asset.AddMember("generator", Value(mAsset.asset.generator, mAl).Move(), mAl);
        if (!mAsset.asset.copyright.empty())
            asset.AddMember("copyright", Value(mAsset.asset.copyright, mAl).Move(), mAl);
        mDoc.AddMember("asset", asset, mAl);
    }

    inline void AssetWriter::WriteExtensionsUsed()
    {
        Value exts;
        exts.SetArray();
        {
            // This is used to export pbrSpecularGlossiness materials with GLTF 2.
            if (this->mAsset.extensionsUsed.KHR_materials_pbrSpecularGlossiness) {
                exts.PushBack(StringRef("KHR_materials_pbrSpecularGlossiness"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_unlit) {
              exts.PushBack(StringRef("KHR_materials_unlit"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_specular) {
                exts.PushBack(StringRef("KHR_materials_specular"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_sheen) {
                exts.PushBack(StringRef("KHR_materials_sheen"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_clearcoat) {
                exts.PushBack(StringRef("KHR_materials_clearcoat"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_transmission) {
                exts.PushBack(StringRef("KHR_materials_transmission"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_volume) {
                exts.PushBack(StringRef("KHR_materials_volume"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_ior) {
                exts.PushBack(StringRef("KHR_materials_ior"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_emissive_strength) {
                exts.PushBack(StringRef("KHR_materials_emissive_strength"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_materials_anisotropy) {
                exts.PushBack(StringRef("KHR_materials_anisotropy"), mAl);
            }

            if (this->mAsset.extensionsUsed.FB_ngon_encoding) {
                exts.PushBack(StringRef("FB_ngon_encoding"), mAl);
            }

            if (this->mAsset.extensionsUsed.KHR_texture_basisu) {
                exts.PushBack(StringRef("KHR_texture_basisu"), mAl);
            }
        }

        if (!exts.Empty())
            mDoc.AddMember("extensionsUsed", exts, mAl);

        //basisu extensionRequired
        Value extsReq;
        extsReq.SetArray();
        if (this->mAsset.extensionsUsed.KHR_texture_basisu) {
            extsReq.PushBack(StringRef("KHR_texture_basisu"), mAl);
            mDoc.AddMember("extensionsRequired", extsReq, mAl);
        }
    }

    template<class T>
    void AssetWriter::WriteObjects(LazyDict<T>& d)
    {
        if (d.mObjs.empty()) return;

        Value* container = &mDoc;
        const char* context = "Document";

        if (d.mExtId) {
            Value* exts = FindObject(mDoc, "extensions");
            if (nullptr != exts) {
                mDoc.AddMember("extensions", Value().SetObject().Move(), mDoc.GetAllocator());
                exts = FindObject(mDoc, "extensions");
            }

            container = FindObjectInContext(*exts, d.mExtId, "extensions");
            if (nullptr != container) {
                exts->AddMember(StringRef(d.mExtId), Value().SetObject().Move(), mDoc.GetAllocator());
                container = FindObjectInContext(*exts, d.mExtId, "extensions");
                context = d.mExtId;
            }
        }

        Value *dict = FindArrayInContext(*container, d.mDictId, context);
        if (nullptr == dict) {
            container->AddMember(StringRef(d.mDictId), Value().SetArray().Move(), mDoc.GetAllocator());
            dict = FindArrayInContext(*container, d.mDictId, context);
            if (nullptr == dict) {
                return;
            }
        }

        for (size_t i = 0; i < d.mObjs.size(); ++i) {
            if (d.mObjs[i]->IsSpecial()) {
                continue;
            }

            Value obj;
            obj.SetObject();

            if (!d.mObjs[i]->name.empty()) {
                obj.AddMember("name", StringRef(d.mObjs[i]->name.c_str()), mAl);
            }

            Write(obj, *d.mObjs[i], *this);

            dict->PushBack(obj, mAl);
        }
    }

    template<class T>
    void WriteLazyDict(LazyDict<T>& d, AssetWriter& w)
    {
        w.WriteObjects(d);
    }

}
