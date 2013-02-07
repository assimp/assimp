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
void ProcessPolygonalBoundedBooleanHalfSpaceDifference(const IfcPolygonalBoundedHalfSpace* hs, TempMesh& result, 
													   const TempMesh& first_operand, 
													   ConversionData& conv)
{
	ai_assert(hs != NULL);

	return; // niy

	
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

