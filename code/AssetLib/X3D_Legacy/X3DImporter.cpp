/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/
/// \file   X3DImporter.cpp
/// \brief  X3D-format files importer for Assimp: main algorithm implementation.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "AssetLib/VRML/VrmlConverter.hpp"
#include "X3DGeoHelper.h"
#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"
#include <assimp/StringUtils.h>

// Header files, Assimp.
#include <assimp/DefaultIOSystem.h>
#include <assimp/fast_atof.h>
#include "FIReader.hpp"

// Header files, stdlib.
#include <memory>
#include <string>
#include <iterator>

#if defined(ASSIMP_BUILD_NO_VRML_IMPORTER)
#define X3D_FORMATS_DESCR_STR "Extensible 3D(X3D, X3DB) Importer"
#define X3D_FORMATS_EXTENSIONS_STR "x3d x3db"
#else
#define X3D_FORMATS_DESCR_STR "VRML(WRL, X3DV) and Extensible 3D(X3D, X3DB) Importer"
#define X3D_FORMATS_EXTENSIONS_STR "wrl x3d x3db x3dv"
#endif // #if defined(ASSIMP_BUILD_NO_VRML_IMPORTER)

using namespace std;

namespace Assimp {

/// \var aiImporterDesc X3DImporter::Description
/// Constant which holds the importer description
const aiImporterDesc X3DImporter::Description = {
	"Extensible 3D(X3D) Importer",
	"smalcom",
	"",
	"See documentation in source code. Chapter: Limitations.",
	aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
	0,
	0,
	0,
	0,
	"x3d x3db"
};

//const std::regex X3DImporter::pattern_nws(R"([^, \t\r\n]+)");
//const std::regex X3DImporter::pattern_true(R"(^\s*(?:true|1)\s*$)", std::regex::icase);

struct WordIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = const char*;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    static const char *whitespace;
    const char *start_, *end_;
    WordIterator(const char *start, const char *end): start_(start), end_(end) {
        start_ = start + strspn(start, whitespace);
        if (start_ >= end_) {
            start_ = 0;
        }
    }
    WordIterator(): start_(0), end_(0) {}
    WordIterator(const WordIterator &other): start_(other.start_), end_(other.end_) {}
    WordIterator &operator=(const WordIterator &other) {
        start_ = other.start_;
        end_ = other.end_;
        return *this;
    }
    bool operator==(const WordIterator &other) const { return start_ == other.start_; }
    bool operator!=(const WordIterator &other) const { return start_ != other.start_; }
    WordIterator &operator++() {
        start_ += strcspn(start_, whitespace);
        start_ += strspn(start_, whitespace);
        if (start_ >= end_) {
            start_ = 0;
        }
        return *this;
    }
    WordIterator operator++(int) {
        WordIterator result(*this);
        ++(*this);
        return result;
    }
    const char *operator*() const { return start_; }
};

const char *WordIterator::whitespace = ", \t\r\n";

X3DImporter::X3DImporter()
: mNodeElementCur( nullptr )
, mReader( nullptr ) {
    // empty
}

X3DImporter::~X3DImporter() {
    // Clear() is accounting if data already is deleted. So, just check again if all data is deleted.
    Clear();
}

void X3DImporter::Clear() {
	mNodeElementCur = nullptr;
	// Delete all elements
	if(!NodeElement_List.empty()) {
        for ( std::list<X3DNodeElementBase*>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); ++it ) {
            delete *it;
        }
		NodeElement_List.clear();
	}
}


/*********************************************************************************************************************************************/
/************************************************************ Functions: find set ************************************************************/
/*********************************************************************************************************************************************/

bool X3DImporter::FindNodeElement_FromRoot(const std::string& pID, const X3DElemType pType, X3DNodeElementBase** pElement)
{
	for(std::list<X3DNodeElementBase*>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); ++it)
	{
		if(((*it)->Type == pType) && ((*it)->ID == pID))
		{
			if(pElement != nullptr) *pElement = *it;

			return true;
		}
	}// for(std::list<X3DNodeElementBase*>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); it++)

	return false;
}

bool X3DImporter::FindNodeElement_FromNode(X3DNodeElementBase* pStartNode, const std::string& pID,
													const X3DElemType pType, X3DNodeElementBase** pElement)
{
    bool found = false;// flag: true - if requested element is found.

	// Check if pStartNode - this is the element, we are looking for.
	if((pStartNode->Type == pType) && (pStartNode->ID == pID))
	{
		found = true;
        if ( pElement != nullptr )
        {
            *pElement = pStartNode;
        }

		goto fne_fn_end;
	}// if((pStartNode->Type() == pType) && (pStartNode->ID() == pID))

	// Check childs of pStartNode.
	for(std::list<X3DNodeElementBase*>::iterator ch_it = pStartNode->Children.begin(); ch_it != pStartNode->Children.end(); ++ch_it)
	{
		found = FindNodeElement_FromNode(*ch_it, pID, pType, pElement);
        if ( found )
        {
            break;
        }
	}// for(std::list<X3DNodeElementBase*>::iterator ch_it = it->Child.begin(); ch_it != it->Child.end(); ch_it++)

fne_fn_end:

	return found;
}

