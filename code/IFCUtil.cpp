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

/** @file  IFCUtil.cpp
 *  @brief Implementation of conversion routines for some common Ifc helper entities.
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER
#include "IFCUtil.h"
#include "ProcessHelper.h"

namespace Assimp {
	namespace IFC {

// ------------------------------------------------------------------------------------------------
void TempOpening::Transform(const aiMatrix4x4& mat) 
{
	if(profileMesh) {
		profileMesh->Transform(mat);
	}
	extrusionDir *= aiMatrix3x3(mat);
}

// ------------------------------------------------------------------------------------------------
aiMesh* TempMesh::ToMesh() 
{
	ai_assert(verts.size() == std::accumulate(vertcnt.begin(),vertcnt.end(),0));

	if (verts.empty()) {
		return NULL;
	}

	std::auto_ptr<aiMesh> mesh(new aiMesh());

	// copy vertices
	mesh->mNumVertices = static_cast<unsigned int>(verts.size());
	mesh->mVertices = new aiVector3D[mesh->mNumVertices];
	std::copy(verts.begin(),verts.end(),mesh->mVertices);

	// and build up faces
	mesh->mNumFaces = static_cast<unsigned int>(vertcnt.size());
	mesh->mFaces = new aiFace[mesh->mNumFaces];

	for(unsigned int i = 0,n=0, acc = 0; i < mesh->mNumFaces; ++n) {
		aiFace& f = mesh->mFaces[i];
		if (!vertcnt[n]) {
			--mesh->mNumFaces;
			continue;
		}

		f.mNumIndices = vertcnt[n];
		f.mIndices = new unsigned int[f.mNumIndices];
		for(unsigned int a = 0; a < f.mNumIndices; ++a) {
			f.mIndices[a] = acc++;
		}

		++i;
	}

	return mesh.release();
}

// ------------------------------------------------------------------------------------------------
void TempMesh::Clear()
{
	verts.clear();
	vertcnt.clear();
}

// ------------------------------------------------------------------------------------------------
void TempMesh::Transform(const aiMatrix4x4& mat) 
{
	BOOST_FOREACH(aiVector3D& v, verts) {
		v *= mat;
	}
}

// ------------------------------------------------------------------------------
aiVector3D TempMesh::Center() const
{
	return std::accumulate(verts.begin(),verts.end(),aiVector3D(0.f,0.f,0.f)) / static_cast<float>(verts.size());
}

// ------------------------------------------------------------------------------------------------
void TempMesh::Append(const TempMesh& other)
{
	verts.insert(verts.end(),other.verts.begin(),other.verts.end());
	vertcnt.insert(vertcnt.end(),other.vertcnt.begin(),other.vertcnt.end());
}

// ------------------------------------------------------------------------------------------------
void TempMesh::RemoveAdjacentDuplicates() 
{

	bool drop = false;
	std::vector<aiVector3D>::iterator base = verts.begin();
	BOOST_FOREACH(unsigned int& cnt, vertcnt) {
		if (cnt < 2){
			base += cnt;
			continue;
		}

		aiVector3D vmin,vmax;
		ArrayBounds(&*base, cnt ,vmin,vmax);


		const float epsilon = (vmax-vmin).SquareLength() / 1e9f;
		//const float dotepsilon = 1e-9;

		//// look for vertices that lie directly on the line between their predecessor and their 
		//// successor and replace them with either of them.

		//for(size_t i = 0; i < cnt; ++i) {
		//	aiVector3D& v1 = *(base+i), &v0 = *(base+(i?i-1:cnt-1)), &v2 = *(base+(i+1)%cnt);
		//	const aiVector3D& d0 = (v1-v0), &d1 = (v2-v1);
		//	const float l0 = d0.SquareLength(), l1 = d1.SquareLength();
		//	if (!l0 || !l1) {
		//		continue;
		//	}

		//	const float d = (d0/sqrt(l0))*(d1/sqrt(l1));

		//	if ( d >= 1.f-dotepsilon ) {
		//		v1 = v0;
		//	}
		//	else if ( d < -1.f+dotepsilon ) {
		//		v2 = v1;
		//		continue;
		//	}
		//}

		// drop any identical, adjacent vertices. this pass will collect the dropouts
		// of the previous pass as a side-effect.
		FuzzyVectorCompare fz(epsilon);
		std::vector<aiVector3D>::iterator end = base+cnt, e = std::unique( base, end, fz );
		if (e != end) {
			cnt -= static_cast<unsigned int>(std::distance(e, end));
			verts.erase(e,end);
			drop  = true;
		}

		// check front and back vertices for this polygon
		if (cnt > 1 && fz(*base,*(base+cnt-1))) {
			verts.erase(base+ --cnt);
			drop  = true;
		}

		// removing adjacent duplicates shouldn't erase everything :-)
		ai_assert(cnt>0);
		base += cnt;
	}
	if(drop) {
		IFCImporter::LogDebug("removed duplicate vertices");
	}
}

// ------------------------------------------------------------------------------------------------
bool IsTrue(const EXPRESS::BOOLEAN& in)
{
	return (std::string)in == "TRUE" || (std::string)in == "T";
}

// ------------------------------------------------------------------------------------------------
float ConvertSIPrefix(const std::string& prefix)
{
	if (prefix == "EXA") {
		return 1e18f;
	}
	else if (prefix == "PETA") {
		return 1e15f;
	}
	else if (prefix == "TERA") {
		return 1e12f;
	}
	else if (prefix == "GIGA") {
		return 1e9f;
	}
	else if (prefix == "MEGA") {
		return 1e6f;
	}
	else if (prefix == "KILO") {
		return 1e3f;
	}
	else if (prefix == "HECTO") {
		return 1e2f;
	}
	else if (prefix == "DECA") {
		return 1e-0f;
	}
	else if (prefix == "DECI") {
		return 1e-1f;
	}
	else if (prefix == "CENTI") {
		return 1e-2f;
	}
	else if (prefix == "MILLI") {
		return 1e-3f;
	}
	else if (prefix == "MICRO") {
		return 1e-6f;
	}
	else if (prefix == "NANO") {
		return 1e-9f;
	}
	else if (prefix == "PICO") {
		return 1e-12f;
	}
	else if (prefix == "FEMTO") {
		return 1e-15f;
	}
	else if (prefix == "ATTO") {
		return 1e-18f;
	}
	else {
		IFCImporter::LogError("Unrecognized SI prefix: " + prefix);
		return 1;
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertColor(aiColor4D& out, const IfcColourRgb& in)
{
	out.r = in.Red;
	out.g = in.Green;
	out.b = in.Blue;
	out.a = 1.f;
}

// ------------------------------------------------------------------------------------------------
void ConvertColor(aiColor4D& out, const IfcColourOrFactor& in,ConversionData& conv,const aiColor4D* base)
{
	if (const EXPRESS::REAL* const r = in.ToPtr<EXPRESS::REAL>()) {
		out.r = out.g = out.b = *r;
		if(base) {
			out.r *= base->r;
			out.g *= base->g;
			out.b *= base->b;
			out.a = base->a;
		}
		else out.a = 1.0;
	}
	else if (const IfcColourRgb* const rgb = in.ResolveSelectPtr<IfcColourRgb>(conv.db)) {
		ConvertColor(out,*rgb);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcColourOrFactor entity");
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertCartesianPoint(aiVector3D& out, const IfcCartesianPoint& in)
{
	out = aiVector3D();
	for(size_t i = 0; i < in.Coordinates.size(); ++i) {
		out[i] = in.Coordinates[i];
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertVector(aiVector3D& out, const IfcVector& in)
{
	ConvertDirection(out,in.Orientation);
	out *= in.Magnitude;
}

// ------------------------------------------------------------------------------------------------
void ConvertDirection(aiVector3D& out, const IfcDirection& in)
{
	out = aiVector3D();
	for(size_t i = 0; i < in.DirectionRatios.size(); ++i) {
		out[i] = in.DirectionRatios[i];
	}
	const float len = out.Length();
	if (len<1e-6) {
		IFCImporter::LogWarn("direction vector magnitude too small, normalization would result in a division by zero");
		return;
	}
	out /= len;
}

// ------------------------------------------------------------------------------------------------
void AssignMatrixAxes(aiMatrix4x4& out, const aiVector3D& x, const aiVector3D& y, const aiVector3D& z)
{
	out.a1 = x.x;
	out.b1 = x.y;
	out.c1 = x.z;

	out.a2 = y.x;
	out.b2 = y.y;
	out.c2 = y.z;

	out.a3 = z.x;
	out.b3 = z.y;
	out.c3 = z.z;
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(aiMatrix4x4& out, const IfcAxis2Placement3D& in)
{
	aiVector3D loc;
	ConvertCartesianPoint(loc,in.Location);

	aiVector3D z(0.f,0.f,1.f),r(1.f,0.f,0.f),x;

	if (in.Axis) { 
		ConvertDirection(z,*in.Axis.Get());
	}
	if (in.RefDirection) {
		ConvertDirection(r,*in.RefDirection.Get());
	}

	aiVector3D v = r.Normalize();
	aiVector3D tmpx = z * (v*z);

	x = (v-tmpx).Normalize();
	aiVector3D y = (z^x);

	aiMatrix4x4::Translation(loc,out);
	AssignMatrixAxes(out,x,y,z);
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(aiMatrix4x4& out, const IfcAxis2Placement2D& in)
{
	aiVector3D loc;
	ConvertCartesianPoint(loc,in.Location);

	aiVector3D x(1.f,0.f,0.f);
	if (in.RefDirection) {
		ConvertDirection(x,*in.RefDirection.Get());
	}

	const aiVector3D y = aiVector3D(x.y,-x.x,0.f);

	aiMatrix4x4::Translation(loc,out);
	AssignMatrixAxes(out,x,y,aiVector3D(0.f,0.f,1.f));
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(aiVector3D& axis, aiVector3D& pos, const IfcAxis1Placement& in)
{
	ConvertCartesianPoint(pos,in.Location);
	if (in.Axis) {
		ConvertDirection(axis,in.Axis.Get());
	}
	else {
		axis = aiVector3D(0.f,0.f,1.f);
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(aiMatrix4x4& out, const IfcAxis2Placement& in, ConversionData& conv)
{
	if(const IfcAxis2Placement3D* pl3 = in.ResolveSelectPtr<IfcAxis2Placement3D>(conv.db)) {
		ConvertAxisPlacement(out,*pl3);
	}
	else if(const IfcAxis2Placement2D* pl2 = in.ResolveSelectPtr<IfcAxis2Placement2D>(conv.db)) {
		ConvertAxisPlacement(out,*pl2);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcAxis2Placement entity");
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertTransformOperator(aiMatrix4x4& out, const IfcCartesianTransformationOperator& op)
{
	aiVector3D loc;
	ConvertCartesianPoint(loc,op.LocalOrigin);

	aiVector3D x(1.f,0.f,0.f),y(0.f,1.f,0.f),z(0.f,0.f,1.f);
	if (op.Axis1) {
		ConvertDirection(x,*op.Axis1.Get());
	}
	if (op.Axis2) {
		ConvertDirection(y,*op.Axis2.Get());
	}
	if (const IfcCartesianTransformationOperator3D* op2 = op.ToPtr<IfcCartesianTransformationOperator3D>()) {
		if(op2->Axis3) {
			ConvertDirection(z,*op2->Axis3.Get());
		}
	}

	aiMatrix4x4 locm;
	aiMatrix4x4::Translation(loc,locm);	
	AssignMatrixAxes(out,x,y,z);


	aiVector3D vscale;
	if (const IfcCartesianTransformationOperator3DnonUniform* nuni = op.ToPtr<IfcCartesianTransformationOperator3DnonUniform>()) {
		vscale.x = nuni->Scale?op.Scale.Get():1.f;
		vscale.y = nuni->Scale2?nuni->Scale2.Get():1.f;
		vscale.z = nuni->Scale3?nuni->Scale3.Get():1.f;
	}
	else {
		const float sc = op.Scale?op.Scale.Get():1.f;
		vscale = aiVector3D(sc,sc,sc);
	}

	aiMatrix4x4 s;
	aiMatrix4x4::Scaling(vscale,s);

	out = locm * out * s;
}

} // ! IFC
} // ! Assimp

#endif
