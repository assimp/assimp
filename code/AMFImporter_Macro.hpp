/// \file AMFImporter_Macro.hpp
/// \brief Useful macrodefines.
/// \date 2016
/// \author smal.root@gmail.com

#ifndef AMFIMPORTER_MACRO_HPP_INCLUDED
#define AMFIMPORTER_MACRO_HPP_INCLUDED

/// \def MACRO_ATTRREAD_LOOPBEG
/// Begin of loop that read attributes values.
#define MACRO_ATTRREAD_LOOPBEG \
	for(int idx = 0, idx_end = mReader->getAttributeCount(); idx < idx_end; idx++) \
	{ \
		std::string an(mReader->getAttributeName(idx));

/// \def MACRO_ATTRREAD_LOOPEND
/// End of loop that read attributes values.
#define MACRO_ATTRREAD_LOOPEND \
		Throw_IncorrectAttr(an); \
	}

/// \def MACRO_ATTRREAD_LOOPEND_WSKIP
/// End of loop that read attributes values. Difference from \ref MACRO_ATTRREAD_LOOPEND in that: current macro skip unknown attributes, but
/// \ref MACRO_ATTRREAD_LOOPEND throw an exception.
#define MACRO_ATTRREAD_LOOPEND_WSKIP \
		continue; \
	}

/// \def MACRO_ATTRREAD_CHECK_REF
/// Check curent attribute name and if it equal to requested then read value. Result write to output variable by reference. If result was read then
/// "continue" will called.
/// \param [in] pAttrName - attribute name.
/// \param [out] pVarName - output variable name.
/// \param [in] pFunction - function which read attribute value and write it to pVarName.
#define MACRO_ATTRREAD_CHECK_REF(pAttrName, pVarName, pFunction) \
	if(an == pAttrName) \
	{ \
		pFunction(idx, pVarName); \
		continue; \
	}

/// \def MACRO_ATTRREAD_CHECK_RET
/// Check curent attribute name and if it equal to requested then read value. Result write to output variable using return value of \ref pFunction.
/// If result was read then  "continue" will called.
/// \param [in] pAttrName - attribute name.
/// \param [out] pVarName - output variable name.
/// \param [in] pFunction - function which read attribute value and write it to pVarName.
#define MACRO_ATTRREAD_CHECK_RET(pAttrName, pVarName, pFunction) \
	if(an == pAttrName) \
	{ \
		pVarName = pFunction(idx); \
		continue; \
	}

/// \def MACRO_NODECHECK_LOOPBEGIN(pNodeName)
/// Begin of loop of parsing child nodes. Do not add ';' at end.
/// \param [in] pNodeName - current node name.
#define MACRO_NODECHECK_LOOPBEGIN(pNodeName) \
	do { \
	bool close_found = false; \
	 \
	while(mReader->read()) \
	{ \
		if(mReader->getNodeType() == irr::io::EXN_ELEMENT) \
		{

/// \def MACRO_NODECHECK_LOOPEND(pNodeName)
/// End of loop of parsing child nodes.
/// \param [in] pNodeName - current node name.
#define MACRO_NODECHECK_LOOPEND(pNodeName) \
			XML_CheckNode_SkipUnsupported(pNodeName); \
		}/* if(mReader->getNodeType() == irr::io::EXN_ELEMENT) */ \
		else if(mReader->getNodeType() == irr::io::EXN_ELEMENT_END) \
		{ \
			if(XML_CheckNode_NameEqual(pNodeName)) \
			{ \
				close_found = true; \
	 \
				break; \
			} \
		}/* else if(mReader->getNodeType() == irr::io::EXN_ELEMENT_END) */ \
	}/* while(mReader->read()) */ \
	 \
	if(!close_found) Throw_CloseNotFound(pNodeName); \
	 \
	} while(false)

/// \def MACRO_NODECHECK_READCOMP_F
/// Check curent node name and if it equal to requested then read value. Result write to output variable of type "float".
/// If result was read then  "continue" will called. Also check if node data already read then raise exception.
/// \param [in] pNodeName - node name.
/// \param [in, out] pReadFlag - read flag.
/// \param [out] pVarName - output variable name.
#define MACRO_NODECHECK_READCOMP_F(pNodeName, pReadFlag, pVarName) \
	if(XML_CheckNode_NameEqual(pNodeName)) \
	{ \
		/* Check if field already read before. */ \
		if(pReadFlag) Throw_MoreThanOnceDefined(pNodeName, "Only one component can be defined."); \
		/* Read color component and assign it to object. */ \
		pVarName = XML_ReadNode_GetVal_AsFloat(); \
		pReadFlag = true; \
		continue; \
	}

/// \def MACRO_NODECHECK_READCOMP_U32
/// Check curent node name and if it equal to requested then read value. Result write to output variable of type "uint32_t".
/// If result was read then  "continue" will called. Also check if node data already read then raise exception.
/// \param [in] pNodeName - node name.
/// \param [in, out] pReadFlag - read flag.
/// \param [out] pVarName - output variable name.
#define MACRO_NODECHECK_READCOMP_U32(pNodeName, pReadFlag, pVarName) \
	if(XML_CheckNode_NameEqual(pNodeName)) \
	{ \
		/* Check if field already read before. */ \
		if(pReadFlag) Throw_MoreThanOnceDefined(pNodeName, "Only one component can be defined."); \
		/* Read color component and assign it to object. */ \
		pVarName = XML_ReadNode_GetVal_AsU32(); \
		pReadFlag = true; \
		continue; \
	}

#endif // AMFIMPORTER_MACRO_HPP_INCLUDED
