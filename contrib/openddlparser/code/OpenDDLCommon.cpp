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
#include <openddlparser/OpenDDLCommon.h>
#include <openddlparser/Value.h>

BEGIN_ODDLPARSER_NS

Text::Text(const char *buffer, size_t numChars) :
        m_capacity(0),
        m_len(0),
        m_buffer(nullptr) {
    set(buffer, numChars);
}

Text::~Text() {
    clear();
}

void Text::clear() {
    delete[] m_buffer;
    m_buffer = nullptr;
    m_capacity = 0;
    m_len = 0;
}

void Text::set(const char *buffer, size_t numChars) {
    clear();
    if (numChars > 0) {
        m_len = numChars;
        m_capacity = m_len + 1;
        m_buffer = new char[m_capacity];
        strncpy(m_buffer, buffer, numChars);
        m_buffer[numChars] = '\0';
    }
}

bool Text::operator==(const std::string &name) const {
    if (m_len != name.size()) {
        return false;
    }
    const int res(strncmp(m_buffer, name.c_str(), name.size()));

    return (0 == res);
}

bool Text::operator==(const Text &rhs) const {
    if (m_len != rhs.m_len) {
        return false;
    }

    const int res(strncmp(m_buffer, rhs.m_buffer, m_len));

    return (0 == res);
}

Name::Name(NameType type, Text *id) :
        m_type(type), m_id(id) {
    // empty
}

Name::~Name() {
    delete m_id;
    m_id = nullptr;
}

Name::Name(const Name &name) {
    m_type = name.m_type;
    m_id = new Text(name.m_id->m_buffer, name.m_id->m_len);
}

Reference::Reference() :
        m_numRefs(0), m_referencedName(nullptr) {
    // empty
}

Reference::Reference(size_t numrefs, Name **names) :
        m_numRefs(numrefs), m_referencedName(nullptr) {
    if (numrefs > 0) {
        m_referencedName = new Name *[numrefs];
        for (size_t i = 0; i < numrefs; i++) {
            m_referencedName[i] = names[i];
        }
    }
}
Reference::Reference(const Reference &ref) {
    m_numRefs = ref.m_numRefs;
    if (m_numRefs != 0) {
        m_referencedName = new Name *[m_numRefs];
        for (size_t i = 0; i < m_numRefs; i++) {
            m_referencedName[i] = new Name(*ref.m_referencedName[i]);
        }
    }
}

Reference::~Reference() {
    for (size_t i = 0; i < m_numRefs; i++) {
        delete m_referencedName[i];
    }
    m_numRefs = 0;
    delete[] m_referencedName;
    m_referencedName = nullptr;
}

size_t Reference::sizeInBytes() {
    if (0 == m_numRefs) {
        return 0;
    }

    size_t size(0);
    for (size_t i = 0; i < m_numRefs; i++) {
        Name *name(m_referencedName[i]);
        if (nullptr != name) {
            size += name->m_id->m_len;
        }
    }

    return size;
}

Property::Property(Text *id) :
        m_key(id), m_value(nullptr), m_ref(nullptr), m_next(nullptr) {
    // empty
}

Property::~Property() {
    delete m_key;
    if (m_value != nullptr)
        delete m_value;
    if (m_ref != nullptr)
        delete (m_ref);
    if (m_next != nullptr)
        delete m_next;
}

DataArrayList::DataArrayList() :
        m_numItems(0), m_dataList(nullptr), m_next(nullptr), m_refs(nullptr), m_numRefs(0) {
    // empty
}

DataArrayList::~DataArrayList() {
    delete m_dataList;
    if (m_next != nullptr)
        delete m_next;
    if (m_refs != nullptr)
        delete m_refs;
}

size_t DataArrayList::size() {
    size_t result(0);
    if (nullptr == m_next) {
        if (m_dataList != nullptr) {
            result = 1;
        }
        return result;
    }

    DataArrayList *n(m_next);
    while (nullptr != n) {
        result++;
        n = n->m_next;
    }
    return result;
}

Context::Context() :
        m_root(nullptr) {
    // empty
}

Context::~Context() {
    clear();
}

void Context::clear() {
    delete m_root;
    m_root = nullptr;
}

END_ODDLPARSER_NS
