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
#include <openddlparser/OpenDDLExport.h>
#include <openddlparser/OpenDDLParser.h>

#include <math.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

BEGIN_ODDLPARSER_NS

static const char *Version = "0.4.0";

namespace Grammar {

static const char *OpenBracketToken = "{";
static const char *CloseBracketToken = "}";
static const char *OpenPropertyToken = "(";
static const char *ClosePropertyToken = ")";
static const char *OpenArrayToken = "[";
static const char *CloseArrayToken = "]";
static const char *BoolTrue = "true";
static const char *BoolFalse = "false";
static const char *CommaSeparator = ",";

static const char *PrimitiveTypeToken[(size_t)Value::ValueType::ddl_types_max] = {
    "bool",
    "int8",
    "int16",
    "int32",
    "int64",
    "unsigned_int8",
    "unsigned_int16",
    "unsigned_int32",
    "unsigned_int64",
    "half",
    "float",
    "double",
    "string",
    "ref"
};
} // Namespace Grammar

const char *getTypeToken(Value::ValueType type) {
    return Grammar::PrimitiveTypeToken[(size_t)type];
}

static void logInvalidTokenError(char *in, const std::string &exp, OpenDDLParser::logCallback callback) {
    if (callback) {
        std::string full(in);
        std::string part(full.substr(0, 50));
        std::stringstream stream;
        stream << "Invalid token \"" << *in << "\" "
               << "(expected \"" << exp << "\") "
               << "in: \"" << part << "\"";
        callback(ddl_error_msg, stream.str());
    }
}

static bool isIntegerType(Value::ValueType integerType) {
    if (integerType != Value::ValueType::ddl_int8 && integerType != Value::ValueType::ddl_int16 &&
            integerType != Value::ValueType::ddl_int32 && integerType != Value::ValueType::ddl_int64) {
        return false;
    }

    return true;
}

static bool isUnsignedIntegerType(Value::ValueType integerType) {
    if (integerType != Value::ValueType::ddl_unsigned_int8 && integerType != Value::ValueType::ddl_unsigned_int16 &&
            integerType != Value::ValueType::ddl_unsigned_int32 && integerType != Value::ValueType::ddl_unsigned_int64) {
        return false;
    }

    return true;
}

static DDLNode *createDDLNode(Text *id, OpenDDLParser *parser) {
    if (nullptr == id || nullptr == parser || id->m_buffer == nullptr) {
        return nullptr;
    }

    const std::string type(id->m_buffer);
    DDLNode *parent(parser->top());
    DDLNode *node = DDLNode::create(type, "", parent);

    return node;
}

OpenDDLParser::OpenDDLParser() :
        m_logCallback(nullptr),
        m_buffer(),
        m_stack(),
        m_context(nullptr) {
    // empty
}

OpenDDLParser::OpenDDLParser(const char *buffer, size_t len) :
        m_logCallback(nullptr), m_buffer(), m_context(nullptr) {
    if (0 != len) {
        setBuffer(buffer, len);
    }
}

OpenDDLParser::~OpenDDLParser() {
    clear();
}

void OpenDDLParser::logToStream(FILE *f, LogSeverity severity, const std::string &message) {
    if (f) {
        const char *tag = "none";
        switch (severity) {
        case ddl_debug_msg: tag = "debug"; break;
        case ddl_info_msg:  tag = "info";  break;
        case ddl_warn_msg:  tag = "warn";  break;
        case ddl_error_msg: tag = "error"; break;
        }
        fprintf(f, "OpenDDLParser: (%5s) %s\n", tag, message.c_str());
    }
}

OpenDDLParser::logCallback OpenDDLParser::StdLogCallback (FILE *destination) {
    using namespace std::placeholders;
    return std::bind(logToStream, destination ? destination : stderr, _1, _2);
}

void OpenDDLParser::setLogCallback(logCallback callback) {
    // install user-specific log callback; null = no log callback
    m_logCallback = callback;
}

