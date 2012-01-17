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

#include "../contrib/poly2tri/poly2tri/poly2tri.h"
#include "../contrib/clipper/clipper.hpp"

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
aiMatrix3x3 DerivePlaneCoordinateSpace(const TempMesh& curmesh) {

	const std::vector<aiVector3D>& out = curmesh.verts;
	aiMatrix3x3 m;

	const size_t s = out.size();
	assert(curmesh.vertcnt.size() == 1 && curmesh.vertcnt.back() == s);

	const aiVector3D any_point = out[s-1];
	aiVector3D nor; 

	// The input polygon is arbitrarily shaped, so we might need some tries
	// until we find a suitable normal (and it does not even need to be
	// right in all cases, Newell's algorithm would be the correct one ... ).
	size_t base = s-curmesh.vertcnt.back(), t = base, i, j;
	for (i = base; i < s-1; ++i) {
		for (j = i+1; j < s; ++j) {
			nor = ((out[i]-any_point)^(out[j]-any_point));
			if(fabs(nor.Length()) > 1e-8f) {
				goto out;
			}
		}
	}

	assert(0);

out:

	nor.Normalize();

	aiVector3D r = (out[i]-any_point);
	r.Normalize();

	// reconstruct orthonormal basis
	aiVector3D u = r ^ nor;
	u.Normalize();

	m.a1 = r.x;
	m.a2 = r.y;
	m.a3 = r.z;

	m.b1 = u.x;
	m.b2 = u.y;
	m.b3 = u.z;

	m.c1 = nor.x;
	m.c2 = nor.y;
	m.c3 = nor.z;

	return m;
}

