#pragma once

#include <assimp/XmlParser.h>
#include <assimp/types.h>
#include <list>

namespace Assimp {

class X3DXmlHelper {
public:
    static bool getColor3DAttribute(XmlNode &node, const char *attributeName, aiColor3D &color);
    static bool getVector2DAttribute(XmlNode &node, const char *attributeName, aiVector2D &vector);
    static bool getVector3DAttribute(XmlNode &node, const char *attributeName, aiVector3D &vector);

    static bool getBooleanArrayAttribute(XmlNode &node, const char *attributeName, std::vector<bool> &boolArray);
    static bool getDoubleArrayAttribute(XmlNode &node, const char *attributeName, std::vector<double> &doubleArray);
    static bool getFloatArrayAttribute(XmlNode &node, const char *attributeName, std::vector<float> &floatArray);
    static bool getInt32ArrayAttribute(XmlNode &node, const char *attributeName, std::vector<int32_t> &intArray);
    static bool getStringListAttribute(XmlNode &node, const char *attributeName, std::list<std::string> &stringArray);
    static bool getStringArrayAttribute(XmlNode &node, const char *attributeName, std::vector<std::string> &stringArray);

    static bool getVector2DListAttribute(XmlNode &node, const char *attributeName, std::list<aiVector2D> &vectorList);
    static bool getVector2DArrayAttribute(XmlNode &node, const char *attributeName, std::vector<aiVector2D> &vectorArray);
    static bool getVector3DListAttribute(XmlNode &node, const char *attributeName, std::list<aiVector3D> &vectorList);
    static bool getVector3DArrayAttribute(XmlNode &node, const char *attributeName, std::vector<aiVector3D> &vectorArray);
    static bool getColor3DListAttribute(XmlNode &node, const char *attributeName, std::list<aiColor3D> &colorList);
    static bool getColor4DListAttribute(XmlNode &node, const char *attributeName, std::list<aiColor4D> &colorList);
};

} // namespace Assimp