OpenDDLParser::logCallback OpenDDLParser::getLogCallback() const {
    return m_logCallback;
}

void OpenDDLParser::setBuffer(const char *buffer, size_t len) {
    clear();
    if (0 == len) {
        return;
    }

    m_buffer.resize(len);
    ::memcpy(&m_buffer[0], buffer, len);
}

void OpenDDLParser::setBuffer(const std::vector<char> &buffer) {
    clear();
    m_buffer.resize(buffer.size());
    std::copy(buffer.begin(), buffer.end(), m_buffer.begin());
}

const char *OpenDDLParser::getBuffer() const {
    if (m_buffer.empty()) {
        return nullptr;
    }

    return &m_buffer[0];
}

size_t OpenDDLParser::getBufferSize() const {
    return m_buffer.size();
}

void OpenDDLParser::clear() {
    m_buffer.resize(0);
    delete m_context;
    m_context = nullptr;
}

bool OpenDDLParser::validate() {
    if (m_buffer.empty()) {
        return true;
    }

    if (!isCharacter(m_buffer[0]) && !isNumeric(m_buffer[0])) {
        return false;
    }

    return true;
}

bool OpenDDLParser::parse() {
    if (m_buffer.empty()) {
        return false;
    }

    normalizeBuffer(m_buffer);
    if (!validate()) {
        return false;
    }

    m_context = new Context;
    m_context->m_root = DDLNode::create("root", "", nullptr);
    pushNode(m_context->m_root);

    // do the main parsing
    char *current(&m_buffer[0]);
    char *end(&m_buffer[m_buffer.size() - 1] + 1);
    size_t pos(current - &m_buffer[0]);
    while (pos < m_buffer.size()) {
        current = parseNextNode(current, end);
        if (current == nullptr) {
            return false;
        }
        pos = current - &m_buffer[0];
    }
    return true;
}

bool OpenDDLParser::exportContext(Context *ctx, const std::string &filename) {
    if (nullptr == ctx) {
        return false;
    }

    OpenDDLExport myExporter;
    return myExporter.exportContext(ctx, filename);
}

char *OpenDDLParser::parseNextNode(char *in, char *end) {
    in = parseHeader(in, end);
    in = parseStructure(in, end);

    return in;
}

#ifdef DEBUG_HEADER_NAME
static void dumpId(Identifier *id) {
    if (nullptr != id) {
        if (nullptr != id->m_text.m_buffer) {
            std::cout << id->m_text.m_buffer << std::endl;
        }
    }
}
#endif

char *OpenDDLParser::parseHeader(char *in, char *end) {
    if (nullptr == in || in == end) {
        return in;
    }

    Text *id(nullptr);
    in = OpenDDLParser::parseIdentifier(in, end, &id);

#ifdef DEBUG_HEADER_NAME
    dumpId(id);
#endif // DEBUG_HEADER_NAME

    in = lookForNextToken(in, end);
    if (nullptr != id) {
        // store the node
        DDLNode *node(createDDLNode(id, this));
        if (nullptr != node) {
            pushNode(node);
        } else {
            std::cerr << "nullptr returned by creating DDLNode." << std::endl;
        }
        delete id;

        Name *name(nullptr);
        in = OpenDDLParser::parseName(in, end, &name);
        if (nullptr != name && nullptr != node && nullptr != name->m_id->m_buffer) {
            const std::string nodeName(name->m_id->m_buffer);
            node->setName(nodeName);
            delete name;
        }

        Property *first(nullptr);
        in = lookForNextToken(in, end);
        if (in != end && *in == Grammar::OpenPropertyToken[0]) {
            in++;
            Property *prop(nullptr), *prev(nullptr);
            while (in != end && *in != Grammar::ClosePropertyToken[0]) {
                in = OpenDDLParser::parseProperty(in, end, &prop);
                in = lookForNextToken(in, end);
                if(in == end) {
                    break;
                }

                if (*in != Grammar::CommaSeparator[0] && *in != Grammar::ClosePropertyToken[0]) {
                    logInvalidTokenError(in, Grammar::ClosePropertyToken, m_logCallback);
                    return nullptr;
                }

                if (nullptr != prop && *in != Grammar::CommaSeparator[0]) {
                    if (nullptr == first) {
                        first = prop;
                    }
                    if (nullptr != prev) {
                        prev->m_next = prop;
                    }
                    prev = prop;
                }
            }
            if(in != end) {
                ++in;
            }
        }

        // set the properties
        if (nullptr != first && nullptr != node) {
            node->setProperties(first);
        }
    }

    return in;
}

