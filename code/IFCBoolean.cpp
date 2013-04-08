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
// note: this functions works on 3D vectors, but performs its intersection checks solely in xy.
bool IntersectsBoundaryProfile( const IfcVector3& e0, const IfcVector3& e1, const std::vector<IfcVector3>& boundary,
	std::vector<size_t>& intersected_boundary_segments,
	std::vector<IfcVector3>& intersected_boundary_points,
	bool half_open = false,
	bool* e0_hits_border = NULL)
{
	ai_assert(intersected_boundary_segments.empty());
	ai_assert(intersected_boundary_points.empty());

	if(e0_hits_border) {
		*e0_hits_border = false;
	}

	const IfcVector3& e = e1 - e0;

	for (size_t i = 0, bcount = boundary.size(); i < bcount; ++i) {
		// boundary segment i: b0-b1
		const IfcVector3& b0 = boundary[i];
		const IfcVector3& b1 = boundary[(i+1) % bcount];

		const IfcVector3& b = b1 - b0;

		// segment-segment intersection
		// solve b0 + b*s = e0 + e*t for (s,t)
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

		// for a valid intersection, s-t should be in range [0,1].
		// note that for t (i.e. the segment point) we only use a
		// half-sided epsilon because the next segment should catch
		// this case.
		const IfcFloat epsilon = 1e-6;
		if (t >= -epsilon && (t <= 1.0+epsilon || half_open) && s >= -epsilon && s <= 1.0) {

			if (e0_hits_border && !*e0_hits_border) {
				*e0_hits_border = fabs(t) < 1e-5f;
			}
	
			const IfcVector3& p = e0 + e*t;

			// only insert the point into the list if it is sufficiently
			// far away from the previous intersection point. This way,
			// we avoid duplicate detection if the intersection is
			// directly on the vertex between two segments.
			if (!intersected_boundary_points.empty() && intersected_boundary_segments.back()==i-1 ) {
				const IfcVector3 diff = intersected_boundary_points.back() - p;
				if(IfcVector2(diff.x, diff.y).SquareLength() < 1e-7) {
					continue;
				}
			}
			intersected_boundary_segments.push_back(i);
			intersected_boundary_points.push_back(p);
		}
	}

	return !intersected_boundary_segments.empty();
}


