/*-----------------------------------------------------------------------------------------------
The MIT License (MIT)

Copyright (c) 2014-2020 Kim Kulling

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-----------------------------------------------------------------------------------------------*/
#include <openddlparser/DDLNode.h>
#include <openddlparser/OpenDDLParser.h>
#include <openddlparser/OpenDDLStream.h>

#include <algorithm>

BEGIN_ODDLPARSER_NS

DDLNode::DllNodeList DDLNode::s_allocatedNodes;

template <class T>
inline static void releaseDataType(T *ptr) {
    if (nullptr == ptr) {
        return;
    }

    T *current(nullptr);
    while (ptr) {
        current = ptr;
        ptr = ptr->m_next;
        delete current;
    }
}

static void releaseReferencedNames(Reference *ref) {
    if (nullptr == ref) {
        return;
    }

    delete ref;
}

DDLNode::DDLNode(const std::string &type, const std::string &name, size_t idx, DDLNode *parent) :
        m_type(type),
        m_name(name),
        m_parent(parent),
        m_children(),
        m_properties(nullptr),
        m_value(nullptr),
        m_dtArrayList(nullptr),
        m_references(nullptr),
        m_idx(idx) {
    if (m_parent) {
        m_parent->m_children.push_back(this);
    }
}

DDLNode::~DDLNode() {
    delete m_properties;
    delete m_value;
    releaseReferencedNames(m_references);

    delete m_dtArrayList;
    m_dtArrayList = nullptr;
    if (s_allocatedNodes[m_idx] == this) {
        s_allocatedNodes[m_idx] = nullptr;
    }
    for (size_t i = 0; i < m_children.size(); i++) {
        delete m_children[i];
    }
}

void DDLNode::attachParent(DDLNode *parent) {
    if (m_parent == parent) {
        return;
    }

    m_parent = parent;
    if (nullptr != m_parent) {
        m_parent->m_children.push_back(this);
    }
}

void DDLNode::detachParent() {
    if (nullptr != m_parent) {
        DDLNodeIt it = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
        if (m_parent->m_children.end() != it) {
            m_parent->m_children.erase(it);
        }
        m_parent = nullptr;
    }
}

DDLNode *DDLNode::getParent() const {
    return m_parent;
}

const DDLNode::DllNodeList &DDLNode::getChildNodeList() const {
    return m_children;
}

void DDLNode::setType(const std::string &type) {
    m_type = type;
}

const std::string &DDLNode::getType() const {
    return m_type;
}

void DDLNode::setName(const std::string &name) {
    m_name = name;
}

const std::string &DDLNode::getName() const {
    return m_name;
}

void DDLNode::setProperties(Property *prop) {
    if (m_properties != nullptr)
        delete m_properties;
    m_properties = prop;
}

Property *DDLNode::getProperties() const {
    return m_properties;
}

bool DDLNode::hasProperty(const std::string &name) {
    const Property *prop(findPropertyByName(name));
    return (nullptr != prop);
}

bool DDLNode::hasProperties() const {
    return (nullptr != m_properties);
}

Property *DDLNode::findPropertyByName(const std::string &name) {
    if (name.empty()) {
        return nullptr;
    }

    if (nullptr == m_properties) {
        return nullptr;
    }

    Property *current(m_properties);
    while (nullptr != current) {
        int res = strncmp(current->m_key->m_buffer, name.c_str(), name.size());
        if (0 == res) {
            return current;
        }
        current = current->m_next;
    }

    return nullptr;
}

void DDLNode::setValue(Value *val) {
    m_value = val;
}

Value *DDLNode::getValue() const {
    return m_value;
}

void DDLNode::setDataArrayList(DataArrayList *dtArrayList) {
    m_dtArrayList = dtArrayList;
}

DataArrayList *DDLNode::getDataArrayList() const {
    return m_dtArrayList;
}

void DDLNode::setReferences(Reference *refs) {
    m_references = refs;
}

Reference *DDLNode::getReferences() const {
    return m_references;
}

void DDLNode::dump(IOStreamBase &stream) {
    if (!stream.isOpen()) {
        return;
    }

    const std::string &type = this->getType();
    stream.write("type = " + type);
    Value::Iterator it(getValue());
    while (it.hasNext()) {
        Value *v = it.getNext();
        v->dump(stream);
    }
}

DDLNode *DDLNode::create(const std::string &type, const std::string &name, DDLNode *parent) {
    const size_t idx(s_allocatedNodes.size());
    DDLNode *node = new DDLNode(type, name, idx, parent);
    s_allocatedNodes.push_back(node);

    return node;
}

void DDLNode::releaseNodes() {
    if (s_allocatedNodes.size() > 0) {
        for (DDLNodeIt it = s_allocatedNodes.begin(); it != s_allocatedNodes.end(); it++) {
            if (*it) {
                delete *it;
            }
        }
        s_allocatedNodes.clear();
    }
}

END_ODDLPARSER_NS