char *OpenDDLParser::parseStructure(char *in, char *end) {
    if (nullptr == in || in == end) {
        return in;
    }

    bool error(false);
    in = lookForNextToken(in, end);
    if (*in == *Grammar::OpenBracketToken) {
        // loop over all children ( data and nodes )
        do {
            in = parseStructureBody(in, end, error);
            if (in == nullptr) {
                return nullptr;
            }
        } while (*in != *Grammar::CloseBracketToken);
        ++in;
    } else {
        ++in;
        logInvalidTokenError(in, std::string(Grammar::OpenBracketToken), m_logCallback);
        error = true;
        return nullptr;
    }
    in = lookForNextToken(in, end);

    // pop node from stack after successful parsing
    if (!error) {
        popNode();
    }

    return in;
}

static void setNodeValues(DDLNode *currentNode, Value *values) {
    if (nullptr != values) {
        if (nullptr != currentNode) {
            currentNode->setValue(values);
        }
    }
}

static void setNodeReferences(DDLNode *currentNode, Reference *refs) {
    if (nullptr != refs) {
        if (nullptr != currentNode) {
            currentNode->setReferences(refs);
        }
    }
}

static void setNodeDataArrayList(DDLNode *currentNode, DataArrayList *dtArrayList) {
    if (nullptr != dtArrayList) {
        if (nullptr != currentNode) {
            currentNode->setDataArrayList(dtArrayList);
        }
    }
}

char *OpenDDLParser::parseStructureBody(char *in, char *end, bool &error) {
    if (!isNumeric(*in) && !isCharacter(*in)) {
        ++in;
    }

    in = lookForNextToken(in, end);
    Value::ValueType type(Value::ValueType::ddl_none);
    size_t arrayLen(0);
    in = OpenDDLParser::parsePrimitiveDataType(in, end, type, arrayLen);
    if (Value::ValueType::ddl_none != type) {
        // parse a primitive data type
        in = lookForNextToken(in, end);
        if (*in == Grammar::OpenBracketToken[0]) {
            Reference *refs(nullptr);
            DataArrayList *dtArrayList(nullptr);
            Value *values(nullptr);
            if (1 == arrayLen) {
                size_t numRefs(0), numValues(0);
                in = parseDataList(in, end, type, &values, numValues, &refs, numRefs);
                setNodeValues(top(), values);
                setNodeReferences(top(), refs);
            } else if (arrayLen > 1) {
                in = parseDataArrayList(in, end, type, &dtArrayList);
                setNodeDataArrayList(top(), dtArrayList);
            } else {
                std::cerr << "0 for array is invalid." << std::endl;
                error = true;
            }
        }

        in = lookForNextToken(in, end);
        if (*in != '}') {
            logInvalidTokenError(in, std::string(Grammar::CloseBracketToken), m_logCallback);
            return nullptr;
        } else {
            //in++;
        }
    } else {
        // parse a complex data type
        in = parseNextNode(in, end);
    }

    return in;
}

void OpenDDLParser::pushNode(DDLNode *node) {
    if (nullptr == node) {
        return;
    }

    m_stack.push_back(node);
}

