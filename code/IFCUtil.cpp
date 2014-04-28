/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

/** @file  IFCUtil.cpp
 *  @brief Implementation of conversion routines for some common Ifc helper entities.
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER

#include "IFCUtil.h"
#include "PolyTools.h"
#include "ProcessHelper.h"

namespace Assimp {
	namespace IFC {

// ------------------------------------------------------------------------------------------------
void TempOpening::Transform(const IfcMatrix4& mat) 
{
	if(profileMesh) {
		profileMesh->Transform(mat);
	}
	if(profileMesh2D) {
		profileMesh2D->Transform(mat);
	}
	extrusionDir *= IfcMatrix3(mat);
}

// ------------------------------------------------------------------------------------------------
aiMesh* TempMesh::ToMesh() 
{
	ai_assert(verts.size() == std::accumulate(vertcnt.begin(),vertcnt.end(),size_t(0)));

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
void TempMesh::Transform(const IfcMatrix4& mat) 
{
	BOOST_FOREACH(IfcVector3& v, verts) {
		v *= mat;
	}
}

// ------------------------------------------------------------------------------
IfcVector3 TempMesh::Center() const
{
	return std::accumulate(verts.begin(),verts.end(),IfcVector3()) / static_cast<IfcFloat>(verts.size());
}

// ------------------------------------------------------------------------------------------------
void TempMesh::Append(const TempMesh& other)
{
	verts.insert(verts.end(),other.verts.begin(),other.verts.end());
	vertcnt.insert(vertcnt.end(),other.vertcnt.begin(),other.vertcnt.end());
}

// ------------------------------------------------------------------------------------------------
void TempMesh::RemoveDegenerates()
{
	// The strategy is simple: walk the mesh and compute normals using
	// Newell's algorithm. The length of the normals gives the area
	// of the polygons, which is close to zero for lines.

	std::vector<IfcVector3> normals;
	ComputePolygonNormals(normals, false);

	bool drop = false;
	size_t inor = 0;

	std::vector<IfcVector3>::iterator vit = verts.begin();
	for (std::vector<unsigned int>::iterator it = vertcnt.begin(); it != vertcnt.end(); ++inor) {
		const unsigned int pcount = *it;
		
		if (normals[inor].SquareLength() < 1e-5f) {
			it = vertcnt.erase(it);
			vit = verts.erase(vit, vit + pcount);

			drop = true;
			continue;
		}

		vit += pcount;
		++it;
	}

	if(drop) {
		IFCImporter::LogDebug("removing degenerate faces");
	}
}

// ------------------------------------------------------------------------------------------------
void TempMesh::ComputePolygonNormals(std::vector<IfcVector3>& normals, 
	bool normalize, 
	size_t ofs) const
{
	size_t max_vcount = 0;
	std::vector<unsigned int>::const_iterator begin = vertcnt.begin()+ofs, end = vertcnt.end(),  iit;
	for(iit = begin; iit != end; ++iit) {
		max_vcount = std::max(max_vcount,static_cast<size_t>(*iit));
	}

	std::vector<IfcFloat> temp((max_vcount+2)*4);
	normals.reserve( normals.size() + vertcnt.size()-ofs );

	// `NewellNormal()` currently has a relatively strange interface and need to 
	// re-structure things a bit to meet them.
	size_t vidx = std::accumulate(vertcnt.begin(),begin,0);
	for(iit = begin; iit != end; vidx += *iit++) {
		if (!*iit) {
			normals.push_back(IfcVector3());
			continue;
		}
		for(size_t vofs = 0, cnt = 0; vofs < *iit; ++vofs) {
			const IfcVector3& v = verts[vidx+vofs];
			temp[cnt++] = v.x;
			temp[cnt++] = v.y;
			temp[cnt++] = v.z;
#ifdef ASSIMP_BUILD_DEBUG
			temp[cnt] = std::numeric_limits<IfcFloat>::quiet_NaN();
#endif
			++cnt;
		}

		normals.push_back(IfcVector3());
		NewellNormal<4,4,4>(normals.back(),*iit,&temp[0],&temp[1],&temp[2]);
	}

	if(normalize) {
		BOOST_FOREACH(IfcVector3& n, normals) {
			n.Normalize();
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Compute the normal of the last polygon in the given mesh
IfcVector3 TempMesh::ComputeLastPolygonNormal(bool normalize) const
{
	size_t total = vertcnt.back(), vidx = verts.size() - total;
	std::vector<IfcFloat> temp((total+2)*3);
	for(size_t vofs = 0, cnt = 0; vofs < total; ++vofs) {
		const IfcVector3& v = verts[vidx+vofs];
		temp[cnt++] = v.x;
		temp[cnt++] = v.y;
		temp[cnt++] = v.z;
	}
	IfcVector3 nor;
	NewellNormal<3,3,3>(nor,total,&temp[0],&temp[1],&temp[2]);
	return normalize ? nor.Normalize() : nor;
}

// ------------------------------------------------------------------------------------------------
void TempMesh::FixupFaceOrientation()
{
	const IfcVector3 vavg = Center();

	std::vector<IfcVector3> normals;
	ComputePolygonNormals(normals);

	size_t c = 0, ofs = 0;
	BOOST_FOREACH(unsigned int cnt, vertcnt) {
		if (cnt>2){
			const IfcVector3& thisvert = verts[c];
			if (normals[ofs]*(thisvert-vavg) < 0) {
				std::reverse(verts.begin()+c,verts.begin()+cnt+c);
			}
		}
		c += cnt;
		++ofs;
	}
}

// ------------------------------------------------------------------------------------------------
void TempMesh::RemoveAdjacentDuplicates() 
{

	bool drop = false;
	std::vector<IfcVector3>::iterator base = verts.begin();
	BOOST_FOREACH(unsigned int& cnt, vertcnt) {
		if (cnt < 2){
			base += cnt;
			continue;
		}

		IfcVector3 vmin,vmax;
		ArrayBounds(&*base, cnt ,vmin,vmax);


		const IfcFloat epsilon = (vmax-vmin).SquareLength() / static_cast<IfcFloat>(1e9);
		//const IfcFloat dotepsilon = 1e-9;

		//// look for vertices that lie directly on the line between their predecessor and their 
		//// successor and replace them with either of them.

		//for(size_t i = 0; i < cnt; ++i) {
		//	IfcVector3& v1 = *(base+i), &v0 = *(base+(i?i-1:cnt-1)), &v2 = *(base+(i+1)%cnt);
		//	const IfcVector3& d0 = (v1-v0), &d1 = (v2-v1);
		//	const IfcFloat l0 = d0.SquareLength(), l1 = d1.SquareLength();
		//	if (!l0 || !l1) {
		//		continue;
		//	}

		//	const IfcFloat d = (d0/sqrt(l0))*(d1/sqrt(l1));

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
		std::vector<IfcVector3>::iterator end = base+cnt, e = std::unique( base, end, fz );
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
		IFCImporter::LogDebug("removing duplicate vertices");
	}
}

// ------------------------------------------------------------------------------------------------
void TempMesh::Swap(TempMesh& other)
{
	vertcnt.swap(other.vertcnt);
	verts.swap(other.verts);
}

// ------------------------------------------------------------------------------------------------
bool IsTrue(const EXPRESS::BOOLEAN& in)
{
	return (std::string)in == "TRUE" || (std::string)in == "T";
}

// ------------------------------------------------------------------------------------------------
IfcFloat ConvertSIPrefix(const std::string& prefix)
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
	out.r = static_cast<float>( in.Red );
	out.g = static_cast<float>( in.Green );
	out.b = static_cast<float>( in.Blue );
	out.a = static_cast<float>( 1.f );
}

// ------------------------------------------------------------------------------------------------
void ConvertColor(aiColor4D& out, const IfcColourOrFactor& in,ConversionData& conv,const aiColor4D* base)
{
	if (const EXPRESS::REAL* const r = in.ToPtr<EXPRESS::REAL>()) {
		out.r = out.g = out.b = static_cast<float>(*r);
		if(base) {
			out.r *= static_cast<float>( base->r );
			out.g *= static_cast<float>( base->g );
			out.b *= static_cast<float>( base->b );
			out.a = static_cast<float>( base->a );
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
void ConvertCartesianPoint(IfcVector3& out, const IfcCartesianPoint& in)
{
	out = IfcVector3();
	for(size_t i = 0; i < in.Coordinates.size(); ++i) {
		out[i] = in.Coordinates[i];
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertVector(IfcVector3& out, const IfcVector& in)
{
	ConvertDirection(out,in.Orientation);
	out *= in.Magnitude;
}

// ------------------------------------------------------------------------------------------------
void ConvertDirection(IfcVector3& out, const IfcDirection& in)
{
	out = IfcVector3();
	for(size_t i = 0; i < in.DirectionRatios.size(); ++i) {
		out[i] = in.DirectionRatios[i];
	}
	const IfcFloat len = out.Length();
	if (len<1e-6) {
		IFCImporter::LogWarn("direction vector magnitude too small, normalization would result in a division by zero");
		return;
	}
	out /= len;
}

// ------------------------------------------------------------------------------------------------
void AssignMatrixAxes(IfcMatrix4& out, const IfcVector3& x, const IfcVector3& y, const IfcVector3& z)
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
void ConvertAxisPlacement(IfcMatrix4& out, const IfcAxis2Placement3D& in)
{
	IfcVector3 loc;
	ConvertCartesianPoint(loc,in.Location);

	IfcVector3 z(0.f,0.f,1.f),r(1.f,0.f,0.f),x;

	if (in.Axis) { 
		ConvertDirection(z,*in.Axis.Get());
	}
	if (in.RefDirection) {
		ConvertDirection(r,*in.RefDirection.Get());
	}

	IfcVector3 v = r.Normalize();
	IfcVector3 tmpx = z * (v*z);

	x = (v-tmpx).Normalize();
	IfcVector3 y = (z^x);

	IfcMatrix4::Translation(loc,out);
	AssignMatrixAxes(out,x,y,z);
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(IfcMatrix4& out, const IfcAxis2Placement2D& in)
{
	IfcVector3 loc;
	ConvertCartesianPoint(loc,in.Location);

	IfcVector3 x(1.f,0.f,0.f);
	if (in.RefDirection) {
		ConvertDirection(x,*in.RefDirection.Get());
	}

	const IfcVector3 y = IfcVector3(x.y,-x.x,0.f);

	IfcMatrix4::Translation(loc,out);
	AssignMatrixAxes(out,x,y,IfcVector3(0.f,0.f,1.f));
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(IfcVector3& axis, IfcVector3& pos, const IfcAxis1Placement& in)
{
	ConvertCartesianPoint(pos,in.Location);
	if (in.Axis) {
		ConvertDirection(axis,in.Axis.Get());
	}
	else {
		axis = IfcVector3(0.f,0.f,1.f);
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertAxisPlacement(IfcMatrix4& out, const IfcAxis2Placement& in, ConversionData& conv)
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
void ConvertTransformOperator(IfcMatrix4& out, const IfcCartesianTransformationOperator& op)
{
	IfcVector3 loc;
	ConvertCartesianPoint(loc,op.LocalOrigin);

	IfcVector3 x(1.f,0.f,0.f),y(0.f,1.f,0.f),z(0.f,0.f,1.f);
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

	IfcMatrix4 locm;
	IfcMatrix4::Translation(loc,locm);	
	AssignMatrixAxes(out,x,y,z);


	IfcVector3 vscale;
	if (const IfcCartesianTransformationOperator3DnonUniform* nuni = op.ToPtr<IfcCartesianTransformationOperator3DnonUniform>()) {
		vscale.x = nuni->Scale?op.Scale.Get():1.f;
		vscale.y = nuni->Scale2?nuni->Scale2.Get():1.f;
		vscale.z = nuni->Scale3?nuni->Scale3.Get():1.f;
	}
	else {
		const IfcFloat sc = op.Scale?op.Scale.Get():1.f;
		vscale = IfcVector3(sc,sc,sc);
	}

	IfcMatrix4 s;
	IfcMatrix4::Scaling(vscale,s);

	out = locm * out * s;
}


} // ! IFC
} // ! Assimp

#endif
