#include "X3DXmlHelper.h"
#include "X3DImporter.hpp"

#include <assimp/ParsingUtils.h>

namespace Assimp {

bool X3DXmlHelper::getColor3DAttribute(XmlNode &node, const char *attributeName, aiColor3D &color) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() != 3) {
            Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        }
        auto it = values.begin();
        color.r = stof(*it++);
        color.g = stof(*it++);
        color.b = stof(*it);
        return true;
    }
    return false;
}

bool X3DXmlHelper::getVector2DAttribute(XmlNode &node, const char *attributeName, aiVector2D &color) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() != 2) {
            Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        }
        auto it = values.begin();
        color.x = stof(*it++);
        color.y = stof(*it);
        return true;
    }
    return false;
}

bool X3DXmlHelper::getVector3DAttribute(XmlNode &node, const char *attributeName, aiVector3D &color) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() != 3) {
            Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        }
        auto it = values.begin();
        color.x = stof(*it++);
        color.y = stof(*it++);
        color.z = stof(*it);
        return true;
    }
    return false;
}

bool X3DXmlHelper::getBooleanArrayAttribute(XmlNode &node, const char *attributeName, std::vector<bool> &boolArray) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        auto it = values.begin();
        while (it != values.end()) {
            auto s = *it++;
            if (!s.empty())
                boolArray.push_back(s[0] == 't' || s[0] == '1');
            else
                Throw_ConvertFail_Str2ArrB(node.name(), attributeName);
        }
        return true;
    }
    return false;
}

bool X3DXmlHelper::getDoubleArrayAttribute(XmlNode &node, const char *attributeName, std::vector<double> &doubleArray) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        auto it = values.begin();
        while (it != values.end()) {
            auto s = *it++;
            if (!s.empty())
                doubleArray.push_back(atof(s.c_str()));
            else
                Throw_ConvertFail_Str2ArrD(node.name(), attributeName);
        }
        return true;
    }
    return false;
}

bool X3DXmlHelper::getFloatArrayAttribute(XmlNode &node, const char *attributeName, std::vector<float> &floatArray) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        auto it = values.begin();
        while (it != values.end()) {
            auto s = *it++;
            if (!s.empty())
                floatArray.push_back((float)atof(s.c_str()));
            else
                Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        }
        return true;
    }
    return false;
}

bool X3DXmlHelper::getInt32ArrayAttribute(XmlNode &node, const char *attributeName, std::vector<int32_t> &intArray) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        auto it = values.begin();
        while (it != values.end()) {
            auto s = *it++;
            if (!s.empty())
                intArray.push_back((int32_t)atof(s.c_str()));
            else
                Throw_ConvertFail_Str2ArrI(node.name(), attributeName);
        }
        return true;
    }
    return false;
}

// An MFString in the XML encoding (ISO/IEC 19776-1, 5.15) is zero or more strings, each in double
// quotes, separated by whitespace (commas count as whitespace between values): url='"a.png"' and
// url='"a.png" "https://example.com/a.png"'. Inside a string, \" is a double quote and \\ a backslash;
// any other character, whitespace included, is taken as it is. A word outside quotes is read as a
// string of its own, as before, since files written that way (url="a.png") are common.
bool X3DXmlHelper::getStringListAttribute(XmlNode &node, const char *attributeName, std::list<std::string> &stringList) {
    std::string val;
    if (!XmlParser::getStdStrAttribute(node, attributeName, val)) {
        return false;
    }
    const auto isWhitespace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    size_t pos = 0;
    while (pos < val.size()) {
        if (isWhitespace(val[pos]) || val[pos] == ',') {
            ++pos;
        } else if (val[pos] == '"') {
            std::string str;
            bool closed = false;
            for (++pos; pos < val.size(); ++pos) {
                if (val[pos] == '\\' && pos + 1 < val.size() && (val[pos + 1] == '"' || val[pos + 1] == '\\')) {
                    str += val[++pos];
                } else if (val[pos] == '"') {
                    closed = true;
                    ++pos;
                    break;
                } else {
                    str += val[pos];
                }
            }
            if (!closed) Throw_ConvertFail_Str2ArrS(node.name(), attributeName);
            stringList.push_back(str);
        } else {
            const size_t start = pos;
            while (pos < val.size() && !isWhitespace(val[pos])) {
                ++pos;
            }
            stringList.push_back(val.substr(start, pos - start));
        }
    }
    return true;
}