DDLNode *OpenDDLParser::popNode() {
    if (m_stack.empty()) {
        return nullptr;
    }

    DDLNode *topNode(top());
    m_stack.pop_back();
    return topNode;
}

DDLNode *OpenDDLParser::top() {
    if (m_stack.empty()) {
        return nullptr;
    }

    DDLNode *top(m_stack.back());
    return top;
}

DDLNode *OpenDDLParser::getRoot() const {
    if (nullptr == m_context) {
        return nullptr;
    }

    return m_context->m_root;
}

Context *OpenDDLParser::getContext() const {
    return m_context;
}

void OpenDDLParser::normalizeBuffer(std::vector<char> &buffer) {
    if (buffer.empty()) {
        return;
    }

    std::vector<char> newBuffer;
    const size_t len(buffer.size());
    char *end(&buffer[len - 1] + 1);
    for (size_t readIdx = 0; readIdx < len; ++readIdx) {
        char *c(&buffer[readIdx]);
        // check for a comment
        if (isCommentOpenTag(c, end)) {
            ++readIdx;
            while (readIdx < len && !isCommentCloseTag(&buffer[readIdx], end)) {
                ++readIdx;
            }
            ++readIdx;
        } else if (!isComment<char>(c, end) && !isNewLine(*c)) {
            newBuffer.push_back(buffer[readIdx]);
        } else {
            if (isComment<char>(c, end)) {
                ++readIdx;
                // skip the comment and the rest of the line
                while (readIdx < len && !isEndofLine(buffer[readIdx])) {
                    ++readIdx;
                }
            }
        }
    }
    buffer = newBuffer;
}

char *OpenDDLParser::parseName(char *in, char *end, Name **name) {
    *name = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    // ignore blanks
    in = lookForNextToken(in, end);
    if (*in != '$' && *in != '%') {
        return in;
    }

    NameType ntype(GlobalName);
    if (*in == '%') {
        ntype = LocalName;
    }
    in++;
    Name *currentName(nullptr);
    Text *id(nullptr);
    in = parseIdentifier(in, end, &id);
    if (id) {
        currentName = new Name(ntype, id);
        if (currentName) {
            *name = currentName;
        }
    }

    return in;
}

char *OpenDDLParser::parseIdentifier(char *in, char *end, Text **id) {
    *id = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    // ignore blanks
    in = lookForNextToken(in, end);
    if (in == end) {
        return in;
    }

    // staring with a number is forbidden
    if (isNumeric<const char>(*in)) {
        return in;
    }

    // get size of id
    size_t idLen(0);
    char *start(in);
    while ((in != end) && !isSeparator(*in) && !isNewLine(*in) &&
            *in != Grammar::OpenPropertyToken[0] &&
            *in != Grammar::ClosePropertyToken[0] &&
            *in != '$') {
        ++in;
        ++idLen;
    }

    const size_t len(idLen);
    *id = new Text(start, len);

    return in;
}

char *OpenDDLParser::parsePrimitiveDataType(char *in, char *end, Value::ValueType &type, size_t &len) {
    type = Value::ValueType::ddl_none;
    len = 0;
    if (nullptr == in || in == end) {
        return in;
    }

    size_t prim_len(0);
    for (size_t i = 0; i < (size_t) Value::ValueType::ddl_types_max; i++) {
        prim_len = strlen(Grammar::PrimitiveTypeToken[i]);
        if (0 == strncmp(in, Grammar::PrimitiveTypeToken[i], prim_len)) {
            type = static_cast<Value::ValueType>(i);
            break;
        }
    }

    if (Value::ValueType::ddl_none == type) {
        in = lookForNextToken(in, end);
        return in;
    } else {
        in += prim_len;
    }

    bool ok(true);
    if (*in == Grammar::OpenArrayToken[0]) {
        ok = false;
        ++in;
        char *start(in);
        while (in != end) {
            ++in;
            if (*in == Grammar::CloseArrayToken[0]) {
                len = ::atoi(start);
                ok = true;
                ++in;
                break;
            }
        }
    } else {
        len = 1;
    }
    if (!ok) {
        type = Value::ValueType::ddl_none;
    }

    return in;
}

