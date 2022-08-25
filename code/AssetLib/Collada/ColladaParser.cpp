/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

/** @file ColladaParser.cpp
 *  @brief Implementation of the Collada parser helper
 */

#ifndef ASSIMP_BUILD_NO_COLLADA_IMPORTER

#include "ColladaParser.h"
#include <assimp/ParsingUtils.h>
#include <assimp/StringUtils.h>
#include <assimp/ZipArchiveIOSystem.h>
#include <assimp/commonMetaData.h>
#include <assimp/fast_atof.h>
#include <assimp/light.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <memory>

using namespace Assimp;
using namespace Assimp::Collada;
using namespace Assimp::Formatter;

static void ReportWarning(const char *msg, ...) {
    ai_assert(nullptr != msg);

    va_list args;
    va_start(args, msg);

    char szBuffer[3000];
    const int iLen = vsprintf(szBuffer, msg, args);
    ai_assert(iLen > 0);

    va_end(args);
    ASSIMP_LOG_WARN("Validation warning: ", std::string(szBuffer, iLen));
}

static bool FindCommonKey(const std::string &collada_key, const MetaKeyPairVector &key_renaming, size_t &found_index) {
    for (size_t i = 0; i < key_renaming.size(); ++i) {
        if (key_renaming[i].first == collada_key) {
            found_index = i;
            return true;
        }
    }
    found_index = std::numeric_limits<size_t>::max();

    return false;
}

static void readUrlAttribute(XmlNode &node, std::string &url) {
    url.clear();
    if (!XmlParser::getStdStrAttribute(node, "url", url)) {
        return;
    }
    if (url[0] != '#') {
        throw DeadlyImportError("Unknown reference format");
    }
    url = url.c_str() + 1;
}

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ColladaParser::ColladaParser(IOSystem *pIOHandler, const std::string &pFile) :
        mFileName(pFile),
        mXmlParser(),
        mDataLibrary(),
        mAccessorLibrary(),
        mMeshLibrary(),
        mNodeLibrary(),
        mImageLibrary(),
        mEffectLibrary(),
        mMaterialLibrary(),
        mLightLibrary(),
        mCameraLibrary(),
        mControllerLibrary(),
        mRootNode(nullptr),
        mAnims(),
        mUnitSize(1.0f),
        mUpDirection(UP_Y),
        mFormat(FV_1_5_n) {
    if (nullptr == pIOHandler) {
        throw DeadlyImportError("IOSystem is nullptr.");
    }

    std::unique_ptr<IOStream> daefile;
    std::unique_ptr<ZipArchiveIOSystem> zip_archive;

    // Determine type
    std::string extension = BaseImporter::GetExtension(pFile);
    if (extension != "dae") {
        zip_archive.reset(new ZipArchiveIOSystem(pIOHandler, pFile));
    }

    if (zip_archive && zip_archive->isOpen()) {
        std::string dae_filename = ReadZaeManifest(*zip_archive);

        if (dae_filename.empty()) {
            throw DeadlyImportError("Invalid ZAE");
        }

        daefile.reset(zip_archive->Open(dae_filename.c_str()));
        if (daefile == nullptr) {
            throw DeadlyImportError("Invalid ZAE manifest: '", dae_filename, "' is missing");
        }
    } else {
        // attempt to open the file directly
        daefile.reset(pIOHandler->Open(pFile));
        if (daefile.get() == nullptr) {
            throw DeadlyImportError("Failed to open file '", pFile, "'.");
        }
    }

    // generate a XML reader for it
    if (!mXmlParser.parse(daefile.get())) {
        throw DeadlyImportError("Unable to read file, malformed XML");
    }
    // start reading
    XmlNode node = mXmlParser.getRootNode();
    XmlNode colladaNode = node.child("COLLADA");
    if (colladaNode.empty()) {
        return;
    }

    // Read content and embedded textures
    ReadContents(colladaNode);
    if (zip_archive && zip_archive->isOpen()) {
        ReadEmbeddedTextures(*zip_archive);
    }
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ColladaParser::~ColladaParser() {
    for (auto &it : mNodeLibrary) {
        delete it.second;
    }
    for (auto &it : mMeshLibrary) {
        delete it.second;
    }
}

// ------------------------------------------------------------------------------------------------
// Read a ZAE manifest and return the filename to attempt to open
std::string ColladaParser::ReadZaeManifest(ZipArchiveIOSystem &zip_archive) {
    // Open the manifest
    std::unique_ptr<IOStream> manifestfile(zip_archive.Open("manifest.xml"));
    if (manifestfile == nullptr) {
        // No manifest, hope there is only one .DAE inside
        std::vector<std::string> file_list;
        zip_archive.getFileListExtension(file_list, "dae");

        if (file_list.empty()) {
            return std::string();
        }

        return file_list.front();
    }
    XmlParser manifestParser;
    if (!manifestParser.parse(manifestfile.get())) {
        return std::string();
    }

    XmlNode root = manifestParser.getRootNode();
    const std::string &name = root.name();
    if (name != "dae_root") {
        root = *manifestParser.findNode("dae_root");
        if (nullptr == root) {
            return std::string();
        }
        std::string v;
        XmlParser::getValueAsString(root, v);
        aiString ai_str(v);
        UriDecodePath(ai_str);
        return std::string(ai_str.C_Str());
    }

    return std::string();
}

// ------------------------------------------------------------------------------------------------
// Convert a path read from a collada file to the usual representation
void ColladaParser::UriDecodePath(aiString &ss) {
    // TODO: collada spec, p 22. Handle URI correctly.
    // For the moment we're just stripping the file:// away to make it work.
    // Windows doesn't seem to be able to find stuff like
    // 'file://..\LWO\LWO2\MappingModes\earthSpherical.jpg'
    if (0 == strncmp(ss.data, "file://", 7)) {
        ss.length -= 7;
        memmove(ss.data, ss.data + 7, ss.length);
        ss.data[ss.length] = '\0';
    }

    // Maxon Cinema Collada Export writes "file:///C:\andsoon" with three slashes...
    // I need to filter it without destroying linux paths starting with "/somewhere"
    if (ss.data[0] == '/' && isalpha((unsigned char)ss.data[1]) && ss.data[2] == ':') {
        --ss.length;
        ::memmove(ss.data, ss.data + 1, ss.length);
        ss.data[ss.length] = 0;
    }

    // find and convert all %xy special chars
    char *out = ss.data;
    for (const char *it = ss.data; it != ss.data + ss.length; /**/) {
        if (*it == '%' && (it + 3) < ss.data + ss.length) {
            // separate the number to avoid dragging in chars from behind into the parsing
            char mychar[3] = { it[1], it[2], 0 };
            size_t nbr = strtoul16(mychar);
            it += 3;
            *out++ = (char)(nbr & 0xFF);
        } else {
            *out++ = *it++;
        }
    }

    // adjust length and terminator of the shortened string
    *out = 0;
    ai_assert(out > ss.data);
    ss.length = static_cast<ai_uint32>(out - ss.data);
}

// ------------------------------------------------------------------------------------------------
// Reads the contents of the file
void ColladaParser::ReadContents(XmlNode &node) {
    const std::string name = node.name();
    if (name == "COLLADA") {
        std::string version;
        if (XmlParser::getStdStrAttribute(node, "version", version)) {
            aiString v;
            v.Set(version.c_str());
            mAssetMetaData.emplace(AI_METADATA_SOURCE_FORMAT_VERSION, v);
            if (!::strncmp(version.c_str(), "1.5", 3)) {
                mFormat = FV_1_5_n;
                ASSIMP_LOG_DEBUG("Collada schema version is 1.5.n");
            } else if (!::strncmp(version.c_str(), "1.4", 3)) {
                mFormat = FV_1_4_n;
                ASSIMP_LOG_DEBUG("Collada schema version is 1.4.n");
            } else if (!::strncmp(version.c_str(), "1.3", 3)) {
                mFormat = FV_1_3_n;
                ASSIMP_LOG_DEBUG("Collada schema version is 1.3.n");
            }
        }
        ReadStructure(node);
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the structure of the file
void ColladaParser::ReadStructure(XmlNode &node) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "asset") {
            ReadAssetInfo(currentNode);
        } else if (currentName == "library_animations") {
            ReadAnimationLibrary(currentNode);
        } else if (currentName == "library_animation_clips") {
            ReadAnimationClipLibrary(currentNode);
        } else if (currentName == "library_controllers") {
            ReadControllerLibrary(currentNode);
        } else if (currentName == "library_images") {
            ReadImageLibrary(currentNode);
        } else if (currentName == "library_materials") {
            ReadMaterialLibrary(currentNode);
        } else if (currentName == "library_effects") {
            ReadEffectLibrary(currentNode);
        } else if (currentName == "library_geometries") {
            ReadGeometryLibrary(currentNode);
        } else if (currentName == "library_visual_scenes") {
            ReadSceneLibrary(currentNode);
        } else if (currentName == "library_lights") {
            ReadLightLibrary(currentNode);
        } else if (currentName == "library_cameras") {
            ReadCameraLibrary(currentNode);
        } else if (currentName == "library_nodes") {
            ReadSceneNode(currentNode, nullptr); /* some hacking to reuse this piece of code */
        } else if (currentName == "scene") {
            ReadScene(currentNode);
        }
    }

    PostProcessRootAnimations();
    PostProcessControllers();
}

