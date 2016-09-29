/// \file X3DImporter_Metadata.cpp
/// \brief Parsing data from nodes of "Metadata" set of X3D.
/// \date 2015-2016
/// \author nevorek@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"

namespace Assimp
{

/// \def MACRO_METADATA_FINDCREATE(pDEF_Var, pUSE_Var, pReference, pValue, pNE, pMetaName)
/// Find element by "USE" or create new one.
/// \param [in] pDEF_Var - variable name with "DEF" value.
/// \param [in] pUSE_Var - variable name with "USE" value.
/// \param [in] pReference - variable name with "reference" value.
/// \param [in] pValue - variable name with "value" value.
/// \param [in, out] pNE - pointer to node element.
/// \param [in] pMetaClass - Class of node.
/// \param [in] pMetaName - Name of node.
/// \param [in] pType - type of element to find.
#define MACRO_METADATA_FINDCREATE(pDEF_Var, pUSE_Var, pReference, pValue, pNE, pMetaClass, pMetaName, pType) \
	/* if "USE" defined then find already defined element. */ \
	if(!pUSE_Var.empty()) \
	{ \
		MACRO_USE_CHECKANDAPPLY(pDEF_Var, pUSE_Var, pType, pNE); \
	} \
	else \
	{ \
		pNE = new pMetaClass(NodeElement_Cur); \
		if(!pDEF_Var.empty()) pNE->ID = pDEF_Var; \
	 \
		((pMetaClass*)pNE)->Reference = pReference; \
		((pMetaClass*)pNE)->Value = pValue; \
		/* also metadata node can contain childs */ \
		if(!mReader->isEmptyElement()) \
			ParseNode_Metadata(pNE, pMetaName);/* in that case node element will be added to child elements list of current node. */ \
		else \
			NodeElement_Cur->Child.push_back(pNE);/* else - add element to child list manualy */ \
	 \
		NodeElement_List.push_back(pNE);/* add new element to elements list. */ \
	}/* if(!pUSE_Var.empty()) else */ \
	 \
	do {} while(false)

bool X3DImporter::ParseHelper_CheckRead_X3DMetadataObject()
{
	if(XML_CheckNode_NameEqual("MetadataBoolean"))
		ParseNode_MetadataBoolean();
	else if(XML_CheckNode_NameEqual("MetadataDouble"))
		ParseNode_MetadataDouble();
	else if(XML_CheckNode_NameEqual("MetadataFloat"))
		ParseNode_MetadataFloat();
	else if(XML_CheckNode_NameEqual("MetadataInteger"))
		ParseNode_MetadataInteger();
	else if(XML_CheckNode_NameEqual("MetadataSet"))
		ParseNode_MetadataSet();
	else if(XML_CheckNode_NameEqual("MetadataString"))
		ParseNode_MetadataString();
	else
		return false;

	return true;
}

void X3DImporter::ParseNode_Metadata(CX3DImporter_NodeElement* pParentElement, const std::string& pNodeName)
{
	ParseHelper_Node_Enter(pParentElement);
	MACRO_NODECHECK_METADATA(mReader->getNodeName());
	ParseHelper_Node_Exit();
}

// <MetadataBoolean
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFBool   [inputOutput]
// />
void X3DImporter::ParseNode_MetadataBoolean()
{
std::string def, use;
std::string name, reference;
std::list<bool> value;
CX3DImporter_NodeElement* ne;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("name", name, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_RET("reference", reference, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_REF("value", value, XML_ReadNode_GetAttrVal_AsListB);
	MACRO_ATTRREAD_LOOPEND;

	MACRO_METADATA_FINDCREATE(def, use, reference, value, ne, CX3DImporter_NodeElement_MetaBoolean, "MetadataBoolean", ENET_MetaBoolean);
}

// <MetadataDouble
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFDouble [inputOutput]
// />
void X3DImporter::ParseNode_MetadataDouble()
{
std::string def, use;
std::string name, reference;
std::list<double> value;
CX3DImporter_NodeElement* ne;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("name", name, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_RET("reference", reference, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_REF("value", value, XML_ReadNode_GetAttrVal_AsListD);
	MACRO_ATTRREAD_LOOPEND;

	MACRO_METADATA_FINDCREATE(def, use, reference, value, ne, CX3DImporter_NodeElement_MetaDouble, "MetadataDouble", ENET_MetaDouble);
}

// <MetadataFloat
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFFloat  [inputOutput]
// />
void X3DImporter::ParseNode_MetadataFloat()
{
std::string def, use;
std::string name, reference;
std::list<float> value;
CX3DImporter_NodeElement* ne;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("name", name, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_RET("reference", reference, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_REF("value", value, XML_ReadNode_GetAttrVal_AsListF);
	MACRO_ATTRREAD_LOOPEND;

	MACRO_METADATA_FINDCREATE(def, use, reference, value, ne, CX3DImporter_NodeElement_MetaFloat, "MetadataFloat", ENET_MetaFloat);
}

// <MetadataInteger
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString  [inputOutput]
// reference="" SFString  [inputOutput]
// value=""     MFInteger [inputOutput]
// />
void X3DImporter::ParseNode_MetadataInteger()
{
std::string def, use;
std::string name, reference;
std::list<int32_t> value;
CX3DImporter_NodeElement* ne;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("name", name, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_RET("reference", reference, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_REF("value", value, XML_ReadNode_GetAttrVal_AsListI32);
	MACRO_ATTRREAD_LOOPEND;

	MACRO_METADATA_FINDCREATE(def, use, reference, value, ne, CX3DImporter_NodeElement_MetaInteger, "MetadataInteger", ENET_MetaInteger);
}

// <MetadataSet
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// />
void X3DImporter::ParseNode_MetadataSet()
{
std::string def, use;
std::string name, reference;
CX3DImporter_NodeElement* ne;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("name", name, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_RET("reference", reference, mReader->getAttributeValue);
	MACRO_ATTRREAD_LOOPEND;

	// if "USE" defined then find already defined element.
	if(!use.empty())
	{
		MACRO_USE_CHECKANDAPPLY(def, use, ENET_MetaSet, ne);
	}
	else
	{
		ne = new CX3DImporter_NodeElement_MetaSet(NodeElement_Cur);
		if(!def.empty()) ne->ID = def;

		((CX3DImporter_NodeElement_MetaSet*)ne)->Reference = reference;
		// also metadata node can contain childs
		if(!mReader->isEmptyElement())
			ParseNode_Metadata(ne, "MetadataSet");
		else
			NodeElement_Cur->Child.push_back(ne);// made object as child to current element

		NodeElement_List.push_back(ne);// add new element to elements list.
	}// if(!use.empty()) else
}

// <MetadataString
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFString [inputOutput]
// />
void X3DImporter::ParseNode_MetadataString()
{
std::string def, use;
std::string name, reference;
std::list<std::string> value;
CX3DImporter_NodeElement* ne;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("name", name, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_RET("reference", reference, mReader->getAttributeValue);
		MACRO_ATTRREAD_CHECK_REF("value", value, XML_ReadNode_GetAttrVal_AsListS);
	MACRO_ATTRREAD_LOOPEND;

	MACRO_METADATA_FINDCREATE(def, use, reference, value, ne, CX3DImporter_NodeElement_MetaString, "MetadataString", ENET_MetaString);
}

}// namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