char *OpenDDLParser::parseReference(char *in, char *end, std::vector<Name *> &names) {
    if (nullptr == in || in == end) {
        return in;
    }

    Name *nextName(nullptr);
    in = parseName(in, end, &nextName);
    if (nextName) {
        names.push_back(nextName);
    }
    while (Grammar::CommaSeparator[0] == *in) {
        in = getNextSeparator(in, end);
        if (Grammar::CommaSeparator[0] == *in) {
            in = parseName(in, end, &nextName);
            if (nextName) {
                names.push_back(nextName);
            }
        } else {
            break;
        }
    }

    return in;
}

char *OpenDDLParser::parseBooleanLiteral(char *in, char *end, Value **boolean) {
    *boolean = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    char *start(in);
    size_t len(0);
    while (!isSeparator(*in) && in != end) {
        ++in;
        ++len;
    }
    ++len;
    int res = ::strncmp(Grammar::BoolTrue, start, strlen(Grammar::BoolTrue));
    if (0 != res) {
        res = ::strncmp(Grammar::BoolFalse, start, strlen(Grammar::BoolFalse));
        if (0 != res) {
            *boolean = nullptr;
            return in;
        }
        *boolean = ValueAllocator::allocPrimData(Value::ValueType::ddl_bool);
        (*boolean)->setBool(false);
    } else {
        *boolean = ValueAllocator::allocPrimData(Value::ValueType::ddl_bool);
        (*boolean)->setBool(true);
    }

    return in;
}

char *OpenDDLParser::parseIntegerLiteral(char *in, char *end, Value **integer, Value::ValueType integerType) {
    *integer = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    if (!(isIntegerType(integerType) || isUnsignedIntegerType(integerType))) {
        return in;
    }

    in = lookForNextToken(in, end);
    char *start(in);
    while (!isSeparator(*in) && in != end) {
        ++in;
    }

    if (isNumeric(*start)) {
#ifdef OPENDDL_NO_USE_CPP11
        const int64 value(atol(start)); // maybe not really 64bit as atoll is but exists without c++11
        const uint64 uvalue(strtoul(start, nullptr, 10));
#else
        const int64 value(atoll(start));
        const uint64 uvalue(strtoull(start, nullptr, 10));
#endif
        *integer = ValueAllocator::allocPrimData(integerType);
        switch (integerType) {
            case Value::ValueType::ddl_int8:
                (*integer)->setInt8((int8)value);
                break;
            case Value::ValueType::ddl_int16:
                (*integer)->setInt16((int16)value);
                break;
            case Value::ValueType::ddl_int32:
                (*integer)->setInt32((int32)value);
                break;
            case Value::ValueType::ddl_int64:
                (*integer)->setInt64((int64)value);
                break;
            case Value::ValueType::ddl_unsigned_int8:
                (*integer)->setUnsignedInt8((uint8)uvalue);
                break;
            case Value::ValueType::ddl_unsigned_int16:
                (*integer)->setUnsignedInt16((uint16)uvalue);
                break;
            case Value::ValueType::ddl_unsigned_int32:
                (*integer)->setUnsignedInt32((uint32)uvalue);
                break;
            case Value::ValueType::ddl_unsigned_int64:
                (*integer)->setUnsignedInt64((uint64)uvalue);
                break;
            default:
                break;
        }
    }

    return in;
}