// ------------------------------------------------------------------------------------------------
// note: this functions works on 3D vectors, but performs its intersection checks solely in xy.
bool PointInPoly(const IfcVector3& p, const std::vector<IfcVector3>& boundary)
{
	// even-odd algorithm: take a random vector that extends from p to infinite
	// and counts how many times it intersects edges of the boundary.
	// because checking for segment intersections is prone to numeric inaccuracies
	// or double detections (i.e. when hitting multiple adjacent segments at their
	// shared vertices) we do it thrice with different rays and vote on it.

	// the even-odd algorithm doesn't work for points which lie directly on
	// the border of the polygon. If any of our attempts produces this result,
	// we return false immediately.

	std::vector<size_t> intersected_boundary_segments;
	std::vector<IfcVector3> intersected_boundary_points;
	size_t votes = 0;

	bool is_border;
	IntersectsBoundaryProfile(p, p + IfcVector3(1.0,0,0), boundary, 
		intersected_boundary_segments, 
		intersected_boundary_points, true, &is_border);

	if(is_border) {
		return false;
	}

	votes += intersected_boundary_segments.size() % 2;

	intersected_boundary_segments.clear();
	intersected_boundary_points.clear();

	IntersectsBoundaryProfile(p, p + IfcVector3(0,1.0,0), boundary, 
		intersected_boundary_segments, 
		intersected_boundary_points, true, &is_border);

	if(is_border) {
		return false;
	}

	votes += intersected_boundary_segments.size() % 2;

	intersected_boundary_segments.clear();
	intersected_boundary_points.clear();

	IntersectsBoundaryProfile(p, p + IfcVector3(0.6,-0.6,0.0), boundary, 
		intersected_boundary_segments, 
		intersected_boundary_points, true, &is_border);

	if(is_border) {
		return false;
	}

	votes += intersected_boundary_segments.size() % 2;
	//ai_assert(votes == 3 || votes == 0);
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

	IfcMatrix4 proj_inv;
	ConvertAxisPlacement(proj_inv,hs->Position);

	// and map everything into a plane coordinate space so all intersection
	// tests can be done in 2D space.
	IfcMatrix4 proj = proj_inv;
	proj.Inverse();

	// clip the current contents of `meshout` against the plane we obtained from the second operand
	const std::vector<IfcVector3>& in = first_operand.verts;
	std::vector<IfcVector3>& outvert = result.verts;

	std::vector<unsigned int>::const_iterator begin = first_operand.vertcnt.begin(), 
		end = first_operand.vertcnt.end(), iit;

	outvert.reserve(in.size());
	result.vertcnt.reserve(first_operand.vertcnt.size());

	std::vector<size_t> intersected_boundary_segments;
	std::vector<IfcVector3> intersected_boundary_points;

	// TODO: the following algorithm doesn't handle all cases. 
	unsigned int vidx = 0;
	for(iit = begin; iit != end; vidx += *iit++) {
		if (!*iit) {
			continue;
		}

		unsigned int newcount = 0;
		bool was_outside_boundary = !PointInPoly(proj * in[vidx], profile->verts);

		// used any more?
		//size_t last_intersected_boundary_segment;
		IfcVector3 last_intersected_boundary_point;

		bool extra_point_flag = false;
		IfcVector3 extra_point;

		IfcVector3 enter_volume;
		bool entered_volume_flag = false;

		for(unsigned int i = 0; i < *iit; ++i) {
			// current segment: [i,i+1 mod size] or [*extra_point,i] if extra_point_flag is set
			const IfcVector3& e0 = extra_point_flag ? extra_point : in[vidx+i];
			const IfcVector3& e1 = extra_point_flag ? in[vidx+i] : in[vidx+(i+1)%*iit];

			// does the current segment intersect the polygonal boundary?
			const IfcVector3& e0_plane = proj * e0;
			const IfcVector3& e1_plane = proj * e1;

			intersected_boundary_segments.clear();
			intersected_boundary_points.clear();

			const bool is_outside_boundary = !PointInPoly(e1_plane, profile->verts);
			const bool is_boundary_intersection = is_outside_boundary != was_outside_boundary;
			
			IntersectsBoundaryProfile(e0_plane, e1_plane, profile->verts, 
				intersected_boundary_segments, 
				intersected_boundary_points);
		
			ai_assert(!is_boundary_intersection || !intersected_boundary_segments.empty());

			// does the current segment intersect the plane?
			// (no extra check if this is an extra point)
			IfcVector3 isectpos;
			const Intersect isect =  extra_point_flag ? Intersect_No : IntersectSegmentPlane(p,n,e0,e1,isectpos);

#ifdef _DEBUG
			if (isect == Intersect_Yes) {
				const IfcFloat f = fabs((isectpos - p)*n);
				ai_assert(f < 1e-5);
			}
#endif

			const bool is_white_side = (e0-p)*n >= -1e-6;

			// e0 on good side of plane? (i.e. we should keep all geometry on this side)
			if (is_white_side) {
				// but is there an intersection in e0-e1 and is e1 in the clipping
				// boundary? In this case, generate a line that only goes to the
				// intersection point.
				if (isect == Intersect_Yes  && !is_outside_boundary) {
					outvert.push_back(e0);
					++newcount;

					outvert.push_back(isectpos);
					++newcount;
					
					/*
					// this is, however, only a line that goes to the plane, but not
					// necessarily to the point where the bounding volume on the
					// black side of the plane is hit. So basically, we need another 
					// check for [isectpos-e1], which should yield an intersection
					// point.
					extra_point_flag = true;
					extra_point = isectpos;

					was_outside_boundary = true; 
					continue; */

					// [isectpos, enter_volume] potentially needs extra points.
					// For this, we determine the intersection point with the
					// bounding volume and project it onto the plane. 
					/*
					const IfcVector3& enter_volume_proj = proj * enter_volume;
					const IfcVector3& enter_isectpos = proj * isectpos;

					intersected_boundary_segments.clear();
					intersected_boundary_points.clear();

					IntersectsBoundaryProfile(enter_volume_proj, enter_isectpos, profile->verts, 
						intersected_boundary_segments, 
						intersected_boundary_points);

					if(!intersected_boundary_segments.empty()) {

						vec = vec + ((p - vec) * n) * n;
					}
					*/				

					//entered_volume_flag = true;
				}
				else {
					outvert.push_back(e0);
					++newcount;
				}
			}
			// e0 on bad side of plane, e1 on good (i.e. we should remove geometry on this side,
			// but only if it is within the bounding volume).
			else if (isect == Intersect_Yes) {
				// is e0 within the clipping volume? Insert the intersection point
				// of [e0,e1] and the plane instead of e0.
				if(was_outside_boundary) {
					outvert.push_back(e0);
				}
				else {
					if(entered_volume_flag) {
						const IfcVector3& fix_point = enter_volume + ((p - enter_volume) * n) * n;
						outvert.push_back(fix_point);
						++newcount;
					}

					outvert.push_back(isectpos);	
				}
				entered_volume_flag = false;
				++newcount;
			}
			else { // no intersection with plane or parallel; e0,e1 are on the bad side
			
				// did we just pass the boundary line to the poly bounding?
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

						IfcFloat d = 1e20;
						IfcVector3 vclosest;
						BOOST_FOREACH(const IfcVector3& v, intersected_boundary_points) {
							const IfcFloat dn = (v-e1_plane).SquareLength();
							if (dn < d) {
								d = dn;
								vclosest = v;
							}
						}

						vclosest = proj_inv * vclosest;
						if(entered_volume_flag) {
							const IfcVector3& fix_point = vclosest + ((p - vclosest) * n) * n;
							outvert.push_back(fix_point);
							++newcount;

							entered_volume_flag = false;
						}

						outvert.push_back(vclosest);
						++newcount;

						//outvert.push_back(e1);
						//++newcount;
					}
					else {
						entered_volume_flag = true;

						// we just entered the clipping boundary. Record the point
						// and the segment where we entered and also generate this point.
						//last_intersected_boundary_segment = intersected_boundary_segments.front();
						//last_intersected_boundary_point = intersected_boundary_points.front();

						outvert.push_back(e0);
						++newcount;

						IfcFloat d = 1e20;
						IfcVector3 vclosest;
						BOOST_FOREACH(const IfcVector3& v, intersected_boundary_points) {
							const IfcFloat dn = (v-e0_plane).SquareLength();
							if (dn < d) {
								d = dn;
								vclosest = v;
							}
						}

						enter_volume = proj_inv * vclosest;
						outvert.push_back(enter_volume);
						++newcount;
					}
				}				
				// if not, we just keep the vertex
				else if (is_outside_boundary) {
					outvert.push_back(e0);
					++newcount;

					entered_volume_flag = false;
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

	boost::shared_ptr<TempMesh> meshtmp = boost::shared_ptr<TempMesh>(new TempMesh());
	ProcessExtrudedAreaSolid(*as,*meshtmp,conv,false);

	std::vector<TempOpening> openings(1, TempOpening(as,IfcVector3(0,0,0),meshtmp,boost::shared_ptr<TempMesh>()));

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

