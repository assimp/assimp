
#ifndef AI_OGREPARSINGUTILS_H_INC
#define AI_OGREPARSINGUTILS_H_INC

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "ParsingUtils.h"
#include "irrXMLWrapper.h"
#include "fast_atof.h"
#include <functional>
namespace Assimp
{
namespace Ogre
{

typedef irr::io::IrrXMLReader XmlReader;

static void ThrowAttibuteError(const XmlReader* reader, const std::string &name, const std::string &error = "")
{
	if (!error.empty())
	{
		throw DeadlyImportError(error + " in node '" + std::string(reader->getNodeName()) + "' and attribute '" + name + "'");
	}
	else
	{
		throw DeadlyImportError("Attribute '" + name + "' does not exist in node '" + std::string(reader->getNodeName()) + "'");
	}
}

template<typename T> 
inline T GetAttribute(const XmlReader* reader, const std::string &name);

template<> 
inline int GetAttribute<int>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
	{
		return atoi(value);
	}
	else
	{
		ThrowAttibuteError(reader, name);
		return 0;
	}
}

template<> 
inline unsigned int GetAttribute<unsigned int>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
	{
		return static_cast<unsigned int>(atoi(value)); ///< @todo Find a better way...
	}
	else
	{
		ThrowAttibuteError(reader, name);
		return 0;
	}
}

template<> 
inline float GetAttribute<float>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
	{
		return fast_atof(value);
	}
	else
	{
		ThrowAttibuteError(reader, name);
		return 0.f;
	}
}

template<> 
inline std::string GetAttribute<std::string>(const XmlReader* reader, const std::string &name)
{
	const char* value = reader->getAttributeValue(name.c_str());
	if (value)
	{
		return std::string(value);
	}
	else
	{
		ThrowAttibuteError(reader, name);
		return "";
	}
}

template<> 
inline bool GetAttribute<bool>(const XmlReader* reader, const std::string &name)
{
	std::string value = GetAttribute<std::string>(reader, name);
	if (ASSIMP_stricmp(value, "true") == 0)
	{
		return true;
	}
	else if (ASSIMP_stricmp(value, "false") == 0)
	{
		return false;
	}
	else
	{
		ThrowAttibuteError(reader, name, "Boolean value is expected to be 'true' or 'false', encountered '" + value + "'");
		return false;
	}
}

inline bool NextNode(XmlReader* reader)
{
	do
	{
		if (!reader->read()) {
			return false;
		}
	}
	while(reader->getNodeType() != irr::io::EXN_ELEMENT);
	return true;
}

inline bool CurrentNodeNameEquals(const XmlReader* reader, const std::string &name)
{
	return (ASSIMP_stricmp(std::string(reader->getNodeName()), name) == 0);
}

/// Skips a line from current @ss position until a newline. Returns the skipped part.
static inline std::string SkipLine(std::stringstream &ss)
{
	std::string skipped;
	getline(ss, skipped);
	return skipped;
}

/// Skips a line and reads next element from @c ss to @c nextElement.
/** @return Skipped line content until newline. */
static inline std::string NextAfterNewLine(std::stringstream &ss, std::string &nextElement)
{
	std::string skipped = SkipLine(ss);
	ss >> nextElement;
	return skipped;
}

/// Returns a lower cased copy of @s.
static inline std::string ToLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

/// Returns if @c s ends with @c suffix. If @c caseSensitive is false, both strings will be lower cased before matching.
static inline bool EndsWith(const std::string &s, const std::string &suffix, bool caseSensitive = true)
{
	if (s.empty() || suffix.empty())
	{
		return false;
	}
	else if (s.length() < suffix.length())
	{
		return false;
	}

	if (!caseSensitive) {
		return EndsWith(ToLower(s), ToLower(suffix), true);
	}

	size_t len = suffix.length();
	std::string sSuffix = s.substr(s.length()-len, len);
	return (ASSIMP_stricmp(sSuffix, suffix) == 0);
}

// Below trim functions adapted from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

/// Trim from start
static inline std::string &TrimLeft(std::string &s, bool newlines = true)
{
	if (!newlines)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(Assimp::IsSpace<char>))));
	}
	else
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(Assimp::IsSpaceOrNewLine<char>))));
	}
	return s;
}

/// Trim from end
static inline std::string &TrimRight(std::string &s, bool newlines = true)
{
	if (!newlines)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun(Assimp::IsSpace<char>))).base(),s.end());
	}
	else
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(Assimp::IsSpaceOrNewLine<char>))));
	}
	return s;
}

/// Trim from both ends
static inline std::string &Trim(std::string &s, bool newlines = true)
{
	return TrimLeft(TrimRight(s, newlines), newlines);
}

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
#endif // AI_OGREPARSINGUTILS_H_INC
