/// \file   X3DImporter_Networking.cpp
/// \brief  Parsing data from nodes of "Networking" set of X3D.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"

// Header files, Assimp.
#include "DefaultIOSystem.h"

namespace Assimp
{

// <Inline
// DEF=""              ID
// USE=""              IDREF
// bboxCenter="0 0 0"  SFVec3f  [initializeOnly]
// bboxSize="-1 -1 -1" SFVec3f  [initializeOnly]
// load="true"         SFBool   [inputOutput]
// url=""              MFString [inputOutput]
// />
void X3DImporter::ParseNode_Networking_Inline()
{
std::string def, use;
bool load = true;
std::list<std::string> url;

	MACRO_ATTRREAD_LOOPBEG;
		MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
		MACRO_ATTRREAD_CHECK_RET("load", load, XML_ReadNode_GetAttrVal_AsBool);
		MACRO_ATTRREAD_CHECK_REF("url", url, XML_ReadNode_GetAttrVal_AsListS);
	MACRO_ATTRREAD_LOOPEND;

	// if "USE" defined then find already defined element.
	if(!use.empty())
	{
		CX3DImporter_NodeElement* ne;

		MACRO_USE_CHECKANDAPPLY(def, use, ENET_Group, ne);
	}
	else
	{
		ParseHelper_Group_Begin(true);// create new grouping element and go deeper if node has children.
		// at this place new group mode created and made current, so we can name it.
		if(!def.empty()) NodeElement_Cur->ID = def;

		if(load && (url.size() > 0))
		{
			DefaultIOSystem io_handler;
			std::string full_path;

			full_path = mFileDir + "/" + url.front();
			// Attribute "url" can contain list of strings. But we need only one - first.
			ParseFile(full_path, &io_handler);
		}

		// check for X3DMetadataObject childs.
		if(!mReader->isEmptyElement()) ParseNode_Metadata(NodeElement_Cur, "Inline");

		// exit from node in that place
		ParseHelper_Node_Exit();
	}// if(!use.empty()) else
}

}// namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
