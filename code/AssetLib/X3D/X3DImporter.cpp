/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
/// \file   X3DImporter.cpp
/// \brief  X3D-format files importer for Assimp: main algorithm implementation.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"

#include <assimp/StringUtils.h>
#include <assimp/ParsingUtils.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/fast_atof.h>

// Header files, stdlib.
#include <iterator>
#include <memory>

namespace Assimp {

/// Constant which holds the importer description
const aiImporterDesc X3DImporter::Description = {
    "Extensible 3D(X3D) Importer",
    "smalcom",
    "",
    "See documentation in source code. Chapter: Limitations.",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "x3d x3db"
};

struct WordIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = const char *;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    static const char *whitespace;
    const char *mStart, *mEnd;

    WordIterator(const char *start, const char *end) :
            mStart(start),
            mEnd(end) {
        mStart = start + ::strspn(start, whitespace);
        if (mStart >= mEnd) {
            mStart = 0;
        }
    }
    WordIterator() :
            mStart(0),
            mEnd(0) {}
    WordIterator(const WordIterator &other) :
            mStart(other.mStart),
            mEnd(other.mEnd) {}
    WordIterator &operator=(const WordIterator &other) {
        mStart = other.mStart;
        mEnd = other.mEnd;
        return *this;
    }

    bool operator==(const WordIterator &other) const { return mStart == other.mStart; }

    bool operator!=(const WordIterator &other) const { return mStart != other.mStart; }

    WordIterator &operator++() {
        mStart += strcspn(mStart, whitespace);
        mStart += strspn(mStart, whitespace);
        if (mStart >= mEnd) {
            mStart = 0;
        }
        return *this;
    }
    WordIterator operator++(int) {
        WordIterator result(*this);
        ++(*this);
        return result;
    }
    const char *operator*() const { return mStart; }
};

const char *WordIterator::whitespace = ", \t\r\n";

X3DImporter::X3DImporter() :
        mNodeElementCur(nullptr),
        mScene(nullptr) {
    // empty
}

X3DImporter::~X3DImporter() {
    // Clear() is accounting if data already is deleted. So, just check again if all data is deleted.
    Clear();
}

void X3DImporter::Clear() {
    mNodeElementCur = nullptr;
    // Delete all elements
    if (!NodeElement_List.empty()) {
        for (std::list<X3DNodeElementBase *>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); ++it) {
            delete *it;
        }
        NodeElement_List.clear();
    }
}

void X3DImporter::ParseFile(const std::string &file, IOSystem *pIOHandler) {
    ai_assert(nullptr != pIOHandler);

    static const std::string mode = "rb";
    std::unique_ptr<IOStream> fileStream(pIOHandler->Open(file, mode));
    if (!fileStream.get()) {
        throw DeadlyImportError("Failed to open file " + file + ".");
    }

    XmlParser theParser;
    if (!theParser.parse(fileStream.get())) {
        return;
    }

    XmlNode *node = theParser.findNode("X3D");
    if (nullptr == node) {
        return;
    }

    for (auto &currentNode : node->children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "head") {
            readMetadata(currentNode);
        } else if (currentName == "Scene") {
            readScene(currentNode);
        }
    }
}

bool X3DImporter::CanRead(const std::string &pFile, IOSystem * /*pIOHandler*/, bool checkSig) const {
    if (checkSig) {
        std::string::size_type pos = pFile.find_last_of(".x3d");
        if (pos != std::string::npos) {
            return true;
        }
    }

    return false;
}

void X3DImporter::GetExtensionList(std::set<std::string> &extensionList) {
    extensionList.insert("x3d");
}

void X3DImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
    std::shared_ptr<IOStream> stream(pIOHandler->Open(pFile, "rb"));
    if (!stream) {
        throw DeadlyImportError("Could not open file for reading");
    }

    mScene = pScene;
    pScene->mRootNode = new aiNode(pFile);
}

const aiImporterDesc *X3DImporter::GetInfo() const {
    return &Description;
}

struct meta_entry {
    std::string name;
    std::string value;
};

void X3DImporter::readMetadata(XmlNode &node) {
    std::vector<meta_entry> metaArray;
    for (auto currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "meta") {
            meta_entry entry;
            if (XmlParser::getStdStrAttribute(currentNode, "name", entry.name)) {
                XmlParser::getStdStrAttribute(currentNode, "content", entry.value);
                metaArray.emplace_back(entry);
            }
        }
    }
    mScene->mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(metaArray.size()));
    unsigned int i = 0;
    for (auto currentMeta : metaArray) {
        mScene->mMetaData->Set(i, currentMeta.name, currentMeta.value);
        ++i;
    }
}

void X3DImporter::readScene(XmlNode &node) {
    for (auto currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "Viewpoint") {
            readViewpoint(currentNode);
        }
    }
}

void X3DImporter::readViewpoint(XmlNode &node) {
    for (auto currentNode : node.children()) {
        //const std::string &currentName = currentNode.name();
    }
}

void readMetadataBoolean(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaBoolean *boolean = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        boolean = new X3DNodeElementMetaBoolean(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            bool current_boolean = false;
            if (values[i] == "true") {
                current_boolean = true;
            }
            boolean->Value.emplace_back(current_boolean);
        }
    }
}

void readMetadataDouble(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaDouble *doubleNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        doubleNode = new X3DNodeElementMetaDouble(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            double current_double = static_cast<double>(fast_atof(values[i].c_str()));
            doubleNode->Value.emplace_back(current_double);
        }
    }
}

void readMetadataFloat(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaFloat *floatNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        floatNode = new X3DNodeElementMetaFloat(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            float current_float = static_cast<float>(fast_atof(values[i].c_str()));
            floatNode->Value.emplace_back(current_float);
        }
    }
}

void readMetadataInteger(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaInt *intNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        intNode = new X3DNodeElementMetaInt(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            int current_int = static_cast<int>(std::atoi(values[i].c_str()));
            intNode->Value.emplace_back(current_int);
        }
    }
}

void readMetadataSet(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaSet *setNode = new X3DNodeElementMetaSet(parent);
    if (XmlParser::getStdStrAttribute(node, "name", val)) {
        setNode->Name = val;
    }

    if (XmlParser::getStdStrAttribute(node, "reference", val)) {
        setNode->Reference = val;
    }
}

void readMetadataString(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaString *strNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        strNode = new X3DNodeElementMetaString(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            strNode->Value.emplace_back(values[i]);
        }
    }
}

void X3DImporter::readMetadataObject(XmlNode &node) {
    const std::string &name = node.name();
    if (name == "MetadataBoolean") {
        readMetadataBoolean(node, mNodeElementCur);
    } else if (name == "MetadataDouble") {
        readMetadataDouble(node, mNodeElementCur);
    } else if (name == "MetadataFloat") {
        readMetadataFloat(node, mNodeElementCur);
    } else if (name == "MetadataInteger") {
        readMetadataInteger(node, mNodeElementCur);
    } else if (name == "MetadataSet") {
        readMetadataSet(node, mNodeElementCur);
    } else if (name == "MetadataString") {
        readMetadataString(node, mNodeElementCur);
    }
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
