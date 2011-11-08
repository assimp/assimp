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

/** @file  IFCGeometry.cpp
 *  @brief Geometry conversion and synthesis for IFC
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER
#include "IFCUtil.h"
#include "PolyTools.h"
#include "ProcessHelper.h"

#include <iterator>

namespace Assimp {
	namespace IFC {

// ------------------------------------------------------------------------------------------------
bool ProcessPolyloop(const IfcPolyLoop& loop, TempMesh& meshout, ConversionData& /*conv*/)
{
	size_t cnt = 0;
	BOOST_FOREACH(const IfcCartesianPoint& c, loop.Polygon) {
		aiVector3D tmp;
		ConvertCartesianPoint(tmp,c);

		meshout.verts.push_back(tmp);
		++cnt;
	}

	meshout.vertcnt.push_back(cnt);

	// zero- or one- vertex polyloops simply ignored
	if (meshout.vertcnt.back() > 1) { 
		return true;
	}
	
	if (meshout.vertcnt.back()==1) {
		meshout.vertcnt.pop_back();
		meshout.verts.pop_back();
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void ComputePolygonNormals(const TempMesh& meshout, std::vector<aiVector3D>& normals, bool normalize = true, size_t ofs = 0) 
{
	size_t max_vcount = 0;
	std::vector<unsigned int>::const_iterator begin=meshout.vertcnt.begin()+ofs, end=meshout.vertcnt.end(),  iit;
	for(iit = begin; iit != end; ++iit) {
		max_vcount = std::max(max_vcount,static_cast<size_t>(*iit));
	}

	std::vector<float> temp((max_vcount+2)*4);
	normals.reserve( normals.size() + meshout.vertcnt.size()-ofs );

	// `NewellNormal()` currently has a relatively strange interface and need to 
	// re-structure things a bit to meet them.
	size_t vidx = std::accumulate(meshout.vertcnt.begin(),begin,0);
	for(iit = begin; iit != end; vidx += *iit++) {
		if (!*iit) {
			normals.push_back(aiVector3D());
			continue;
		}
		for(size_t vofs = 0, cnt = 0; vofs < *iit; ++vofs) {
			const aiVector3D& v = meshout.verts[vidx+vofs];
			temp[cnt++] = v.x;
			temp[cnt++] = v.y;
			temp[cnt++] = v.z;
#ifdef _DEBUG
			temp[cnt] = std::numeric_limits<float>::quiet_NaN();
#endif
			++cnt;
		}

		normals.push_back(aiVector3D());
		NewellNormal<4,4,4>(normals.back(),*iit,&temp[0],&temp[1],&temp[2]);
	}

	if(normalize) {
		BOOST_FOREACH(aiVector3D& n, normals) {
			n.Normalize();
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Compute the normal of the last polygon in the given mesh
aiVector3D ComputePolygonNormal(const TempMesh& inmesh, bool normalize = true) 
{
	size_t total = inmesh.vertcnt.back(), vidx = inmesh.verts.size() - total;
	std::vector<float> temp((total+2)*3);
	for(size_t vofs = 0, cnt = 0; vofs < total; ++vofs) {
		const aiVector3D& v = inmesh.verts[vidx+vofs];
		temp[cnt++] = v.x;
		temp[cnt++] = v.y;
		temp[cnt++] = v.z;
	}
	aiVector3D nor;
	NewellNormal<3,3,3>(nor,total,&temp[0],&temp[1],&temp[2]);
	return normalize ? nor.Normalize() : nor;
}

// ------------------------------------------------------------------------------------------------
void FixupFaceOrientation(TempMesh& result)
{
	const aiVector3D vavg = result.Center();

	std::vector<aiVector3D> normals;
	ComputePolygonNormals(result,normals);

	size_t c = 0, ofs = 0;
	BOOST_FOREACH(unsigned int cnt, result.vertcnt) {
		if (cnt>2){
			const aiVector3D& thisvert = result.verts[c];
			if (normals[ofs]*(thisvert-vavg) < 0) {
				std::reverse(result.verts.begin()+c,result.verts.begin()+cnt+c);
			}
		}
		c += cnt;
		++ofs;
	}
}

// ------------------------------------------------------------------------------------------------
void RecursiveMergeBoundaries(TempMesh& final_result, const TempMesh& in, const TempMesh& boundary, std::vector<aiVector3D>& normals, const aiVector3D& nor_boundary)
{
	ai_assert(in.vertcnt.size() >= 1);
	ai_assert(boundary.vertcnt.size() == 1);
	std::vector<unsigned int>::const_iterator end = in.vertcnt.end(), begin=in.vertcnt.begin(), iit, best_iit;

	TempMesh out;

	// iterate through all other bounds and find the one for which the shortest connection
	// to the outer boundary is actually the shortest possible.
	size_t vidx = 0, best_vidx_start = 0;
	size_t best_ofs, best_outer = boundary.verts.size();
	float best_dist = 1e10;
	for(std::vector<unsigned int>::const_iterator iit = begin; iit != end; vidx += *iit++) {
		
		for(size_t vofs = 0; vofs < *iit; ++vofs) {
			const aiVector3D& v = in.verts[vidx+vofs];

			for(size_t outer = 0; outer < boundary.verts.size(); ++outer) {
				const aiVector3D& o = boundary.verts[outer];
				const float d = (o-v).SquareLength();

				if (d < best_dist) {
					best_dist = d;
					best_ofs = vofs;
					best_outer = outer;
					best_iit = iit;
					best_vidx_start = vidx;
				}
			}		
		}
	}

	ai_assert(best_outer != boundary.verts.size());


	// now that we collected all vertex connections to be added, build the output polygon
	const size_t cnt = boundary.verts.size() + *best_iit+2;
	out.verts.reserve(cnt);

	for(size_t outer = 0; outer < boundary.verts.size(); ++outer) {
		const aiVector3D& o = boundary.verts[outer];
		out.verts.push_back(o);

		if (outer == best_outer) {
			for(size_t i = best_ofs; i < *best_iit; ++i) {
				out.verts.push_back(in.verts[best_vidx_start + i]);
			}

			// we need the first vertex of the inner polygon twice as we return to the
			// outer loop through the very same connection through which we got there.
			for(size_t i = 0; i <= best_ofs; ++i) {
				out.verts.push_back(in.verts[best_vidx_start + i]);
			}

			// reverse face winding if the normal of the sub-polygon points in the
			// same direction as the normal of the outer polygonal boundary
			if (normals[std::distance(begin,best_iit)] * nor_boundary > 0) {
				std::reverse(out.verts.rbegin(),out.verts.rbegin()+*best_iit+1);
			}

			// also append a copy of the initial insertion point to be able to continue the outer polygon
			out.verts.push_back(o);
		}
	}
	out.vertcnt.push_back(cnt);
	ai_assert(out.verts.size() == cnt);

	if (in.vertcnt.size()-std::count(begin,end,0) > 1) {
		// Recursively apply the same algorithm if there are more boundaries to merge. The
		// current implementation is relatively inefficient, though.
		
		TempMesh temp;
		
		// drop the boundary that we just processed
		const size_t dist = std::distance(begin, best_iit);
		TempMesh remaining = in;
		remaining.vertcnt.erase(remaining.vertcnt.begin() + dist);
		remaining.verts.erase(remaining.verts.begin()+best_vidx_start,remaining.verts.begin()+best_vidx_start+*best_iit);

		normals.erase(normals.begin() + dist);
		RecursiveMergeBoundaries(temp,remaining,out,normals,nor_boundary);

		final_result.Append(temp);
	}
	else final_result.Append(out);
}

// ------------------------------------------------------------------------------------------------
void MergePolygonBoundaries(TempMesh& result, const TempMesh& inmesh, size_t master_bounds = -1) 
{
	// standard case - only one boundary, just copy it to the result vector
	if (inmesh.vertcnt.size() <= 1) {
		result.Append(inmesh);
		return;
	}

	result.vertcnt.reserve(inmesh.vertcnt.size()+result.vertcnt.size());

	// XXX get rid of the extra copy if possible
	TempMesh meshout = inmesh;

	// handle polygons with holes. Our built in triangulation won't handle them as is, but
	// the ear cutting algorithm is solid enough to deal with them if we join the inner
	// holes with the outer boundaries by dummy connections.
	IFCImporter::LogDebug("fixing polygon with holes for triangulation via ear-cutting");
	std::vector<unsigned int>::iterator outer_polygon = meshout.vertcnt.end(), begin=meshout.vertcnt.begin(), end=outer_polygon,  iit;

	// each hole results in two extra vertices
	result.verts.reserve(meshout.verts.size()+meshout.vertcnt.size()*2+result.verts.size());
	size_t outer_polygon_start = 0;

	// do not normalize 'normals', we need the original length for computing the polygon area
	std::vector<aiVector3D> normals;
	ComputePolygonNormals(meshout,normals,false);

	// see if one of the polygons is a IfcFaceOuterBound (in which case `master_bounds` is its index).
	// sadly we can't rely on it, the docs say 'At most one of the bounds shall be of the type IfcFaceOuterBound' 
	float area_outer_polygon = 1e-10f;
	if (master_bounds != (size_t)-1) {
		outer_polygon = begin + master_bounds;
		outer_polygon_start = std::accumulate(begin,outer_polygon,0);
		area_outer_polygon = normals[master_bounds].SquareLength();
	}
	else {
		size_t vidx = 0;
		for(iit = begin; iit != meshout.vertcnt.end(); vidx += *iit++) {
			// find the polygon with the largest area, it must be the outer bound. 
			aiVector3D& n = normals[std::distance(begin,iit)];
			const float area = n.SquareLength();
			if (area > area_outer_polygon) {
				area_outer_polygon = area;
				outer_polygon = iit;
				outer_polygon_start = vidx;
			}
		}
	}

	ai_assert(outer_polygon != meshout.vertcnt.end());	
	std::vector<aiVector3D>& in = meshout.verts;

	// skip over extremely small boundaries - this is a workaround to fix cases
	// in which the number of holes is so extremely large that the
	// triangulation code fails.
#define IFC_VERTICAL_HOLE_SIZE_THRESHOLD 0.000001f
	size_t vidx = 0, removed = 0, index = 0;
	const float threshold = area_outer_polygon * IFC_VERTICAL_HOLE_SIZE_THRESHOLD;
	for(iit = begin; iit != end ;++index) {
		const float sqlen = normals[index].SquareLength();
		if (sqlen < threshold) {
			std::vector<aiVector3D>::iterator inbase = in.begin()+vidx;
			in.erase(inbase,inbase+*iit);
			
			outer_polygon_start -= outer_polygon_start>vidx ? *iit : 0;
			*iit++ = 0;
			++removed;

			IFCImporter::LogDebug("skip small hole below threshold");
		}
		else {
			normals[index] /= sqrt(sqlen);
			vidx += *iit++;
		}
	}

	// see if one or more of the hole has a face that lies directly on an outer bound.
	// this happens for doors, for example.
	vidx = 0;
	for(iit = begin; ; vidx += *iit++) {
next_loop:
		if (iit == end) {
			break;
		}
		if (iit == outer_polygon) {
			continue;
		}

		for(size_t vofs = 0; vofs < *iit; ++vofs) {
			if (!*iit) {
				continue;
			}
			const size_t next = (vofs+1)%*iit;
			const aiVector3D& v = in[vidx+vofs], &vnext = in[vidx+next],&vd = (vnext-v).Normalize();

			for(size_t outer = 0; outer < *outer_polygon; ++outer) {
				const aiVector3D& o = in[outer_polygon_start+outer], &onext = in[outer_polygon_start+(outer+1)%*outer_polygon], &od = (onext-o).Normalize();

				if (fabs(vd * od) > 1.f-1e-6f && (onext-v).Normalize() * vd > 1.f-1e-6f && (onext-v)*(o-v) < 0) {
					IFCImporter::LogDebug("got an inner hole that lies partly on the outer polygonal boundary, merging them to a single contour");

					// in between outer and outer+1 insert all vertices of this loop, then drop the original altogether.
					std::vector<aiVector3D> tmp(*iit);

					const size_t start = (v-o).SquareLength() > (vnext-o).SquareLength() ? vofs :  next;
					std::vector<aiVector3D>::iterator inbase = in.begin()+vidx, it = std::copy(inbase+start, inbase+*iit,tmp.begin());
					std::copy(inbase, inbase+start,it);
					std::reverse(tmp.begin(),tmp.end());

					in.insert(in.begin()+outer_polygon_start+(outer+1)%*outer_polygon,tmp.begin(),tmp.end());
					vidx += outer_polygon_start<vidx ? *iit : 0;

					inbase = in.begin()+vidx;
					in.erase(inbase,inbase+*iit);

					outer_polygon_start -= outer_polygon_start>vidx ? *iit : 0;
					
					*outer_polygon += tmp.size();
					*iit++ = 0;
					++removed;
					goto next_loop;
				}
			}
		}
	}

	if ( meshout.vertcnt.size() - removed <= 1) {
		result.Append(meshout);
		return;
	}

	// extract the outer boundary and move it to a separate mesh
	TempMesh boundary;
	boundary.vertcnt.resize(1,*outer_polygon);
	boundary.verts.resize(*outer_polygon);

	std::vector<aiVector3D>::iterator b = in.begin()+outer_polygon_start;
	std::copy(b,b+*outer_polygon,boundary.verts.begin());
	in.erase(b,b+*outer_polygon);

	std::vector<aiVector3D>::iterator norit = normals.begin()+std::distance(meshout.vertcnt.begin(),outer_polygon);
	const aiVector3D nor_boundary = *norit;
	normals.erase(norit);
	meshout.vertcnt.erase(outer_polygon);

	// keep merging the closest inner boundary with the outer boundary until no more boundaries are left
	RecursiveMergeBoundaries(result,meshout,boundary,normals,nor_boundary);
}


// ------------------------------------------------------------------------------------------------
void ProcessConnectedFaceSet(const IfcConnectedFaceSet& fset, TempMesh& result, ConversionData& conv)
{
	BOOST_FOREACH(const IfcFace& face, fset.CfsFaces) {

		// size_t ob = -1, cnt = 0;
		TempMesh meshout;
		BOOST_FOREACH(const IfcFaceBound& bound, face.Bounds) {
			
			// XXX implement proper merging for polygonal loops
			if(const IfcPolyLoop* const polyloop = bound.Bound->ToPtr<IfcPolyLoop>()) {
				if(ProcessPolyloop(*polyloop, meshout,conv)) {

					//if(bound.ToPtr<IfcFaceOuterBound>()) {
					//	ob = cnt;
					//}
					//++cnt;

				}
			}
			else {
				IFCImporter::LogWarn("skipping unknown IfcFaceBound entity, type is " + bound.Bound->GetClassName());
				continue;
			}

			/*if(!IsTrue(bound.Orientation)) {
				size_t c = 0;
				BOOST_FOREACH(unsigned int& c, meshout.vertcnt) {
					std::reverse(result.verts.begin() + cnt,result.verts.begin() + cnt + c);
					cnt += c;
				}
			}*/

		}
		MergePolygonBoundaries(result,meshout);
	}
}




// ------------------------------------------------------------------------------------------------
void ProcessRevolvedAreaSolid(const IfcRevolvedAreaSolid& solid, TempMesh& result, ConversionData& conv)
{
	TempMesh meshout;

	// first read the profile description
	if(!ProcessProfile(*solid.SweptArea,meshout,conv) || meshout.verts.size()<=1) {
		return;
	}

	aiVector3D axis, pos;
	ConvertAxisPlacement(axis,pos,solid.Axis);

	aiMatrix4x4 tb0,tb1;
	aiMatrix4x4::Translation(pos,tb0);
	aiMatrix4x4::Translation(-pos,tb1);

	const std::vector<aiVector3D>& in = meshout.verts;
	const size_t size=in.size();
	
	bool has_area = solid.SweptArea->ProfileType == "AREA" && size>2;
	const float max_angle = solid.Angle*conv.angle_scale;
	if(fabs(max_angle) < 1e-3) {
		if(has_area) {
			result = meshout;
		}
		return;
	}

	const unsigned int cnt_segments = std::max(2u,static_cast<unsigned int>(16 * fabs(max_angle)/AI_MATH_HALF_PI_F));
	const float delta = max_angle/cnt_segments;

	has_area = has_area && fabs(max_angle) < AI_MATH_TWO_PI_F*0.99;
	
	result.verts.reserve(size*((cnt_segments+1)*4+(has_area?2:0)));
	result.vertcnt.reserve(size*cnt_segments+2);

	aiMatrix4x4 rot;
	rot = tb0 * aiMatrix4x4::Rotation(delta,axis,rot) * tb1;

	size_t base = 0;
	std::vector<aiVector3D>& out = result.verts;

	// dummy data to simplify later processing
	for(size_t i = 0; i < size; ++i) {
		out.insert(out.end(),4,in[i]);
	}

	for(unsigned int seg = 0; seg < cnt_segments; ++seg) {
		for(size_t i = 0; i < size; ++i) {
			const size_t next = (i+1)%size;

			result.vertcnt.push_back(4);
			const aiVector3D& base_0 = out[base+i*4+3],base_1 = out[base+next*4+3];

			out.push_back(base_0);
			out.push_back(base_1);
			out.push_back(rot*base_1);
			out.push_back(rot*base_0);
		}
		base += size*4;
	}

	out.erase(out.begin(),out.begin()+size*4);

	if(has_area) {
		// leave the triangulation of the profile area to the ear cutting 
		// implementation in aiProcess_Triangulate - for now we just
		// feed in two huge polygons.
		base -= size*8;
		for(size_t i = size; i--; ) {
			out.push_back(out[base+i*4+3]);
		}
		for(size_t i = 0; i < size; ++i ) {
			out.push_back(out[i*4]);
		}
		result.vertcnt.push_back(size);
		result.vertcnt.push_back(size);
	}

	aiMatrix4x4 trafo;
	ConvertAxisPlacement(trafo, solid.Position);
	
	result.Transform(trafo);
	IFCImporter::LogDebug("generate mesh procedurally by radial extrusion (IfcRevolvedAreaSolid)");
}


// ------------------------------------------------------------------------------------------------
bool TryAddOpenings(const std::vector<TempOpening>& openings,const std::vector<aiVector3D>& nors, TempMesh& curmesh)
{
	std::vector<aiVector3D>& out = curmesh.verts;

	const size_t s = out.size();

	const aiVector3D any_point = out[s-1];
	const aiVector3D nor = ComputePolygonNormal(curmesh); ;
	
	bool got_openings = false;
	TempMesh res;

	size_t c = 0;
	BOOST_FOREACH(const TempOpening& t,openings) {
		const aiVector3D& outernor = nors[c++];
		const float dot = nor * outernor;
		if (fabs(dot)<1.f-1e-6f) {
			continue;
		}

		// const aiVector3D diff = t.extrusionDir;
		const std::vector<aiVector3D>& va = t.profileMesh->verts;
		if(va.size() <= 2) {
			continue;	
		}

		// const float dd = t.extrusionDir*nor;
		IFCImporter::LogDebug("apply an IfcOpeningElement linked via IfcRelVoidsElement to this polygon");

		got_openings = true;

		// project va[i] onto the plane formed by the current polygon [given by (any_point,nor)]
		for(size_t i = 0; i < va.size(); ++i) {
			const aiVector3D& v = va[i];
			out.push_back(v-(nor*(v-any_point))*nor);
		}
		

		curmesh.vertcnt.push_back(va.size());

		res.Clear();
		MergePolygonBoundaries(res,curmesh,0);
		curmesh = res;
	}
	return got_openings;
}

// ------------------------------------------------------------------------------------------------
struct DistanceSorter {

	DistanceSorter(const aiVector3D& base) : base(base) {}

	bool operator () (const TempOpening& a, const TempOpening& b) const {
		return (a.profileMesh->Center()-base).SquareLength() < (b.profileMesh->Center()-base).SquareLength();
	}

	aiVector3D base;
};

// ------------------------------------------------------------------------------------------------
struct XYSorter {

	// sort first by X coordinates, then by Y coordinates
	bool operator () (const aiVector2D&a, const aiVector2D& b) const {
		if (a.x == b.x) {
			return a.y < b.y;
		}
		return a.x < b.x;
	}
};

// ------------------------------------------------------------------------------------------------
struct ProjectionInfo {
	unsigned int ac, bc;
	aiVector3D p,u,v;
};

typedef std::pair< aiVector2D, aiVector2D > BoundingBox;
typedef std::map<aiVector2D,size_t,XYSorter> XYSortedField;

// ------------------------------------------------------------------------------------------------
aiVector2D ProjectPositionVectorOntoPlane(const aiVector3D& x, const ProjectionInfo& proj) 
{
	const aiVector3D xx = x-proj.p;
	return aiVector2D(xx[proj.ac]/proj.u[proj.ac],xx[proj.bc]/proj.v[proj.bc]);
}

// ------------------------------------------------------------------------------------------------
void QuadrifyPart(const aiVector2D& pmin, const aiVector2D& pmax, XYSortedField& field, const std::vector< BoundingBox >& bbs, 
	std::vector<aiVector2D>& out)
{
	if (!(pmin.x-pmax.x) || !(pmin.y-pmax.y)) {
		return;
	}

	float xs = 1e10, xe = 1e10;	
	bool found = false;

	// Search along the x-axis until we find an opening
	XYSortedField::iterator start = field.begin();
	for(; start != field.end(); ++start) {
		const BoundingBox& bb = bbs[(*start).second];
		if(bb.first.x >= pmax.x) {
			break;
		} 

		if (bb.second.x > pmin.x && bb.second.y > pmin.y && bb.first.y < pmax.y) {
			xs = bb.first.x;
			xe = bb.second.x;
			found = true;
			break;
		}
	}

	if (!found) {
		// the rectangle [pmin,pend] is opaque, fill it
		out.push_back(pmin);
		out.push_back(aiVector2D(pmin.x,pmax.y));
		out.push_back(pmax);
		out.push_back(aiVector2D(pmax.x,pmin.y));
		return;
	}

	xs = std::max(pmin.x,xs);
	xe = std::min(pmax.x,xe);

	// see if there's an offset to fill at the top of our quad
	if (xs - pmin.x) {
		out.push_back(pmin);
		out.push_back(aiVector2D(pmin.x,pmax.y));
		out.push_back(aiVector2D(xs,pmax.y));
		out.push_back(aiVector2D(xs,pmin.y));
	}

	// search along the y-axis for all openings that overlap xs and our quad
	float ylast = pmin.y;
	found = false;
	for(; start != field.end(); ++start) {
		const BoundingBox& bb = bbs[(*start).second];
		if (bb.first.x > xs || bb.first.y >= pmax.y) {
			break;
		}

		if (bb.second.y > ylast) {

			found = true;
			const float ys = std::max(bb.first.y,pmin.y), ye = std::min(bb.second.y,pmax.y);
			if (ys - ylast) {
				QuadrifyPart( aiVector2D(xs,ylast), aiVector2D(xe,ys) ,field,bbs,out);
			}

			// the following are the window vertices

			/*wnd.push_back(aiVector2D(xs,ys));
			wnd.push_back(aiVector2D(xs,ye));
			wnd.push_back(aiVector2D(xe,ye));
			wnd.push_back(aiVector2D(xe,ys));*/
			ylast = ye;
		}
	}
	if (!found) {
		// the rectangle [pmin,pend] is opaque, fill it
		out.push_back(aiVector2D(xs,pmin.y));
		out.push_back(aiVector2D(xs,pmax.y));
		out.push_back(aiVector2D(xe,pmax.y));
		out.push_back(aiVector2D(xe,pmin.y));
		return;
	}
	if (ylast < pmax.y) {
		QuadrifyPart( aiVector2D(xs,ylast), aiVector2D(xe,pmax.y) ,field,bbs,out);
	}

	// now for the whole rest
	if (pmax.x-xe) {
		QuadrifyPart(aiVector2D(xe,pmin.y), pmax ,field,bbs,out);
	}
}

// ------------------------------------------------------------------------------------------------
enum Intersect {
	Intersect_No,
	Intersect_LiesOnPlane,
	Intersect_Yes
};

// ------------------------------------------------------------------------------------------------
Intersect IntersectSegmentPlane(const aiVector3D& p,const aiVector3D& n, const aiVector3D& e0, const aiVector3D& e1, aiVector3D& out) 
{
	const aiVector3D pdelta = e0 - p, seg = e1-e0;
	const float dotOne = n*seg, dotTwo = -(n*pdelta);

	if (fabs(dotOne) < 1e-6) {
		return fabs(dotTwo) < 1e-6f ? Intersect_LiesOnPlane : Intersect_No;
	}

	const float t = dotTwo/dotOne;
	// t must be in [0..1] if the intersection point is within the given segment
	if (t > 1.f || t < 0.f) {
		return Intersect_No;
	}
	out = e0+t*seg;
	return Intersect_Yes;
}



// ------------------------------------------------------------------------------------------------
aiVector3D Unproject(const aiVector2D& vproj, const  ProjectionInfo& proj)
{
	return vproj.x*proj.u + vproj.y*proj.v + proj.p;
}

// ------------------------------------------------------------------------------------------------
void InsertWindowContours(const std::vector< BoundingBox >& bbs,const std::vector< std::vector<aiVector2D> >& contours,const ProjectionInfo& proj, TempMesh& curmesh)
{
	ai_assert(contours.size() == bbs.size());

	// fix windows - we need to insert the real, polygonal shapes into the quadratic holes that we have now
	for(size_t i = 0; i < contours.size();++i) {
		const BoundingBox& bb = bbs[i];
		const std::vector<aiVector2D>& contour = contours[i];

		// check if we need to do it at all - many windows just fit perfectly into their quadratic holes,
		// i.e. their contours *are* already their bounding boxes.
		if (contour.size() == 4) {
			std::set<aiVector2D,XYSorter> verts;
			for(size_t n = 0; n < 4; ++n) {
				verts.insert(contour[n]);
			}
			const std::set<aiVector2D,XYSorter>::const_iterator end = verts.end();
			if (verts.find(bb.first)!=end && verts.find(bb.second)!=end
				&& verts.find(aiVector2D(bb.first.x,bb.second.y))!=end 
				&& verts.find(aiVector2D(bb.second.x,bb.first.y))!=end 
			) {
				continue;
			}
		}

		const float epsilon = (bb.first-bb.second).Length()/1000.f;

		// walk through all contour points and find those that lie on the BB corner
		size_t last_hit = -1, very_first_hit = -1;
		aiVector2D edge;
		for(size_t n = 0, e=0, size = contour.size();; n=(n+1)%size, ++e) {

			// sanity checking
			if (e == size*2) {
				IFCImporter::LogError("encountered unexpected topology while generating window contour");
				break;
			}

			const aiVector2D& v = contour[n];

			bool hit = false;
			if (fabs(v.x-bb.first.x)<epsilon) {
				edge.x = bb.first.x;
				hit = true;
			}
			else if (fabs(v.x-bb.second.x)<epsilon) {
				edge.x = bb.second.x;
				hit = true;
			}

			if (fabs(v.y-bb.first.y)<epsilon) {
				edge.y = bb.first.y;
				hit = true;
			}
			else if (fabs(v.y-bb.second.y)<epsilon) {
				edge.y = bb.second.y;
				hit = true;
			}

			if (hit) {
				if (last_hit != (size_t)-1) {

					const size_t old = curmesh.verts.size();
					size_t cnt = last_hit > n ? size-(last_hit-n) : n-last_hit;
					for(size_t a = last_hit, e = 0; e <= cnt; a=(a+1)%size, ++e) {
						curmesh.verts.push_back(Unproject(contour[a],proj));
					}
					
					if (edge != contour[last_hit] && edge != contour[n]) {
						curmesh.verts.push_back(Unproject(edge,proj));
					}
					else if (cnt == 1) {
						// avoid degenerate polygons (also known as lines or points)
						curmesh.verts.erase(curmesh.verts.begin()+old,curmesh.verts.end());
					}

					if (const size_t d = curmesh.verts.size()-old) {
						curmesh.vertcnt.push_back(d);
						std::reverse(curmesh.verts.rbegin(),curmesh.verts.rbegin()+d);
					}
					if (n == very_first_hit) {
						break;
					}
				}
				else {
					very_first_hit = n;
				}
				
				last_hit = n;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
bool TryAddOpenings_Quadrulate(const std::vector<TempOpening>& openings,const std::vector<aiVector3D>& nors, TempMesh& curmesh)
{
	std::vector<aiVector3D>& out = curmesh.verts;

	// Try to derive a solid base plane within the current surface for use as 
	// working coordinate system. 
	aiVector3D vmin,vmax;
	ArrayBounds(&out[0],out.size(),vmin,vmax);

	const size_t s = out.size();

	const aiVector3D any_point = out[s-4];
	const aiVector3D nor = ((out[s-3]-any_point)^(out[s-2]-any_point)).Normalize();

	const aiVector3D diag = vmax-vmin;
	const float ax = fabs(nor.x);    
	const float ay = fabs(nor.y);   
	const float az = fabs(nor.z);    

	unsigned int ac = 0, bc = 1; /* no z coord. -> projection to xy */
	if (ax > ay) {
		if (ax > az) { /* no x coord. -> projection to yz */
			ac = 1; bc = 2;
		}
	}
	else if (ay > az) { /* no y coord. -> projection to zy */
		ac = 2; bc = 0;
	}

	ProjectionInfo proj;
	proj.u = proj.v = diag;
	proj.u[bc]=0;
	proj.v[ac]=0;
	proj.ac = ac;
	proj.bc = bc;
	proj.p = vmin;

	// project all points into the coordinate system defined by the p+sv*tu plane
	// and compute bounding boxes for them
	std::vector< BoundingBox > bbs;
	XYSortedField field;

	std::vector<aiVector2D> contour_flat;
	contour_flat.reserve(out.size());
	BOOST_FOREACH(const aiVector3D& x, out) {
		contour_flat.push_back(ProjectPositionVectorOntoPlane(x,proj));
	}

	std::vector< std::vector<aiVector2D> > contours;

	size_t c = 0;
	BOOST_FOREACH(const TempOpening& t,openings) {
		const aiVector3D& outernor = nors[c++];
		const float dot = nor * outernor;
		if (fabs(dot)<1.f-1e-6f) {
			continue;
		}


		// const aiVector3D diff = t.extrusionDir;

		const std::vector<aiVector3D>& va = t.profileMesh->verts;
		if(va.size() <= 2) {
			continue;	
		}

		aiVector2D vpmin,vpmax;
		MinMaxChooser<aiVector2D>()(vpmin,vpmax);

		contours.push_back(std::vector<aiVector2D>());
		std::vector<aiVector2D>& contour = contours.back();

		BOOST_FOREACH(const aiVector3D& x, t.profileMesh->verts) {
			const aiVector2D& vproj = ProjectPositionVectorOntoPlane(x,proj);

			vpmin = std::min(vpmin,vproj);
			vpmax = std::max(vpmax,vproj);

			contour.push_back(vproj);
		}

		
		if (field.find(vpmin) != field.end()) {
			IFCImporter::LogWarn("constraint failure during generation of wall openings, results may be faulty");
		}
		field[vpmin] = bbs.size();
		bbs.push_back(BoundingBox(vpmin,vpmax));
	}

	if (bbs.empty()) {
		return false;
	}


	std::vector<aiVector2D> outflat;
	outflat.reserve(openings.size()*4);
	QuadrifyPart(aiVector2D(0.f,0.f),aiVector2D(1.f,1.f),field,bbs,outflat);
	ai_assert(!(outflat.size() % 4));

	//FixOuterBoundaries(outflat,contour_flat);

	// undo the projection, generate output quads
	std::vector<aiVector3D> vold;
	vold.reserve(outflat.size());
	std::swap(vold,curmesh.verts);

	std::vector<unsigned int> iold;
	iold.resize(outflat.size()/4,4);
	std::swap(iold,curmesh.vertcnt);

	BOOST_FOREACH(const aiVector2D& vproj, outflat) {
		out.push_back(Unproject(vproj,proj));
	}

	InsertWindowContours(bbs,contours,proj,curmesh);
	return true;
}


// ------------------------------------------------------------------------------------------------
void ProcessExtrudedAreaSolid(const IfcExtrudedAreaSolid& solid, TempMesh& result, ConversionData& conv)
{
	TempMesh meshout;
	
	// first read the profile description
	if(!ProcessProfile(*solid.SweptArea,meshout,conv) || meshout.verts.size()<=1) {
		return;
	}

	aiVector3D dir;
	ConvertDirection(dir,solid.ExtrudedDirection);

	dir *= solid.Depth;

	// assuming that `meshout.verts` is now a list of vertex points forming 
	// the underlying profile, extrude along the given axis, forming new
	// triangles.
	
	std::vector<aiVector3D>& in = meshout.verts;
	const size_t size=in.size();

	const bool has_area = solid.SweptArea->ProfileType == "AREA" && size>2;
	if(solid.Depth < 1e-3) {
		if(has_area) {
			meshout = result;
		}
		return;
	}

	result.verts.reserve(size*(has_area?4:2));
	result.vertcnt.reserve(meshout.vertcnt.size()+2);

	// transform to target space
	aiMatrix4x4 trafo;
	ConvertAxisPlacement(trafo, solid.Position);
	BOOST_FOREACH(aiVector3D& v,in) {
		v *= trafo;
	}

	
	aiVector3D min = in[0];
	dir *= aiMatrix3x3(trafo);

	std::vector<aiVector3D> nors;
	
	// compute the normal vectors for all opening polygons
	if (conv.apply_openings) {
		if (!conv.settings.useCustomTriangulation) {
			// it is essential to apply the openings in the correct spatial order. The direction
			// doesn't matter, but we would screw up if we started with e.g. a door in between
			// two windows.
			std::sort(conv.apply_openings->begin(),conv.apply_openings->end(),DistanceSorter(min));
		}

		nors.reserve(conv.apply_openings->size());
		BOOST_FOREACH(TempOpening& t,*conv.apply_openings) {
			TempMesh& bounds = *t.profileMesh.get();
		
			if (bounds.verts.size() <= 2) {
				nors.push_back(aiVector3D());
				continue;
			}
			nors.push_back(((bounds.verts[2]-bounds.verts[0])^(bounds.verts[1]-bounds.verts[0]) ).Normalize());
		}
	}

	TempMesh temp;
	TempMesh& curmesh = conv.apply_openings ? temp : result;
	std::vector<aiVector3D>& out = curmesh.verts;

	bool (* const gen_openings)(const std::vector<TempOpening>&,const std::vector<aiVector3D>&, TempMesh&) = conv.settings.useCustomTriangulation 
		? &TryAddOpenings_Quadrulate 
		: &TryAddOpenings;
 
	size_t sides_with_openings = 0;
	for(size_t i = 0; i < size; ++i) {
		const size_t next = (i+1)%size;

		curmesh.vertcnt.push_back(4);
		
		out.push_back(in[i]);
		out.push_back(in[i]+dir);
		out.push_back(in[next]+dir);
		out.push_back(in[next]);

		if(conv.apply_openings) {
			if(gen_openings(*conv.apply_openings,nors,temp)) {
				++sides_with_openings;
			}
			
			result.Append(temp);
			temp.Clear();
		}
	}
	
	size_t sides_with_v_openings = 0;
	if(has_area) {

		for(size_t n = 0; n < 2; ++n) {
			for(size_t i = size; i--; ) {
				out.push_back(in[i]+(n?dir:aiVector3D()));
			}

			curmesh.vertcnt.push_back(size);
			if(conv.apply_openings && size > 2) {
				// XXX here we are forced to use the un-triangulated version of TryAddOpening, with
				// all the problems it causes. The reason is that vertical walls (ehm, floors)
				// can have an arbitrary outer shape, so the usual approach of projecting
				// the surface and all openings onto a flat quad and triangulating the quad 
				// fails.
				if(TryAddOpenings(*conv.apply_openings,nors,temp)) {
					++sides_with_v_openings;
				}

				result.Append(temp);
				temp.Clear();
			}
		}
	}

	// add connection geometry to close the 'holes' for the openings
	if(conv.apply_openings) {
		//result.infacing.resize(result.verts.size()+);
		BOOST_FOREACH(const TempOpening& t,*conv.apply_openings) {
			const std::vector<aiVector3D>& in = t.profileMesh->verts;
			std::vector<aiVector3D>& out = result.verts; 

			const aiVector3D dir = t.extrusionDir;
			for(size_t i = 0, size = in.size(); i < size; ++i) {
				const size_t next = (i+1)%size;

				result.vertcnt.push_back(4);

				out.push_back(in[i]);
				out.push_back(in[i]+dir);
				out.push_back(in[next]+dir);
				out.push_back(in[next]);
			}
		}
	}

	if(conv.apply_openings && ((sides_with_openings != 2 && sides_with_openings) || (sides_with_v_openings != 2 && sides_with_v_openings))) {
		IFCImporter::LogWarn("failed to resolve all openings, presumably their topology is not supported by Assimp");
	}

	IFCImporter::LogDebug("generate mesh procedurally by extrusion (IfcExtrudedAreaSolid)");
}



// ------------------------------------------------------------------------------------------------
void ProcessSweptAreaSolid(const IfcSweptAreaSolid& swept, TempMesh& meshout, ConversionData& conv)
{
	if(const IfcExtrudedAreaSolid* const solid = swept.ToPtr<IfcExtrudedAreaSolid>()) {
		// Do we just collect openings for a parent element (i.e. a wall)? 
		// In this case we don't extrude the surface yet, just keep the profile and transform it correctly
		if(conv.collect_openings) {
			boost::shared_ptr<TempMesh> meshtmp(new TempMesh());
			ProcessProfile(swept.SweptArea,*meshtmp,conv);

			aiMatrix4x4 m;
			ConvertAxisPlacement(m,solid->Position);
			meshtmp->Transform(m);

			aiVector3D dir;
			ConvertDirection(dir,solid->ExtrudedDirection);
			conv.collect_openings->push_back(TempOpening(solid, aiMatrix3x3(m) * (dir*solid->Depth),meshtmp));
			return;
		}

		ProcessExtrudedAreaSolid(*solid,meshout,conv);
	}
	else if(const IfcRevolvedAreaSolid* const rev = swept.ToPtr<IfcRevolvedAreaSolid>()) {
		ProcessRevolvedAreaSolid(*rev,meshout,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcSweptAreaSolid entity, type is " + swept.GetClassName());
	}
}

// ------------------------------------------------------------------------------------------------
void ProcessBoolean(const IfcBooleanResult& boolean, TempMesh& result, ConversionData& conv)
{
	if(const IfcBooleanResult* const clip = boolean.ToPtr<IfcBooleanResult>()) {
		if(clip->Operator != "DIFFERENCE") {
			IFCImporter::LogWarn("encountered unsupported boolean operator: " + (std::string)clip->Operator);
			return;
		}

		TempMesh meshout;
		const IfcHalfSpaceSolid* const hs = clip->SecondOperand->ResolveSelectPtr<IfcHalfSpaceSolid>(conv.db);
		if(!hs) {
			IFCImporter::LogError("expected IfcHalfSpaceSolid as second clipping operand");
			return;
		}

		const IfcPlane* const plane = hs->BaseSurface->ToPtr<IfcPlane>();
		if(!plane) {
			IFCImporter::LogError("expected IfcPlane as base surface for the IfcHalfSpaceSolid");
			return;
		}

		if(const IfcBooleanResult* const op0 = clip->FirstOperand->ResolveSelectPtr<IfcBooleanResult>(conv.db)) {
			ProcessBoolean(*op0,meshout,conv);
		}
		else if (const IfcSweptAreaSolid* const swept = clip->FirstOperand->ResolveSelectPtr<IfcSweptAreaSolid>(conv.db)) {
			ProcessSweptAreaSolid(*swept,meshout,conv);
		}
		else {
			IFCImporter::LogError("expected IfcSweptAreaSolid or IfcBooleanResult as first clipping operand");
			return;
		}

		// extract plane base position vector and normal vector
		aiVector3D p,n(0.f,0.f,1.f);
		if (plane->Position->Axis) {
			ConvertDirection(n,plane->Position->Axis.Get());
		}
		ConvertCartesianPoint(p,plane->Position->Location);

		if(!IsTrue(hs->AgreementFlag)) {
			n *= -1.f;
		}

		// clip the current contents of `meshout` against the plane we obtained from the second operand
		const std::vector<aiVector3D>& in = meshout.verts;
		std::vector<aiVector3D>& outvert = result.verts;
		std::vector<unsigned int>::const_iterator begin=meshout.vertcnt.begin(), end=meshout.vertcnt.end(), iit;

		outvert.reserve(in.size());
		result.vertcnt.reserve(meshout.vertcnt.size());

		unsigned int vidx = 0;
		for(iit = begin; iit != end; vidx += *iit++) {

			unsigned int newcount = 0;
			for(unsigned int i = 0; i < *iit; ++i) {
				const aiVector3D& e0 = in[vidx+i], e1 = in[vidx+(i+1)%*iit];

				// does the next segment intersect the plane?
				aiVector3D isectpos;
				const Intersect isect = IntersectSegmentPlane(p,n,e0,e1,isectpos);
				if (isect == Intersect_No || isect == Intersect_LiesOnPlane) {
					if ( (e0-p).Normalize()*n > 0 ) {
						outvert.push_back(e0);
						++newcount;
					}
				}
				else if (isect == Intersect_Yes) {
					if ( (e0-p).Normalize()*n > 0 ) {
						// e0 is on the right side, so keep it 
						outvert.push_back(e0);
						outvert.push_back(isectpos);
						newcount += 2;
					}
					else {
						// e0 is on the wrong side, so drop it and keep e1 instead
						outvert.push_back(isectpos);
						++newcount;
					}
				}
			}	

			if (!newcount) {
				continue;
			}

			aiVector3D vmin,vmax;
			ArrayBounds(&*(outvert.end()-newcount),newcount,vmin,vmax);

			// filter our double points - those may happen if a point lies
			// directly on the intersection line. However, due to float
			// precision a bitwise comparison is not feasible to detect
			// this case.
			const float epsilon = (vmax-vmin).SquareLength() / 1e6f;
			FuzzyVectorCompare fz(epsilon);

			std::vector<aiVector3D>::iterator e = std::unique( outvert.end()-newcount, outvert.end(), fz );
			if (e != outvert.end()) {
				newcount -= static_cast<unsigned int>(std::distance(e,outvert.end()));
				outvert.erase(e,outvert.end());
			}
			if (fz(*( outvert.end()-newcount),outvert.back())) {
				outvert.pop_back();
				--newcount;
			}
			if(newcount > 2) {
				result.vertcnt.push_back(newcount);
			}
			else while(newcount-->0)result.verts.pop_back();

		}
		IFCImporter::LogDebug("generating CSG geometry by plane clipping (IfcBooleanClippingResult)");
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcBooleanResult entity, type is " + boolean.GetClassName());
	}
}



// ------------------------------------------------------------------------------------------------
bool ProcessGeometricItem(const IfcRepresentationItem& geo, std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	TempMesh meshtmp;
	if(const IfcShellBasedSurfaceModel* shellmod = geo.ToPtr<IfcShellBasedSurfaceModel>()) {
		BOOST_FOREACH(boost::shared_ptr<const IfcShell> shell,shellmod->SbsmBoundary) {
			try {
				const EXPRESS::ENTITY& e = shell->To<ENTITY>();
				const IfcConnectedFaceSet& fs = conv.db.MustGetObject(e).To<IfcConnectedFaceSet>(); 

				ProcessConnectedFaceSet(fs,meshtmp,conv);
			}
			catch(std::bad_cast&) {
				IFCImporter::LogWarn("unexpected type error, IfcShell ought to inherit from IfcConnectedFaceSet");
			}
		}
	}
	else if(const IfcConnectedFaceSet* fset = geo.ToPtr<IfcConnectedFaceSet>()) {
		ProcessConnectedFaceSet(*fset,meshtmp,conv);
	}
	else if(const IfcSweptAreaSolid* swept = geo.ToPtr<IfcSweptAreaSolid>()) {
		ProcessSweptAreaSolid(*swept,meshtmp,conv);
	}
	else if(const IfcManifoldSolidBrep* brep = geo.ToPtr<IfcManifoldSolidBrep>()) {
		ProcessConnectedFaceSet(brep->Outer,meshtmp,conv);
	}
	else if(const IfcFaceBasedSurfaceModel* surf = geo.ToPtr<IfcFaceBasedSurfaceModel>()) {
		BOOST_FOREACH(const IfcConnectedFaceSet& fc, surf->FbsmFaces) {
			ProcessConnectedFaceSet(fc,meshtmp,conv);
		}
	}
	else if(const IfcBooleanResult* boolean = geo.ToPtr<IfcBooleanResult>()) {
		ProcessBoolean(*boolean,meshtmp,conv);
	}
	else if(geo.ToPtr<IfcBoundingBox>()) {
		// silently skip over bounding boxes
		return false; 
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcGeometricRepresentationItem entity, type is " + geo.GetClassName());
		return false;
	}

	meshtmp.RemoveAdjacentDuplicates();
	FixupFaceOrientation(meshtmp);

	aiMesh* const mesh = meshtmp.ToMesh();
	if(mesh) {
		mesh->mMaterialIndex = ProcessMaterials(geo,conv);
		mesh_indices.push_back(conv.meshes.size());
		conv.meshes.push_back(mesh);
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void AssignAddedMeshes(std::vector<unsigned int>& mesh_indices,aiNode* nd,ConversionData& /*conv*/)
{
	if (!mesh_indices.empty()) {

		// make unique
		std::sort(mesh_indices.begin(),mesh_indices.end());
		std::vector<unsigned int>::iterator it_end = std::unique(mesh_indices.begin(),mesh_indices.end());

		const size_t size = std::distance(mesh_indices.begin(),it_end);

		nd->mNumMeshes = size;
		nd->mMeshes = new unsigned int[nd->mNumMeshes];
		for(unsigned int i = 0; i < nd->mNumMeshes; ++i) {
			nd->mMeshes[i] = mesh_indices[i];
		}
	}
}

// ------------------------------------------------------------------------------------------------
bool TryQueryMeshCache(const IfcRepresentationItem& item, std::vector<unsigned int>& mesh_indices, ConversionData& conv) 
{
	ConversionData::MeshCache::const_iterator it = conv.cached_meshes.find(&item);
	if (it != conv.cached_meshes.end()) {
		std::copy((*it).second.begin(),(*it).second.end(),std::back_inserter(mesh_indices));
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void PopulateMeshCache(const IfcRepresentationItem& item, const std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	conv.cached_meshes[&item] = mesh_indices;
}

// ------------------------------------------------------------------------------------------------
bool ProcessRepresentationItem(const IfcRepresentationItem& item, std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	if (!TryQueryMeshCache(item,mesh_indices,conv)) {
		if(ProcessGeometricItem(item,mesh_indices,conv)) {
			if(mesh_indices.size()) {
				PopulateMeshCache(item,mesh_indices,conv);
			}
		}
		else return false;
	}
	return true;
}

} // ! IFC
} // ! Assimp

#endif 