bool X3DXmlHelper::getStringArrayAttribute(XmlNode &node, const char *attributeName, std::vector<std::string> &stringArray) {
    std::list<std::string> tlist;

    if (getStringListAttribute(node, attributeName, tlist)) {
        if (!tlist.empty()) {
            stringArray.reserve(tlist.size());
            for (std::list<std::string>::iterator it = tlist.begin(); it != tlist.end(); ++it) {
                stringArray.push_back(*it);
            }
            return true;
        }
    }
    return false;
}

bool X3DXmlHelper::getVector2DListAttribute(XmlNode &node, const char *attributeName, std::list<aiVector2D> &vectorList) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() % 2) Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        auto it = values.begin();
        while (it != values.end()) {
            aiVector2D tvec;

            tvec.x = (float)atof((*it++).c_str());
            tvec.y = (float)atof((*it++).c_str());
            vectorList.push_back(tvec);
        }
        return true;
    }
    return false;
}

bool X3DXmlHelper::getVector2DArrayAttribute(XmlNode &node, const char *attributeName, std::vector<aiVector2D> &vectorArray) {
    std::list<aiVector2D> tlist;

    if (getVector2DListAttribute(node, attributeName, tlist)) {
        if (!tlist.empty()) {
            vectorArray.reserve(tlist.size());
            for (std::list<aiVector2D>::iterator it = tlist.begin(); it != tlist.end(); ++it) {
                vectorArray.push_back(*it);
            }
            return true;
        }
    }
    return false;
}

bool X3DXmlHelper::getVector3DListAttribute(XmlNode &node, const char *attributeName, std::list<aiVector3D> &vectorList) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() % 3 != 0) Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        auto it = values.begin();
        while (it != values.end()) {
            aiVector3D tvec;

            tvec.x = (float)atof((*it++).c_str());
            tvec.y = (float)atof((*it++).c_str());
            tvec.z = (float)atof((*it++).c_str());
            vectorList.push_back(tvec);
        }
        return true;
    }
    return false;
}

bool X3DXmlHelper::getVector3DArrayAttribute(XmlNode &node, const char *attributeName, std::vector<aiVector3D> &vectorArray) {
    std::list<aiVector3D> tlist;

    if (getVector3DListAttribute(node, attributeName, tlist)) {
        if (!tlist.empty()) {
            vectorArray.reserve(tlist.size());
            for (std::list<aiVector3D>::iterator it = tlist.begin(); it != tlist.end(); ++it) {
                vectorArray.push_back(*it);
            }
            return true;
        }
    }
    return false;
}

bool X3DXmlHelper::getColor3DListAttribute(XmlNode &node, const char *attributeName, std::list<aiColor3D> &colorList) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() % 3 != 0) Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        auto it = values.begin();
        while (it != values.end()) {
            aiColor3D tvec;

            tvec.r = (float)atof((*it++).c_str());
            tvec.g = (float)atof((*it++).c_str());
            tvec.b = (float)atof((*it++).c_str());
            colorList.push_back(tvec);
        }
        return true;
    }
    return false;
}

bool X3DXmlHelper::getColor4DListAttribute(XmlNode &node, const char *attributeName, std::list<aiColor4D> &colorList) {
    std::string val;
    if (XmlParser::getStdStrAttribute(node, attributeName, val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        if (values.size() % 4 != 0) Throw_ConvertFail_Str2ArrF(node.name(), attributeName);
        auto it = values.begin();
        while (it != values.end()) {
            aiColor4D tvec;

            tvec.r = (float)atof((*it++).c_str());
            tvec.g = (float)atof((*it++).c_str());
            tvec.b = (float)atof((*it++).c_str());
            tvec.a = (float)atof((*it++).c_str());
            colorList.push_back(tvec);
        }
        return true;
    }
    return false;
}

} // namespace Assimp