// ------------------------------------------------------------------------------------------------
bool TryAddOpenings_Poly2Tri(const std::vector<TempOpening>& openings,const std::vector<aiVector3D>& nors, TempMesh& curmesh)
{
	std::vector<aiVector3D>& out = curmesh.verts;

	bool result = false;

	// Try to derive a solid base plane within the current surface for use as 
	// working coordinate system. 
	const aiMatrix3x3& m = DerivePlaneCoordinateSpace(curmesh);
	const aiMatrix3x3 minv = aiMatrix3x3(m).Inverse();
	const aiVector3D& nor = aiVector3D(m.c1, m.c2, m.c3);

	float coord = -1;

	std::vector<aiVector2D> contour_flat;
	contour_flat.reserve(out.size());

	aiVector2D vmin, vmax;
	MinMaxChooser<aiVector2D>()(vmin, vmax);
	
	// Move all points into the new coordinate system, collecting min/max verts on the way
	BOOST_FOREACH(aiVector3D& x, out) {
		const aiVector3D vv = m * x;

		// keep Z offset in the plane coordinate system. Ignoring precision issues
		// (which  are present, of course), this should be the same value for
		// all polygon vertices (assuming the polygon is planar).


		// XXX this should be guarded, but we somehow need to pick a suitable
		// epsilon
		// if(coord != -1.0f) {
		//	assert(fabs(coord - vv.z) < 1e-3f);
		// }

		coord = vv.z;

		vmin = std::min(aiVector2D(vv.x, vv.y), vmin);
		vmax = std::max(aiVector2D(vv.x, vv.y), vmax);

		contour_flat.push_back(aiVector2D(vv.x,vv.y));
	}
		
	// With the current code in DerivePlaneCoordinateSpace, 
	// vmin,vmax should always be the 0...1 rectangle (+- numeric inaccuracies) 
	// but here we won't rely on this.

	vmax -= vmin;

	// If this happens then the projection must have been wrong.
	assert(vmax.Length());

	using ClipperLib::ulong64;
	// XXX use full -+ range ...
	const ClipperLib::long64 max_ulong64 = 1518500249; // clipper.cpp / hiRange var

//#define to_int64(p)  (static_cast<ulong64>( std::max( 0., std::min( static_cast<double>((p)), 1.) ) * max_ulong64 ))
#define to_int64(p)  (static_cast<ulong64>(static_cast<double>((p) ) * max_ulong64 ))
#define from_int64(p) (static_cast<double>((p)) / max_ulong64)
#define from_int64_f(p) (static_cast<float>(from_int64((p))))


	ClipperLib::ExPolygons clipped;
	ClipperLib::Polygons holes_union;


	aiVector3D wall_extrusion;
	bool do_connections = false, first = true;

	try {

		ClipperLib::Clipper clipper_holes;
		size_t c = 0;

		BOOST_FOREACH(const TempOpening& t,openings) {
			const aiVector3D& outernor = nors[c++];
			const float dot = nor * outernor;
			if (fabs(dot)<1.f-1e-6f) {
				continue;
			}

			const std::vector<aiVector3D>& va = t.profileMesh->verts;
			if(va.size() <= 2) {
				continue;	
			}
		
			std::vector<aiVector2D> contour;

			BOOST_FOREACH(const aiVector3D& xx, t.profileMesh->verts) {
				aiVector3D vv = m *  xx, vv_extr = m * (xx + t.extrusionDir);
				
				const bool is_extruded_side = fabs(vv.z - coord) > fabs(vv_extr.z - coord);
				if (first) {
					first = false;
					if (dot > 0.f) {
						do_connections = true;
						wall_extrusion = t.extrusionDir;
						if (is_extruded_side) {
							wall_extrusion = - wall_extrusion;
						}
					}
				}

				// XXX should not be necessary - but it is. Why? For precision reasons?
				vv = is_extruded_side ? vv_extr : vv;
				contour.push_back(aiVector2D(vv.x,vv.y));
			}

			ClipperLib::Polygon hole;
			BOOST_FOREACH(aiVector2D& pip, contour) {
				pip.x  = (pip.x - vmin.x) / vmax.x;
				pip.y  = (pip.y - vmin.y) / vmax.y;

				hole.push_back(ClipperLib::IntPoint(  to_int64(pip.x), to_int64(pip.y) ));
			}

			if (!ClipperLib::Orientation(hole)) {
				std::reverse(hole.begin(), hole.end());
			//	assert(ClipperLib::Orientation(hole));
			}

			clipper_holes.AddPolygon(hole,ClipperLib::ptSubject);
		}

		clipper_holes.Execute(ClipperLib::ctUnion,holes_union,
			ClipperLib::pftNonZero,
			ClipperLib::pftNonZero);

		if (holes_union.empty()) {
			return false;
		}

		// Now that we have the big union of all holes, subtract it from the outer contour
		// to obtain the final polygon to feed into the triangulator.
		{
			ClipperLib::Polygon poly;
			BOOST_FOREACH(aiVector2D& pip, contour_flat) {
				pip.x  = (pip.x - vmin.x) / vmax.x;
				pip.y  = (pip.y - vmin.y) / vmax.y;

				poly.push_back(ClipperLib::IntPoint( to_int64(pip.x), to_int64(pip.y) ));
			}

			if (ClipperLib::Orientation(poly)) {
				std::reverse(poly.begin(), poly.end());
			}
			clipper_holes.Clear();
			clipper_holes.AddPolygon(poly,ClipperLib::ptSubject);

			clipper_holes.AddPolygons(holes_union,ClipperLib::ptClip);
			clipper_holes.Execute(ClipperLib::ctDifference,clipped,
				ClipperLib::pftNonZero,
				ClipperLib::pftNonZero);
		}

	}
	catch (const char* sx) {
		IFCImporter::LogError("Ifc: error during polygon clipping, skipping openings for this face: (Clipper: " 
			+ std::string(sx) + ")");

		return false;
	}

	curmesh.verts.clear();
	curmesh.vertcnt.clear();


	// add connection geometry to close the adjacent 'holes' for the openings
	// this should only be done from one side of the wall or the polygons 
	// would be emitted twice.
	if (do_connections) {

		std::vector<aiVector3D> tmpvec;
		BOOST_FOREACH(ClipperLib::Polygon& opening, holes_union) {

			assert(ClipperLib::Orientation(opening));

			tmpvec.clear();

			BOOST_FOREACH(ClipperLib::IntPoint& point, opening) {

				tmpvec.push_back( minv * aiVector3D(
					vmin.x + from_int64_f(point.X) * vmax.x, 
					vmin.y + from_int64_f(point.Y) * vmax.y,
					coord));
			}

			for(size_t i = 0, size = tmpvec.size(); i < size; ++i) {
				const size_t next = (i+1)%size;

				curmesh.vertcnt.push_back(4);

				const aiVector3D& in_world = tmpvec[i];
				const aiVector3D& next_world = tmpvec[next];

				// Assumptions: no 'partial' openings, wall thickness roughly the same across the wall
				curmesh.verts.push_back(in_world);
				curmesh.verts.push_back(in_world+wall_extrusion);
				curmesh.verts.push_back(next_world+wall_extrusion);
				curmesh.verts.push_back(next_world);
			}
		}
	}
	
	std::vector< std::vector<p2t::Point*> > contours;
	BOOST_FOREACH(ClipperLib::ExPolygon& clip, clipped) {
		
		contours.clear();

		// Build the outer polygon contour line for feeding into poly2tri
		std::vector<p2t::Point*> contour_points;
		BOOST_FOREACH(ClipperLib::IntPoint& point, clip.outer) {
			contour_points.push_back( new p2t::Point(from_int64(point.X), from_int64(point.Y)) );
		}

		p2t::CDT* cdt ;
		try {
			// Note: this relies on custom modifications in poly2tri to raise runtime_error's
			// instead if assertions. These failures are not debug only, they can actually
			// happen in production use if the input data is broken. An assertion would be
			// inappropriate.
			cdt = new p2t::CDT(contour_points);
		}
		catch(const std::exception& e) {
			IFCImporter::LogError("Ifc: error during polygon triangulation, skipping some openings: (poly2tri: " 
				+ std::string(e.what()) + ")");
			continue;
		}
		

		// Build the poly2tri inner contours for all holes we got from ClipperLib
		BOOST_FOREACH(ClipperLib::Polygon& opening, clip.holes) {
			
			contours.push_back(std::vector<p2t::Point*>());
			std::vector<p2t::Point*>& contour = contours.back();

			BOOST_FOREACH(ClipperLib::IntPoint& point, opening) {
				contour.push_back( new p2t::Point(from_int64(point.X), from_int64(point.Y)) );
			}

			cdt->AddHole(contour);
		}
		
		try {
			// Note: See above
			cdt->Triangulate();
		}
		catch(const std::exception& e) {
			IFCImporter::LogError("Ifc: error during polygon triangulation, skipping some openings: (poly2tri: " 
				+ std::string(e.what()) + ")");
			continue;
		}

		const std::vector<p2t::Triangle*>& tris = cdt->GetTriangles();

		// Collect the triangles we just produced
		BOOST_FOREACH(p2t::Triangle* tri, tris) {
			for(int i = 0; i < 3; ++i) {

				const aiVector2D& v = aiVector2D( 
					static_cast<float>( tri->GetPoint(i)->x ), 
					static_cast<float>( tri->GetPoint(i)->y )
				);

				assert(v.x <= 1.0 && v.x >= 0.0 && v.y <= 1.0 && v.y >= 0.0);
				const aiVector3D v3 = minv * aiVector3D(vmin.x + v.x * vmax.x, vmin.y + v.y * vmax.y,coord) ; 

				curmesh.verts.push_back(v3);
			}
			curmesh.vertcnt.push_back(3);
		}

		result = true;
	}

#undef to_int64
#undef from_int64
#undef from_int64_f

	return result;
}