// ------------------------------------------------------------------------------------------------
// Reads asset information such as coordinate system information and legal blah
void ColladaParser::ReadAssetInfo(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "unit") {
            mUnitSize = 1.f;
            std::string tUnitSizeString;
            if (XmlParser::getStdStrAttribute(currentNode, "meter", tUnitSizeString)) {
                try {
                    fast_atoreal_move<ai_real>(tUnitSizeString.data(), mUnitSize);
                } catch (const DeadlyImportError& die) {
                    std::string warning("Collada: Failed to parse meter parameter to real number. Exception:\n");
                    warning.append(die.what());
                    ASSIMP_LOG_WARN(warning.data());
                }
            }
        } else if (currentName == "up_axis") {
            std::string v;
            if (!XmlParser::getValueAsString(currentNode, v)) {
                continue;
            }
            if (v == "X_UP") {
                mUpDirection = UP_X;
            } else if (v == "Z_UP") {
                mUpDirection = UP_Z;
            } else {
                mUpDirection = UP_Y;
            }
        } else if (currentName == "contributor") {
            for (XmlNode currentChildNode : currentNode.children()) {
                ReadMetaDataItem(currentChildNode, mAssetMetaData);
            }
        } else {
            ReadMetaDataItem(currentNode, mAssetMetaData);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a single string metadata item
void ColladaParser::ReadMetaDataItem(XmlNode &node, StringMetaData &metadata) {
    const Collada::MetaKeyPairVector &key_renaming = GetColladaAssimpMetaKeysCamelCase();
    const std::string name = node.name();
    if (name.empty()) {
        return;
    }

    std::string v;
    if (!XmlParser::getValueAsString(node, v)) {
        return;
    }

    v = ai_trim(v);
    aiString aistr;
    aistr.Set(v);

    std::string camel_key_str(name);
    ToCamelCase(camel_key_str);

    size_t found_index;
    if (FindCommonKey(camel_key_str, key_renaming, found_index)) {
        metadata.emplace(key_renaming[found_index].second, aistr);
    } else {
        metadata.emplace(camel_key_str, aistr);
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the animation clips
void ColladaParser::ReadAnimationClipLibrary(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    std::string animName;
    if (!XmlParser::getStdStrAttribute(node, "name", animName)) {
        if (!XmlParser::getStdStrAttribute(node, "id", animName)) {
            animName = std::string("animation_") + ai_to_string(mAnimationClipLibrary.size());
        }
    }

    std::pair<std::string, std::vector<std::string>> clip;
    clip.first = animName;

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "instance_animation") {
            std::string url;
            readUrlAttribute(currentNode, url);
            clip.second.push_back(url);
        }

        if (clip.second.size() > 0) {
            mAnimationClipLibrary.push_back(clip);
        }
    }
}

void ColladaParser::PostProcessControllers() {
    std::string meshId;
    for (auto &it : mControllerLibrary) {
        meshId = it.second.mMeshId;
        if (meshId.empty()) {
            continue;
        }

        ControllerLibrary::iterator findItr = mControllerLibrary.find(meshId);
        while (findItr != mControllerLibrary.end()) {
            meshId = findItr->second.mMeshId;
            findItr = mControllerLibrary.find(meshId);
        }

        it.second.mMeshId = meshId;
    }
}

// ------------------------------------------------------------------------------------------------
// Re-build animations from animation clip library, if present, otherwise combine single-channel animations
void ColladaParser::PostProcessRootAnimations() {
    if (mAnimationClipLibrary.empty()) {
        mAnims.CombineSingleChannelAnimations();
        return;
    }

    Animation temp;
    for (auto &it : mAnimationClipLibrary) {
        std::string clipName = it.first;

        Animation *clip = new Animation();
        clip->mName = clipName;

        temp.mSubAnims.push_back(clip);

        for (const std::string &animationID : it.second) {
            AnimationLibrary::iterator animation = mAnimationLibrary.find(animationID);

            if (animation != mAnimationLibrary.end()) {
                Animation *pSourceAnimation = animation->second;
                pSourceAnimation->CollectChannelsRecursively(clip->mChannels);
            }
        }
    }

    mAnims = temp;

    // Ensure no double deletes.
    temp.mSubAnims.clear();
}

// ------------------------------------------------------------------------------------------------
// Reads the animation library
void ColladaParser::ReadAnimationLibrary(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "animation") {
            ReadAnimation(currentNode, &mAnims);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an animation into the given parent structure
void ColladaParser::ReadAnimation(XmlNode &node, Collada::Animation *pParent) {
    if (node.empty()) {
        return;
    }

    // an <animation> element may be a container for grouping sub-elements or an animation channel
    // this is the channel collection by ID, in case it has channels
    using ChannelMap = std::map<std::string, AnimationChannel>;
    ChannelMap channels;
    // this is the anim container in case we're a container
    Animation *anim = nullptr;

    // optional name given as an attribute
    std::string animName;
    if (!XmlParser::getStdStrAttribute(node, "name", animName)) {
        animName = "animation";
    }

    std::string animID;
    pugi::xml_attribute idAttr = node.attribute("id");
    if (idAttr) {
        animID = idAttr.as_string();
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "animation") {
            if (!anim) {
                anim = new Animation;
                anim->mName = animName;
                pParent->mSubAnims.push_back(anim);
            }

            // recurse into the sub-element
            ReadAnimation(currentNode, anim);
        } else if (currentName == "source") {
            ReadSource(currentNode);
        } else if (currentName == "sampler") {
            std::string id;
            if (XmlParser::getStdStrAttribute(currentNode, "id", id)) {
                // have it read into a channel
                ChannelMap::iterator newChannel = channels.insert(std::make_pair(id, AnimationChannel())).first;
                ReadAnimationSampler(currentNode, newChannel->second);
            }
        } else if (currentName == "channel") {
            std::string source_name, target;
            XmlParser::getStdStrAttribute(currentNode, "source", source_name);
            XmlParser::getStdStrAttribute(currentNode, "target", target);
            if (source_name[0] == '#') {
                source_name = source_name.substr(1, source_name.size() - 1);
            }
            ChannelMap::iterator cit = channels.find(source_name);
            if (cit != channels.end()) {
                cit->second.mTarget = target;
            }
        }
    }

    // it turned out to have channels - add them
    if (!channels.empty()) {
        if (nullptr == anim) {
            anim = new Animation;
            anim->mName = animName;
            pParent->mSubAnims.push_back(anim);
        }

        for (const auto &channel : channels) {
            anim->mChannels.push_back(channel.second);
        }

        if (idAttr) {
            mAnimationLibrary[animID] = anim;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an animation sampler into the given anim channel
void ColladaParser::ReadAnimationSampler(XmlNode &node, Collada::AnimationChannel &pChannel) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "input") {
            if (XmlParser::hasAttribute(currentNode, "semantic")) {
                std::string semantic, sourceAttr;
                XmlParser::getStdStrAttribute(currentNode, "semantic", semantic);
                if (XmlParser::hasAttribute(currentNode, "source")) {
                    XmlParser::getStdStrAttribute(currentNode, "source", sourceAttr);
                    const char *source = sourceAttr.c_str();
                    if (source[0] != '#') {
                        throw DeadlyImportError("Unsupported URL format");
                    }
                    source++;

                    if (semantic == "INPUT") {
                        pChannel.mSourceTimes = source;
                    } else if (semantic == "OUTPUT") {
                        pChannel.mSourceValues = source;
                    } else if (semantic == "IN_TANGENT") {
                        pChannel.mInTanValues = source;
                    } else if (semantic == "OUT_TANGENT") {
                        pChannel.mOutTanValues = source;
                    } else if (semantic == "INTERPOLATION") {
                        pChannel.mInterpolationValues = source;
                    }
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the skeleton controller library
void ColladaParser::ReadControllerLibrary(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName != "controller") {
            continue;
        }
        std::string id;
        if (XmlParser::getStdStrAttribute(currentNode, "id", id)) {
            mControllerLibrary[id] = Controller();
            ReadController(currentNode, mControllerLibrary[id]);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a controller into the given mesh structure
void ColladaParser::ReadController(XmlNode &node, Collada::Controller &controller) {
    // initial values
    controller.mType = Skin;
    controller.mMethod = Normalized;

    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "morph") {
            controller.mType = Morph;
            controller.mMeshId = currentNode.attribute("source").as_string();
            int methodIndex = currentNode.attribute("method").as_int();
            if (methodIndex > 0) {
                std::string method;
                XmlParser::getValueAsString(currentNode, method);

                if (method == "RELATIVE") {
                    controller.mMethod = Relative;
                }
            }
        } else if (currentName == "skin") {
            std::string id;
            if (XmlParser::getStdStrAttribute(currentNode, "source", id)) {
                controller.mMeshId = id.substr(1, id.size() - 1);
            }
        } else if (currentName == "bind_shape_matrix") {
            std::string v;
            XmlParser::getValueAsString(currentNode, v);
            const char *content = v.c_str();
            for (unsigned int a = 0; a < 16; a++) {
                SkipSpacesAndLineEnd(&content);
                // read a number
                content = fast_atoreal_move<ai_real>(content, controller.mBindShapeMatrix[a]);
                // skip whitespace after it
                SkipSpacesAndLineEnd(&content);
            }
        } else if (currentName == "source") {
            ReadSource(currentNode);
        } else if (currentName == "joints") {
            ReadControllerJoints(currentNode, controller);
        } else if (currentName == "vertex_weights") {
            ReadControllerWeights(currentNode, controller);
        } else if (currentName == "targets") {
            for (XmlNode currentChildNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
                const std::string &currentChildName = currentChildNode.name();
                if (currentChildName == "input") {
                    const char *semantics = currentChildNode.attribute("semantic").as_string();
                    const char *source = currentChildNode.attribute("source").as_string();
                    if (strcmp(semantics, "MORPH_TARGET") == 0) {
                        controller.mMorphTarget = source + 1;
                    } else if (strcmp(semantics, "MORPH_WEIGHT") == 0) {
                        controller.mMorphWeight = source + 1;
                    }
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the joint definitions for the given controller
void ColladaParser::ReadControllerJoints(XmlNode &node, Collada::Controller &pController) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "input") {
            const char *attrSemantic = currentNode.attribute("semantic").as_string();
            const char *attrSource = currentNode.attribute("source").as_string();
            if (attrSource[0] != '#') {
                throw DeadlyImportError("Unsupported URL format in \"", attrSource, "\" in source attribute of <joints> data <input> element");
            }
            ++attrSource;
            // parse source URL to corresponding source
            if (strcmp(attrSemantic, "JOINT") == 0) {
                pController.mJointNameSource = attrSource;
            } else if (strcmp(attrSemantic, "INV_BIND_MATRIX") == 0) {
                pController.mJointOffsetMatrixSource = attrSource;
            } else {
                throw DeadlyImportError("Unknown semantic \"", attrSemantic, "\" in <joints> data <input> element");
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the joint weights for the given controller
void ColladaParser::ReadControllerWeights(XmlNode &node, Collada::Controller &pController) {
    // Read vertex count from attributes and resize the array accordingly
    int vertexCount = 0;
    XmlParser::getIntAttribute(node, "count", vertexCount);
    pController.mWeightCounts.resize(vertexCount);

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "input") {
            InputChannel channel;

            const char *attrSemantic = currentNode.attribute("semantic").as_string();
            const char *attrSource = currentNode.attribute("source").as_string();
            channel.mOffset = currentNode.attribute("offset").as_int();

            // local URLS always start with a '#'. We don't support global URLs
            if (attrSource[0] != '#') {
                throw DeadlyImportError("Unsupported URL format in \"", attrSource, "\" in source attribute of <vertex_weights> data <input> element");
            }
            channel.mAccessor = attrSource + 1;

            // parse source URL to corresponding source
            if (strcmp(attrSemantic, "JOINT") == 0) {
                pController.mWeightInputJoints = channel;
            } else if (strcmp(attrSemantic, "WEIGHT") == 0) {
                pController.mWeightInputWeights = channel;
            } else {
                throw DeadlyImportError("Unknown semantic \"", attrSemantic, "\" in <vertex_weights> data <input> element");
            }
        } else if (currentName == "vcount" && vertexCount > 0) {
            const char *text = currentNode.text().as_string();
            size_t numWeights = 0;
            for (std::vector<size_t>::iterator it = pController.mWeightCounts.begin(); it != pController.mWeightCounts.end(); ++it) {
                if (*text == 0) {
                    throw DeadlyImportError("Out of data while reading <vcount>");
                }

                *it = strtoul10(text, &text);
                numWeights += *it;
                SkipSpacesAndLineEnd(&text);
            }
            // reserve weight count
            pController.mWeights.resize(numWeights);
        } else if (currentName == "v" && vertexCount > 0) {
            // read JointIndex - WeightIndex pairs
            std::string stdText;
            XmlParser::getValueAsString(currentNode, stdText);
            const char *text = stdText.c_str();
            for (std::vector<std::pair<size_t, size_t>>::iterator it = pController.mWeights.begin(); it != pController.mWeights.end(); ++it) {
                if (text == 0) {
                    throw DeadlyImportError("Out of data while reading <vertex_weights>");
                }
                it->first = strtoul10(text, &text);
                SkipSpacesAndLineEnd(&text);
                if (*text == 0) {
                    throw DeadlyImportError("Out of data while reading <vertex_weights>");
                }
                it->second = strtoul10(text, &text);
                SkipSpacesAndLineEnd(&text);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the image library contents
void ColladaParser::ReadImageLibrary(XmlNode &node) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "image") {
            std::string id;
            if (XmlParser::getStdStrAttribute(currentNode, "id", id)) {
                mImageLibrary[id] = Image();
                // read on from there
                ReadImage(currentNode, mImageLibrary[id]);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an image entry into the given image
void ColladaParser::ReadImage(XmlNode &node, Collada::Image &pImage) {
    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == "image") {
            // Ignore
            continue;
        } else if (currentName == "init_from") {
            if (mFormat == FV_1_4_n) {
                // FIX: C4D exporter writes empty <init_from/> tags
                if (!currentNode.empty()) {
                    // element content is filename - hopefully
                    const char *sz = currentNode.text().as_string();
                    if (nullptr != sz) {
                        aiString filepath(sz);
                        UriDecodePath(filepath);
                        pImage.mFileName = filepath.C_Str();
                    }
                }
                if (!pImage.mFileName.length()) {
                    pImage.mFileName = "unknown_texture";
                }
            }
        } else if (mFormat == FV_1_5_n) {
            std::string value;
            XmlNode refChild = currentNode.child("ref");
            XmlNode hexChild = currentNode.child("hex");
            if (refChild) {
                // element content is filename - hopefully
                if (XmlParser::getValueAsString(refChild, value)) {
                    aiString filepath(value);
                    UriDecodePath(filepath);
                    pImage.mFileName = filepath.C_Str();
                }
            } else if (hexChild && !pImage.mFileName.length()) {
                // embedded image. get format
                pImage.mEmbeddedFormat = hexChild.attribute("format").as_string();
                if (pImage.mEmbeddedFormat.empty()) {
                    ASSIMP_LOG_WARN("Collada: Unknown image file format");
                }

                XmlParser::getValueAsString(hexChild, value);
                const char *data = value.c_str();
                // hexadecimal-encoded binary octets. First of all, find the
                // required buffer size to reserve enough storage.
                const char *cur = data;
                while (!IsSpaceOrNewLine(*cur)) {
                    ++cur;
                }

                const unsigned int size = (unsigned int)(cur - data) * 2;
                pImage.mImageData.resize(size);
                for (unsigned int i = 0; i < size; ++i) {
                    pImage.mImageData[i] = HexOctetToDecimal(data + (i << 1));
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the material library
void ColladaParser::ReadMaterialLibrary(XmlNode &node) {
    std::map<std::string, int> names;
    for (XmlNode &currentNode : node.children()) {
        std::string id = currentNode.attribute("id").as_string();
        std::string name = currentNode.attribute("name").as_string();
        mMaterialLibrary[id] = Material();

        if (!name.empty()) {
            std::map<std::string, int>::iterator it = names.find(name);
            if (it != names.end()) {
                std::ostringstream strStream;
                strStream << ++it->second;
                name.append(" " + strStream.str());
            } else {
                names[name] = 0;
            }

            mMaterialLibrary[id].mName = name;
        }

        ReadMaterial(currentNode, mMaterialLibrary[id]);
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the light library
void ColladaParser::ReadLightLibrary(XmlNode &node) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "light") {
            std::string id;
            if (XmlParser::getStdStrAttribute(currentNode, "id", id)) {
                ReadLight(currentNode, mLightLibrary[id] = Light());
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the camera library
void ColladaParser::ReadCameraLibrary(XmlNode &node) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "camera") {
            std::string id;
            if (!XmlParser::getStdStrAttribute(currentNode, "id", id)) {
                continue;
            }

            // create an entry and store it in the library under its ID
            Camera &cam = mCameraLibrary[id];
            std::string name;
            if (!XmlParser::getStdStrAttribute(currentNode, "name", name)) {
                continue;
            }
            if (!name.empty()) {
                cam.mName = name;
            }
            ReadCamera(currentNode, cam);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a material entry into the given material
void ColladaParser::ReadMaterial(XmlNode &node, Collada::Material &pMaterial) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "instance_effect") {
            std::string url;
            readUrlAttribute(currentNode, url);
            pMaterial.mEffect = url;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a light entry into the given light
void ColladaParser::ReadLight(XmlNode &node, Collada::Light &pLight) {
    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    // TODO: Check the current technique and skip over unsupported extra techniques

    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "spot") {
            pLight.mType = aiLightSource_SPOT;
        } else if (currentName == "ambient") {
            pLight.mType = aiLightSource_AMBIENT;
        } else if (currentName == "directional") {
            pLight.mType = aiLightSource_DIRECTIONAL;
        } else if (currentName == "point") {
            pLight.mType = aiLightSource_POINT;
        } else if (currentName == "color") {
            // text content contains 3 floats
            std::string v;
            XmlParser::getValueAsString(currentNode, v);
            const char *content = v.c_str();

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pLight.mColor.r);
            SkipSpacesAndLineEnd(&content);

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pLight.mColor.g);
            SkipSpacesAndLineEnd(&content);

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pLight.mColor.b);
            SkipSpacesAndLineEnd(&content);
        } else if (currentName == "constant_attenuation") {
            XmlParser::getValueAsFloat(currentNode, pLight.mAttConstant);
        } else if (currentName == "linear_attenuation") {
            XmlParser::getValueAsFloat(currentNode, pLight.mAttLinear);
        } else if (currentName == "quadratic_attenuation") {
            XmlParser::getValueAsFloat(currentNode, pLight.mAttQuadratic);
        } else if (currentName == "falloff_angle") {
            XmlParser::getValueAsFloat(currentNode, pLight.mFalloffAngle);
        } else if (currentName == "falloff_exponent") {
            XmlParser::getValueAsFloat(currentNode, pLight.mFalloffExponent);
        }
        // FCOLLADA extensions
        // -------------------------------------------------------
        else if (currentName == "outer_cone") {
            XmlParser::getValueAsFloat(currentNode, pLight.mOuterAngle);
        } else if (currentName == "penumbra_angle") { // this one is deprecated, now calculated using outer_cone
            XmlParser::getValueAsFloat(currentNode, pLight.mPenumbraAngle);
        } else if (currentName == "intensity") {
            XmlParser::getValueAsFloat(currentNode, pLight.mIntensity);
        }
        else if (currentName == "falloff") {
            XmlParser::getValueAsFloat(currentNode, pLight.mOuterAngle);
        } else if (currentName == "hotspot_beam") {
            XmlParser::getValueAsFloat(currentNode, pLight.mFalloffAngle);
        }
        // OpenCOLLADA extensions
        // -------------------------------------------------------
        else if (currentName == "decay_falloff") {
            XmlParser::getValueAsFloat(currentNode, pLight.mOuterAngle);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a camera entry into the given light
void ColladaParser::ReadCamera(XmlNode &node, Collada::Camera &camera) {
    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "orthographic") {
            camera.mOrtho = true;
        } else if (currentName == "xfov" || currentName == "xmag") {
            XmlParser::getValueAsFloat(currentNode, camera.mHorFov);
        } else if (currentName == "yfov" || currentName == "ymag") {
            XmlParser::getValueAsFloat(currentNode, camera.mVerFov);
        } else if (currentName == "aspect_ratio") {
            XmlParser::getValueAsFloat(currentNode, camera.mAspect);
        } else if (currentName == "znear") {
            XmlParser::getValueAsFloat(currentNode, camera.mZNear);
        } else if (currentName == "zfar") {
            XmlParser::getValueAsFloat(currentNode, camera.mZFar);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the effect library
void ColladaParser::ReadEffectLibrary(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "effect") {
            // read ID. Do I have to repeat my ranting about "optional" attributes?
            std::string id;
            XmlParser::getStdStrAttribute(currentNode, "id", id);

            // create an entry and store it in the library under its ID
            mEffectLibrary[id] = Effect();

            // read on from there
            ReadEffect(currentNode, mEffectLibrary[id]);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an effect entry into the given effect
void ColladaParser::ReadEffect(XmlNode &node, Collada::Effect &pEffect) {
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "profile_COMMON") {
            ReadEffectProfileCommon(currentNode, pEffect);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an COMMON effect profile
void ColladaParser::ReadEffectProfileCommon(XmlNode &node, Collada::Effect &pEffect) {
    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string currentName = currentNode.name();
        if (currentName == "newparam") {
            // save ID
            std::string sid = currentNode.attribute("sid").as_string();
            pEffect.mParams[sid] = EffectParam();
            ReadEffectParam(currentNode, pEffect.mParams[sid]);
        } else if (currentName == "technique" || currentName == "extra") {
            // just syntactic sugar
        } else if (mFormat == FV_1_4_n && currentName == "image") {
            // read ID. Another entry which is "optional" by design but obligatory in reality
            std::string id = currentNode.attribute("id").as_string();

            // create an entry and store it in the library under its ID
            mImageLibrary[id] = Image();

            // read on from there
            ReadImage(currentNode, mImageLibrary[id]);
        } else if (currentName == "phong")
            pEffect.mShadeType = Shade_Phong;
        else if (currentName == "constant")
            pEffect.mShadeType = Shade_Constant;
        else if (currentName == "lambert")
            pEffect.mShadeType = Shade_Lambert;
        else if (currentName == "blinn")
            pEffect.mShadeType = Shade_Blinn;

        /* Color + texture properties */
        else if (currentName == "emission")
            ReadEffectColor(currentNode, pEffect.mEmissive, pEffect.mTexEmissive);
        else if (currentName == "ambient")
            ReadEffectColor(currentNode, pEffect.mAmbient, pEffect.mTexAmbient);
        else if (currentName == "diffuse")
            ReadEffectColor(currentNode, pEffect.mDiffuse, pEffect.mTexDiffuse);
        else if (currentName == "specular")
            ReadEffectColor(currentNode, pEffect.mSpecular, pEffect.mTexSpecular);
        else if (currentName == "reflective") {
            ReadEffectColor(currentNode, pEffect.mReflective, pEffect.mTexReflective);
        } else if (currentName == "transparent") {
            pEffect.mHasTransparency = true;
            const char *opaque = currentNode.attribute("opaque").as_string();
            //const char *opaque = mReader->getAttributeValueSafe("opaque");

            if (::strcmp(opaque, "RGB_ZERO") == 0 || ::strcmp(opaque, "RGB_ONE") == 0) {
                pEffect.mRGBTransparency = true;
            }

            // In RGB_ZERO mode, the transparency is interpreted in reverse, go figure...
            if (::strcmp(opaque, "RGB_ZERO") == 0 || ::strcmp(opaque, "A_ZERO") == 0) {
                pEffect.mInvertTransparency = true;
            }

            ReadEffectColor(currentNode, pEffect.mTransparent, pEffect.mTexTransparent);
        } else if (currentName == "shininess")
            ReadEffectFloat(currentNode, pEffect.mShininess);
        else if (currentName == "reflectivity")
            ReadEffectFloat(currentNode, pEffect.mReflectivity);

        /* Single scalar properties */
        else if (currentName == "transparency")
            ReadEffectFloat(currentNode, pEffect.mTransparency);
        else if (currentName == "index_of_refraction")
            ReadEffectFloat(currentNode, pEffect.mRefractIndex);

        // GOOGLEEARTH/OKINO extensions
        // -------------------------------------------------------
        else if (currentName == "double_sided")
            XmlParser::getValueAsBool(currentNode, pEffect.mDoubleSided);

        // FCOLLADA extensions
        // -------------------------------------------------------
        else if (currentName == "bump") {
            aiColor4D dummy;
            ReadEffectColor(currentNode, dummy, pEffect.mTexBump);
        }

        // MAX3D extensions
        // -------------------------------------------------------
        else if (currentName == "wireframe") {
            XmlParser::getValueAsBool(currentNode, pEffect.mWireframe);
        } else if (currentName == "faceted") {
            XmlParser::getValueAsBool(currentNode, pEffect.mFaceted);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Read texture wrapping + UV transform settings from a profile==Maya chunk
void ColladaParser::ReadSamplerProperties(XmlNode &node, Sampler &out) {
    if (node.empty()) {
        return;
    }

    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        // MAYA extensions
        // -------------------------------------------------------
        if (currentName == "wrapU") {
            XmlParser::getValueAsBool(currentNode, out.mWrapU);
        } else if (currentName == "wrapV") {
            XmlParser::getValueAsBool(currentNode, out.mWrapV);
        } else if (currentName == "mirrorU") {
            XmlParser::getValueAsBool(currentNode, out.mMirrorU);
        } else if (currentName == "mirrorV") {
            XmlParser::getValueAsBool(currentNode, out.mMirrorV);
        } else if (currentName == "repeatU") {
            XmlParser::getValueAsFloat(currentNode, out.mTransform.mScaling.x);
        } else if (currentName == "repeatV") {
            XmlParser::getValueAsFloat(currentNode, out.mTransform.mScaling.y);
        } else if (currentName == "offsetU") {
            XmlParser::getValueAsFloat(currentNode, out.mTransform.mTranslation.x);
        } else if (currentName == "offsetV") {
            XmlParser::getValueAsFloat(currentNode, out.mTransform.mTranslation.y);
        } else if (currentName == "rotateUV") {
            XmlParser::getValueAsFloat(currentNode, out.mTransform.mRotation);
        } else if (currentName == "blend_mode") {
            std::string v;
            XmlParser::getValueAsString(currentNode, v);
            const char *sz = v.c_str();
            // http://www.feelingsoftware.com/content/view/55/72/lang,en/
            // NONE, OVER, IN, OUT, ADD, SUBTRACT, MULTIPLY, DIFFERENCE, LIGHTEN, DARKEN, SATURATE, DESATURATE and ILLUMINATE
            if (0 == ASSIMP_strincmp(sz, "ADD", 3))
                out.mOp = aiTextureOp_Add;
            else if (0 == ASSIMP_strincmp(sz, "SUBTRACT", 8))
                out.mOp = aiTextureOp_Subtract;
            else if (0 == ASSIMP_strincmp(sz, "MULTIPLY", 8))
                out.mOp = aiTextureOp_Multiply;
            else {
                ASSIMP_LOG_WARN("Collada: Unsupported MAYA texture blend mode");
            }
        }
        // OKINO extensions
        // -------------------------------------------------------
        else if (currentName == "weighting") {
            XmlParser::getValueAsFloat(currentNode, out.mWeighting);
        } else if (currentName == "mix_with_previous_layer") {
            XmlParser::getValueAsFloat(currentNode, out.mMixWithPrevious);
        }
        // MAX3D extensions
        // -------------------------------------------------------
        else if (currentName == "amount") {
            XmlParser::getValueAsFloat(currentNode, out.mWeighting);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an effect entry containing a color or a texture defining that color
void ColladaParser::ReadEffectColor(XmlNode &node, aiColor4D &pColor, Sampler &pSampler) {
    if (node.empty()) {
        return;
    }

    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "color") {
            // text content contains 4 floats
            std::string v;
            XmlParser::getValueAsString(currentNode, v);
            const char *content = v.c_str();

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pColor.r);
            SkipSpacesAndLineEnd(&content);

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pColor.g);
            SkipSpacesAndLineEnd(&content);

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pColor.b);
            SkipSpacesAndLineEnd(&content);

            content = fast_atoreal_move<ai_real>(content, (ai_real &)pColor.a);
            SkipSpacesAndLineEnd(&content);
        } else if (currentName == "texture") {
            // get name of source texture/sampler
            XmlParser::getStdStrAttribute(currentNode, "texture", pSampler.mName);

            // get name of UV source channel. Specification demands it to be there, but some exporters
            // don't write it. It will be the default UV channel in case it's missing.
            XmlParser::getStdStrAttribute(currentNode, "texcoord", pSampler.mUVChannel);

            // as we've read texture, the color needs to be 1,1,1,1
            pColor = aiColor4D(1.f, 1.f, 1.f, 1.f);
        } else if (currentName == "technique") {
            std::string profile;
            XmlParser::getStdStrAttribute(currentNode, "profile", profile);

            // Some extensions are quite useful ... ReadSamplerProperties processes
            // several extensions in MAYA, OKINO and MAX3D profiles.
            if (!::strcmp(profile.c_str(), "MAYA") || !::strcmp(profile.c_str(), "MAX3D") || !::strcmp(profile.c_str(), "OKINO")) {
                // get more information on this sampler
                ReadSamplerProperties(currentNode, pSampler);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an effect entry containing a float
void ColladaParser::ReadEffectFloat(XmlNode &node, ai_real &pFloat) {
    pFloat = 0.f;
    XmlNode floatNode = node.child("float");
    if (floatNode.empty()) {
        return;
    }
    XmlParser::getValueAsFloat(floatNode, pFloat);
}

// ------------------------------------------------------------------------------------------------
// Reads an effect parameter specification of any kind
void ColladaParser::ReadEffectParam(XmlNode &node, Collada::EffectParam &pParam) {
    if (node.empty()) {
        return;
    }

    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "surface") {
            // image ID given inside <init_from> tags
            XmlNode initNode = currentNode.child("init_from");
            if (initNode) {
                std::string v;
                XmlParser::getValueAsString(initNode, v);
                pParam.mType = Param_Surface;
                pParam.mReference = v.c_str();
            }
        } else if (currentName == "sampler2D" && (FV_1_4_n == mFormat || FV_1_3_n == mFormat)) {
            // surface ID is given inside <source> tags
            const char *content = currentNode.value();
            pParam.mType = Param_Sampler;
            pParam.mReference = content;
        } else if (currentName == "sampler2D") {
            // surface ID is given inside <instance_image> tags
            std::string url;
            XmlParser::getStdStrAttribute(currentNode, "url", url);
            if (url[0] != '#') {
                throw DeadlyImportError("Unsupported URL format in instance_image");
            }
            pParam.mType = Param_Sampler;
            pParam.mReference = url.c_str() + 1;
        } else if (currentName == "source") {
            const char *source = currentNode.child_value();
            if (nullptr != source) {
                pParam.mReference = source;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the geometry library contents
void ColladaParser::ReadGeometryLibrary(XmlNode &node) {
    if (node.empty()) {
        return;
    }
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "geometry") {
            // read ID. Another entry which is "optional" by design but obligatory in reality

            std::string id;
            XmlParser::getStdStrAttribute(currentNode, "id", id);
            // create a mesh and store it in the library under its (resolved) ID
            // Skip and warn if ID is not unique
            if (mMeshLibrary.find(id) == mMeshLibrary.cend()) {
                std::unique_ptr<Mesh> mesh(new Mesh(id));

                XmlParser::getStdStrAttribute(currentNode, "name", mesh->mName);

                // read on from there
                ReadGeometry(currentNode, *mesh);
                // Read successfully, add to library
                mMeshLibrary.insert({ id, mesh.release() });
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a geometry from the geometry library.
void ColladaParser::ReadGeometry(XmlNode &node, Collada::Mesh &pMesh) {
    if (node.empty()) {
        return;
    }
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "mesh") {
            ReadMesh(currentNode, pMesh);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a mesh from the geometry library
void ColladaParser::ReadMesh(XmlNode &node, Mesh &pMesh) {
    if (node.empty()) {
        return;
    }

    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "source") {
            ReadSource(currentNode);
        } else if (currentName == "vertices") {
            ReadVertexData(currentNode, pMesh);
        } else if (currentName == "triangles" || currentName == "lines" || currentName == "linestrips" ||
                   currentName == "polygons" || currentName == "polylist" || currentName == "trifans" ||
                   currentName == "tristrips") {
            ReadIndexData(currentNode, pMesh);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a source element
void ColladaParser::ReadSource(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    std::string sourceID;
    XmlParser::getStdStrAttribute(node, "id", sourceID);
    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "float_array" || currentName == "IDREF_array" || currentName == "Name_array") {
            ReadDataArray(currentNode);
        } else if (currentName == "technique_common") {
            XmlNode technique = currentNode.child("accessor");
            if (!technique.empty()) {
                ReadAccessor(technique, sourceID);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a data array holding a number of floats, and stores it in the global library
void ColladaParser::ReadDataArray(XmlNode &node) {
    std::string name = node.name();
    bool isStringArray = (name == "IDREF_array" || name == "Name_array");

    // read attributes
    std::string id;
    XmlParser::getStdStrAttribute(node, "id", id);
    unsigned int count = 0;
    XmlParser::getUIntAttribute(node, "count", count);
    std::string v;
    XmlParser::getValueAsString(node, v);
    v = ai_trim(v);
    const char *content = v.c_str();

    // read values and store inside an array in the data library
    mDataLibrary[id] = Data();
    Data &data = mDataLibrary[id];
    data.mIsStringArray = isStringArray;

    // some exporters write empty data arrays, but we need to conserve them anyways because others might reference them
    if (content) {
        if (isStringArray) {
            data.mStrings.reserve(count);
            std::string s;

            for (unsigned int a = 0; a < count; a++) {
                if (*content == 0) {
                    throw DeadlyImportError("Expected more values while reading IDREF_array contents.");
                }

                s.clear();
                while (!IsSpaceOrNewLine(*content))
                    s += *content++;
                data.mStrings.push_back(s);

                SkipSpacesAndLineEnd(&content);
            }
        } else {
            data.mValues.reserve(count);

            for (unsigned int a = 0; a < count; a++) {
                if (*content == 0) {
                    throw DeadlyImportError("Expected more values while reading float_array contents.");
                }

                // read a number
                ai_real value;
                content = fast_atoreal_move<ai_real>(content, value);
                data.mValues.push_back(value);
                // skip whitespace after it
                SkipSpacesAndLineEnd(&content);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads an accessor and stores it in the global library
void ColladaParser::ReadAccessor(XmlNode &node, const std::string &pID) {
    // read accessor attributes
    std::string source;
    XmlParser::getStdStrAttribute(node, "source", source);
    if (source[0] != '#') {
        throw DeadlyImportError("Unknown reference format in url \"", source, "\" in source attribute of <accessor> element.");
    }
    int count = 0;
    XmlParser::getIntAttribute(node, "count", count);

    unsigned int offset = 0;
    if (XmlParser::hasAttribute(node, "offset")) {
        XmlParser::getUIntAttribute(node, "offset", offset);
    }
    unsigned int stride = 1;
    if (XmlParser::hasAttribute(node, "stride")) {
        XmlParser::getUIntAttribute(node, "stride", stride);
    }
    // store in the library under the given ID
    mAccessorLibrary[pID] = Accessor();
    Accessor &acc = mAccessorLibrary[pID];
    acc.mCount = count;
    acc.mOffset = offset;
    acc.mStride = stride;
    acc.mSource = source.c_str() + 1; // ignore the leading '#'
    acc.mSize = 0; // gets incremented with every param

    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "param") {
            // read data param
            std::string name;
            if (XmlParser::hasAttribute(currentNode, "name")) {
                XmlParser::getStdStrAttribute(currentNode, "name", name);

                // analyse for common type components and store it's sub-offset in the corresponding field

                // Cartesian coordinates
                if (name == "X")
                    acc.mSubOffset[0] = acc.mParams.size();
                else if (name == "Y")
                    acc.mSubOffset[1] = acc.mParams.size();
                else if (name == "Z")
                    acc.mSubOffset[2] = acc.mParams.size();

                /* RGBA colors */
                else if (name == "R")
                    acc.mSubOffset[0] = acc.mParams.size();
                else if (name == "G")
                    acc.mSubOffset[1] = acc.mParams.size();
                else if (name == "B")
                    acc.mSubOffset[2] = acc.mParams.size();
                else if (name == "A")
                    acc.mSubOffset[3] = acc.mParams.size();

                /* UVWQ (STPQ) texture coordinates */
                else if (name == "S")
                    acc.mSubOffset[0] = acc.mParams.size();
                else if (name == "T")
                    acc.mSubOffset[1] = acc.mParams.size();
                else if (name == "P")
                    acc.mSubOffset[2] = acc.mParams.size();
                /* Generic extra data, interpreted as UV data, too*/
                else if (name == "U")
                    acc.mSubOffset[0] = acc.mParams.size();
                else if (name == "V")
                    acc.mSubOffset[1] = acc.mParams.size();
            }
            if (XmlParser::hasAttribute(currentNode, "type")) {
                // read data type
                // TODO: (thom) I don't have a spec here at work. Check if there are other multi-value types
                // which should be tested for here.
                std::string type;

                XmlParser::getStdStrAttribute(currentNode, "type", type);
                if (type == "float4x4")
                    acc.mSize += 16;
                else
                    acc.mSize += 1;
            }

            acc.mParams.push_back(name);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads input declarations of per-vertex mesh data into the given mesh
void ColladaParser::ReadVertexData(XmlNode &node, Mesh &pMesh) {
    // extract the ID of the <vertices> element. Not that we care, but to catch strange referencing schemes we should warn about
    XmlParser::getStdStrAttribute(node, "id", pMesh.mVertexID);
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "input") {
            ReadInputChannel(currentNode, pMesh.mPerVertexData);
        } else {
            throw DeadlyImportError("Unexpected sub element <", currentName, "> in tag <vertices>");
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads input declarations of per-index mesh data into the given mesh
void ColladaParser::ReadIndexData(XmlNode &node, Mesh &pMesh) {
    std::vector<size_t> vcount;
    std::vector<InputChannel> perIndexData;

    unsigned int numPrimitives = 0;
    XmlParser::getUIntAttribute(node, "count", numPrimitives);
    // read primitive count from the attribute
    //int attrCount = GetAttribute("count");
    //size_t numPrimitives = (size_t)mReader->getAttributeValueAsInt(attrCount);
    // some mesh types (e.g. tristrips) don't specify primitive count upfront,
    // so we need to sum up the actual number of primitives while we read the <p>-tags
    size_t actualPrimitives = 0;
    SubMesh subgroup;
    if (XmlParser::hasAttribute(node, "material")) {
        XmlParser::getStdStrAttribute(node, "material", subgroup.mMaterial);
    }

    // distinguish between polys and triangles
    std::string elementName = node.name();
    PrimitiveType primType = Prim_Invalid;
    if (elementName == "lines")
        primType = Prim_Lines;
    else if (elementName == "linestrips")
        primType = Prim_LineStrip;
    else if (elementName == "polygons")
        primType = Prim_Polygon;
    else if (elementName == "polylist")
        primType = Prim_Polylist;
    else if (elementName == "triangles")
        primType = Prim_Triangles;
    else if (elementName == "trifans")
        primType = Prim_TriFans;
    else if (elementName == "tristrips")
        primType = Prim_TriStrips;

    ai_assert(primType != Prim_Invalid);

    // also a number of <input> elements, but in addition a <p> primitive collection and probably index counts for all primitives
    XmlNodeIterator xmlIt(node, XmlNodeIterator::PreOrderMode);
    XmlNode currentNode;
    while (xmlIt.getNext(currentNode)) {
        const std::string &currentName = currentNode.name();
        if (currentName == "input") {
            ReadInputChannel(currentNode, perIndexData);
        } else if (currentName == "vcount") {
            if (!currentNode.empty()) {
                if (numPrimitives) // It is possible to define a mesh without any primitives
                {
                    // case <polylist> - specifies the number of indices for each polygon
                    std::string v;
                    XmlParser::getValueAsString(currentNode, v);
                    const char *content = v.c_str();
                    vcount.reserve(numPrimitives);
                    SkipSpacesAndLineEnd(&content);
                    for (unsigned int a = 0; a < numPrimitives; a++) {
                        if (*content == 0) {
                            throw DeadlyImportError("Expected more values while reading <vcount> contents.");
                        }
                        // read a number
                        vcount.push_back((size_t)strtoul10(content, &content));
                        // skip whitespace after it
                        SkipSpacesAndLineEnd(&content);
                    }
                }
            }
        } else if (currentName == "p") {
            if (!currentNode.empty()) {
                // now here the actual fun starts - these are the indices to construct the mesh data from
                actualPrimitives += ReadPrimitives(currentNode, pMesh, perIndexData, numPrimitives, vcount, primType);
            }
        } else if (currentName == "extra") {
            // skip
        } else if (currentName == "ph") {
            // skip
        } else {
            throw DeadlyImportError("Unexpected sub element <", currentName, "> in tag <", elementName, ">");
        }
    }

#ifdef ASSIMP_BUILD_DEBUG
    if (primType != Prim_TriFans && primType != Prim_TriStrips && primType != Prim_LineStrip &&
            primType != Prim_Lines) { // this is ONLY to workaround a bug in SketchUp 15.3.331 where it writes the wrong 'count' when it writes out the 'lines'.
        ai_assert(actualPrimitives == numPrimitives);
    }
#endif

    // only when we're done reading all <p> tags (and thus know the final vertex count) can we commit the submesh
    subgroup.mNumFaces = actualPrimitives;
    pMesh.mSubMeshes.push_back(subgroup);
}

// ------------------------------------------------------------------------------------------------
// Reads a single input channel element and stores it in the given array, if valid
void ColladaParser::ReadInputChannel(XmlNode &node, std::vector<InputChannel> &poChannels) {
    InputChannel channel;

    // read semantic
    std::string semantic;
    XmlParser::getStdStrAttribute(node, "semantic", semantic);
    channel.mType = GetTypeForSemantic(semantic);

    // read source
    std::string source;
    XmlParser::getStdStrAttribute(node, "source", source);
    if (source[0] != '#') {
        throw DeadlyImportError("Unknown reference format in url \"", source, "\" in source attribute of <input> element.");
    }
    channel.mAccessor = source.c_str() + 1; // skipping the leading #, hopefully the remaining text is the accessor ID only

    // read index offset, if per-index <input>
    if (XmlParser::hasAttribute(node, "offset")) {
        XmlParser::getUIntAttribute(node, "offset", (unsigned int &)channel.mOffset);
    }

    // read set if texture coordinates
    if (channel.mType == IT_Texcoord || channel.mType == IT_Color) {
        unsigned int attrSet = 0;
        if (XmlParser::getUIntAttribute(node, "set", attrSet))
            channel.mIndex = attrSet;
    }

    // store, if valid type
    if (channel.mType != IT_Invalid)
        poChannels.push_back(channel);
}

// ------------------------------------------------------------------------------------------------
// Reads a <p> primitive index list and assembles the mesh data into the given mesh
size_t ColladaParser::ReadPrimitives(XmlNode &node, Mesh &pMesh, std::vector<InputChannel> &pPerIndexChannels,
        size_t pNumPrimitives, const std::vector<size_t> &pVCount, PrimitiveType pPrimType) {
    // determine number of indices coming per vertex
    // find the offset index for all per-vertex channels
    size_t numOffsets = 1;
    size_t perVertexOffset = SIZE_MAX; // invalid value
    for (const InputChannel &channel : pPerIndexChannels) {
        numOffsets = std::max(numOffsets, channel.mOffset + 1);
        if (channel.mType == IT_Vertex)
            perVertexOffset = channel.mOffset;
    }

    // determine the expected number of indices
    size_t expectedPointCount = 0;
    switch (pPrimType) {
    case Prim_Polylist: {
        for (size_t i : pVCount)
            expectedPointCount += i;
        break;
    }
    case Prim_Lines:
        expectedPointCount = 2 * pNumPrimitives;
        break;
    case Prim_Triangles:
        expectedPointCount = 3 * pNumPrimitives;
        break;
    default:
        // other primitive types don't state the index count upfront... we need to guess
        break;
    }

    // and read all indices into a temporary array
    std::vector<size_t> indices;
    if (expectedPointCount > 0) {
        indices.reserve(expectedPointCount * numOffsets);
    }

    // It is possible to not contain any indices
    if (pNumPrimitives > 0) {
        std::string v;
        XmlParser::getValueAsString(node, v);
        const char *content = v.c_str();
        SkipSpacesAndLineEnd(&content);
        while (*content != 0) {
            // read a value.
            // Hack: (thom) Some exporters put negative indices sometimes. We just try to carry on anyways.
            int value = std::max(0, strtol10(content, &content));
            indices.push_back(size_t(value));
            // skip whitespace after it
            SkipSpacesAndLineEnd(&content);
        }
    }

    // complain if the index count doesn't fit
    if (expectedPointCount > 0 && indices.size() != expectedPointCount * numOffsets) {
        if (pPrimType == Prim_Lines) {
            // HACK: We just fix this number since SketchUp 15.3.331 writes the wrong 'count' for 'lines'
            ReportWarning("Expected different index count in <p> element, %zu instead of %zu.", indices.size(), expectedPointCount * numOffsets);
            pNumPrimitives = (indices.size() / numOffsets) / 2;
        } else {
            throw DeadlyImportError("Expected different index count in <p> element.");
        }
    } else if (expectedPointCount == 0 && (indices.size() % numOffsets) != 0) {
        throw DeadlyImportError("Expected different index count in <p> element.");
    }

    // find the data for all sources
    for (std::vector<InputChannel>::iterator it = pMesh.mPerVertexData.begin(); it != pMesh.mPerVertexData.end(); ++it) {
        InputChannel &input = *it;
        if (input.mResolved) {
            continue;
        }

        // find accessor
        input.mResolved = &ResolveLibraryReference(mAccessorLibrary, input.mAccessor);
        // resolve accessor's data pointer as well, if necessary
        const Accessor *acc = input.mResolved;
        if (!acc->mData) {
            acc->mData = &ResolveLibraryReference(mDataLibrary, acc->mSource);
        }
    }
    // and the same for the per-index channels
    for (std::vector<InputChannel>::iterator it = pPerIndexChannels.begin(); it != pPerIndexChannels.end(); ++it) {
        InputChannel &input = *it;
        if (input.mResolved) {
            continue;
        }

        // ignore vertex pointer, it doesn't refer to an accessor
        if (input.mType == IT_Vertex) {
            // warn if the vertex channel does not refer to the <vertices> element in the same mesh
            if (input.mAccessor != pMesh.mVertexID) {
                throw DeadlyImportError("Unsupported vertex referencing scheme.");
            }
            continue;
        }

        // find accessor
        input.mResolved = &ResolveLibraryReference(mAccessorLibrary, input.mAccessor);
        // resolve accessor's data pointer as well, if necessary
        const Accessor *acc = input.mResolved;
        if (!acc->mData) {
            acc->mData = &ResolveLibraryReference(mDataLibrary, acc->mSource);
        }
    }

    // For continued primitives, the given count does not come all in one <p>, but only one primitive per <p>
    size_t numPrimitives = pNumPrimitives;
    if (pPrimType == Prim_TriFans || pPrimType == Prim_Polygon)
        numPrimitives = 1;
    // For continued primitives, the given count is actually the number of <p>'s inside the parent tag
    if (pPrimType == Prim_TriStrips) {
        size_t numberOfVertices = indices.size() / numOffsets;
        numPrimitives = numberOfVertices - 2;
    }
    if (pPrimType == Prim_LineStrip) {
        size_t numberOfVertices = indices.size() / numOffsets;
        numPrimitives = numberOfVertices - 1;
    }

    pMesh.mFaceSize.reserve(numPrimitives);
    pMesh.mFacePosIndices.reserve(indices.size() / numOffsets);

    size_t polylistStartVertex = 0;
    for (size_t currentPrimitive = 0; currentPrimitive < numPrimitives; currentPrimitive++) {
        // determine number of points for this primitive
        size_t numPoints = 0;
        switch (pPrimType) {
        case Prim_Lines:
            numPoints = 2;
            for (size_t currentVertex = 0; currentVertex < numPoints; currentVertex++)
                CopyVertex(currentVertex, numOffsets, numPoints, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
            break;
        case Prim_LineStrip:
            numPoints = 2;
            for (size_t currentVertex = 0; currentVertex < numPoints; currentVertex++)
                CopyVertex(currentVertex, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
            break;
        case Prim_Triangles:
            numPoints = 3;
            for (size_t currentVertex = 0; currentVertex < numPoints; currentVertex++)
                CopyVertex(currentVertex, numOffsets, numPoints, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
            break;
        case Prim_TriStrips:
            numPoints = 3;
            ReadPrimTriStrips(numOffsets, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
            break;
        case Prim_Polylist:
            numPoints = pVCount[currentPrimitive];
            for (size_t currentVertex = 0; currentVertex < numPoints; currentVertex++)
                CopyVertex(polylistStartVertex + currentVertex, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, 0, indices);
            polylistStartVertex += numPoints;
            break;
        case Prim_TriFans:
        case Prim_Polygon:
            numPoints = indices.size() / numOffsets;
            for (size_t currentVertex = 0; currentVertex < numPoints; currentVertex++)
                CopyVertex(currentVertex, numOffsets, numPoints, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
            break;
        default:
            // LineStrip is not supported due to expected index unmangling
            throw DeadlyImportError("Unsupported primitive type.");
            break;
        }

        // store the face size to later reconstruct the face from
        pMesh.mFaceSize.push_back(numPoints);
    }

    // if I ever get my hands on that guy who invented this steaming pile of indirection...
    return numPrimitives;
}

///@note This function won't work correctly if both PerIndex and PerVertex channels have same channels.
///For example if TEXCOORD present in both <vertices> and <polylist> tags this function will create wrong uv coordinates.
///It's not clear from COLLADA documentation is this allowed or not. For now only exporter fixed to avoid such behavior
void ColladaParser::CopyVertex(size_t currentVertex, size_t numOffsets, size_t numPoints, size_t perVertexOffset, Mesh &pMesh,
        std::vector<InputChannel> &pPerIndexChannels, size_t currentPrimitive, const std::vector<size_t> &indices) {
    // calculate the base offset of the vertex whose attributes we ant to copy
    size_t baseOffset = currentPrimitive * numOffsets * numPoints + currentVertex * numOffsets;

    // don't overrun the boundaries of the index list
    ai_assert((baseOffset + numOffsets - 1) < indices.size());

    // extract per-vertex channels using the global per-vertex offset
    for (std::vector<InputChannel>::iterator it = pMesh.mPerVertexData.begin(); it != pMesh.mPerVertexData.end(); ++it) {
        ExtractDataObjectFromChannel(*it, indices[baseOffset + perVertexOffset], pMesh);
    }
    // and extract per-index channels using there specified offset
    for (std::vector<InputChannel>::iterator it = pPerIndexChannels.begin(); it != pPerIndexChannels.end(); ++it) {
        ExtractDataObjectFromChannel(*it, indices[baseOffset + it->mOffset], pMesh);
    }

    // store the vertex-data index for later assignment of bone vertex weights
    pMesh.mFacePosIndices.push_back(indices[baseOffset + perVertexOffset]);
}

void ColladaParser::ReadPrimTriStrips(size_t numOffsets, size_t perVertexOffset, Mesh &pMesh, std::vector<InputChannel> &pPerIndexChannels,
        size_t currentPrimitive, const std::vector<size_t> &indices) {
    if (currentPrimitive % 2 != 0) {
        //odd tristrip triangles need their indices mangled, to preserve winding direction
        CopyVertex(1, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
        CopyVertex(0, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
        CopyVertex(2, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
    } else { //for non tristrips or even tristrip triangles
        CopyVertex(0, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
        CopyVertex(1, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
        CopyVertex(2, numOffsets, 1, perVertexOffset, pMesh, pPerIndexChannels, currentPrimitive, indices);
    }
}

// ------------------------------------------------------------------------------------------------
// Extracts a single object from an input channel and stores it in the appropriate mesh data array
void ColladaParser::ExtractDataObjectFromChannel(const InputChannel &pInput, size_t pLocalIndex, Mesh &pMesh) {
    // ignore vertex referrer - we handle them that separate
    if (pInput.mType == IT_Vertex) {
        return;
    }

    const Accessor &acc = *pInput.mResolved;
    if (pLocalIndex >= acc.mCount) {
        throw DeadlyImportError("Invalid data index (", pLocalIndex, "/", acc.mCount, ") in primitive specification");
    }

    // get a pointer to the start of the data object referred to by the accessor and the local index
    const ai_real *dataObject = &(acc.mData->mValues[0]) + acc.mOffset + pLocalIndex * acc.mStride;

    // assemble according to the accessors component sub-offset list. We don't care, yet,
    // what kind of object exactly we're extracting here
    ai_real obj[4];
    for (size_t c = 0; c < 4; ++c) {
        obj[c] = dataObject[acc.mSubOffset[c]];
    }

    // now we reinterpret it according to the type we're reading here
    switch (pInput.mType) {
    case IT_Position: // ignore all position streams except 0 - there can be only one position
        if (pInput.mIndex == 0) {
            pMesh.mPositions.emplace_back(obj[0], obj[1], obj[2]);
        } else {
            ASSIMP_LOG_ERROR("Collada: just one vertex position stream supported");
        }
        break;
    case IT_Normal:
        // pad to current vertex count if necessary
        if (pMesh.mNormals.size() < pMesh.mPositions.size() - 1)
            pMesh.mNormals.insert(pMesh.mNormals.end(), pMesh.mPositions.size() - pMesh.mNormals.size() - 1, aiVector3D(0, 1, 0));

        // ignore all normal streams except 0 - there can be only one normal
        if (pInput.mIndex == 0) {
            pMesh.mNormals.emplace_back(obj[0], obj[1], obj[2]);
        } else {
            ASSIMP_LOG_ERROR("Collada: just one vertex normal stream supported");
        }
        break;
    case IT_Tangent:
        // pad to current vertex count if necessary
        if (pMesh.mTangents.size() < pMesh.mPositions.size() - 1)
            pMesh.mTangents.insert(pMesh.mTangents.end(), pMesh.mPositions.size() - pMesh.mTangents.size() - 1, aiVector3D(1, 0, 0));

        // ignore all tangent streams except 0 - there can be only one tangent
        if (pInput.mIndex == 0) {
            pMesh.mTangents.emplace_back(obj[0], obj[1], obj[2]);
        } else {
            ASSIMP_LOG_ERROR("Collada: just one vertex tangent stream supported");
        }
        break;
    case IT_Bitangent:
        // pad to current vertex count if necessary
        if (pMesh.mBitangents.size() < pMesh.mPositions.size() - 1) {
            pMesh.mBitangents.insert(pMesh.mBitangents.end(), pMesh.mPositions.size() - pMesh.mBitangents.size() - 1, aiVector3D(0, 0, 1));
        }

        // ignore all bitangent streams except 0 - there can be only one bitangent
        if (pInput.mIndex == 0) {
            pMesh.mBitangents.emplace_back(obj[0], obj[1], obj[2]);
        } else {
            ASSIMP_LOG_ERROR("Collada: just one vertex bitangent stream supported");
        }
        break;
    case IT_Texcoord:
        // up to 4 texture coord sets are fine, ignore the others
        if (pInput.mIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            // pad to current vertex count if necessary
            if (pMesh.mTexCoords[pInput.mIndex].size() < pMesh.mPositions.size() - 1)
                pMesh.mTexCoords[pInput.mIndex].insert(pMesh.mTexCoords[pInput.mIndex].end(),
                        pMesh.mPositions.size() - pMesh.mTexCoords[pInput.mIndex].size() - 1, aiVector3D(0, 0, 0));

            pMesh.mTexCoords[pInput.mIndex].emplace_back(obj[0], obj[1], obj[2]);
            if (0 != acc.mSubOffset[2] || 0 != acc.mSubOffset[3]) {
                pMesh.mNumUVComponents[pInput.mIndex] = 3;
            }
        } else {
            ASSIMP_LOG_ERROR("Collada: too many texture coordinate sets. Skipping.");
        }
        break;
    case IT_Color:
        // up to 4 color sets are fine, ignore the others
        if (pInput.mIndex < AI_MAX_NUMBER_OF_COLOR_SETS) {
            // pad to current vertex count if necessary
            if (pMesh.mColors[pInput.mIndex].size() < pMesh.mPositions.size() - 1)
                pMesh.mColors[pInput.mIndex].insert(pMesh.mColors[pInput.mIndex].end(),
                        pMesh.mPositions.size() - pMesh.mColors[pInput.mIndex].size() - 1, aiColor4D(0, 0, 0, 1));

            aiColor4D result(0, 0, 0, 1);
            for (size_t i = 0; i < pInput.mResolved->mSize; ++i) {
                result[static_cast<unsigned int>(i)] = obj[pInput.mResolved->mSubOffset[i]];
            }
            pMesh.mColors[pInput.mIndex].push_back(result);
        } else {
            ASSIMP_LOG_ERROR("Collada: too many vertex color sets. Skipping.");
        }

        break;
    default:
        // IT_Invalid and IT_Vertex
        ai_assert(false && "shouldn't ever get here");
    }
}

// ------------------------------------------------------------------------------------------------
// Reads the library of node hierarchies and scene parts
void ColladaParser::ReadSceneLibrary(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "visual_scene") {
            // read ID. Is optional according to the spec, but how on earth should a scene_instance refer to it then?
            std::string id;
            XmlParser::getStdStrAttribute(currentNode, "id", id);

            // read name if given.
            std::string attrName = "Scene";
            if (XmlParser::hasAttribute(currentNode, "name")) {
                XmlParser::getStdStrAttribute(currentNode, "name", attrName);
            }

            // create a node and store it in the library under its ID
            Node *sceneNode = new Node;
            sceneNode->mID = id;
            sceneNode->mName = attrName;
            mNodeLibrary[sceneNode->mID] = sceneNode;

            ReadSceneNode(currentNode, sceneNode);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a scene node's contents including children and stores it in the given node
void ColladaParser::ReadSceneNode(XmlNode &node, Node *pNode) {
    // quit immediately on <bla/> elements
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "node") {
            Node *child = new Node;
            if (XmlParser::hasAttribute(currentNode, "id")) {
                XmlParser::getStdStrAttribute(currentNode, "id", child->mID);
            }
            if (XmlParser::hasAttribute(currentNode, "sid")) {
                XmlParser::getStdStrAttribute(currentNode, "sid", child->mSID);
            }
            if (XmlParser::hasAttribute(currentNode, "name")) {
                XmlParser::getStdStrAttribute(currentNode, "name", child->mName);
            }
            if (pNode) {
                pNode->mChildren.push_back(child);
                child->mParent = pNode;
            } else {
                // no parent node given, probably called from <library_nodes> element.
                // create new node in node library
                mNodeLibrary[child->mID] = child;
            }

            // read on recursively from there
            ReadSceneNode(currentNode, child);
            continue;
        } else if (!pNode) {
            // For any further stuff we need a valid node to work on
            continue;
        }
        if (currentName == "lookat") {
            ReadNodeTransformation(currentNode, pNode, TF_LOOKAT);
        } else if (currentName == "matrix") {
            ReadNodeTransformation(currentNode, pNode, TF_MATRIX);
        } else if (currentName == "rotate") {
            ReadNodeTransformation(currentNode, pNode, TF_ROTATE);
        } else if (currentName == "scale") {
            ReadNodeTransformation(currentNode, pNode, TF_SCALE);
        } else if (currentName == "skew") {
            ReadNodeTransformation(currentNode, pNode, TF_SKEW);
        } else if (currentName == "translate") {
            ReadNodeTransformation(currentNode, pNode, TF_TRANSLATE);
        } else if (currentName == "render" && pNode->mParent == nullptr && 0 == pNode->mPrimaryCamera.length()) {
            // ... scene evaluation or, in other words, postprocessing pipeline,
            // or, again in other words, a turing-complete description how to
            // render a Collada scene. The only thing that is interesting for
            // us is the primary camera.
            if (XmlParser::hasAttribute(currentNode, "camera_node")) {
                std::string s;
                XmlParser::getStdStrAttribute(currentNode, "camera_node", s);
                if (s[0] != '#') {
                    ASSIMP_LOG_ERROR("Collada: Unresolved reference format of camera");
                } else {
                    pNode->mPrimaryCamera = s.c_str() + 1;
                }
            }
        } else if (currentName == "instance_node") {
            // find the node in the library
            if (XmlParser::hasAttribute(currentNode, "url")) {
                std::string s;
                XmlParser::getStdStrAttribute(currentNode, "url", s);
                if (s[0] != '#') {
                    ASSIMP_LOG_ERROR("Collada: Unresolved reference format of node");
                } else {
                    pNode->mNodeInstances.emplace_back();
                    pNode->mNodeInstances.back().mNode = s.c_str() + 1;
                }
            }
        } else if (currentName == "instance_geometry" || currentName == "instance_controller") {
            // Reference to a mesh or controller, with possible material associations
            ReadNodeGeometry(currentNode, pNode);
        } else if (currentName == "instance_light") {
            // Reference to a light, name given in 'url' attribute
            if (XmlParser::hasAttribute(currentNode, "url")) {
                std::string url;
                XmlParser::getStdStrAttribute(currentNode, "url", url);
                if (url[0] != '#') {
                    throw DeadlyImportError("Unknown reference format in <instance_light> element");
                }

                pNode->mLights.emplace_back();
                pNode->mLights.back().mLight = url.c_str() + 1;
            }
        } else if (currentName == "instance_camera") {
            // Reference to a camera, name given in 'url' attribute
            if (XmlParser::hasAttribute(currentNode, "url")) {
                std::string url;
                XmlParser::getStdStrAttribute(currentNode, "url", url);
                if (url[0] != '#') {
                    throw DeadlyImportError("Unknown reference format in <instance_camera> element");
                }
                pNode->mCameras.emplace_back();
                pNode->mCameras.back().mCamera = url.c_str() + 1;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a node transformation entry of the given type and adds it to the given node's transformation list.
void ColladaParser::ReadNodeTransformation(XmlNode &node, Node *pNode, TransformType pType) {
    if (node.empty()) {
        return;
    }

    std::string tagName = node.name();

    Transform tf;
    tf.mType = pType;

    // read SID
    if (XmlParser::hasAttribute(node, "sid")) {
        XmlParser::getStdStrAttribute(node, "sid", tf.mID);
    }

    // how many parameters to read per transformation type
    static const unsigned int sNumParameters[] = { 9, 4, 3, 3, 7, 16 };
    std::string value;
    XmlParser::getValueAsString(node, value);
    const char *content = value.c_str();

    // read as many parameters and store in the transformation
    for (unsigned int a = 0; a < sNumParameters[pType]; a++) {
        // skip whitespace before the number
        SkipSpacesAndLineEnd(&content);
        // read a number
        content = fast_atoreal_move<ai_real>(content, tf.f[a]);
    }

    // place the transformation at the queue of the node
    pNode->mTransforms.push_back(tf);
}

// ------------------------------------------------------------------------------------------------
// Processes bind_vertex_input and bind elements
void ColladaParser::ReadMaterialVertexInputBinding(XmlNode &node, Collada::SemanticMappingTable &tbl) {
    std::string name = node.name();
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "bind_vertex_input") {
            Collada::InputSemanticMapEntry vn;

            // effect semantic
            if (XmlParser::hasAttribute(currentNode, "semantic")) {
                std::string s;
                XmlParser::getStdStrAttribute(currentNode, "semantic", s);
                XmlParser::getUIntAttribute(currentNode, "input_semantic", (unsigned int &)vn.mType);
            }
            std::string s;
            XmlParser::getStdStrAttribute(currentNode, "semantic", s);

            // input semantic
            XmlParser::getUIntAttribute(currentNode, "input_semantic", (unsigned int &)vn.mType);

            // index of input set
            if (XmlParser::hasAttribute(currentNode, "input_set")) {
                XmlParser::getUIntAttribute(currentNode, "input_set", vn.mSet);
            }

            tbl.mMap[s] = vn;
        } else if (currentName == "bind") {
            ASSIMP_LOG_WARN("Collada: Found unsupported <bind> element");
        }
    }
}

void ColladaParser::ReadEmbeddedTextures(ZipArchiveIOSystem &zip_archive) {
    // Attempt to load any undefined Collada::Image in ImageLibrary
    for (auto &it : mImageLibrary) {
        Collada::Image &image = it.second;

        if (image.mImageData.empty()) {
            std::unique_ptr<IOStream> image_file(zip_archive.Open(image.mFileName.c_str()));
            if (image_file) {
                image.mImageData.resize(image_file->FileSize());
                image_file->Read(image.mImageData.data(), image_file->FileSize(), 1);
                image.mEmbeddedFormat = BaseImporter::GetExtension(image.mFileName);
                if (image.mEmbeddedFormat == "jpeg") {
                    image.mEmbeddedFormat = "jpg";
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Reads a mesh reference in a node and adds it to the node's mesh list
void ColladaParser::ReadNodeGeometry(XmlNode &node, Node *pNode) {
    // referred mesh is given as an attribute of the <instance_geometry> element
    std::string url;
    XmlParser::getStdStrAttribute(node, "url", url);
    if (url[0] != '#') {
        throw DeadlyImportError("Unknown reference format");
    }

    Collada::MeshInstance instance;
    instance.mMeshOrController = url.c_str() + 1; // skipping the leading #

    for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "bind_material") {
            XmlNode techNode = currentNode.child("technique_common");
            if (techNode) {
                for (XmlNode instanceMatNode = techNode.child("instance_material"); instanceMatNode; instanceMatNode = instanceMatNode.next_sibling())
                {
                    const std::string &instance_name = instanceMatNode.name();
                    if (instance_name == "instance_material")
                    {
                        // read ID of the geometry subgroup and the target material
                        std::string group;
                        XmlParser::getStdStrAttribute(instanceMatNode, "symbol", group);
                        XmlParser::getStdStrAttribute(instanceMatNode, "target", url);
                        const char *urlMat = url.c_str();
                        Collada::SemanticMappingTable s;
                        if (urlMat[0] == '#')
                            urlMat++;

                        s.mMatName = urlMat;
                        // store the association
                        instance.mMaterials[group] = s;
                        ReadMaterialVertexInputBinding(instanceMatNode, s);
                    }
                }
            }
        }
    }

    // store it
    pNode->mMeshes.push_back(instance);
}

// ------------------------------------------------------------------------------------------------
// Reads the collada scene
void ColladaParser::ReadScene(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "instance_visual_scene") {
            // should be the first and only occurrence
            if (mRootNode) {
                throw DeadlyImportError("Invalid scene containing multiple root nodes in <instance_visual_scene> element");
            }

            // read the url of the scene to instance. Should be of format "#some_name"
            std::string url;
            XmlParser::getStdStrAttribute(currentNode, "url", url);
            if (url[0] != '#') {
                throw DeadlyImportError("Unknown reference format in <instance_visual_scene> element");
            }

            // find the referred scene, skip the leading #
            NodeLibrary::const_iterator sit = mNodeLibrary.find(url.c_str() + 1);
            if (sit == mNodeLibrary.end()) {
                throw DeadlyImportError("Unable to resolve visual_scene reference \"", std::string(url), "\" in <instance_visual_scene> element.");
            }
            mRootNode = sit->second;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Calculates the resulting transformation from all the given transform steps
aiMatrix4x4 ColladaParser::CalculateResultTransform(const std::vector<Transform> &pTransforms) const {
    aiMatrix4x4 res;

    for (std::vector<Transform>::const_iterator it = pTransforms.begin(); it != pTransforms.end(); ++it) {
        const Transform &tf = *it;
        switch (tf.mType) {
        case TF_LOOKAT: {
            aiVector3D pos(tf.f[0], tf.f[1], tf.f[2]);
            aiVector3D dstPos(tf.f[3], tf.f[4], tf.f[5]);
            aiVector3D up = aiVector3D(tf.f[6], tf.f[7], tf.f[8]).Normalize();
            aiVector3D dir = aiVector3D(dstPos - pos).Normalize();
            aiVector3D right = (dir ^ up).Normalize();

            res *= aiMatrix4x4(
                    right.x, up.x, -dir.x, pos.x,
                    right.y, up.y, -dir.y, pos.y,
                    right.z, up.z, -dir.z, pos.z,
                    0, 0, 0, 1);
            break;
        }
        case TF_ROTATE: {
            aiMatrix4x4 rot;
            ai_real angle = tf.f[3] * ai_real(AI_MATH_PI) / ai_real(180.0);
            aiVector3D axis(tf.f[0], tf.f[1], tf.f[2]);
            aiMatrix4x4::Rotation(angle, axis, rot);
            res *= rot;
            break;
        }
        case TF_TRANSLATE: {
            aiMatrix4x4 trans;
            aiMatrix4x4::Translation(aiVector3D(tf.f[0], tf.f[1], tf.f[2]), trans);
            res *= trans;
            break;
        }
        case TF_SCALE: {
            aiMatrix4x4 scale(tf.f[0], 0.0f, 0.0f, 0.0f, 0.0f, tf.f[1], 0.0f, 0.0f, 0.0f, 0.0f, tf.f[2], 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
            res *= scale;
            break;
        }
        case TF_SKEW:
            // TODO: (thom)
            ai_assert(false);
            break;
        case TF_MATRIX: {
            aiMatrix4x4 mat(tf.f[0], tf.f[1], tf.f[2], tf.f[3], tf.f[4], tf.f[5], tf.f[6], tf.f[7],
                    tf.f[8], tf.f[9], tf.f[10], tf.f[11], tf.f[12], tf.f[13], tf.f[14], tf.f[15]);
            res *= mat;
            break;
        }
        default:
            ai_assert(false);
            break;
        }
    }

    return res;
}

// ------------------------------------------------------------------------------------------------
// Determines the input data type for the given semantic string
Collada::InputType ColladaParser::GetTypeForSemantic(const std::string &semantic) {
    if (semantic.empty()) {
        ASSIMP_LOG_WARN("Vertex input type is empty.");
        return IT_Invalid;
    }

    if (semantic == "POSITION")
        return IT_Position;
    else if (semantic == "TEXCOORD")
        return IT_Texcoord;
    else if (semantic == "NORMAL")
        return IT_Normal;
    else if (semantic == "COLOR")
        return IT_Color;
    else if (semantic == "VERTEX")
        return IT_Vertex;
    else if (semantic == "BINORMAL" || semantic == "TEXBINORMAL")
        return IT_Bitangent;
    else if (semantic == "TANGENT" || semantic == "TEXTANGENT")
        return IT_Tangent;

    ASSIMP_LOG_WARN("Unknown vertex input type \"", semantic, "\". Ignoring.");
    return IT_Invalid;
}

#endif // !! ASSIMP_BUILD_NO_DAE_IMPORTER
