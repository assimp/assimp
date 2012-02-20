
#include "irrXMLWrapper.h"
#include "fast_atof.h"

namespace Assimp
{
namespace Ogre
{
	
typedef irr::io::IrrXMLReader XmlReader;


//------------Helper Funktion to Get a Attribute Save---------------
template<typename t> inline t GetAttribute(XmlReader* Reader, std::string Name);

/*
{
	BOOST_STATIC_ASSERT(false);
	return t();
}
*/

template<> inline int GetAttribute<int>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return atoi(Value);
	else
		throw DeadlyImportError(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline float GetAttribute<float>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return fast_atof(Value);
	else
		throw DeadlyImportError(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline std::string GetAttribute<std::string>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return std::string(Value);
	else
		throw DeadlyImportError(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline bool GetAttribute<bool>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
	{
		if(Value==std::string("true"))
			return true;
		else if(Value==std::string("false"))
			return false;
		else
			throw DeadlyImportError(std::string("Bool value has invalid value: "+Name+" / "+Value+" / "+Reader->getNodeName()));
	}
	else
		throw DeadlyImportError(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}
//__________________________________________________________________

inline bool XmlRead(XmlReader* Reader)
{
	do
	{
		if(!Reader->read())
			return false;
	}
	while(Reader->getNodeType()!=irr::io::EXN_ELEMENT);
	return true;
}

}//namespace Ogre
}//namespace Assimp