char *OpenDDLParser::parseFloatingLiteral(char *in, char *end, Value **floating, Value::ValueType floatType) {
    *floating = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    char *start(in);
    while (!isSeparator(*in) && in != end) {
        ++in;
    }

    // parse the float value
    bool ok(false);
    if (isHexLiteral(start, end)) {
        parseHexaLiteral(start, end, floating);
        return in;
    }

    if (isNumeric(*start)) {
        ok = true;
    } else {
        if (*start == '-') {
            if (isNumeric(*(start + 1))) {
                ok = true;
            }
        }
    }

    if (ok) {
        if (floatType == Value::ValueType::ddl_double) {
            const double value(atof(start));
            *floating = ValueAllocator::allocPrimData(Value::ValueType::ddl_double);
            (*floating)->setDouble(value);
        } else {
            const float value((float)atof(start));
            *floating = ValueAllocator::allocPrimData(Value::ValueType::ddl_float);
            (*floating)->setFloat(value);
        }
    }

    return in;
}

char *OpenDDLParser::parseStringLiteral(char *in, char *end, Value **stringData) {
    *stringData = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    size_t len(0);
    char *start(in);
    if (*start == '\"') {
        ++start;
        ++in;
        while (*in != '\"' && in != end) {
            ++in;
            ++len;
        }

        *stringData = ValueAllocator::allocPrimData(Value::ValueType::ddl_string, len);
        ::strncpy((char *)(*stringData)->m_data, start, len);
        (*stringData)->m_data[len] = '\0';
        ++in;
    }

    return in;
}

static void createPropertyWithData(Text *id, Value *primData, Property **prop) {
    if (nullptr != primData) {
        (*prop) = new Property(id);
        (*prop)->m_value = primData;
    }
}

char *OpenDDLParser::parseHexaLiteral(char *in, char *end, Value **data) {
    *data = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    if (*in != '0') {
        return in;
    }

    ++in;
    if (*in != 'x' && *in != 'X') {
        return in;
    }

    ++in;
    bool ok(true);
    char *start(in);
    int pos(0);
    while (!isSeparator(*in) && in != end) {
        if ((*in < '0' && *in > '9') || (*in < 'a' && *in > 'f') || (*in < 'A' && *in > 'F')) {
            ok = false;
            break;
        }
        ++pos;
        ++in;
    }

    if (!ok) {
        return in;
    }

    int value(0);
    while (pos > 0) {
        int v = hex2Decimal(*start);
        --pos;
        value = (value << 4) | v;
        ++start;
    }

    *data = ValueAllocator::allocPrimData(Value::ValueType::ddl_unsigned_int64);
    if (nullptr != *data) {
        (*data)->setUnsignedInt64(value);
    }

    return in;
}

char *OpenDDLParser::parseProperty(char *in, char *end, Property **prop) {
    *prop = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    Text *id = nullptr;
    in = parseIdentifier(in, end, &id);
    if (nullptr != id) {
        in = lookForNextToken(in, end);
        if (in != end && *in == '=') {
            ++in;
            in = getNextToken(in, end);
            Value *primData(nullptr);
            if (isInteger(in, end)) {
                in = parseIntegerLiteral(in, end, &primData);
                createPropertyWithData(id, primData, prop);
            } else if (isFloat(in, end)) {
                in = parseFloatingLiteral(in, end, &primData);
                createPropertyWithData(id, primData, prop);
            } else if (isStringLiteral(*in)) { // string data
                in = parseStringLiteral(in, end, &primData);
                createPropertyWithData(id, primData, prop);
            } else { // reference data
                std::vector<Name *> names;
                in = parseReference(in, end, names);
                if (!names.empty()) {
                    Reference *ref = new Reference(names.size(), &names[0]);
                    (*prop) = new Property(id);
                    (*prop)->m_ref = ref;
                }
            }
        } else {
            delete id;
        }
    }

    return in;
}

