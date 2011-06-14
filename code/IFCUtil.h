/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

/** @file  IFC.cpp
 *  @brief Implementation of the Industry Foundation Classes loader.
 */

#ifndef INCLUDED_IFCUTIL_H
#define INCLUDED_IFCUTIL_H

#include "IFCReaderGen.h"
#include "IFCLoader.h"

namespace Assimp {
namespace IFC {

// helper for std::for_each to delete all heap-allocated items in a container
template<typename T>
struct delete_fun
{
	void operator()(T* del) {
		delete del;
	}
};

// ------------------------------------------------------------------------------------------------
// Temporary representation of an opening in a wall or a floor
// ------------------------------------------------------------------------------------------------
struct TempMesh;
struct TempOpening 
{
	const IFC::IfcExtrudedAreaSolid* solid;
	aiVector3D extrusionDir;
	boost::shared_ptr<TempMesh> profileMesh;

	// ------------------------------------------------------------------------------
	TempOpening(const IFC::IfcExtrudedAreaSolid* solid,aiVector3D extrusionDir,boost::shared_ptr<TempMesh> profileMesh)
		: solid(solid)
		, extrusionDir(extrusionDir)
		, profileMesh(profileMesh)
	{
	}

	// ------------------------------------------------------------------------------
	void Transform(const aiMatrix4x4& mat); // defined later since TempMesh is not complete yet
};


// ------------------------------------------------------------------------------------------------
// Intermediate data storage during conversion. Keeps everything and a bit more.
// ------------------------------------------------------------------------------------------------
struct ConversionData 
{
	ConversionData(const STEP::DB& db, const IFC::IfcProject& proj, aiScene* out,const IFCImporter::Settings& settings)
		: len_scale(1.0)
		, angle_scale(1.0)
		, db(db)
		, proj(proj)
		, out(out)
		, settings(settings)
		, apply_openings()
		, collect_openings()
	{}

	~ConversionData() {
		std::for_each(meshes.begin(),meshes.end(),delete_fun<aiMesh>());
		std::for_each(materials.begin(),materials.end(),delete_fun<aiMaterial>());
	}

	float len_scale, angle_scale;
	bool plane_angle_in_radians;

	const STEP::DB& db;
	const IFC::IfcProject& proj;
	aiScene* out;

	aiMatrix4x4 wcs;
	std::vector<aiMesh*> meshes;
	std::vector<aiMaterial*> materials;

	typedef std::map<const IFC::IfcRepresentationItem*, std::vector<unsigned int> > MeshCache;
	MeshCache cached_meshes;

	const IFCImporter::Settings& settings;

	// Intermediate arrays used to resolve openings in walls: only one of them
	// can be given at a time. apply_openings if present if the current element
	// is a wall and needs its openings to be poured into its geometry while
	// collect_openings is present only if the current element is an 
	// IfcOpeningElement, for which all the geometry needs to be preserved
	// for later processing by a parent, which is a wall. 
	std::vector<TempOpening>* apply_openings;
	std::vector<TempOpening>* collect_openings;
};

// ------------------------------------------------------------------------------------------------
struct FuzzyVectorCompare {

	FuzzyVectorCompare(float epsilon) : epsilon(epsilon) {}
	bool operator()(const aiVector3D& a, const aiVector3D& b) {
		return fabs((a-b).SquareLength()) < epsilon;
	}

	const float epsilon;
};


// ------------------------------------------------------------------------------------------------
// Helper used during mesh construction. Aids at creating aiMesh'es out of relatively few polygons.
// ------------------------------------------------------------------------------------------------
struct TempMesh
{
	std::vector<aiVector3D> verts;
	std::vector<unsigned int> vertcnt;

	// utilities
	aiMesh* ToMesh();
	void Clear();
	void Transform(const aiMatrix4x4& mat);
	aiVector3D Center() const;
	void Append(const TempMesh& other);
	void RemoveAdjacentDuplicates();
};





// conversion routines for common IFC entities, implemented in IFCUtil.cpp
void ConvertColor(aiColor4D& out, const IFC::IfcColourRgb& in);
void ConvertColor(aiColor4D& out, const IFC::IfcColourOrFactor& in,ConversionData& conv,const aiColor4D* base);
void ConvertCartesianPoint(aiVector3D& out, const IFC::IfcCartesianPoint& in);
void ConvertDirection(aiVector3D& out, const IFC::IfcDirection& in);
void AssignMatrixAxes(aiMatrix4x4& out, const aiVector3D& x, const aiVector3D& y, const aiVector3D& z);
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement3D& in);
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement2D& in);
void ConvertAxisPlacement(aiVector3D& axis, aiVector3D& pos, const IFC::IfcAxis1Placement& in);
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement& in, ConversionData& conv);
void ConvertTransformOperator(aiMatrix4x4& out, const IFC::IfcCartesianTransformationOperator& op);
bool IsTrue(const EXPRESS::BOOLEAN& in);
float ConvertSIPrefix(const std::string& prefix);


// IFCProfile.cpp
bool ProcessProfile(const IfcProfileDef& prof, TempMesh& meshout, ConversionData& conv);

// IFCMaterial.cpp
unsigned int ProcessMaterials(const IFC::IfcRepresentationItem& item, ConversionData& conv);

// IFCGeometry.cpp
bool ProcessRepresentationItem(const IfcRepresentationItem& item, std::vector<unsigned int>& mesh_indices, ConversionData& conv);
void AssignAddedMeshes(std::vector<unsigned int>& mesh_indices,aiNode* nd,ConversionData& /*conv*/);

}
}

#endif 
