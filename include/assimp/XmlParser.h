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

#ifndef INCLUDED_AI_IRRXML_WRAPPER
#define INCLUDED_AI_IRRXML_WRAPPER

#include <assimp/ai_assert.h>
#include <assimp/DefaultLogger.hpp>

#include "BaseImporter.h"
#include "IOStream.hpp"

#include <pugixml.hpp>
#include <utility>
#include <vector>

namespace Assimp {

/// @brief  Will find a node by its name.
struct find_node_by_name_predicate {
    /// @brief The default constructor.
    find_node_by_name_predicate() = default;


    std::string mName; ///< The name to find.
    find_node_by_name_predicate(const std::string &name) :
            mName(name) {
        // empty
    }

    bool operator()(pugi::xml_node node) const {
        return node.name() == mName;
    }
};

/// @brief  Will convert an attribute to its int value.
/// @tparam[in] TNodeType  The node type.
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

/// @brief The Xml-Parser class.
///
/// Use this parser if you have to import any kind of xml-format.
///
/// An example:
/// @code
/// TXmlParser<XmlNode> theParser;
/// if (theParser.parse(fileStream)) {
///     auto node = theParser.getRootNode();
///     for ( auto currentNode : node.children()) {
///         // Will loop over all children
///     }
/// }
/// @endcode
/// @tparam TNodeType
template <class TNodeType>
class TXmlParser {
public:
    /// @brief The default class constructor.
    TXmlParser();

    ///	@brief  The class destructor.
    ~TXmlParser();

    ///	@brief  Will clear the parsed xml-file.
    void clear();

    ///	@brief  Will search for a child-node by its name
    /// @param[in] name     The name of the child-node.
    /// @return The node instance or nullptr, if nothing was found.
    TNodeType *findNode(const std::string &name);

    /// @brief  Will return true, if the node is a child-node.
    /// @param[in]  name    The name of the child node to look for.
    /// @return true, if the node is a child-node or false if not.
    bool hasNode(const std::string &name);

    /// @brief  Will parse an xml-file from a given stream.
    /// @param[in] stream      The input stream.
    /// @return true, if the parsing was successful, false if not.
    bool parse(IOStream *stream);

    /// @brief  Will return true if a root node is there.
    /// @return true in case of an existing root.
    bool hasRoot() const;

    /// @brief  Will return the document pointer, is nullptr if no xml-file was parsed.
    /// @return The pointer showing to the document.
    pugi::xml_document *getDocument() const;

    /// @brief  Will return the root node, const version.
    /// @return The root node.
    const TNodeType getRootNode() const;

    /// @brief  Will return the root node, non-const version.
    /// @return The root node.
    TNodeType getRootNode();

    /// @brief Will check if a node with the given name is in.
    /// @param[in] node     The node to look in.
    /// @param[in] name     The name of the child-node.
    /// @return true, if node was found, false if not.
    static inline bool hasNode(XmlNode &node, const char *name);

    /// @brief Will check if an attribute is part of the XmlNode.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @return true, if the was found, false if not.
    static inline bool hasAttribute(XmlNode &xmlNode, const char *name);

    /// @brief Will try to get an unsigned int attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The unsigned int value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is an unsigned int.
    static inline bool getUIntAttribute(XmlNode &xmlNode, const char *name, unsigned int &val);

    /// @brief Will try to get an int attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The int value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is an int.
    static inline bool getIntAttribute(XmlNode &xmlNode, const char *name, int &val);

    /// @brief Will try to get a real attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The real value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is a real.
    static inline bool getRealAttribute(XmlNode &xmlNode, const char *name, ai_real &val);

    /// @brief Will try to get a float attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The float value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is a float.
    static inline bool getFloatAttribute(XmlNode &xmlNode, const char *name, float &val);

    /// @brief Will try to get a double attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The double value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is a double.
    static inline bool getDoubleAttribute(XmlNode &xmlNode, const char *name, double &val);

    /// @brief Will try to get a std::string attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The std::string value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is a std::string.
    static inline bool getStdStrAttribute(XmlNode &xmlNode, const char *name, std::string &val);