char *OpenDDLParser::parseDataList(char *in, char *end, Value::ValueType type, Value **data,
        size_t &numValues, Reference **refs, size_t &numRefs) {
    *data = nullptr;
    numValues = numRefs = 0;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    if (*in == '{') {
        ++in;
        Value *current(nullptr), *prev(nullptr);
        while ('}' != *in) {
            current = nullptr;
            in = lookForNextToken(in, end);
            if (Value::ValueType::ddl_ref == type) {
                std::vector<Name *> names;
                in = parseReference(in, end, names);
                if (!names.empty()) {
                    Reference *ref = new Reference(names.size(), &names[0]);
                    *refs = ref;
                    numRefs = names.size();
                }
            } else if (Value::ValueType::ddl_none == type) {
                if (isInteger(in, end)) {
                    in = parseIntegerLiteral(in, end, &current);
                } else if (isFloat(in, end)) {
                    in = parseFloatingLiteral(in, end, &current);
                } else if (isStringLiteral(*in)) {
                    in = parseStringLiteral(in, end, &current);
                } else if (isHexLiteral(in, end)) {
                    in = parseHexaLiteral(in, end, &current);
                }
            } else {
                switch (type) {
                    case Value::ValueType::ddl_int8:
                    case Value::ValueType::ddl_int16:
                    case Value::ValueType::ddl_int32:
                    case Value::ValueType::ddl_int64:
                    case Value::ValueType::ddl_unsigned_int8:
                    case Value::ValueType::ddl_unsigned_int16:
                    case Value::ValueType::ddl_unsigned_int32:
                    case Value::ValueType::ddl_unsigned_int64:
                        in = parseIntegerLiteral(in, end, &current, type);
                        break;
                    case Value::ValueType::ddl_half:
                    case Value::ValueType::ddl_float:
                    case Value::ValueType::ddl_double:
                        in = parseFloatingLiteral(in, end, &current, type);
                        break;
                    case Value::ValueType::ddl_string:
                        in = parseStringLiteral(in, end, &current);
                        break;
                    default:
                        break;
                }
            }

            if (nullptr != current) {
                if (nullptr == *data) {
                    *data = current;
                    prev = current;
                } else {
                    prev->setNext(current);
                    prev = current;
                }
                ++numValues;
            }

            in = getNextSeparator(in, end);
            if (',' != *in && Grammar::CloseBracketToken[0] != *in && !isSpace(*in)) {
                break;
            }
        }
        ++in;
    }

    return in;
}

static DataArrayList *createDataArrayList(Value *currentValue, size_t numValues,
        Reference *refs, size_t numRefs) {
    DataArrayList *dataList(new DataArrayList);
    dataList->m_dataList = currentValue;
    dataList->m_numItems = numValues;
    dataList->m_refs = refs;
    dataList->m_numRefs = numRefs;

    return dataList;
}

char *OpenDDLParser::parseDataArrayList(char *in, char *end, Value::ValueType type,
        DataArrayList **dataArrayList) {
    if (nullptr == dataArrayList) {
        return in;
    }

    *dataArrayList = nullptr;
    if (nullptr == in || in == end) {
        return in;
    }

    in = lookForNextToken(in, end);
    if (*in == Grammar::OpenBracketToken[0]) {
        ++in;
        Value *currentValue(nullptr);
        Reference *refs(nullptr);
        DataArrayList *prev(nullptr), *currentDataList(nullptr);
        do {
            size_t numRefs(0), numValues(0);
            currentValue = nullptr;

            in = parseDataList(in, end, type, &currentValue, numValues, &refs, numRefs);
            if (nullptr != currentValue || 0 != numRefs) {
                if (nullptr == prev) {
                    *dataArrayList = createDataArrayList(currentValue, numValues, refs, numRefs);
                    prev = *dataArrayList;
                } else {
                    currentDataList = createDataArrayList(currentValue, numValues, refs, numRefs);
                    if (nullptr != prev) {
                        prev->m_next = currentDataList;
                        prev = currentDataList;
                    }
                }
            }
        } while (Grammar::CommaSeparator[0] == *in && in != end);
        in = lookForNextToken(in, end);
        ++in;
    }

    return in;
}

const char *OpenDDLParser::getVersion() {
    return Version;
}

END_ODDLPARSER_NS
