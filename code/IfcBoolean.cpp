/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2010, assimp team
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

/** @file  IFCBoolean.cpp
 *  @brief Implements a subset of Ifc boolean operations
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
enum Intersect {
	Intersect_No,
	Intersect_LiesOnPlane,
	Intersect_Yes
};

// ------------------------------------------------------------------------------------------------
Intersect IntersectSegmentPlane(const IfcVector3& p,const IfcVector3& n, const IfcVector3& e0, 
								const IfcVector3& e1, 
								IfcVector3& out) 
{
	const IfcVector3 pdelta = e0 - p, seg = e1-e0;
	const IfcFloat dotOne = n*seg, dotTwo = -(n*pdelta);

	if (fabs(dotOne) < 1e-6) {
		return fabs(dotTwo) < 1e-6f ? Intersect_LiesOnPlane : Intersect_No;
	}

	const IfcFloat t = dotTwo/dotOne;
	// t must be in [0..1] if the intersection point is within the given segment
	if (t > 1.f || t < 0.f) {
		return Intersect_No;
	}
	out = e0+t*seg;
	return Intersect_Yes;
}

// ------------------------------------------------------------------------------------------------
void ProcessBooleanHalfSpaceDifference(const IfcHalfSpaceSolid* hs, TempMesh& result, 
									   const TempMesh& first_operand, 
									   ConversionData& conv)
{
	ai_assert(hs != NULL);

	const IfcPlane* const plane = hs->BaseSurface->ToPtr<IfcPlane>();
	if(!plane) {
		IFCImporter::LogError("expected IfcPlane as base surface for the IfcHalfSpaceSolid");
		return;
	}

	// extract plane base position vector and normal vector
	IfcVector3 p,n(0.f,0.f,1.f);
	if (plane->Position->Axis) {
		ConvertDirection(n,plane->Position->Axis.Get());
	}
	ConvertCartesianPoint(p,plane->Position->Location);

	if(!IsTrue(hs->AgreementFlag)) {
		n *= -1.f;
	}

	// clip the current contents of `meshout` against the plane we obtained from the second operand
	const std::vector<IfcVector3>& in = first_operand.verts;
	std::vector<IfcVector3>& outvert = result.verts;

	std::vector<unsigned int>::const_iterator begin = first_operand.vertcnt.begin(), 
		end = first_operand.vertcnt.end(), iit;

	outvert.reserve(in.size());
	result.vertcnt.reserve(first_operand.vertcnt.size());

	unsigned int vidx = 0;
	for(iit = begin; iit != end; vidx += *iit++) {

		unsigned int newcount = 0;
		for(unsigned int i = 0; i < *iit; ++i) {
			const IfcVector3& e0 = in[vidx+i], e1 = in[vidx+(i+1)%*iit];

			// does the next segment intersect the plane?
			IfcVector3 isectpos;
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

		IfcVector3 vmin,vmax;
		ArrayBounds(&*(outvert.end()-newcount),newcount,vmin,vmax);

		// filter our IfcFloat points - those may happen if a point lies
		// directly on the intersection line. However, due to IfcFloat
		// precision a bitwise comparison is not feasible to detect
		// this case.
		const IfcFloat epsilon = (vmax-vmin).SquareLength() / 1e6f;
		FuzzyVectorCompare fz(epsilon);

		std::vector<IfcVector3>::iterator e = std::unique( outvert.end()-newcount, outvert.end(), fz );

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
		else while(newcount-->0) {
			result.verts.pop_back();
		}

	}
	IFCImporter::LogDebug("generating CSG geometry by plane clipping (IfcBooleanClippingResult)");
}

// ------------------------------------------------------------------------------------------------
// Check if e0-e1 intersects a sub-segment of the given boundary line.
// note: this method works on 3D vectors, but performs its intersection checks solely in xy.
bool IntersectsBoundaryProfile( const IfcVector3& e0, const IfcVector3& e1, const std::vector<IfcVector3>& boundary,
	std::vector<size_t>& intersected_boundary_segments,
	std::vector<IfcVector3>& intersected_boundary_points,
	bool half_open = false)
{
	ai_assert(intersected_boundary_segments.empty());
	ai_assert(intersected_boundary_points.empty());

	const IfcVector3& e = e1 - e0;

	for (size_t i = 0, bcount = boundary.size(); i < bcount; ++i) {
		// boundary segment i: b0-b1
		const IfcVector3& b0 = boundary[i];
		const IfcVector3& b1 = boundary[(i+1) % bcount];

		const IfcVector3& b = b1 - b0;

		// segment-segment intersection
		// solve b0 + b*s = e0 + e*s for (s,t)
		const IfcFloat det = (-b.x * e.y + e.x * b.y);
		if(fabs(det) < 1e-6) {
			// no solutions (parallel lines)
			continue;
		}

		const IfcFloat x = b0.x - e0.x;
		const IfcFloat y = b0.y - e0.y;

		const IfcFloat s = (x*e.y - e.x*y)/det;
		const IfcFloat t = (x*b.y - b.x*y)/det;

#ifdef _DEBUG
		const IfcVector3 check = b0 + b*s  - (e0 + e*t);
		ai_assert((IfcVector2(check.x,check.y)).SquareLength() < 1e-5);
#endif

		// for a valid intersection, s-t should be in range [0,1]
		if (s >= 0.0 && (s <= 1.0 || half_open) && t >= 0.0 && t <= 1.0) {
	
			const IfcVector3& p = b0 + b*s;

			// only insert the point into the list if it is sufficiently
			// far away from the previous intersection point. This way,
			// we avoid duplicate detection if the intersection is
			// directly on the vertex between two segments.
			if (!intersected_boundary_points.empty() && intersected_boundary_segments.back()==(i==0?bcount-1:i-1) ) {
				if((intersected_boundary_points.back() - p).SquareLength() < 1e-5) {
					continue;
				}
			}
			intersected_boundary_segments.push_back(i);
			intersected_boundary_points.push_back(p);
		}
	}

	return false;
}


// ------------------------------------------------------------------------------------------------
bool PointInPoly(const IfcVector3& p, const std::vector<IfcVector3>& boundary)
{
	// even-odd algorithm: take a random vector that extends from p to infinite
	// and counts how many times it intersects edges of the boundary.
	// because checking for segment intersections is prone to numeric inaccuracies
	// or double detections (i.e. when hitting multiple adjacent segments at their
	// shared vertices) we do it thrice with different rays and vote on it.

	std::vector<size_t> intersected_boundary_segments;
	std::vector<IfcVector3> intersected_boundary_points;
	size_t votes = 0;

	IntersectsBoundaryProfile(p, p + IfcVector3(1.0,0,0), boundary, 
		intersected_boundary_segments, 
		intersected_boundary_points, true);

	votes += intersected_boundary_segments.size() % 2;

	intersected_boundary_segments.clear();
	intersected_boundary_points.clear();

	IntersectsBoundaryProfile(p, p + IfcVector3(0,1.0,0), boundary, 
		intersected_boundary_segments, 
		intersected_boundary_points, true);

	votes += intersected_boundary_segments.size() % 2;

	intersected_boundary_segments.clear();
	intersected_boundary_points.clear();

	IntersectsBoundaryProfile(p, p + IfcVector3(0,0,1.0), boundary, 
		intersected_boundary_segments, 
		intersected_boundary_points, true);

	votes += intersected_boundary_segments.size() % 2;
	return votes > 1;
}


// ------------------------------------------------------------------------------------------------
void ProcessPolygonalBoundedBooleanHalfSpaceDifference(const IfcPolygonalBoundedHalfSpace* hs, TempMesh& result, 
													   const TempMesh& first_operand, 
													   ConversionData& conv)
{
	ai_assert(hs != NULL);

	const IfcPlane* const plane = hs->BaseSurface->ToPtr<IfcPlane>();
	if(!plane) {
		IFCImporter::LogError("expected IfcPlane as base surface for the IfcHalfSpaceSolid");
		return;
	}

	// extract plane base position vector and normal vector
	IfcVector3 p,n(0.f,0.f,1.f);
	if (plane->Position->Axis) {
		ConvertDirection(n,plane->Position->Axis.Get());
	}
	ConvertCartesianPoint(p,plane->Position->Location);

	if(!IsTrue(hs->AgreementFlag)) {
		n *= -1.f;
	}

	n.Normalize();

	// obtain the polygonal bounding volume
	boost::shared_ptr<TempMesh> profile = boost::shared_ptr<TempMesh>(new TempMesh());
	if(!ProcessCurve(hs->PolygonalBoundary, *profile.get(), conv)) {
		IFCImporter::LogError("expected valid polyline for boundary of boolean halfspace");
		return;
	}

	IfcMatrix4 mat;
	ConvertAxisPlacement(mat,hs->Position);
	profile->Transform(mat);

	// project the profile onto the plane (orthogonally along the plane normal)
	IfcVector3 r;
	bool have_r = false;
	BOOST_FOREACH(IfcVector3& vec, profile->verts) {
		vec = vec + ((p - vec) * n) * n;
		ai_assert(fabs((vec-p) * n) < 1e-6);

		if (!have_r && (vec-p).SquareLength() > 1e-8) {
			r = vec-p;
			have_r = true;
		}
	}

	if (!have_r) {
		IFCImporter::LogError("polyline for boundary of boolean halfspace is degenerate");
		return;
	}

	// and map everything into a plane coordinate space so all intersection
	// tests can be done in 2D space.
	IfcMatrix4 proj;

	r.Normalize();

	IfcVector3 u = n ^ r;
	u.Normalize();

	proj.a1 = r.x;
	proj.a2 = r.y;
	proj.a3 = r.z;
	proj.b1 = u.x;
	proj.b2 = u.y;
	proj.b3 = u.z;
	proj.c1 = n.x;
	proj.c2 = n.y;
	proj.c3 = n.z;
	
	BOOST_FOREACH(IfcVector3& vec, profile->verts) {
		vec *= proj;
	}

	const IfcMatrix4 proj_inv = IfcMatrix4(proj).Inverse();

	// clip the current contents of `meshout` against the plane we obtained from the second operand
	const std::vector<IfcVector3>& in = first_operand.verts;
	std::vector<IfcVector3>& outvert = result.verts;

	std::vector<unsigned int>::const_iterator begin = first_operand.vertcnt.begin(), 
		end = first_operand.vertcnt.end(), iit;

	outvert.reserve(in.size());
	result.vertcnt.reserve(first_operand.vertcnt.size());

	std::vector<size_t> intersected_boundary_segments;
	std::vector<IfcVector3> intersected_boundary_points;

	unsigned int vidx = 0;
	for(iit = begin; iit != end; vidx += *iit++) {
		if (!*iit) {
			continue;
		}

		unsigned int newcount = 0;
		bool was_outside_boundary = !PointInPoly(in[vidx], profile->verts);

		size_t last_intersected_boundary_segment;
		IfcVector3 last_intersected_boundary_point;

		bool extra_point_flag = false;
		IfcVector3 extra_point;

		for(unsigned int i = 0; i < *iit; ++i) {
			// current segment: [i,i+1 mod size] or [*extra_point,i] if extra_point_flag is set
			const IfcVector3& e0 = extra_point_flag ? extra_point : in[vidx+i];
			const IfcVector3& e1 = extra_point_flag ? in[vidx+i] : in[vidx+(i+1)%*iit];

			// does the current segment intersect the polygonal boundary?
			const IfcVector3& e0_plane = proj * e0;
			const IfcVector3& e1_plane = proj * e1;

			intersected_boundary_segments.clear();
			intersected_boundary_points.clear();

			const bool is_boundary_intersection = IntersectsBoundaryProfile(e0_plane, e1_plane, profile->verts, 
				intersected_boundary_segments, 
				intersected_boundary_points);

			const bool is_outside_boundary = is_boundary_intersection ? !was_outside_boundary : was_outside_boundary;

			// does the current segment intersect the plane?
			// (no extra check if this is an extra point)
			IfcVector3 isectpos;
			const Intersect isect = extra_point_flag ? Intersect_No : IntersectSegmentPlane(p,n,e0,e1,isectpos);

			// is it on the side of the plane that we keep?
			const bool is_white_side =(e0-p).Normalize()*n > 0;

			// e0 on good side of plane? (i.e. we should keep geometry on this side)
			if (is_white_side) {
				// but is there an intersection in e0-e1 and is e1 in the clipping
				// boundary? In this case, generate a line that only goes to the
				// intersection point.
				if (isect == Intersect_Yes && PointInPoly(e1, profile->verts)) {
					outvert.push_back(e0);
					++newcount;

					outvert.push_back(isectpos);
					++newcount;

					// this is, however, only a line that goes to the plane, but not
					// necessarily to the point where the bounding volume on the
					// black side of the plane is hit. So basically, we need another 
					// check for [isectpos-e1], which should give an intersection
					// point and also set the last_intersected_boundary_*'s.
					extra_point_flag = true;
					extra_point = isectpos;
					continue;
				}
				else {
					outvert.push_back(e0);
					++newcount;
				}
			}
			// e0 on bad side of plane (i.e. we should remove geometry on this side,
			// but only if it is within the bounding volume).
			else if (isect == Intersect_Yes) {
				if (is_boundary_intersection) {}
				// drop it and keep e1 instead
				outvert.push_back(isectpos);
				++newcount;
			}
			else {

				// did we just pass the boundary line?
				if (is_boundary_intersection) {

					// and are now outside the clipping boundary?
					if (is_outside_boundary) {
						// in this case, get the point where the clipping boundary
						// was entered first. Then, get the point where the clipping
						// boundary volume was left! These two points with the plane
						// normal form another plane that intersects the clipping
						// volume. There are two ways to get from the first to the
						// second point along the intersection curve, try to pick the
						// one that lies within the current polygon.

						// TODO this approach doesn't handle all cases

						// ...

						outvert.push_back(proj_inv * intersected_boundary_points.back());
						++newcount;

						outvert.push_back(e1);
						++newcount;
					}
					else {
						// we just entered the clipping boundary. Record the point
						// and the segment where we entered and also generate this point.
						last_intersected_boundary_segment = intersected_boundary_segments.front();
						last_intersected_boundary_point = intersected_boundary_points.front();

						outvert.push_back(e0);
						++newcount;

						outvert.push_back(proj_inv * last_intersected_boundary_point);
						++newcount;
					}
				}				
				// if not, we just keep the vertex
				else if (is_outside_boundary) {
					outvert.push_back(e0);
					++newcount;
				}
			}

			was_outside_boundary = is_outside_boundary;
			extra_point_flag = false;
		}
	
		if (!newcount) {
			continue;
		}

		IfcVector3 vmin,vmax;
		ArrayBounds(&*(outvert.end()-newcount),newcount,vmin,vmax);

		// filter our IfcFloat points - those may happen if a point lies
		// directly on the intersection line. However, due to IfcFloat
		// precision a bitwise comparison is not feasible to detect
		// this case.
		const IfcFloat epsilon = (vmax-vmin).SquareLength() / 1e6f;
		FuzzyVectorCompare fz(epsilon);

		std::vector<IfcVector3>::iterator e = std::unique( outvert.end()-newcount, outvert.end(), fz );

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
		else while(newcount-->0) {
			result.verts.pop_back();
		}

	}
	IFCImporter::LogDebug("generating CSG geometry by plane clipping with polygonal bounding (IfcBooleanClippingResult)");	
}

// ------------------------------------------------------------------------------------------------
void ProcessBooleanExtrudedAreaSolidDifference(const IfcExtrudedAreaSolid* as, TempMesh& result, 
											   const TempMesh& first_operand, 
											   ConversionData& conv)
{
	ai_assert(as != NULL);

	// This case is handled by reduction to an instance of the quadrify() algorithm.
	// Obviously, this won't work for arbitrarily complex cases. In fact, the first
	// operand should be near-planar. Luckily, this is usually the case in Ifc 
	// buildings.

	boost::shared_ptr<TempMesh> meshtmp(new TempMesh());
	ProcessExtrudedAreaSolid(*as,*meshtmp,conv,false);

	std::vector<TempOpening> openings(1, TempOpening(as,IfcVector3(0,0,0),meshtmp,boost::shared_ptr<TempMesh>(NULL)));

	result = first_operand;

	TempMesh temp;

	std::vector<IfcVector3>::const_iterator vit = first_operand.verts.begin();
	BOOST_FOREACH(unsigned int pcount, first_operand.vertcnt) {
		temp.Clear();

		temp.verts.insert(temp.verts.end(), vit, vit + pcount);
		temp.vertcnt.push_back(pcount);

		// The algorithms used to generate mesh geometry sometimes
		// spit out lines or other degenerates which must be
		// filtered to avoid running into assertions later on.

		// ComputePolygonNormal returns the Newell normal, so the
		// length of the normal is the area of the polygon.
		const IfcVector3& normal = temp.ComputeLastPolygonNormal(false);
		if (normal.SquareLength() < static_cast<IfcFloat>(1e-5)) {
			IFCImporter::LogWarn("skipping degenerate polygon (ProcessBooleanExtrudedAreaSolidDifference)");
			continue;
		}

		GenerateOpenings(openings, std::vector<IfcVector3>(1,IfcVector3(1,0,0)), temp, false, true);
		result.Append(temp);

		vit += pcount;
	}

	IFCImporter::LogDebug("generating CSG geometry by geometric difference to a solid (IfcExtrudedAreaSolid)");
}

// ------------------------------------------------------------------------------------------------
void ProcessBoolean(const IfcBooleanResult& boolean, TempMesh& result, ConversionData& conv)
{
	// supported CSG operations:
	//   DIFFERENCE
	if(const IfcBooleanResult* const clip = boolean.ToPtr<IfcBooleanResult>()) {
		if(clip->Operator != "DIFFERENCE") {
			IFCImporter::LogWarn("encountered unsupported boolean operator: " + (std::string)clip->Operator);
			return;
		}

		// supported cases (1st operand):
		//  IfcBooleanResult -- call ProcessBoolean recursively
		//  IfcSweptAreaSolid -- obtain polygonal geometry first

		// supported cases (2nd operand):
		//  IfcHalfSpaceSolid -- easy, clip against plane
		//  IfcExtrudedAreaSolid -- reduce to an instance of the quadrify() algorithm


		const IfcHalfSpaceSolid* const hs = clip->SecondOperand->ResolveSelectPtr<IfcHalfSpaceSolid>(conv.db);
		const IfcExtrudedAreaSolid* const as = clip->SecondOperand->ResolveSelectPtr<IfcExtrudedAreaSolid>(conv.db);
		if(!hs && !as) {
			IFCImporter::LogError("expected IfcHalfSpaceSolid or IfcExtrudedAreaSolid as second clipping operand");
			return;
		}

		TempMesh first_operand;
		if(const IfcBooleanResult* const op0 = clip->FirstOperand->ResolveSelectPtr<IfcBooleanResult>(conv.db)) {
			ProcessBoolean(*op0,first_operand,conv);
		}
		else if (const IfcSweptAreaSolid* const swept = clip->FirstOperand->ResolveSelectPtr<IfcSweptAreaSolid>(conv.db)) {
			ProcessSweptAreaSolid(*swept,first_operand,conv);
		}
		else {
			IFCImporter::LogError("expected IfcSweptAreaSolid or IfcBooleanResult as first clipping operand");
			return;
		}

		if(hs) {

			const IfcPolygonalBoundedHalfSpace* const hs_bounded = clip->SecondOperand->ResolveSelectPtr<IfcPolygonalBoundedHalfSpace>(conv.db);
			if (hs_bounded) {
				ProcessPolygonalBoundedBooleanHalfSpaceDifference(hs_bounded, result, first_operand, conv);
			}
			else {
				ProcessBooleanHalfSpaceDifference(hs, result, first_operand, conv);
			}
		}
		else {
			ProcessBooleanExtrudedAreaSolidDifference(as, result, first_operand, conv);
		}
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcBooleanResult entity, type is " + boolean.GetClassName());
	}
}

} // ! IFC
} // ! Assimp

#endif 