    /// @brief Will try to get a bool attribute value.
    /// @param[in] xmlNode  The node to search in.
    /// @param[in] name     The attribute name to look for.
    /// @param[out] val     The bool value from the attribute.
    /// @return true, if the node contains an attribute with the given name and if the value is a bool.
    static inline bool getBoolAttribute(XmlNode &xmlNode, const char *name, bool &val);

    /// @brief Will try to get the value of the node as a string.
    /// @param[in] node     The node to search in.
    /// @param[out] text    The value as a text.
    /// @return true, if the value can be read out.
    static inline bool getValueAsString(XmlNode &node, std::string &text);

    /// @brief Will try to get the value of the node as a float.
    /// @param[in] node     The node to search in.
    /// @param[out] text    The value as a float.
    /// @return true, if the value can be read out.
    static inline bool getValueAsFloat(XmlNode &node, ai_real &v);

    /// @brief Will try to get the value of the node as an integer.
    /// @param[in] node     The node to search in.
    /// @param[out] text    The value as a int.
    /// @return true, if the value can be read out.
    static inline bool getValueAsInt(XmlNode &node, int &v);

    /// @brief Will try to get the value of the node as an bool.
    /// @param[in] node     The node to search in.
    /// @param[out] text    The value as a bool.
    /// @return true, if the value can be read out.
    static inline bool getValueAsBool(XmlNode &node, bool &v);

private:
    pugi::xml_document *mDoc;
    TNodeType mCurrent;
    std::vector<char> mData;
};

template <class TNodeType>
inline TXmlParser<TNodeType>::TXmlParser() :
        mDoc(nullptr),
        mData() {
    // empty
}

template <class TNodeType>
inline TXmlParser<TNodeType>::~TXmlParser() {
    clear();
}

template <class TNodeType>
inline void TXmlParser<TNodeType>::clear() {
    if (mData.empty()) {
        if (mDoc) {
            delete mDoc;
        }
        mDoc = nullptr;
        return;
    }

    mData.clear();
    delete mDoc;
    mDoc = nullptr;
}

template <class TNodeType>
inline TNodeType *TXmlParser<TNodeType>::findNode(const std::string &name) {
    if (name.empty()) {
        return nullptr;
    }

    if (nullptr == mDoc) {
        return nullptr;
    }

    find_node_by_name_predicate predicate(name);
    mCurrent = mDoc->find_node(std::move(predicate));
    if (mCurrent.empty()) {
        return nullptr;
    }

    return &mCurrent;
}

template <class TNodeType>
bool TXmlParser<TNodeType>::hasNode(const std::string &name) {
    return nullptr != findNode(name);
}

template <class TNodeType>
bool TXmlParser<TNodeType>::parse(IOStream *stream) {
    if (hasRoot()) {
        clear();
    }

    if (nullptr == stream) {
        ASSIMP_LOG_DEBUG("Stream is nullptr.");
        return false;
    }

    const size_t len = stream->FileSize();
    mData.resize(len + 1);
    memset(&mData[0], '\0', len + 1);
    stream->Read(&mData[0], 1, len);

    mDoc = new pugi::xml_document();
    // load_string assumes native encoding (aka always utf-8 per build options)
    //pugi::xml_parse_result parse_result = mDoc->load_string(&mData[0], pugi::parse_full);
     pugi::xml_parse_result parse_result = mDoc->load_buffer(&mData[0], mData.size(), pugi::parse_full);
    if (parse_result.status == pugi::status_ok) {
        return true;
    }

    ASSIMP_LOG_DEBUG("Error while parse xml.", std::string(parse_result.description()), " @ ", parse_result.offset);

    return false;
}

template <class TNodeType>
bool TXmlParser<TNodeType>::hasRoot() const {
    return nullptr != mDoc;
}

template <class TNodeType>
pugi::xml_document *TXmlParser<TNodeType>::getDocument() const {
    return mDoc;
}

template <class TNodeType>
const TNodeType TXmlParser<TNodeType>::getRootNode() const {
    static pugi::xml_node none;
    if (nullptr == mDoc) {
        return none;
    }
    return mDoc->root();
}

template <class TNodeType>
TNodeType TXmlParser<TNodeType>::getRootNode() {
    static pugi::xml_node none;
    if (nullptr == mDoc) {
        return none;
    }

    return mDoc->root();
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::hasNode(XmlNode &node, const char *name) {
    pugi::xml_node child = node.find_child(find_node_by_name_predicate(name));
    return !child.empty();
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::hasAttribute(XmlNode &xmlNode, const char *name) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    return !attr.empty();
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getUIntAttribute(XmlNode &xmlNode, const char *name, unsigned int &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_uint();
    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getIntAttribute(XmlNode &xmlNode, const char *name, int &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_int();
    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getRealAttribute(XmlNode &xmlNode, const char *name, ai_real &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }
#ifdef ASSIMP_DOUBLE_PRECISION
    val = attr.as_double();
#else
    val = attr.as_float();
#endif
    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getFloatAttribute(XmlNode &xmlNode, const char *name, float &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_float();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getDoubleAttribute(XmlNode &xmlNode, const char *name, double &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_double();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getStdStrAttribute(XmlNode &xmlNode, const char *name, std::string &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_string();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getBoolAttribute(XmlNode &xmlNode, const char *name, bool &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_bool();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsString(XmlNode &node, std::string &text) {
    text = std::string();
    if (node.empty()) {
        return false;
    }

    text = node.text().as_string();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsFloat(XmlNode &node, ai_real &v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_float();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsInt(XmlNode &node, int &v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_int();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsBool(XmlNode &node, bool &v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_bool();

    return true;
}

using XmlParser = TXmlParser<pugi::xml_node>;

///	@brief  This class declares an iterator to loop through all children of the root node.
class XmlNodeIterator {
public:
    /// @brief The iteration mode.
    enum IterationMode {
        PreOrderMode, ///< Pre-ordering, get the values, continue the iteration.
        PostOrderMode ///< Post-ordering, continue the iteration, get the values.
    };
    ///	@brief  The class constructor
    /// @param  parent      [in] The xml parent to to iterate through.
    /// @param  mode        [in] The iteration mode.
    explicit XmlNodeIterator(XmlNode &parent, IterationMode mode) :
            mParent(parent),
            mNodes(),
            mIndex(0) {
        if (mode == PreOrderMode) {
            collectChildrenPreOrder(parent);
        } else {
            collectChildrenPostOrder(parent);
        }
    }

    ///	@brief  The class destructor, default implementation.
    ~XmlNodeIterator() = default;

    ///	@brief  Will iterate through all children in pre-order iteration.
    /// @param  node    [in] The nod to iterate through.
    void collectChildrenPreOrder(XmlNode &node) {
        if (node != mParent && node.type() == pugi::node_element) {
            mNodes.push_back(node);
        }
        for (XmlNode currentNode : node.children()) {
            collectChildrenPreOrder(currentNode);
        }
    }

    ///	@brief  Will iterate through all children in post-order iteration.
    /// @param  node    [in] The nod to iterate through.
    void collectChildrenPostOrder(XmlNode &node) {
        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            collectChildrenPostOrder(currentNode);
        }
        if (node != mParent) {
            mNodes.push_back(node);
        }
    }

    ///	@brief  Will iterate through all collected nodes.
    /// @param  next    The next node, if there is any.
    /// @return true, if there is a node left.
    bool getNext(XmlNode &next) {
        if (mIndex == mNodes.size()) {
            return false;
        }

        next = mNodes[mIndex];
        ++mIndex;

        return true;
    }

    ///	@brief  Will return the number of collected nodes.
    /// @return The number of collected nodes.
    size_t size() const {
        return mNodes.size();
    }

    ///	@brief  Returns true, if the node is empty.
    /// @return true, if the node is empty, false if not.
    bool isEmpty() const {
        return mNodes.empty();
    }

    ///	@brief  Will clear all collected nodes.
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
