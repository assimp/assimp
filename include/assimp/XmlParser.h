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

#ifndef INCLUDED_AI_IRRXML_WRAPPER
#define INCLUDED_AI_IRRXML_WRAPPER

#include <assimp/DefaultLogger.hpp>
#include "BaseImporter.h"
#include "IOStream.hpp"
#include <pugixml.hpp>
#include <vector>

namespace Assimp {

struct find_node_by_name_predicate {
    std::string mName;
    find_node_by_name_predicate(const std::string &name) :
            mName(name) {
        // empty
    }

    bool operator()(pugi::xml_node node) const {
        return node.name() == mName;
    }
};

template <class TNodeType>
struct NodeConverter {
public:
    static int to_int(TNodeType &node, const char *attribName) {
        ai_assert(nullptr != attribName);
        return node.attribute(attribName).to_int();
    }
};

using XmlNode = pugi::xml_node;
using XmlAttribute = pugi::xml_attribute;

template <class TNodeType>
class TXmlParser {
public:
    TXmlParser() :
            mDoc(nullptr),
            mData() {
        // empty
    }

    ~TXmlParser() {
        clear();
    }

    void clear() {
        mData.resize(0);
        delete mDoc;
        mDoc = nullptr;
    }

    TNodeType *findNode(const std::string &name) {
        if (name.empty()) {
            return nullptr;
        }

        if (nullptr == mDoc) {
            return nullptr;
        }

        find_node_by_name_predicate predicate(name);
        mCurrent = mDoc->find_node(predicate);
        if (mCurrent.empty()) {
            return nullptr;
        }

        return &mCurrent;
    }

    bool hasNode(const std::string &name) {
        return nullptr != findNode(name);
    }

    bool parse(IOStream *stream) {
        if (nullptr == stream) {
            ASSIMP_LOG_DEBUG("Stream is nullptr.");
            return false;
        }

        const size_t len = stream->FileSize();
        mData.resize(len + 1);
        memset(&mData[0], '\0', len + 1);
        stream->Read(&mData[0], 1, len);
        
        mDoc = new pugi::xml_document();
        pugi::xml_parse_result parse_result = mDoc->load_string(&mData[0], pugi::parse_full);
        if (parse_result.status == pugi::status_ok) {
            return true;
        } else {
            ASSIMP_LOG_DEBUG("Error while parse xml.");
            return false;
        }
    }

    pugi::xml_document *getDocument() const {
        return mDoc;
    }

    const TNodeType getRootNode() const {
        return mDoc->root();
    }

    TNodeType getRootNode() {
        return mDoc->root();
    }

    static inline bool hasNode(XmlNode &node, const char *name) {
        pugi::xml_node child = node.find_child(find_node_by_name_predicate(name));
        return !child.empty();
    }

    static inline bool hasAttribute(XmlNode &xmlNode, const char *name) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        return !attr.empty();
    }

    static inline bool getUIntAttribute(XmlNode &xmlNode, const char *name, unsigned int &val) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        if (attr.empty()) {
            return false;
        }

        val = attr.as_uint();
        return true;
    }

    static inline bool getIntAttribute(XmlNode &xmlNode, const char *name, int &val ) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        if (attr.empty()) {
            return false;
        }

        val = attr.as_int();
        return true;
    }

    static inline bool getFloatAttribute(XmlNode &xmlNode, const char *name, float &val ) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        if (attr.empty()) {
            return false;
        }

        val = attr.as_float();
        return true;

    }

    static inline bool getDoubleAttribute( XmlNode &xmlNode, const char *name, double &val ) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        if (attr.empty()) {
            return false;
        }

        val = attr.as_double();
        return true;
    }

    static inline bool getStdStrAttribute(XmlNode &xmlNode, const char *name, std::string &val) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        if (attr.empty()) {
            return false;
        }

        val = attr.as_string();
        return true;
    }

    static inline bool getBoolAttribute( XmlNode &xmlNode, const char *name, bool &val ) {
        pugi::xml_attribute attr = xmlNode.attribute(name);
        if (attr.empty()) {
            return false;
        }

        val = attr.as_bool();
        return true;

    }

    static inline bool getValueAsString( XmlNode &node, std::string &text ) {
        text = "";
        if (node.empty()) {
            return false;
        }

        text = node.text().as_string();

        return true;
    }

    static inline bool getValueAsFloat( XmlNode &node, ai_real &v ) {
        if (node.empty()) {
            return false;
        }

        v = node.text().as_float();

        return true;

    }

 private:
    pugi::xml_document *mDoc;
    TNodeType mCurrent;
    std::vector<char> mData;
};

using XmlParser = TXmlParser<pugi::xml_node>;

class XmlNodeIterator {
public:
    XmlNodeIterator(XmlNode &parent) :
            mParent(parent),
            mNodes(),
            mIndex(0) {
        // empty
    }

    void collectChildrenPreOrder( XmlNode &node ) {
        
        if (node != mParent && node.type() == pugi::node_element) {
            mNodes.push_back(node);
        }
        for (XmlNode currentNode : node.children()) {
            collectChildrenPreOrder(currentNode);
        }
    }

    void collectChildrenPostOrder(XmlNode &node) {
        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            collectChildrenPostOrder(currentNode);
        }
        if (node != mParent) {
            mNodes.push_back(node);
        }
    }

    bool getNext(XmlNode &next) {
        if (mIndex == mNodes.size()) {
            return false;
        }

        next = mNodes[mIndex];
        ++mIndex;

        return true;
    }

    size_t size() const {
        return mNodes.size();
    }

    bool isEmpty() const {
        return mNodes.empty();
    }

    void clear() {
        if (mNodes.empty()) {
            return;
        }

        mNodes.clear();
        mIndex = 0;
    }

private:
    XmlNode &mParent; 
    std::vector<XmlNode> mNodes;
    size_t mIndex;
};

} // namespace Assimp

#endif // !! INCLUDED_AI_IRRXML_WRAPPER
