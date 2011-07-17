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

/** @file  IFCProfile.cpp
 *  @brief Read profile and curves entities from IFC files
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER
#include "IFCUtil.h"

namespace Assimp {
	namespace IFC {
		namespace {


// --------------------------------------------------------------------------------
// Conic is the base class for Circle and Ellipse
// --------------------------------------------------------------------------------
class Conic : public Curve
{

public:

	// --------------------------------------------------
	Conic(const IfcConic& entity, ConversionData& conv) 
		: Curve(entity,conv)
	{
		aiMatrix4x4 trafo;
		ConvertAxisPlacement(trafo,*entity.Position,conv);

		// for convenience, extract the matrix rows
		location = aiVector3D(trafo.a4,trafo.b4,trafo.c4);
		p[0] = aiVector3D(trafo.a1,trafo.b1,trafo.c1);
		p[1] = aiVector3D(trafo.a2,trafo.b2,trafo.c2);
		p[2] = aiVector3D(trafo.a3,trafo.b3,trafo.c3);
	}

public:

	// --------------------------------------------------
	std::pair<float,float> GetParametricRange() const {
		return std::make_pair(0.f,360.f);
	}

protected:
	aiVector3D location, p[3];
};


// --------------------------------------------------------------------------------
// Circle
// --------------------------------------------------------------------------------
class Circle : public Conic
{

public:

	// --------------------------------------------------
	Circle(const IfcCircle& entity, ConversionData& conv) 
		: Conic(entity,conv)
		, entity(entity)
	{
	}

public:

	// --------------------------------------------------
	aiVector3D Eval(float u) const {
		u = -conv.angle_scale * u;
		return location + entity.Radius*(::cos(u)*p[0] + ::sin(u)*p[1]);
	}

private:
	const IfcCircle& entity;
};


// --------------------------------------------------------------------------------
// Ellipse
// --------------------------------------------------------------------------------
class Ellipse : public Conic
{

public:

	// --------------------------------------------------
	Ellipse(const IfcEllipse& entity, ConversionData& conv) 
		: Conic(entity,conv)
		, entity(entity)
	{
	}

public:

	// --------------------------------------------------
	aiVector3D Eval(float u) const {
		u = -conv.angle_scale * u;
		return location + entity.SemiAxis1*::cos(u)*p[0] + entity.SemiAxis2*::sin(u)*p[1];
	}

private:
	const IfcEllipse& entity;
};


// --------------------------------------------------------------------------------
// Line
// --------------------------------------------------------------------------------
class Line : public Curve 
{

public:

	// --------------------------------------------------
	Line(const IfcLine& entity, ConversionData& conv) 
		: Curve(entity,conv)
		, entity(entity)
	{
		ConvertCartesianPoint(p,entity.Pnt);
		ConvertVector(v,entity.Dir);
	}

public:

	// --------------------------------------------------
	aiVector3D Eval(float u) const {
		return p + u*v;
	}

	// --------------------------------------------------
	std::pair<float,float> GetParametricRange() const {
		const float inf = std::numeric_limits<float>::infinity();

		return std::make_pair(-inf,+inf);
	}

private:
	const IfcLine& entity;
	aiVector3D p,v;
};

// --------------------------------------------------------------------------------
// CompositeCurve joins multiple smaller, bounded curves
// --------------------------------------------------------------------------------
class CompositeCurve : public BoundedCurve 
{

	// XXX the implementation currently ignores curve transitions

public:

	// --------------------------------------------------
	CompositeCurve(const IfcCompositeCurve& entity, ConversionData& conv) 
		: BoundedCurve(entity,conv)
		, entity(entity)
	{
		curves.reserve(entity.Segments.size());
		BOOST_FOREACH(const IfcCompositeCurveSegment& curveSegment,entity.Segments) {
			// according to the specification, this must be a bounded curve
			boost::shared_ptr< Curve > cv(Curve::Convert(curveSegment.ParentCurve,conv));
			boost::shared_ptr< BoundedCurve > bc = boost::dynamic_pointer_cast<BoundedCurve>(cv);

			if (!bc) {
				IFCImporter::LogError("expected segment of composite curve to be a bounded curve");
				continue;
			}

			if ( (std::string)curveSegment.Transition != "CONTINUOUS" ) {
				IFCImporter::LogDebug("ignoring transition code on composite curve segment, only continuous transitions are supported");
			}

			curves.push_back(bc);
		}

		if (curves.empty()) {
			IFCImporter::LogError("empty composite curve");
			return;
		}

		total = 0.f;
		BOOST_FOREACH(boost::shared_ptr< const BoundedCurve > curve, curves) {
			const std::pair<float,float> range = curve->GetParametricRange();
			total += range.second-range.first;
		}
	}

public:

	// --------------------------------------------------
	aiVector3D Eval(float u) const {
		if (curves.empty()) {
			return aiVector3D();
		}

		float acc = 0;
		BOOST_FOREACH(boost::shared_ptr< const BoundedCurve > curve, curves) {
			const std::pair<float,float> range = curve->GetParametricRange();
			const float delta = range.second-range.first;
			if (u < acc+delta) {
				return curve->Eval( (u-acc) + range.first );
			}

			acc += delta;
		}
		// clamp to end
		return curves.back()->Eval(curves.back()->GetParametricRange().second);
	}

	// --------------------------------------------------
	float SuggestNext(float u) const {

		float acc = 0;
		BOOST_FOREACH(boost::shared_ptr< const BoundedCurve > curve, curves) {
			const std::pair<float,float> range = curve->GetParametricRange();
			const float delta = range.second-range.first;
			if (u < acc+delta) {
				return curve->SuggestNext( (u-acc) + range.first ) - range.first + acc;
			}
			acc += delta;
		}
		return std::numeric_limits<float>::infinity();
	}

	// --------------------------------------------------
	std::pair<float,float> GetParametricRange() const {
		return std::make_pair(0.f,total);
	}

private:
	const IfcCompositeCurve& entity;
	std::vector< boost::shared_ptr< const BoundedCurve> > curves;

	float total;
};


// --------------------------------------------------------------------------------
// TrimmedCurve can be used to trim an unbounded curve to a bounded range
// --------------------------------------------------------------------------------
class TrimmedCurve : public BoundedCurve 
{

public:

	// --------------------------------------------------
	TrimmedCurve(const IfcTrimmedCurve& entity, ConversionData& conv) 
		: BoundedCurve(entity,conv)
		, entity(entity)
	{
		base = boost::shared_ptr<const Curve>(Curve::Convert(entity.BasisCurve,conv));

		typedef boost::shared_ptr<const STEP::EXPRESS::DataType> Entry;
	
		// for some reason, trimmed curves can either specify a parametric value
		// or a point on the curve, or both. And they can even specify which of the
		// two representations they prefer, even though an information invariant
		// claims that they must be identical if both are present.
		// oh well.
		bool ok = false;
		BOOST_FOREACH(const Entry sel,entity.Trim1) {
			if (const EXPRESS::REAL* const r = sel->ToPtr<EXPRESS::REAL>()) {
				range.first = *r;
				ok = true;
				break;
			}
		}
		if (!ok) {
			IFCImporter::LogError("trimming by curve points not currently supported, skipping first cut point");
			range.first = base->GetParametricRange().first;
			if (range.first == std::numeric_limits<float>::infinity()) {
				range.first = 0;
			}
		}
		ok = false;
		BOOST_FOREACH(const Entry sel,entity.Trim2) {
			if (const EXPRESS::REAL* const r = sel->ToPtr<EXPRESS::REAL>()) {
				range.second = *r;
				ok = true;
				break;
			}
		}
		if (!ok) {
			IFCImporter::LogError("trimming by curve points not currently supported, skipping second cut point");
			range.second = base->GetParametricRange().second;
			if (range.second == std::numeric_limits<float>::infinity()) {
				range.second = 0;
			}
		}

		maxval = range.second-range.first;
	}

public:

	// --------------------------------------------------
	aiVector3D Eval(float p) const {
		p = std::min(range.second, std::max(range.first+p,range.first));
		if (!IsTrue(entity.SenseAgreement)) {
			p = range.second - (p-range.first);
		}
		return base->Eval( p );
	}

	// --------------------------------------------------
	float SuggestNext(float u) const {
		if (u >= maxval) {
			return std::numeric_limits<float>::infinity();
		}

		if (const Line* const l = dynamic_cast<const Line*>(base.get())) {
			// a line is, well, a line .. so two points are always sufficient to represent it
			return maxval;
		}

		if (const Conic* const l = dynamic_cast<const Conic*>(base.get())) {
			// the suitable sampling density for conics is a configuration property
			return std::min(maxval, static_cast<float>( u + maxval/ceil(maxval/conv.settings.conicSamplingAngle)) );
		}
		
		return BoundedCurve::SuggestNext(u);
	}

	// --------------------------------------------------
	std::pair<float,float> GetParametricRange() const {
		return std::make_pair(0,maxval);
	}

private:
	const IfcTrimmedCurve& entity;
	std::pair<float,float> range;
	float maxval;

	boost::shared_ptr<const Curve> base;
};


// --------------------------------------------------------------------------------
// PolyLine is a 'curve' defined by linear interpolation over a set of discrete points
// --------------------------------------------------------------------------------
class PolyLine : public BoundedCurve 
{

public:

	// --------------------------------------------------
	PolyLine(const IfcPolyline& entity, ConversionData& conv) 
		: BoundedCurve(entity,conv)
		, entity(entity)
	{
		points.reserve(entity.Points.size());

		aiVector3D t;
		BOOST_FOREACH(const IfcCartesianPoint& cp, entity.Points) {
			ConvertCartesianPoint(t,cp);
			points.push_back(t);
		}
	}

public:

	// --------------------------------------------------
	aiVector3D Eval(float p) const {
		if (p < 0.f) {
			return points.front();
		}
		const size_t b = static_cast<size_t>(floor(p));  
		if (b >= points.size()-1) {
			return points.back();
		}

		const float d = p-static_cast<float>(b);
		return points[b+1] * d + points[b] * (1.f-d);
	}

	// --------------------------------------------------
	float SuggestNext(float u) const {
		if (u > points.size()-1) {
			return std::numeric_limits<float>::infinity();
		}
		return ::floor(u)+1;
	}

	// --------------------------------------------------
	std::pair<float,float> GetParametricRange() const {
		return std::make_pair(0.f,static_cast<float>(points.size()-1));
	}

private:
	const IfcPolyline& entity;
	std::vector<aiVector3D> points;
};


} // anon


// ------------------------------------------------------------------------------------------------
Curve* Curve :: Convert(const IFC::IfcCurve& curve,ConversionData& conv) 
{
	if(curve.ToPtr<IfcBoundedCurve>()) {
		if(const IfcPolyline* c = curve.ToPtr<IfcPolyline>()) {
			return new PolyLine(*c,conv);
		}
		if(const IfcTrimmedCurve* c = curve.ToPtr<IfcTrimmedCurve>()) {
			return new TrimmedCurve(*c,conv);
		}
		if(const IfcCompositeCurve* c = curve.ToPtr<IfcCompositeCurve>()) {
			return new CompositeCurve(*c,conv);
		}
		//if(const IfcBSplineCurve* c = curve.ToPtr<IfcBSplineCurve>()) {
		//	return new BSplineCurve(*c,conv);
		//}
	}

	if(curve.ToPtr<IfcConic>()) {
		if(const IfcCircle* c = curve.ToPtr<IfcCircle>()) {
			return new Circle(*c,conv);
		}
		if(const IfcEllipse* c = curve.ToPtr<IfcEllipse>()) {
			return new Ellipse(*c,conv);
		}
	}

	if(const IfcLine* c = curve.ToPtr<IfcLine>()) {
		return new Line(*c,conv);
	}

	// XXX OffsetCurve2D, OffsetCurve3D not currently supported
	return NULL;
}

// ------------------------------------------------------------------------------------------------
float BoundedCurve :: SuggestNext(float u) const
{
	// the default behavior is to subdivide each curve into approximately 32 linear segments
	const unsigned int segments = 32;

	const std::pair<float,float> range = GetParametricRange();
	const float delta = range.second - range.first, perseg = delta/segments;

	if (u < range.first) {
		return range.first;
	}

	u = u+perseg;
	if (u > range.second) {
		return std::numeric_limits<float>::infinity();
	}

	return u;
}

// ------------------------------------------------------------------------------------------------
void BoundedCurve :: SampleDiscrete(TempMesh& out) const
{
	const std::pair<float,float> range = GetParametricRange();
	const float inf = std::numeric_limits<float>::infinity();

	size_t cnt = 0;
	float u = range.first;
	do ++cnt; while( (u = SuggestNext(u)) != inf );
	out.verts.reserve(cnt);

	u = range.first;
	do out.verts.push_back(Eval(u)); while( (u = SuggestNext(u)) != inf );
}

} // IFC
} // Assimp

#endif // ASSIMP_BUILD_NO_IFC_IMPORTER
