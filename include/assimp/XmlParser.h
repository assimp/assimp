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

// some long includes ....
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

template<class TNodeType>
struct NodeConverter {
public:
    static int to_int(TNodeType &node, const char *attribName ) {
		ai_assert(nullptr != attribName);
        return node.attribute(attribName).to_int();
    }
};

template<class TNodeType>
class TXmlParser {
public:
	TXmlParser() :
			mDoc(nullptr), mRoot(nullptr), mData() {
        // empty
	}

    ~TXmlParser() {
		clear();
    }

    void clear() {
		mData.resize(0);
		mRoot = nullptr;
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

    bool hasNode( const std::string &name ) {
        return nullptr != findNode(name);
    }

    TNodeType *parse(IOStream *stream) {
		if (nullptr == stream) {
			return nullptr;
		}

        mData.resize(stream->FileSize());
		stream->Read(&mData[0], mData.size(), 1);
		mDoc = new pugi::xml_document();
		pugi::xml_parse_result result = mDoc->load_string(&mData[0]);
        if (result.status == pugi::status_ok) {
            pugi::xml_node root = *(mDoc->children().begin());
            
            mRoot = &root;
			//mRoot = &mDoc->root();
        }

        return mRoot;
    }

    pugi::xml_document *getDocument() const {
		return mDoc;
    }

    const TNodeType *getRootNode() const {
		return mRoot;
    }

    TNodeType *getRootNode() {
        return mRoot;
    }

private:
	pugi::xml_document *mDoc;
	TNodeType *mRoot;
    TNodeType mCurrent;
	std::vector<char> mData;
};

using XmlParser = TXmlParser<pugi::xml_node>;
using XmlNode = pugi::xml_node;

static inline bool hasAttribute(XmlNode &xmlNode, const char *name) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    return !attr.empty();
}

} // namespace Assimp

#endif // !! INCLUDED_AI_IRRXML_WRAPPER
