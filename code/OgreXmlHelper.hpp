
#include "ParsingUtils.h"
#include "irrXMLWrapper.h"
#include "fast_atof.h"

namespace Assimp
{
namespace Ogre
{

typedef irr::io::IrrXMLReader XmlReader;

static void ThrowAttibuteError(const XmlReader* reader, const std::string &name, const std::string &error = "")
{
	if (!error.empty())
		throw DeadlyImportError(error + " in node '" + std::string(reader->getNodeName()) + "' and attribute '" + name + "'");
	else
		throw DeadlyImportError("Attribute '" + name + "' does not exist in node '" + std::string(reader->getNodeName()) + "'");
}

template<typename T> 
inline T GetAttribute(const XmlReader* reader, const std::string &name);

template<> 
inline int GetAttribute<int>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
		return atoi(value);
	else
		ThrowAttibuteError(reader, name);
}

template<> 
inline unsigned int GetAttribute<unsigned int>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
		return static_cast<unsigned int>(atoi(value)); ///< @todo Find a better way...
	else
		ThrowAttibuteError(reader, name);
}

template<> 
inline float GetAttribute<float>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
		return fast_atof(value);
	else
		ThrowAttibuteError(reader, name);
}

template<> 
inline std::string GetAttribute<std::string>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
		return std::string(value);
	else
		ThrowAttibuteError(reader, name);
}

template<> 
inline bool GetAttribute<bool>(const XmlReader* reader, const std::string &name)
{
	std::string value = GetAttribute<std::string>(reader, name);
	if (ASSIMP_stricmp(value, "true") == 0)
		return true;
	else if (ASSIMP_stricmp(value, "false") == 0)
		return false;
	else
		ThrowAttibuteError(reader, name, "Boolean value is expected to be 'true' or 'false', encountered '" + value + "'");
}

inline bool NextNode(XmlReader* reader)
{
	do
	{
		if (!reader->read())
			return false;
	}
	while(reader->getNodeType() != irr::io::EXN_ELEMENT);
	return true;
}

inline bool CurrentNodeNameEquals(const XmlReader* reader, const std::string &name)
{
	return (ASSIMP_stricmp(std::string(reader->getNodeName()), name) == 0);
}

/// Returns a lower cased copy of @s.
static inline std::string ToLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

// ------------------------------------------------------------------------------------------------
// From http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start
static inline std::string &ltrim(std::string &s, bool newlines = true)
{
	if (!newlines)
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(Assimp::IsSpace<char>))));
	else
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(Assimp::IsSpaceOrNewLine<char>))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s, bool newlines = true)
{
	if (!newlines)
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun(Assimp::IsSpace<char>))).base(),s.end());
	else
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(Assimp::IsSpaceOrNewLine<char>))));
	return s;
}
// trim from both ends
static inline std::string &trim(std::string &s, bool newlines = true)
{
	return ltrim(rtrim(s, newlines), newlines);
}

} // Ogre
} // Assimp
