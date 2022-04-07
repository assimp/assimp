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
#include <openddlparser/OpenDDLStream.h>
#include <openddlparser/Value.h>

#include <cassert>

BEGIN_ODDLPARSER_NS

static Value::Iterator end(nullptr);

Value::Iterator::Iterator() :
        m_start(nullptr),
        m_current(nullptr) {
    // empty
}

Value::Iterator::Iterator(Value *start) :
        m_start(start),
        m_current(start) {
    // empty
}

Value::Iterator::Iterator(const Iterator &rhs) :
        m_start(rhs.m_start),
        m_current(rhs.m_current) {
    // empty
}

Value::Iterator::~Iterator() {
    // empty
}

bool Value::Iterator::hasNext() const {
    if (nullptr == m_current) {
        return false;
    }
    return (nullptr != m_current->getNext());
}

Value *Value::Iterator::getNext() {
    if (!hasNext()) {
        return nullptr;
    }

    Value *v(m_current->getNext());
    m_current = v;

    return v;
}

const Value::Iterator Value::Iterator::operator++(int) {
    if (nullptr == m_current) {
        return end;
    }

    m_current = m_current->getNext();
    Iterator inst(m_current);

    return inst;
}

Value::Iterator &Value::Iterator::operator++() {
    if (nullptr == m_current) {
        return end;
    }

    m_current = m_current->getNext();

    return *this;
}

bool Value::Iterator::operator==(const Iterator &rhs) const {
    return (m_current == rhs.m_current);
}

Value *Value::Iterator::operator->() const {
    if (nullptr == m_current) {
        return nullptr;
    }
    return m_current;
}

Value::Value(ValueType type) :
        m_type(type),
        m_size(0),
        m_data(nullptr),
        m_next(nullptr) {
    // empty
}

Value::~Value() {
    if (m_data != nullptr) {
        if (m_type == ValueType::ddl_ref) {
            Reference *tmp = (Reference *)m_data;
            if (tmp != nullptr) {
                delete tmp;
            }
        } else {
            delete[] m_data;
        }
    }
    delete m_next;
}

void Value::setBool(bool value) {
    assert(ValueType::ddl_bool == m_type);
    ::memcpy(m_data, &value, m_size);
}

bool Value::getBool() {
    assert(ValueType::ddl_bool == m_type);
    return (*m_data == 1);
}

void Value::setInt8(int8 value) {
    assert(ValueType::ddl_int8 == m_type);
    ::memcpy(m_data, &value, m_size);
}

int8 Value::getInt8() {
    assert(ValueType::ddl_int8 == m_type);
    return (int8)(*m_data);
}

void Value::setInt16(int16 value) {
    assert(ValueType::ddl_int16 == m_type);
    ::memcpy(m_data, &value, m_size);
}