bool X3DImporter::FindNodeElement(const std::string& pID, const X3DElemType pType, X3DNodeElementBase** pElement)
{
    X3DNodeElementBase* tnd = mNodeElementCur;// temporary pointer to node.
    bool static_search = false;// flag: true if searching in static node.

    // At first check if we have deal with static node. Go up through parent nodes and check flag.
    while(tnd != nullptr)
    {
		if(tnd->Type == X3DElemType::ENET_Group)
		{
			if(((X3DNodeElementGroup*)tnd)->Static)
			{
				static_search = true;// Flag found, stop walking up. Node with static flag will holded in tnd variable.
				break;
			}
		}

		tnd = tnd->Parent;// go up in graph.
    }// while(tnd != nullptr)

    // at now call appropriate search function.
    if ( static_search )
    {
        return FindNodeElement_FromNode( tnd, pID, pType, pElement );
    }
    else
    {
        return FindNodeElement_FromRoot( pID, pType, pElement );
    }
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: throw set ***********************************************************/
/*********************************************************************************************************************************************/

void X3DImporter::Throw_ArgOutOfRange(const std::string& pArgument)
{
	throw DeadlyImportError("Argument value is out of range for: \"", pArgument, "\".");
}

void X3DImporter::Throw_CloseNotFound(const std::string& pNode)
{
	throw DeadlyImportError("Close tag for node <", pNode, "> not found. Seems file is corrupt.");
}

void X3DImporter::Throw_ConvertFail_Str2ArrF(const std::string& pAttrValue)
{
	throw DeadlyImportError("In <", mReader->getNodeName(), "> failed to convert attribute value \"", pAttrValue,
							"\" from string to array of floats.");
}

void X3DImporter::Throw_DEF_And_USE()
{
	throw DeadlyImportError("\"DEF\" and \"USE\" can not be defined both in <", mReader->getNodeName(), ">.");
}

void X3DImporter::Throw_IncorrectAttr(const std::string& pAttrName)
{
	throw DeadlyImportError("Node <", mReader->getNodeName(), "> has incorrect attribute \"", pAttrName, "\".");
}

void X3DImporter::Throw_IncorrectAttrValue(const std::string& pAttrName)
{
	throw DeadlyImportError("Attribute \"", pAttrName, "\" in node <", mReader->getNodeName(), "> has incorrect value.");
}

void X3DImporter::Throw_MoreThanOnceDefined(const std::string& pNodeType, const std::string& pDescription)
{
	throw DeadlyImportError("\"", pNodeType, "\" node can be used only once in ", mReader->getNodeName(), ". Description: ", pDescription);
}

void X3DImporter::Throw_TagCountIncorrect(const std::string& pNode)
{
	throw DeadlyImportError("Count of open and close tags for node <", pNode, "> are not equivalent. Seems file is corrupt.");
}

void X3DImporter::Throw_USE_NotFound(const std::string& pAttrValue)
{
	throw DeadlyImportError("Not found node with name \"", pAttrValue, "\" in <", mReader->getNodeName(), ">.");
}

/*********************************************************************************************************************************************/
/************************************************************* Functions: XML set ************************************************************/
/*********************************************************************************************************************************************/

void X3DImporter::XML_CheckNode_MustBeEmpty()
{
	if(!mReader->isEmptyElement()) throw DeadlyImportError("Node <", mReader->getNodeName(), "> must be empty.");
}

void X3DImporter::XML_CheckNode_SkipUnsupported(const std::string& pParentNodeName)
{
    static const size_t Uns_Skip_Len = 192;
    const char* Uns_Skip[ Uns_Skip_Len ] = {
	    // CAD geometry component
	    "CADAssembly", "CADFace", "CADLayer", "CADPart", "IndexedQuadSet", "QuadSet",
	    // Core
	    "ROUTE", "ExternProtoDeclare", "ProtoDeclare", "ProtoInstance", "ProtoInterface", "WorldInfo",
	    // Distributed interactive simulation (DIS) component
	    "DISEntityManager", "DISEntityTypeMapping", "EspduTransform", "ReceiverPdu", "SignalPdu", "TransmitterPdu",
	    // Cube map environmental texturing component
	    "ComposedCubeMapTexture", "GeneratedCubeMapTexture", "ImageCubeMapTexture",
	    // Environmental effects component
	    "Background", "Fog", "FogCoordinate", "LocalFog", "TextureBackground",
	    // Environmental sensor component
	    "ProximitySensor", "TransformSensor", "VisibilitySensor",
	    // Followers component
	    "ColorChaser", "ColorDamper", "CoordinateChaser", "CoordinateDamper", "OrientationChaser", "OrientationDamper", "PositionChaser", "PositionChaser2D",
	    "PositionDamper", "PositionDamper2D", "ScalarChaser", "ScalarDamper", "TexCoordChaser2D", "TexCoordDamper2D",
	    // Geospatial component
	    "GeoCoordinate", "GeoElevationGrid", "GeoLocation", "GeoLOD", "GeoMetadata", "GeoOrigin", "GeoPositionInterpolator", "GeoProximitySensor",
	    "GeoTouchSensor", "GeoTransform", "GeoViewpoint",
	    // Humanoid Animation (H-Anim) component
	    "HAnimDisplacer", "HAnimHumanoid", "HAnimJoint", "HAnimSegment", "HAnimSite",
	    // Interpolation component
	    "ColorInterpolator", "CoordinateInterpolator", "CoordinateInterpolator2D", "EaseInEaseOut", "NormalInterpolator", "OrientationInterpolator",
	    "PositionInterpolator", "PositionInterpolator2D", "ScalarInterpolator", "SplinePositionInterpolator", "SplinePositionInterpolator2D",
	    "SplineScalarInterpolator", "SquadOrientationInterpolator",
	    // Key device sensor component
	    "KeySensor", "StringSensor",
	    // Layering component
	    "Layer", "LayerSet", "Viewport",
	    // Layout component
	    "Layout", "LayoutGroup", "LayoutLayer", "ScreenFontStyle", "ScreenGroup",
	    // Navigation component
	    "Billboard", "Collision", "LOD", "NavigationInfo", "OrthoViewpoint", "Viewpoint", "ViewpointGroup",
	    // Networking component
	    "EXPORT", "IMPORT", "Anchor", "LoadSensor",
	    // NURBS component
	    "Contour2D", "ContourPolyline2D", "CoordinateDouble", "NurbsCurve", "NurbsCurve2D", "NurbsOrientationInterpolator", "NurbsPatchSurface",
	    "NurbsPositionInterpolator", "NurbsSet", "NurbsSurfaceInterpolator", "NurbsSweptSurface", "NurbsSwungSurface", "NurbsTextureCoordinate",
	    "NurbsTrimmedSurface",
	    // Particle systems component
	    "BoundedPhysicsModel", "ConeEmitter", "ExplosionEmitter", "ForcePhysicsModel", "ParticleSystem", "PointEmitter", "PolylineEmitter", "SurfaceEmitter",
	    "VolumeEmitter", "WindPhysicsModel",
	    // Picking component
	    "LinePickSensor", "PickableGroup", "PointPickSensor", "PrimitivePickSensor", "VolumePickSensor",
	    // Pointing device sensor component
	    "CylinderSensor", "PlaneSensor", "SphereSensor", "TouchSensor",
	    // Rendering component
	    "ClipPlane",
	    // Rigid body physics
	    "BallJoint", "CollidableOffset", "CollidableShape", "CollisionCollection", "CollisionSensor", "CollisionSpace", "Contact", "DoubleAxisHingeJoint",
	    "MotorJoint", "RigidBody", "RigidBodyCollection", "SingleAxisHingeJoint", "SliderJoint", "UniversalJoint",
	    // Scripting component
	    "Script",
	    // Programmable shaders component
	    "ComposedShader", "FloatVertexAttribute", "Matrix3VertexAttribute", "Matrix4VertexAttribute", "PackagedShader", "ProgramShader", "ShaderPart",
	    "ShaderProgram",
	    // Shape component
	    "FillProperties", "LineProperties", "TwoSidedMaterial",
	    // Sound component
	    "AudioClip", "Sound",
	    // Text component
	    "FontStyle", "Text",
	    // Texturing3D Component
	    "ComposedTexture3D", "ImageTexture3D", "PixelTexture3D", "TextureCoordinate3D", "TextureCoordinate4D", "TextureTransformMatrix3D", "TextureTransform3D",
	    // Texturing component
	    "MovieTexture", "MultiTexture", "MultiTextureCoordinate", "MultiTextureTransform", "PixelTexture", "TextureCoordinateGenerator", "TextureProperties",
	    // Time component
	    "TimeSensor",
	    // Event Utilities component
	    "BooleanFilter", "BooleanSequencer", "BooleanToggle", "BooleanTrigger", "IntegerSequencer", "IntegerTrigger", "TimeTrigger",
	    // Volume rendering component
	    "BlendedVolumeStyle", "BoundaryEnhancementVolumeStyle", "CartoonVolumeStyle", "ComposedVolumeStyle", "EdgeEnhancementVolumeStyle", "IsoSurfaceVolumeData",
	    "OpacityMapVolumeStyle", "ProjectionVolumeStyle", "SegmentedVolumeData", "ShadedVolumeStyle", "SilhouetteEnhancementVolumeStyle", "ToneMappedVolumeStyle",
	    "VolumeData"
    };

    const std::string nn( mReader->getNodeName() );
    bool found = false;
    bool close_found = false;

	for(size_t i = 0; i < Uns_Skip_Len; i++)
	{
		if(nn == Uns_Skip[i])
		{
			found = true;
			if(mReader->isEmptyElement())
			{
				close_found = true;

				goto casu_cres;
			}

			while(mReader->read())
			{
				if((mReader->getNodeType() == irr::io::EXN_ELEMENT_END) && (nn == mReader->getNodeName()))
				{
					close_found = true;

					goto casu_cres;
				}
			}
		}
	}

casu_cres:

	if(!found) throw DeadlyImportError("Unknown node \"", nn, "\" in ", pParentNodeName, ".");

	if(close_found)
		LogInfo("Skipping node \"" + nn + "\" in " + pParentNodeName + ".");
	else
		Throw_CloseNotFound(nn);
}

bool X3DImporter::XML_SearchNode(const std::string& pNodeName)
{
	while(mReader->read())
	{
		if((mReader->getNodeType() == irr::io::EXN_ELEMENT) && XML_CheckNode_NameEqual(pNodeName)) return true;
	}

	return false;
}

bool X3DImporter::XML_ReadNode_GetAttrVal_AsBool(const int pAttrIdx)
{
    auto boolValue = std::dynamic_pointer_cast<const FIBoolValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (boolValue) {
        if (boolValue->value.size() == 1) {
            return boolValue->value.front();
        }
        throw DeadlyImportError("Invalid bool value");
    }
    else {
        std::string val(mReader->getAttributeValue(pAttrIdx));

        if(val == "false")
            return false;
        else if(val == "true")
            return true;
        else
            throw DeadlyImportError("Bool attribute value can contain \"false\" or \"true\" not the \"", val, "\"");
    }
}

float X3DImporter::XML_ReadNode_GetAttrVal_AsFloat(const int pAttrIdx)
{
    auto floatValue = std::dynamic_pointer_cast<const FIFloatValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (floatValue) {
        if (floatValue->value.size() == 1) {
            return floatValue->value.front();
        }
        throw DeadlyImportError("Invalid float value");
    }
    else {
        std::string val;
        float tvalf;

        ParseHelper_FixTruncatedFloatString(mReader->getAttributeValue(pAttrIdx), val);
        fast_atoreal_move(val.c_str(), tvalf, false);

        return tvalf;
    }
}

int32_t X3DImporter::XML_ReadNode_GetAttrVal_AsI32(const int pAttrIdx)
{
    auto intValue = std::dynamic_pointer_cast<const FIIntValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (intValue) {
        if (intValue->value.size() == 1) {
            return intValue->value.front();
        }
        throw DeadlyImportError("Invalid int value");
    }
    else {
        return strtol10(mReader->getAttributeValue(pAttrIdx));
    }
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsCol3f(const int pAttrIdx, aiColor3D& pValue)
{
    std::vector<float> tlist;
    std::vector<float>::iterator it;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);
	if(tlist.size() != 3) Throw_ConvertFail_Str2ArrF(mReader->getAttributeValue(pAttrIdx));

	it = tlist.begin();
	pValue.r = *it++;
	pValue.g = *it++;
	pValue.b = *it;
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsVec2f(const int pAttrIdx, aiVector2D& pValue)
{
    std::vector<float> tlist;
    std::vector<float>::iterator it;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);
	if(tlist.size() != 2) Throw_ConvertFail_Str2ArrF(mReader->getAttributeValue(pAttrIdx));

	it = tlist.begin();
	pValue.x = *it++;
	pValue.y = *it;
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsVec3f(const int pAttrIdx, aiVector3D& pValue)
{
    std::vector<float> tlist;
    std::vector<float>::iterator it;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);
	if(tlist.size() != 3) Throw_ConvertFail_Str2ArrF(mReader->getAttributeValue(pAttrIdx));

	it = tlist.begin();
	pValue.x = *it++;
	pValue.y = *it++;
	pValue.z = *it;
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrB(const int pAttrIdx, std::vector<bool>& pValue)
{
    auto boolValue = std::dynamic_pointer_cast<const FIBoolValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (boolValue) {
        pValue = boolValue->value;
    }
    else {
        const char *val = mReader->getAttributeValue(pAttrIdx);
        pValue.clear();

        //std::cregex_iterator wordItBegin(val, val + strlen(val), pattern_nws);
        //const std::cregex_iterator wordItEnd;
        //std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const std::cmatch &match) { return std::regex_match(match.str(), pattern_true); });

        WordIterator wordItBegin(val, val + strlen(val));
        WordIterator wordItEnd;
        std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const char *match) { return (::tolower(match[0]) == 't') || (match[0] == '1'); });
    }
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrI32(const int pAttrIdx, std::vector<int32_t>& pValue)
{
    auto intValue = std::dynamic_pointer_cast<const FIIntValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (intValue) {
        pValue = intValue->value;
    }
    else {
        const char *val = mReader->getAttributeValue(pAttrIdx);
        pValue.clear();

        //std::cregex_iterator wordItBegin(val, val + strlen(val), pattern_nws);
        //const std::cregex_iterator wordItEnd;
        //std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const std::cmatch &match) { return std::stoi(match.str()); });

        WordIterator wordItBegin(val, val + strlen(val));
        WordIterator wordItEnd;
        std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const char *match) { return atoi(match); });
    }
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrF(const int pAttrIdx, std::vector<float>& pValue)
{
    auto floatValue = std::dynamic_pointer_cast<const FIFloatValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (floatValue) {
        pValue = floatValue->value;
    }
    else {
        const char *val = mReader->getAttributeValue(pAttrIdx);
        pValue.clear();

        //std::cregex_iterator wordItBegin(val, val + strlen(val), pattern_nws);
        //const std::cregex_iterator wordItEnd;
        //std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const std::cmatch &match) { return std::stof(match.str()); });

        WordIterator wordItBegin(val, val + strlen(val));
        WordIterator wordItEnd;
        std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const char *match) { return static_cast<float>(atof(match)); });
    }
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrD(const int pAttrIdx, std::vector<double>& pValue)
{
    auto doubleValue = std::dynamic_pointer_cast<const FIDoubleValue>(mReader->getAttributeEncodedValue(pAttrIdx));
    if (doubleValue) {
        pValue = doubleValue->value;
    }
    else {
        const char *val = mReader->getAttributeValue(pAttrIdx);
        pValue.clear();

        //std::cregex_iterator wordItBegin(val, val + strlen(val), pattern_nws);
        //const std::cregex_iterator wordItEnd;
        //std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const std::cmatch &match) { return std::stod(match.str()); });

        WordIterator wordItBegin(val, val + strlen(val));
        WordIterator wordItEnd;
        std::transform(wordItBegin, wordItEnd, std::back_inserter(pValue), [](const char *match) { return atof(match); });
    }
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsListCol3f(const int pAttrIdx, std::list<aiColor3D>& pValue)
{
    std::vector<float> tlist;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);// read as list
	if(tlist.size() % 3) Throw_ConvertFail_Str2ArrF(mReader->getAttributeValue(pAttrIdx));

	// copy data to array
	for(std::vector<float>::iterator it = tlist.begin(); it != tlist.end();)
	{
		aiColor3D tcol;

		tcol.r = *it++;
		tcol.g = *it++;
		tcol.b = *it++;
		pValue.push_back(tcol);
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrCol3f(const int pAttrIdx, std::vector<aiColor3D>& pValue)
{
    std::list<aiColor3D> tlist;

	XML_ReadNode_GetAttrVal_AsListCol3f(pAttrIdx, tlist);// read as list
	// and copy to array
	if(!tlist.empty())
	{
		pValue.reserve(tlist.size());
		for(std::list<aiColor3D>::iterator it = tlist.begin(); it != tlist.end(); ++it) pValue.push_back(*it);
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsListCol4f(const int pAttrIdx, std::list<aiColor4D>& pValue)
{
    std::vector<float> tlist;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);// read as list
	if(tlist.size() % 4) Throw_ConvertFail_Str2ArrF(mReader->getAttributeValue(pAttrIdx));

	// copy data to array
	for(std::vector<float>::iterator it = tlist.begin(); it != tlist.end();)
	{
		aiColor4D tcol;

		tcol.r = *it++;
		tcol.g = *it++;
		tcol.b = *it++;
		tcol.a = *it++;
		pValue.push_back(tcol);
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrCol4f(const int pAttrIdx, std::vector<aiColor4D>& pValue)
{
    std::list<aiColor4D> tlist;

	XML_ReadNode_GetAttrVal_AsListCol4f(pAttrIdx, tlist);// read as list
	// and copy to array
	if(!tlist.empty())
	{
		pValue.reserve(tlist.size());
        for ( std::list<aiColor4D>::iterator it = tlist.begin(); it != tlist.end(); ++it )
        {
            pValue.push_back( *it );
        }
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsListVec2f(const int pAttrIdx, std::list<aiVector2D>& pValue)
{
    std::vector<float> tlist;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);// read as list
    if ( tlist.size() % 2 )
    {
        Throw_ConvertFail_Str2ArrF( mReader->getAttributeValue( pAttrIdx ) );
    }

	// copy data to array
	for(std::vector<float>::iterator it = tlist.begin(); it != tlist.end();)
	{
		aiVector2D tvec;

		tvec.x = *it++;
		tvec.y = *it++;
		pValue.push_back(tvec);
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrVec2f(const int pAttrIdx, std::vector<aiVector2D>& pValue)
{
    std::list<aiVector2D> tlist;

	XML_ReadNode_GetAttrVal_AsListVec2f(pAttrIdx, tlist);// read as list
	// and copy to array
	if(!tlist.empty())
	{
		pValue.reserve(tlist.size());
        for ( std::list<aiVector2D>::iterator it = tlist.begin(); it != tlist.end(); ++it )
        {
            pValue.push_back( *it );
        }
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsListVec3f(const int pAttrIdx, std::list<aiVector3D>& pValue)
{
    std::vector<float> tlist;

	XML_ReadNode_GetAttrVal_AsArrF(pAttrIdx, tlist);// read as list
    if ( tlist.size() % 3 )
    {
        Throw_ConvertFail_Str2ArrF( mReader->getAttributeValue( pAttrIdx ) );
    }

	// copy data to array
	for(std::vector<float>::iterator it = tlist.begin(); it != tlist.end();)
	{
		aiVector3D tvec;

		tvec.x = *it++;
		tvec.y = *it++;
		tvec.z = *it++;
		pValue.push_back(tvec);
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsArrVec3f(const int pAttrIdx, std::vector<aiVector3D>& pValue)
{
    std::list<aiVector3D> tlist;

	XML_ReadNode_GetAttrVal_AsListVec3f(pAttrIdx, tlist);// read as list
	// and copy to array
	if(!tlist.empty())
	{
		pValue.reserve(tlist.size());
        for ( std::list<aiVector3D>::iterator it = tlist.begin(); it != tlist.end(); ++it )
        {
            pValue.push_back( *it );
        }
	}
}

void X3DImporter::XML_ReadNode_GetAttrVal_AsListS(const int pAttrIdx, std::list<std::string>& pValue)
{
	// make copy of attribute value - strings list.
	const size_t tok_str_len = strlen(mReader->getAttributeValue(pAttrIdx));
    if ( 0 == tok_str_len )
    {
        Throw_IncorrectAttrValue( mReader->getAttributeName( pAttrIdx ) );
    }

	// get pointer to begin of value.
    char *tok_str = const_cast<char*>(mReader->getAttributeValue(pAttrIdx));
    char *tok_str_end = tok_str + tok_str_len;
	// string list has following format: attr_name='"s1" "s2" "sn"'.
	do
	{
		char* tbeg;
		char* tend;
		size_t tlen;
		std::string tstr;

		// find begin of string(element of string list): "sn".
		tbeg = strstr(tok_str, "\"");
		if(tbeg == nullptr) Throw_IncorrectAttrValue(mReader->getAttributeName(pAttrIdx));

		tbeg++;// forward pointer from '\"' symbol to next after it.
		tok_str = tbeg;
		// find end of string(element of string list): "sn".
		tend = strstr(tok_str, "\"");
		if(tend == nullptr) Throw_IncorrectAttrValue(mReader->getAttributeName(pAttrIdx));

		tok_str = tend + 1;
		// create storage for new string
		tlen = tend - tbeg;
		tstr.resize(tlen);// reserve enough space and copy data
		memcpy((void*)tstr.data(), tbeg, tlen);// not strcpy because end of copied string from tok_str has no terminator.
		// and store string in output list.
		pValue.push_back(tstr);
	} while(tok_str < tok_str_end);
}

/*********************************************************************************************************************************************/
/****************************************************** Functions: geometry helper set  ******************************************************/
/*********************************************************************************************************************************************/


void X3DImporter::GeometryHelper_Extend_PolylineIdxToLineIdx(const std::list<int32_t>& pPolylineCoordIdx, std::list<int32_t>& pLineCoordIdx)
{
    std::list<int32_t>::const_iterator plit = pPolylineCoordIdx.begin();

	while(plit != pPolylineCoordIdx.end())
	{
		// add first point of polyline
		pLineCoordIdx.push_back(*plit++);
		while((*plit != (-1)) && (plit != pPolylineCoordIdx.end()))
		{
			std::list<int32_t>::const_iterator plit_next;

			plit_next = plit, ++plit_next;
			pLineCoordIdx.push_back(*plit);// second point of previous line.
			pLineCoordIdx.push_back(-1);// delimiter
			if((*plit_next == (-1)) || (plit_next == pPolylineCoordIdx.end())) break;// current polyline is finished

			pLineCoordIdx.push_back(*plit);// first point of next line.
			plit = plit_next;
		}// while((*plit != (-1)) && (plit != pPolylineCoordIdx.end()))
	}// while(plit != pPolylineCoordIdx.end())
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: parse set ***********************************************************/
/*********************************************************************************************************************************************/

void X3DImporter::ParseHelper_Group_Begin(const bool pStatic)
{
    X3DNodeElementGroup* new_group = new X3DNodeElementGroup(mNodeElementCur, pStatic);// create new node with current node as parent.

	// if we are adding not the root element then add new element to current element child list.
    if ( mNodeElementCur != nullptr )
    {
        mNodeElementCur->Children.push_back( new_group );
    }

	NodeElement_List.push_back(new_group);// it's a new element - add it to list.
	mNodeElementCur = new_group;// switch current element to new one.
}

void X3DImporter::ParseHelper_Node_Enter(X3DNodeElementBase* pNode)
{
	mNodeElementCur->Children.push_back(pNode);// add new element to current element child list.
	mNodeElementCur = pNode;// switch current element to new one.
}

void X3DImporter::ParseHelper_Node_Exit()
{
	// check if we can walk up.
    if ( mNodeElementCur != nullptr )
    {
        mNodeElementCur = mNodeElementCur->Parent;
    }
}

void X3DImporter::ParseHelper_FixTruncatedFloatString(const char* pInStr, std::string& pOutString)
{
	pOutString.clear();
    const size_t instr_len = strlen(pInStr);
    if ( 0 == instr_len )
    {
        return;
    }

	pOutString.reserve(instr_len * 3 / 2);
	// check and correct floats in format ".x". Must be "x.y".
    if ( pInStr[ 0 ] == '.' )
    {
        pOutString.push_back( '0' );
    }

	pOutString.push_back(pInStr[0]);
	for(size_t ci = 1; ci < instr_len; ci++)
	{
		if((pInStr[ci] == '.') && ((pInStr[ci - 1] == ' ') || (pInStr[ci - 1] == '-') || (pInStr[ci - 1] == '+') || (pInStr[ci - 1] == '\t')))
		{
			pOutString.push_back('0');
			pOutString.push_back('.');
		}
		else
		{
			pOutString.push_back(pInStr[ci]);
		}
	}
}

extern FIVocabulary X3D_vocabulary_3_2;
extern FIVocabulary X3D_vocabulary_3_3;

void X3DImporter::ParseFile(const std::string& pFile, IOSystem* pIOHandler)
{
    std::unique_ptr<FIReader> OldReader = std::move(mReader);// store current XMLreader.
    std::unique_ptr<IOStream> file(pIOHandler->Open(pFile, "rb"));

	// Check whether we can read from the file
    if ( file.get() == nullptr )
    {
        throw DeadlyImportError( "Failed to open X3D file ", pFile, "." );
    }
	mReader = FIReader::create(file.get());
    if ( !mReader )
    {
        throw DeadlyImportError( "Failed to create XML reader for file", pFile, "." );
    }
    mReader->registerVocabulary("urn:web3d:x3d:fi-vocabulary-3.2", &X3D_vocabulary_3_2);
    mReader->registerVocabulary("urn:web3d:x3d:fi-vocabulary-3.3", &X3D_vocabulary_3_3);
	// start reading
	ParseNode_Root();

	// restore old XMLreader
	mReader = std::move(OldReader);
}

void X3DImporter::ParseNode_Root()
{
	// search for root tag <X3D>
    if ( !XML_SearchNode( "X3D" ) )
    {
        throw DeadlyImportError( "Root node \"X3D\" not found." );
    }

	ParseHelper_Group_Begin();// create root node element.
	// parse other contents
	while(mReader->read())
	{
        if ( mReader->getNodeType() != irr::io::EXN_ELEMENT )
        {
            continue;
        }

		if(XML_CheckNode_NameEqual("head"))
			ParseNode_Head();
		else if(XML_CheckNode_NameEqual("Scene"))
			ParseNode_Scene();
		else
			XML_CheckNode_SkipUnsupported("Root");
	}

	// exit from root node element.
	ParseHelper_Node_Exit();
}

void X3DImporter::ParseNode_Head()
{
    bool close_found = false;// flag: true if close tag of node are found.

	while(mReader->read())
	{
		if(mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if(XML_CheckNode_NameEqual("meta"))
			{
				XML_CheckNode_MustBeEmpty();

				// adding metadata from <head> as MetaString from <Scene>
                bool added( false );
                X3DNodeElementMetaString* ms = new X3DNodeElementMetaString(mNodeElementCur);

				ms->Name = mReader->getAttributeValueSafe("name");
				// name must not be empty
				if(!ms->Name.empty())
				{
					ms->Value.push_back(mReader->getAttributeValueSafe("content"));
					NodeElement_List.push_back(ms);
                    if ( mNodeElementCur != nullptr )
                    {
                        mNodeElementCur->Children.push_back( ms );
                        added = true;
                    }
				}
                // if an error has occurred, release instance
                if ( !added ) {
                    delete ms;
                }
			}// if(XML_CheckNode_NameEqual("meta"))
		}// if(mReader->getNodeType() == irr::io::EXN_ELEMENT)
		else if(mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if(XML_CheckNode_NameEqual("head"))
			{
				close_found = true;
				break;
			}
		}// if(mReader->getNodeType() == irr::io::EXN_ELEMENT) else
	}// while(mReader->read())

    if ( !close_found )
    {
        Throw_CloseNotFound( "head" );
    }
}

void X3DImporter::ParseNode_Scene()
{
    auto GroupCounter_Increase = [](size_t &pCounter, const char *pGroupName) -> void {
        pCounter++;
        if (pCounter == 0) throw DeadlyImportError("Group counter overflow. Too much groups with type: ", pGroupName, ".");
    };

    auto GroupCounter_Decrease = [&](size_t &pCounter, const char *pGroupName) -> void {
        if (pCounter == 0) Throw_TagCountIncorrect(pGroupName);

        pCounter--;
    };

    static const char *GroupName_Group = "Group";
    static const char *GroupName_StaticGroup = "StaticGroup";
    static const char *GroupName_Transform = "Transform";
    static const char *GroupName_Switch = "Switch";

    bool close_found = false;
    size_t counter_group = 0;
    size_t counter_transform = 0;
    size_t counter_switch = 0;

	// while create static node? Because objects name used deeper in "USE" attribute can be equal to some meta in <head> node.
	ParseHelper_Group_Begin(true);
	while(mReader->read())
	{
		if(mReader->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if(XML_CheckNode_NameEqual("Shape"))
			{
				ParseNode_Shape_Shape();
			}
			else if(XML_CheckNode_NameEqual(GroupName_Group))
			{
				GroupCounter_Increase(counter_group, GroupName_Group);
				startReadGroup();
				// if node is empty then decrease group counter at this place.
				if(mReader->isEmptyElement()) GroupCounter_Decrease(counter_group, GroupName_Group);
			}
			else if(XML_CheckNode_NameEqual(GroupName_StaticGroup))
			{
				GroupCounter_Increase(counter_group, GroupName_StaticGroup);
				startReadStaticGroup();
				// if node is empty then decrease group counter at this place.
				if(mReader->isEmptyElement()) GroupCounter_Decrease(counter_group, GroupName_StaticGroup);
			}
			else if(XML_CheckNode_NameEqual(GroupName_Transform))
			{
				GroupCounter_Increase(counter_transform, GroupName_Transform);
				startReadTransform();
				// if node is empty then decrease group counter at this place.
				if(mReader->isEmptyElement()) GroupCounter_Decrease(counter_transform, GroupName_Transform);
			}
			else if(XML_CheckNode_NameEqual(GroupName_Switch))
			{
				GroupCounter_Increase(counter_switch, GroupName_Switch);
				startReadSwitch();
				// if node is empty then decrease group counter at this place.
				if(mReader->isEmptyElement()) GroupCounter_Decrease(counter_switch, GroupName_Switch);
			}
			else if(XML_CheckNode_NameEqual("DirectionalLight"))
		{
				ParseNode_Lighting_DirectionalLight();
			}
			else if(XML_CheckNode_NameEqual("PointLight"))
			{
				ParseNode_Lighting_PointLight();
			}
			else if(XML_CheckNode_NameEqual("SpotLight"))
			{
				ParseNode_Lighting_SpotLight();
			}
			else if(XML_CheckNode_NameEqual("Inline"))
			{
				ParseNode_Networking_Inline();
			}
			else if(!ParseHelper_CheckRead_X3DMetadataObject())
			{
				XML_CheckNode_SkipUnsupported("Scene");
			}
		}// if(mReader->getNodeType() == irr::io::EXN_ELEMENT)
		else if(mReader->getNodeType() == irr::io::EXN_ELEMENT_END)
		{
			if(XML_CheckNode_NameEqual("Scene"))
			{
				close_found = true;

				break;
			}
			else if(XML_CheckNode_NameEqual(GroupName_Group))
			{
				GroupCounter_Decrease(counter_group, GroupName_Group);
				endReadGroup();
			}
			else if(XML_CheckNode_NameEqual(GroupName_StaticGroup))
			{
				GroupCounter_Decrease(counter_group, GroupName_StaticGroup);
				endReadStaticGroup();
			}
			else if(XML_CheckNode_NameEqual(GroupName_Transform))
			{
				GroupCounter_Decrease(counter_transform, GroupName_Transform);
				endReadTransform();
			}
			else if(XML_CheckNode_NameEqual(GroupName_Switch))
			{
				GroupCounter_Decrease(counter_switch, GroupName_Switch);
				endReadSwitch();
			}
		}// if(mReader->getNodeType() == irr::io::EXN_ELEMENT) else
	}// while(mReader->read())

	ParseHelper_Node_Exit();

	if(counter_group) Throw_TagCountIncorrect("Group");
	if(counter_transform) Throw_TagCountIncorrect("Transform");
	if(counter_switch) Throw_TagCountIncorrect("Switch");
	if(!close_found) Throw_CloseNotFound("Scene");

}

/*********************************************************************************************************************************************/
/******************************************************** Functions: BaseImporter set ********************************************************/
/*********************************************************************************************************************************************/

bool X3DImporter::CanRead(const std::string& pFile, IOSystem* pIOHandler, bool pCheckSig) const
{
    const std::string extension = GetExtension(pFile);

	if((extension == "x3d") || (extension == "x3db")) return true;

	if(!extension.length() || pCheckSig)
	{
		const char* tokens[] = { "DOCTYPE X3D PUBLIC", "http://www.web3d.org/specifications/x3d" };

		return SearchFileHeaderForToken(pIOHandler, pFile, tokens, 2);
	}

	return false;
}

void X3DImporter::GetExtensionList(std::set<std::string>& pExtensionList)
{
	pExtensionList.insert("x3d");
	pExtensionList.insert("x3db");
}

const aiImporterDesc* X3DImporter::GetInfo () const
{
	return &Description;
}

void X3DImporter::InternReadFile(const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	mpIOHandler = pIOHandler;

	Clear();// delete old graph.
	std::string::size_type slashPos = pFile.find_last_of("\\/");
	pIOHandler->PushDirectory(slashPos == std::string::npos ? std::string() : pFile.substr(0, slashPos + 1));
	ParseFile(pFile, pIOHandler);
	pIOHandler->PopDirectory();
	//
	// Assimp use static arrays of objects for fast speed of rendering. That's good, but need some additional operations/
	// We know that geometry objects(meshes) are stored in <Shape>, also in <Shape>-><Appearance> materials(in Assimp logical view)
	// are stored. So at first we need to count how meshes and materials are stored in scene graph.
	//
	// at first creating root node for aiScene.
	pScene->mRootNode = new aiNode;
	pScene->mRootNode->mParent = nullptr;
	pScene->mFlags |= AI_SCENE_FLAGS_ALLOW_SHARED;
	//search for root node element
	mNodeElementCur = NodeElement_List.front();
	while(mNodeElementCur->Parent != nullptr) mNodeElementCur = mNodeElementCur->Parent;

	{// fill aiScene with objects.
		std::list<aiMesh*> mesh_list;
		std::list<aiMaterial*> mat_list;
		std::list<aiLight*> light_list;

		// create nodes tree
		Postprocess_BuildNode(*mNodeElementCur, *pScene->mRootNode, mesh_list, mat_list, light_list);
		// copy needed data to scene
		if(!mesh_list.empty())
		{
			std::list<aiMesh*>::const_iterator it = mesh_list.begin();

			pScene->mNumMeshes = static_cast<unsigned int>(mesh_list.size());
			pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
			for(size_t i = 0; i < pScene->mNumMeshes; i++) pScene->mMeshes[i] = *it++;
		}

		if(!mat_list.empty())
		{
			std::list<aiMaterial*>::const_iterator it = mat_list.begin();

			pScene->mNumMaterials = static_cast<unsigned int>(mat_list.size());
			pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
			for(size_t i = 0; i < pScene->mNumMaterials; i++) pScene->mMaterials[i] = *it++;
		}

		if(!light_list.empty())
		{
			std::list<aiLight*>::const_iterator it = light_list.begin();

			pScene->mNumLights = static_cast<unsigned int>(light_list.size());
			pScene->mLights = new aiLight*[pScene->mNumLights];
			for(size_t i = 0; i < pScene->mNumLights; i++) pScene->mLights[i] = *it++;
		}
	}// END: fill aiScene with objects.

	///TODO: IME optimize tree
}

}// namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