// ------------------------------------------------------------------------------------------------
void ProcessExtrudedAreaSolid(const IfcExtrudedAreaSolid& solid, TempMesh& result, ConversionData& conv)
{
	TempMesh meshout;
	
	// First read the profile description
	if(!ProcessProfile(*solid.SweptArea,meshout,conv) || meshout.verts.size()<=1) {
		return;
	}

	aiVector3D dir;
	ConvertDirection(dir,solid.ExtrudedDirection);

	dir *= solid.Depth;

	// Outline: assuming that `meshout.verts` is now a list of vertex points forming 
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

	// First step: transform all vertices into the target coordinate space
	aiMatrix4x4 trafo;
	ConvertAxisPlacement(trafo, solid.Position);
	BOOST_FOREACH(aiVector3D& v,in) {
		v *= trafo;
	}
	
	aiVector3D min = in[0];
	dir *= aiMatrix3x3(trafo);

	std::vector<aiVector3D> nors;
	const bool openings = !!conv.apply_openings && conv.apply_openings->size();
	
	// Compute the normal vectors for all opening polygons as a prerequisite
	// to TryAddOpenings_Poly2Tri()
	if (openings) {
	
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
	TempMesh& curmesh = openings ? temp : result;
	std::vector<aiVector3D>& out = curmesh.verts;
 
	size_t sides_with_openings = 0;
	for(size_t i = 0; i < size; ++i) {
		const size_t next = (i+1)%size;

		curmesh.vertcnt.push_back(4);
		
		out.push_back(in[i]);
		out.push_back(in[i]+dir);
		out.push_back(in[next]+dir);
		out.push_back(in[next]);

		if(openings) {
			if(TryAddOpenings_Poly2Tri(*conv.apply_openings,nors,temp)) {
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
			if(openings && size > 2) {
				if(TryAddOpenings_Poly2Tri(*conv.apply_openings,nors,temp)) {
					++sides_with_v_openings;
				}

				result.Append(temp);
				temp.Clear();
			}
		}
	}


	if(openings && ((sides_with_openings != 2 && sides_with_openings) || (sides_with_v_openings != 2 && sides_with_v_openings))) {
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