int16 Value::getInt16() {
    assert(ValueType::ddl_int16 == m_type);
    int16 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setInt32(int32 value) {
    assert(ValueType::ddl_int32 == m_type);
    ::memcpy(m_data, &value, m_size);
}

int32 Value::getInt32() {
    assert(ValueType::ddl_int32 == m_type);
    int32 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setInt64(int64 value) {
    assert(ValueType::ddl_int64 == m_type);
    ::memcpy(m_data, &value, m_size);
}

int64 Value::getInt64() {
    assert(ValueType::ddl_int64 == m_type);
    int64 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setUnsignedInt8(uint8 value) {
    assert(ValueType::ddl_unsigned_int8 == m_type);
    ::memcpy(m_data, &value, m_size);
}

uint8 Value::getUnsignedInt8() const {
    assert(ValueType::ddl_unsigned_int8 == m_type);
    uint8 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setUnsignedInt16(uint16 value) {
    assert(ValueType::ddl_unsigned_int16 == m_type);
    ::memcpy(m_data, &value, m_size);
}

uint16 Value::getUnsignedInt16() const {
    assert(ValueType::ddl_unsigned_int16 == m_type);
    uint16 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setUnsignedInt32(uint32 value) {
    assert(ValueType::ddl_unsigned_int32 == m_type);
    ::memcpy(m_data, &value, m_size);
}

uint32 Value::getUnsignedInt32() const {
    assert(ValueType::ddl_unsigned_int32 == m_type);
    uint32 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setUnsignedInt64(uint64 value) {
    assert(ValueType::ddl_unsigned_int64 == m_type);
    ::memcpy(m_data, &value, m_size);
}

uint64 Value::getUnsignedInt64() const {
    assert(ValueType::ddl_unsigned_int64 == m_type);
    uint64 i;
    ::memcpy(&i, m_data, m_size);
    return i;
}

void Value::setFloat(float value) {
    assert(ValueType::ddl_float == m_type);
    ::memcpy(m_data, &value, m_size);
}

float Value::getFloat() const {
    if (m_type == ValueType::ddl_float) {
        float v;
        ::memcpy(&v, m_data, m_size);
        return (float)v;
    } else {
        float tmp;
        ::memcpy(&tmp, m_data, 4);
        return (float)tmp;
    }
}

void Value::setDouble(double value) {
    assert(ValueType::ddl_double == m_type);
    ::memcpy(m_data, &value, m_size);
}

double Value::getDouble() const {
    if (m_type == ValueType::ddl_double) {
        double v;
        ::memcpy(&v, m_data, m_size);
        return (float)v;
    } else {
        double tmp;
        ::memcpy(&tmp, m_data, 4);
        return (double)tmp;
    }
}

void Value::setString(const std::string &str) {
    assert(ValueType::ddl_string == m_type);
    ::memcpy(m_data, str.c_str(), str.size());
    m_data[str.size()] = '\0';
}

const char *Value::getString() const {
    assert(ValueType::ddl_string == m_type);
    return (const char *)m_data;
}

void Value::setRef(Reference *ref) {
    assert(ValueType::ddl_ref == m_type);

    if (nullptr != ref) {
        const size_t sizeInBytes(ref->sizeInBytes());
        if (sizeInBytes > 0) {
            if (nullptr != m_data) {
                delete[] m_data;
            }

            m_data = (unsigned char *)new Reference(*ref);
        }
    }
}

Reference *Value::getRef() const {
    assert(ValueType::ddl_ref == m_type);

    return (Reference *)m_data;
}

void Value::dump(IOStreamBase &stream) {
    switch (m_type) {
        case ValueType::ddl_none:
            stream.write("None\n");
            break;
        case ValueType::ddl_bool:
            stream.write(std::to_string(getBool()) + "\n");
            break;
        case ValueType::ddl_int8:
            stream.write(std::to_string(getInt8()) + "\n");
            break;
        case ValueType::ddl_int16:
            stream.write(std::to_string(getInt16()) + "\n");
            break;
        case ValueType::ddl_int32:
            stream.write(std::to_string(getInt32()) + "\n");
            break;
        case ValueType::ddl_int64:
            stream.write(std::to_string(getInt64()) + "\n");
            break;
        case ValueType::ddl_unsigned_int8:
            stream.write("Not supported\n");
            break;
        case ValueType::ddl_unsigned_int16:
            stream.write("Not supported\n");
            break;
        case ValueType::ddl_unsigned_int32:
            stream.write("Not supported\n");
            break;
        case ValueType::ddl_unsigned_int64:
            stream.write("Not supported\n");
            break;
        case ValueType::ddl_half:
            stream.write("Not supported\n");
            break;
        case ValueType::ddl_float:
            stream.write(std::to_string(getFloat()) + "\n");
            break;
        case ValueType::ddl_double:
            stream.write(std::to_string(getDouble()) + "\n");
            break;
        case ValueType::ddl_string:
            stream.write(std::string(getString()) + "\n");
            break;
        case ValueType::ddl_ref:
            stream.write("Not supported\n");
            break;
        default:
            break;
    }
}

void Value::setNext(Value *next) {
    m_next = next;
}

Value *Value::getNext() const {
    return m_next;
}

size_t Value::size() const {
    size_t result = 1;
    Value *n = m_next;
    while (n != nullptr) {
        result++;
        n = n->m_next;
    }
    return result;
}

Value *ValueAllocator::allocPrimData(Value::ValueType type, size_t len) {
    if (type == Value::ValueType::ddl_none || Value::ValueType::ddl_types_max == type) {
        return nullptr;
    }

    Value *data = new Value(type);
    switch (type) {
        case Value::ValueType::ddl_bool:
            data->m_size = sizeof(bool);
            break;
        case Value::ValueType::ddl_int8:
            data->m_size = sizeof(int8);
            break;
        case Value::ValueType::ddl_int16:
            data->m_size = sizeof(int16);
            break;
        case Value::ValueType::ddl_int32:
            data->m_size = sizeof(int32);
            break;
        case Value::ValueType::ddl_int64:
            data->m_size = sizeof(int64);
            break;
        case Value::ValueType::ddl_unsigned_int8:
            data->m_size = sizeof(uint8);
            break;
        case Value::ValueType::ddl_unsigned_int16:
            data->m_size = sizeof(uint16);
            break;
        case Value::ValueType::ddl_unsigned_int32:
            data->m_size = sizeof(uint32);
            break;
        case Value::ValueType::ddl_unsigned_int64:
            data->m_size = sizeof(uint64);
            break;
        case Value::ValueType::ddl_half:
            data->m_size = sizeof(short);
            break;
        case Value::ValueType::ddl_float:
            data->m_size = sizeof(float);
            break;
        case Value::ValueType::ddl_double:
            data->m_size = sizeof(double);
            break;
        case Value::ValueType::ddl_string:
            data->m_size = sizeof(char) * (len + 1);
            break;
        case Value::ValueType::ddl_ref:
            data->m_size = 0;
            break;
        case Value::ValueType::ddl_none:
        case Value::ValueType::ddl_types_max:
        default:
            break;
    }

    if (data->m_size) {
        data->m_data = new unsigned char[data->m_size];
        ::memset(data->m_data, 0, data->m_size);
    }

    return data;
}

void ValueAllocator::releasePrimData(Value **data) {
    if (!data) {
        return;
    }

    delete *data;
    *data = nullptr;
}

END_ODDLPARSER_NS
