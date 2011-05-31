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
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER

#include <iterator>
#include <boost/tuple/tuple.hpp>


#include "IFCLoader.h"
#include "STEPFileReader.h"
#include "IFCReaderGen.h"


#include "StreamReader.h"
#include "MemoryIOWrapper.h"
#include "ProcessHelper.h"
#include "PolyTools.h"


using namespace Assimp;
using namespace Assimp::Formatter;
namespace EXPRESS = STEP::EXPRESS;

template<> const std::string LogFunctions<IFCImporter>::log_prefix = "IFC: ";


/* DO NOT REMOVE this comment block. The genentitylist.sh script
 * just looks for names adhering to the IFC :: IfcSomething naming scheme
 * and includes all matches in the whitelist for code-generation. Thus,
 * all entity classes that are only indirectly referenced need to be
 * mentioned explicitly.

  IFC::IfcRepresentationMap
  IFC::IfcProductRepresentation
  IFC::IfcUnitAssignment
  IFC::IfcClosedShell
  IFC::IfcDoor

 */

namespace {

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

	// ------------------------------------------------------------------------------
	aiMesh* ToMesh() {
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

	// ------------------------------------------------------------------------------
	void Clear() {
		verts.clear();
		vertcnt.clear();
	}

	// ------------------------------------------------------------------------------
	void Transform(const aiMatrix4x4& mat) {
		BOOST_FOREACH(aiVector3D& v, verts) {
			v *= mat;
		}
	}

	// ------------------------------------------------------------------------------
	aiVector3D Center() {
		return std::accumulate(verts.begin(),verts.end(),aiVector3D(0.f,0.f,0.f)) / static_cast<float>(verts.size());
	}

	// ------------------------------------------------------------------------------
	void Append(const TempMesh& other) {

		verts.insert(verts.end(),other.verts.begin(),other.verts.end());
		vertcnt.insert(vertcnt.end(),other.vertcnt.begin(),other.vertcnt.end());
	}

	// ------------------------------------------------------------------------------
	void RemoveAdjacentDuplicates() {

		bool drop = false;
		std::vector<aiVector3D>::iterator base = verts.begin();
		BOOST_FOREACH(unsigned int& cnt, vertcnt) {
			if (cnt < 2){
				base += cnt;
				continue;
			}

			aiVector3D vmin,vmax;
			ArrayBounds(&*base, cnt ,vmin,vmax);

			const float epsilon = (vmax-vmin).SquareLength() / 1e9f, dotepsilon = 1e-7;

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
			//	else if ( d0*d1 < -1.f+dotepsilon ) {
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
};


// ------------------------------------------------------------------------------
void TempOpening::Transform(const aiMatrix4x4& mat) 
{
	if(profileMesh) {
		profileMesh->Transform(mat);
	}
	extrusionDir *= aiMatrix3x3(mat);
}


// forward declarations
float ConvertSIPrefix(const std::string& prefix);
void SetUnits(ConversionData& conv);
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement& in, ConversionData& conv);
void SetCoordinateSpace(ConversionData& conv);
void ProcessSpatialStructures(ConversionData& conv);
aiNode* ProcessSpatialStructure(aiNode* parent, const IFC::IfcProduct& el ,ConversionData& conv);
void ProcessProductRepresentation(const IFC::IfcProduct& el, aiNode* nd, ConversionData& conv);
void MakeTreeRelative(ConversionData& conv);
void ConvertUnit(const EXPRESS::DataType& dt,ConversionData& conv);
void ProcessSweptAreaSolid(const IFC::IfcSweptAreaSolid& swept, TempMesh& meshout, ConversionData& conv);

} // anon

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
IFCImporter::IFCImporter()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
IFCImporter::~IFCImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool IFCImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	const std::string& extension = GetExtension(pFile);
	if (extension == "ifc") {
		return true;
	}

	else if ((!extension.length() || checkSig) && pIOHandler)	{
		// note: this is the common identification for STEP-encoded files, so
		// it is only unambiguous as long as we don't support any further
		// file formats with STEP as their encoding.
		const char* tokens[] = {"ISO-10303-21"};
		return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// List all extensions handled by this loader
void IFCImporter::GetExtensionList(std::set<std::string>& app) 
{
	app.insert("ifc");
}


// ------------------------------------------------------------------------------------------------
// Setup configuration properties for the loader
void IFCImporter::SetupProperties(const Importer* pImp)
{
	settings.skipSpaceRepresentations = pImp->GetPropertyBool(AI_CONFIG_IMPORT_IFC_SKIP_SPACE_REPRESENTATIONS,true);
	settings.skipCurveRepresentations = pImp->GetPropertyBool(AI_CONFIG_IMPORT_IFC_SKIP_CURVE_REPRESENTATIONS,true);
	settings.useCustomTriangulation = pImp->GetPropertyBool(AI_CONFIG_IMPORT_IFC_CUSTOM_TRIANGULATION,true);
}


// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void IFCImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::shared_ptr<IOStream> stream(pIOHandler->Open(pFile));
	if (!stream) {
		ThrowException("Could not open file for reading");
	}

	boost::scoped_ptr<STEP::DB> db(STEP::ReadFileHeader(stream));
	const STEP::HeaderInfo& head = const_cast<const STEP::DB&>(*db).GetHeader();

	if(!head.fileSchema.size() || head.fileSchema.substr(0,3) != "IFC") {
		ThrowException("Unrecognized file schema: " + head.fileSchema);
	}

	if (!DefaultLogger::isNullLogger()) {
		LogDebug("File schema is \'" + head.fileSchema + '\'');
		if (head.timestamp.length()) {
			LogDebug("Timestamp \'" + head.timestamp + '\'');
		}
		if (head.app.length()) {
			LogDebug("Application/Exporter identline is \'" + head.app  + '\'');
		}
	}

	// obtain a copy of the machine-generated IFC scheme
	EXPRESS::ConversionSchema schema;
	IFC::GetSchema(schema);

	// tell the reader which entity types to track with special care
	static const char* const types_to_track[] = {
		"ifcsite", "ifcbuilding", "ifcproject"
	};

	// tell the reader for which types we need to simulate STEPs reverse indices
	static const char* const inverse_indices_to_track[] = {
		"ifcrelcontainedinspatialstructure", "ifcrelaggregates", "ifcrelvoidselement", "ifcstyleditem"
	};

	// feed the IFC schema into the reader and pre-parse all lines
	STEP::ReadFile(*db, schema, types_to_track, inverse_indices_to_track);

	const STEP::LazyObject* proj =  db->GetObject("ifcproject");
	if (!proj) {
		ThrowException("missing IfcProject entity");
	}

	ConversionData conv(*db,proj->To<IFC::IfcProject>(),pScene,settings);
	SetUnits(conv);
	SetCoordinateSpace(conv);
	ProcessSpatialStructures(conv);
	MakeTreeRelative(conv);

	// NOTE - this is a stress test for the importer, but it works only
	// in a build with no entities disabled. See 
	//     scripts/IFCImporter/CPPGenerator.py
	// for more information.
#ifdef ASSIMP_IFC_TEST
	db->EvaluateAll();
#endif

	// do final data copying
	if (conv.meshes.size()) {
		pScene->mNumMeshes = static_cast<unsigned int>(conv.meshes.size());
		pScene->mMeshes = new aiMesh*[pScene->mNumMeshes]();
		std::copy(conv.meshes.begin(),conv.meshes.end(),pScene->mMeshes);

		// needed to keep the d'tor from burning us
		conv.meshes.clear();
	}

	if (conv.materials.size()) {
		pScene->mNumMaterials = static_cast<unsigned int>(conv.materials.size());
		pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials]();
		std::copy(conv.materials.begin(),conv.materials.end(),pScene->mMaterials);

		// needed to keep the d'tor from burning us
		conv.materials.clear();
	}

	// apply world coordinate system (which includes the scaling to convert to meters and a -90 degrees rotation around x)
	aiMatrix4x4 scale, rot;
	aiMatrix4x4::Scaling(aiVector3D(conv.len_scale,conv.len_scale,conv.len_scale),scale);
	aiMatrix4x4::RotationX(-AI_MATH_HALF_PI_F,rot);

	pScene->mRootNode->mTransformation = rot * scale * conv.wcs * pScene->mRootNode->mTransformation;

	// this must be last because objects are evaluated lazily as we process them
	if ( !DefaultLogger::isNullLogger() ){
		LogDebug((Formatter::format(),"STEP: evaluated ",db->GetEvaluatedObjectCount()," object records"));
	}
}

namespace {

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
void ConvertUnit(const IFC::IfcNamedUnit& unit,ConversionData& conv)
{
	if(const IFC::IfcSIUnit* const si = unit.ToPtr<IFC::IfcSIUnit>()) {

		if(si->UnitType == "LENGTHUNIT") { 
			conv.len_scale = si->Prefix ? ConvertSIPrefix(si->Prefix) : 1.f;
			IFCImporter::LogDebug("got units used for lengths");
		}
		if(si->UnitType == "PLANEANGLEUNIT") { 
			if (si->Name != "RADIAN") {
				IFCImporter::LogWarn("expected base unit for angles to be radian");
			}
		}
	}
	else if(const IFC::IfcConversionBasedUnit* const convu = unit.ToPtr<IFC::IfcConversionBasedUnit>()) {

		if(convu->UnitType == "PLANEANGLEUNIT") { 
			try {
				conv.angle_scale = convu->ConversionFactor->ValueComponent->To<EXPRESS::REAL>();
				ConvertUnit(*convu->ConversionFactor->UnitComponent,conv);
				IFCImporter::LogDebug("got units used for angles");
			}
			catch(std::bad_cast&) {
				IFCImporter::LogError("skipping unknown IfcConversionBasedUnit.ValueComponent entry - expected REAL");
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertUnit(const EXPRESS::DataType& dt,ConversionData& conv)
{
	try {
		const EXPRESS::ENTITY& e = dt.To<IFC::ENTITY>();

		const IFC::IfcNamedUnit& unit = e.ResolveSelect<IFC::IfcNamedUnit>(conv.db);
		if(unit.UnitType != "LENGTHUNIT" && unit.UnitType != "PLANEANGLEUNIT") {
			return;
		}

		ConvertUnit(unit,conv);
	}
	catch(std::bad_cast&) {
		// not entity, somehow
		IFCImporter::LogError("skipping unknown IfcUnit entry - expected entity");
	}
}

// ------------------------------------------------------------------------------------------------
void SetUnits(ConversionData& conv)
{
	// see if we can determine the coordinate space used to express. 
	for(size_t i = 0; i <  conv.proj.UnitsInContext->Units.size(); ++i ) {
		ConvertUnit(*conv.proj.UnitsInContext->Units[i],conv);
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertColor(aiColor4D& out, const IFC::IfcColourRgb& in)
{
	out.r = in.Red;
	out.g = in.Green;
	out.b = in.Blue;
	out.a = 1.f;
}

// ------------------------------------------------------------------------------------------------
void ConvertColor(aiColor4D& out, const IFC::IfcColourOrFactor& in,ConversionData& conv,const aiColor4D* base)
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
	else if (const IFC::IfcColourRgb* const rgb = in.ResolveSelectPtr<IFC::IfcColourRgb>(conv.db)) {
		ConvertColor(out,*rgb);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcColourOrFactor entity");
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertCartesianPoint(aiVector3D& out, const IFC::IfcCartesianPoint& in)
{
	out = aiVector3D();
	for(size_t i = 0; i < in.Coordinates.size(); ++i) {
		out[i] = in.Coordinates[i];
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertDirection(aiVector3D& out, const IFC::IfcDirection& in)
{
	out = aiVector3D();
	for(size_t i = 0; i < in.DirectionRatios.size(); ++i) {
		out[i] = in.DirectionRatios[i];
	}
	const float len = out.Length();
	if (len<1e-6) {
		IFCImporter::LogWarn("direction vector too small, normalizing would result in a division by zero");
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
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement3D& in, ConversionData& conv)
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
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement2D& in, ConversionData& conv)
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
void ConvertAxisPlacement(aiVector3D& axis, aiVector3D& pos, const IFC::IfcAxis1Placement& in, ConversionData& conv)
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
void ConvertAxisPlacement(aiMatrix4x4& out, const IFC::IfcAxis2Placement& in, ConversionData& conv)
{
	if(const IFC::IfcAxis2Placement3D* pl3 = in.ResolveSelectPtr<IFC::IfcAxis2Placement3D>(conv.db)) {
		ConvertAxisPlacement(out,*pl3,conv);
	}
	else if(const IFC::IfcAxis2Placement2D* pl2 = in.ResolveSelectPtr<IFC::IfcAxis2Placement2D>(conv.db)) {
		ConvertAxisPlacement(out,*pl2,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcAxis2Placement entity");
	}
}

// ------------------------------------------------------------------------------------------------
void SetCoordinateSpace(ConversionData& conv)
{
	const IFC::IfcRepresentationContext* fav = NULL;
	BOOST_FOREACH(const IFC::IfcRepresentationContext& v, conv.proj.RepresentationContexts) {
		fav = &v;
		// Model should be the most suitable type of context, hence ignore the others 
		if (v.ContextType && v.ContextType.Get() == "Model") { 
			break;
		}
	}
	if (fav) {
		if(const IFC::IfcGeometricRepresentationContext* const geo = fav->ToPtr<IFC::IfcGeometricRepresentationContext>()) {
			ConvertAxisPlacement(conv.wcs, *geo->WorldCoordinateSystem, conv);
			IFCImporter::LogDebug("got world coordinate system");
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertTransformOperator(aiMatrix4x4& out, const IFC::IfcCartesianTransformationOperator& op)
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
	if (const IFC::IfcCartesianTransformationOperator3D* op2 = op.ToPtr<IFC::IfcCartesianTransformationOperator3D>()) {
		if(op2->Axis3) {
			ConvertDirection(z,*op2->Axis3.Get());
		}
	}

	aiMatrix4x4 locm;
	aiMatrix4x4::Translation(loc,locm);	
	AssignMatrixAxes(out,x,y,z);

	
	aiVector3D vscale;
	if (const IFC::IfcCartesianTransformationOperator3DnonUniform* nuni = op.ToPtr<IFC::IfcCartesianTransformationOperator3DnonUniform>()) {
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

// ------------------------------------------------------------------------------------------------
bool ProcessPolyloop(const IFC::IfcPolyLoop& loop, TempMesh& meshout, ConversionData& conv)
{
	size_t cnt = 0;
	BOOST_FOREACH(const IFC::IfcCartesianPoint& c, loop.Polygon) {
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
	if (master_bounds != -1) {
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
#define IFC_VERTICAL_HOLE_SIZE_TRESHOLD 0.000001f
	size_t vidx = 0, removed = 0, index = 0;
	const float treshold = area_outer_polygon * IFC_VERTICAL_HOLE_SIZE_TRESHOLD;
	for(iit = begin; iit != end ;++index) {
		const float sqlen = normals[index].SquareLength();
		if (sqlen < treshold) {
			std::vector<aiVector3D>::iterator inbase = in.begin()+vidx;
			in.erase(inbase,inbase+*iit);
			
			outer_polygon_start -= outer_polygon_start>vidx ? *iit : 0;
			*iit++ = 0;
			++removed;

			IFCImporter::LogDebug("skip small hole below treshold");
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
void ProcessConnectedFaceSet(const IFC::IfcConnectedFaceSet& fset, TempMesh& result, ConversionData& conv)
{
	BOOST_FOREACH(const IFC::IfcFace& face, fset.CfsFaces) {
		size_t ob = -1, cnt = 0;
		TempMesh meshout;
		BOOST_FOREACH(const IFC::IfcFaceBound& bound, face.Bounds) {
			
			// XXX implement proper merging for polygonal loops
			if(const IFC::IfcPolyLoop* const polyloop = bound.Bound->ToPtr<IFC::IfcPolyLoop>()) {
				if(ProcessPolyloop(*polyloop, meshout,conv)) {
					if(bound.ToPtr<IFC::IfcFaceOuterBound>()) {
						ob = cnt;
					}
					++cnt;
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
void ProcessPolyLine(const IFC::IfcPolyline& def, TempMesh& meshout, ConversionData& conv)
{
	// this won't produce a valid mesh, it just spits out a list of vertices
	aiVector3D t;
	BOOST_FOREACH(const IFC::IfcCartesianPoint& cp, def.Points) {
		ConvertCartesianPoint(t,cp);
		meshout.verts.push_back(t);
	}
	meshout.vertcnt.push_back(meshout.verts.size());
}

// ------------------------------------------------------------------------------------------------
bool ProcessCurve(const IFC::IfcCurve& curve,  TempMesh& meshout, ConversionData& conv)
{
	if(const IFC::IfcPolyline* poly = curve.ToPtr<IFC::IfcPolyline>()) {
		ProcessPolyLine(*poly,meshout,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcCurve entity, type is " + curve.GetClassName());
		return false;
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
void ProcessClosedProfile(const IFC::IfcArbitraryClosedProfileDef& def, TempMesh& meshout, ConversionData& conv)
{
	ProcessCurve(def.OuterCurve,meshout,conv);
}

// ------------------------------------------------------------------------------------------------
void ProcessOpenProfile(const IFC::IfcArbitraryOpenProfileDef& def, TempMesh& meshout, ConversionData& conv)
{
	ProcessCurve(def.Curve,meshout,conv);
}

// ------------------------------------------------------------------------------------------------
void ProcessParametrizedProfile(const IFC::IfcParameterizedProfileDef& def, TempMesh& meshout, ConversionData& conv)
{
	if(const IFC::IfcRectangleProfileDef* const cprofile = def.ToPtr<IFC::IfcRectangleProfileDef>()) {
		const float x = cprofile->XDim*0.5f, y = cprofile->YDim*0.5f;

		meshout.verts.reserve(meshout.verts.size()+4);
		meshout.verts.push_back( aiVector3D( x, y, 0.f ));
		meshout.verts.push_back( aiVector3D(-x, y, 0.f ));
		meshout.verts.push_back( aiVector3D(-x,-y, 0.f ));
		meshout.verts.push_back( aiVector3D( x,-y, 0.f ));
		meshout.vertcnt.push_back(4);
	}
	else if( const IFC::IfcCircleProfileDef* const circle = def.ToPtr<IFC::IfcCircleProfileDef>()) {
		if( const IFC::IfcCircleHollowProfileDef* const hollow = def.ToPtr<IFC::IfcCircleHollowProfileDef>()) {
			// TODO
		}
		const size_t segments = 32;
		const float delta = AI_MATH_TWO_PI_F/segments, radius = circle->Radius;

		meshout.verts.reserve(segments);

		float angle = 0.f;
		for(size_t i = 0; i < segments; ++i, angle += delta) {
			meshout.verts.push_back( aiVector3D( cos(angle)*radius, sin(angle)*radius, 0.f ));
		}
	
		meshout.vertcnt.push_back(segments);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcParameterizedProfileDef entity, type is " + def.GetClassName());
		return;
	}

	aiMatrix4x4 trafo;
	ConvertAxisPlacement(trafo, *def.Position,conv);
	meshout.Transform(trafo);
}

// ------------------------------------------------------------------------------------------------
bool ProcessProfile(const IFC::IfcProfileDef& prof, TempMesh& meshout, ConversionData& conv) 
{
	if(const IFC::IfcArbitraryClosedProfileDef* const cprofile = prof.ToPtr<IFC::IfcArbitraryClosedProfileDef>()) {
		ProcessClosedProfile(*cprofile,meshout,conv);
	}
	else if(const IFC::IfcArbitraryOpenProfileDef* const copen = prof.ToPtr<IFC::IfcArbitraryOpenProfileDef>()) {
		ProcessOpenProfile(*copen,meshout,conv);
	}
	else if(const IFC::IfcParameterizedProfileDef* const cparam = prof.ToPtr<IFC::IfcParameterizedProfileDef>()) {
		ProcessParametrizedProfile(*cparam,meshout,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcProfileDef entity, type is " + prof.GetClassName());
		return false;
	}
	meshout.RemoveAdjacentDuplicates();
	if (!meshout.vertcnt.size() || meshout.vertcnt.front() <= 1) {
		return false;
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
void ProcessRevolvedAreaSolid(const IFC::IfcRevolvedAreaSolid& solid, TempMesh& result, ConversionData& conv)
{
	TempMesh meshout;

	// first read the profile description
	if(!ProcessProfile(*solid.SweptArea,meshout,conv) || meshout.verts.size()<=1) {
		return;
	}

	aiVector3D axis, pos;
	ConvertAxisPlacement(axis,pos,solid.Axis,conv);

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
	ConvertAxisPlacement(trafo, solid.Position,conv);
	
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

		const aiVector3D diff = t.extrusionDir; 
		const std::vector<aiVector3D>& va = t.profileMesh->verts;
		if(va.size() <= 2) {
			continue;	
		}

		const float dd = t.extrusionDir*nor;
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
		if (bb.second.x > pmin.x && bb.first.x < pmax.x && bb.second.y > pmin.y && bb.first.y < pmax.y) {
			xs = bb.first.x;
			xe = bb.second.x;
			found = true;
			break;
		}
	}
	xs = std::max(pmin.x,xs);
	xe = std::min(pmax.x,xe);

	if (!found) {
		// the rectangle [pmin,pend] is opaque, fill it
		out.push_back(pmin);
		out.push_back(aiVector2D(pmin.x,pmax.y));
		out.push_back(pmax);
		out.push_back(aiVector2D(pmax.x,pmin.y));
		return;
	}

	if (xs - pmin.x) {
		out.push_back(pmin);
		out.push_back(aiVector2D(pmin.x,pmax.y));
		out.push_back(aiVector2D(xs,pmax.y));
		out.push_back(aiVector2D(xs,pmin.y));
	}

	// search along the y-axis for all openings that overlap xs and our element
	float ylast = pmin.y;
	found = false;
	for(; start != field.end(); ++start) {
		const BoundingBox& bb = bbs[(*start).second];

		if (bb.second.y > ylast && bb.first.y < pmax.y) {

			found = true;
			const float ys = std::max(bb.first.y,pmin.y), ye = std::min(bb.second.y,pmax.y);
			if (ys - ylast) {
				// Divide et impera!
				QuadrifyPart( aiVector2D(xs,ylast), aiVector2D(xe,ys) ,field,bbs,out);
			}

			// the following are the window vertices

			/*wnd.push_back(aiVector2D(xs,ys));
			wnd.push_back(aiVector2D(xs,ye));
			wnd.push_back(aiVector2D(xe,ye));
			wnd.push_back(aiVector2D(xe,ys));*/
			ylast = ye;
		}

		if (bb.first.x > xs) {
			break;
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
		// Divide et impera!
		QuadrifyPart( aiVector2D(xs,ylast), aiVector2D(xe,pmax.y) ,field,bbs,out);
	}

	// Divide et impera! - now for the whole rest
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
				if (last_hit != -1) {
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

	const aiVector3D diag = vmax-vmin, diagn = aiVector3D(diag).Normalize();
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

		const aiVector3D diff = t.extrusionDir; 
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
void ProcessExtrudedAreaSolid(const IFC::IfcExtrudedAreaSolid& solid, TempMesh& result, ConversionData& conv)
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
	ConvertAxisPlacement(trafo, solid.Position,conv);
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

	if(conv.apply_openings && (sides_with_openings != 2 && sides_with_openings || sides_with_v_openings != 2 && sides_with_v_openings)) {
		IFCImporter::LogWarn("failed to resolve all openings, presumably their topology is not supported by Assimp");
	}

	IFCImporter::LogDebug("generate mesh procedurally by extrusion (IfcExtrudedAreaSolid)");
}

// ------------------------------------------------------------------------------------------------
void ProcessSweptAreaSolid(const IFC::IfcSweptAreaSolid& swept, TempMesh& meshout, ConversionData& conv)
{
	if(const IFC::IfcExtrudedAreaSolid* const solid = swept.ToPtr<IFC::IfcExtrudedAreaSolid>()) {
		// Do we just collect openings for a parent element (i.e. a wall)? 
		// In this case we don't extrude the surface yet, just keep the profile and transform it correctly
		if(conv.collect_openings) {
			boost::shared_ptr<TempMesh> meshtmp(new TempMesh());
			ProcessProfile(swept.SweptArea,*meshtmp,conv);

			aiMatrix4x4 m;
			ConvertAxisPlacement(m,solid->Position,conv);
			meshtmp->Transform(m);

			aiVector3D dir;
			ConvertDirection(dir,solid->ExtrudedDirection);
			conv.collect_openings->push_back(TempOpening(solid, aiMatrix3x3(m) * (dir*solid->Depth),meshtmp));
			return;
		}

		ProcessExtrudedAreaSolid(*solid,meshout,conv);
	}
	else if(const IFC::IfcRevolvedAreaSolid* const rev = swept.ToPtr<IFC::IfcRevolvedAreaSolid>()) {
		ProcessRevolvedAreaSolid(*rev,meshout,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcSweptAreaSolid entity, type is " + swept.GetClassName());
	}
}

// ------------------------------------------------------------------------------------------------
void ProcessBoolean(const IFC::IfcBooleanResult& boolean, TempMesh& result, ConversionData& conv)
{
	if(const IFC::IfcBooleanClippingResult* const clip = boolean.ToPtr<IFC::IfcBooleanClippingResult>()) {
		if(clip->Operator != "DIFFERENCE") {
			IFCImporter::LogWarn("encountered unsupported boolean operator: " + (std::string)clip->Operator);
			return;
		}

		TempMesh meshout;
		const IFC::IfcHalfSpaceSolid* const hs = clip->SecondOperand->ResolveSelectPtr<IFC::IfcHalfSpaceSolid>(conv.db);
		if(!hs) {
			IFCImporter::LogError("expected IfcHalfSpaceSolid as second clipping operand");
			return;
		}

		const IFC::IfcPlane* const plane = hs->BaseSurface->ToPtr<IFC::IfcPlane>();
		if(!plane) {
			IFCImporter::LogError("expected IfcPlane as base surface for the IfcHalfSpaceSolid");
			return;
		}

		if(const IFC::IfcBooleanResult* const op0 = clip->FirstOperand->ResolveSelectPtr<IFC::IfcBooleanResult>(conv.db)) {
			ProcessBoolean(*op0,meshout,conv);
		}
		else if (const IFC::IfcSweptAreaSolid* const swept = clip->FirstOperand->ResolveSelectPtr<IFC::IfcSweptAreaSolid>(conv.db)) {
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
int ConvertShadingMode(const std::string& name)
{
	if (name == "BLINN") {
		return aiShadingMode_Blinn;
	}
	else if (name == "FLAT" || name == "NOTDEFINED") {
		return aiShadingMode_NoShading;
	}
	else if (name == "PHONG") {
		return aiShadingMode_Phong;
	}
	IFCImporter::LogWarn("shading mode "+name+" not recognized by Assimp, using Phong instead");
	return aiShadingMode_Phong;
}

// ------------------------------------------------------------------------------------------------
void FillMaterial(MaterialHelper* mat,const IFC::IfcSurfaceStyle* surf,ConversionData& conv) 
{
	aiString name;
	name.Set((surf->Name? surf->Name.Get() : "IfcSurfaceStyle_Unnamed"));
	mat->AddProperty(&name,AI_MATKEY_NAME);

	// now see which kinds of surface information are present
	BOOST_FOREACH(boost::shared_ptr< const IFC::IfcSurfaceStyleElementSelect > sel2, surf->Styles) {
		if (const IFC::IfcSurfaceStyleShading* shade = sel2->ResolveSelectPtr<IFC::IfcSurfaceStyleShading>(conv.db)) {
			aiColor4D col_base,col;

			ConvertColor(col_base, shade->SurfaceColour);
			mat->AddProperty(&col_base,1, AI_MATKEY_COLOR_DIFFUSE);

			if (const IFC::IfcSurfaceStyleRendering* ren = shade->ToPtr<IFC::IfcSurfaceStyleRendering>()) {

				if (ren->Transparency) {
					const float t = 1.f-ren->Transparency.Get();
					mat->AddProperty(&t,1, AI_MATKEY_OPACITY);
				}

				if (ren->DiffuseColour) {
					ConvertColor(col, *ren->DiffuseColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_DIFFUSE);
				}

				if (ren->SpecularColour) {
					ConvertColor(col, *ren->SpecularColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_SPECULAR);
				}

				if (ren->TransmissionColour) {
					ConvertColor(col, *ren->TransmissionColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_TRANSPARENT);
				}

				if (ren->ReflectionColour) {
					ConvertColor(col, *ren->ReflectionColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_REFLECTIVE);
				}

				const int shading = (ren->SpecularHighlight && ren->SpecularColour)?ConvertShadingMode(ren->ReflectanceMethod):aiShadingMode_Gouraud;
				mat->AddProperty(&shading,1, AI_MATKEY_SHADING_MODEL);

				if (ren->SpecularHighlight) {
					if(const EXPRESS::REAL* rt = ren->SpecularHighlight.Get()->ToPtr<EXPRESS::REAL>()) {
						// at this point we don't distinguish between the two distinct ways of
						// specifying highlight intensities. leave this to the user.
						const float e = *rt;
						mat->AddProperty(&e,1,AI_MATKEY_SHININESS);
					}
					else {
						IFCImporter::LogWarn("unexpected type error, SpecularHighlight should be a REAL");
					}
				}
			}
		}
		else if (const IFC::IfcSurfaceStyleWithTextures* tex = sel2->ResolveSelectPtr<IFC::IfcSurfaceStyleWithTextures>(conv.db)) {
			// XXX
		}
	}

}

// ------------------------------------------------------------------------------------------------
unsigned int ProcessMaterials(const IFC::IfcRepresentationItem& item, ConversionData& conv)
{
	if (conv.materials.empty()) {
		aiString name;
		std::auto_ptr<MaterialHelper> mat(new MaterialHelper());

		name.Set("<IFCDefault>");
		mat->AddProperty(&name,AI_MATKEY_NAME);

		aiColor4D col = aiColor4D(0.6f,0.6f,0.6f,1.0f);
		mat->AddProperty(&col,1, AI_MATKEY_COLOR_DIFFUSE);

		conv.materials.push_back(mat.release());
	}

	STEP::DB::RefMapRange range = conv.db.GetRefs().equal_range(item.GetID());
	for(;range.first != range.second; ++range.first) {
		if(const IFC::IfcStyledItem* const styled = conv.db.GetObject((*range.first).second)->ToPtr<IFC::IfcStyledItem>()) {
			BOOST_FOREACH(const IFC::IfcPresentationStyleAssignment& as, styled->Styles) {
				BOOST_FOREACH(boost::shared_ptr<const IFC::IfcPresentationStyleSelect> sel, as.Styles) {
			
					if (const IFC::IfcSurfaceStyle* const surf =  sel->ResolveSelectPtr<IFC::IfcSurfaceStyle>(conv.db)) {
						const std::string side = static_cast<std::string>(surf->Side);
						if (side != "BOTH") {
							IFCImporter::LogWarn("ignoring surface side marker on IFC::IfcSurfaceStyle: " + side);
						}

						std::auto_ptr<MaterialHelper> mat(new MaterialHelper());

						FillMaterial(mat.get(),surf,conv);
						
						conv.materials.push_back(mat.release());
						return conv.materials.size()-1;
					}
				}
			}
		}
	}
	return 0;
}

// ------------------------------------------------------------------------------------------------
bool ProcessTopologicalItem(const IFC::IfcTopologicalRepresentationItem& topo, std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	TempMesh meshtmp;
	if(const IFC::IfcConnectedFaceSet* fset = topo.ToPtr<IFC::IfcConnectedFaceSet>()) {
		ProcessConnectedFaceSet(*fset,meshtmp,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcTopologicalRepresentationItem entity, type is " + topo.GetClassName());
		return false;
	}

	meshtmp.RemoveAdjacentDuplicates();
	FixupFaceOrientation(meshtmp);

	aiMesh* const mesh = meshtmp.ToMesh();
	if(mesh) {
		mesh->mMaterialIndex = ProcessMaterials(topo,conv);
		mesh_indices.push_back(conv.meshes.size());
		conv.meshes.push_back(mesh);
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
bool ProcessGeometricItem(const IFC::IfcGeometricRepresentationItem& geo, std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	TempMesh meshtmp;
	if(const IFC::IfcShellBasedSurfaceModel* shellmod = geo.ToPtr<IFC::IfcShellBasedSurfaceModel>()) {
		BOOST_FOREACH(boost::shared_ptr<const IFC::IfcShell> shell,shellmod->SbsmBoundary) {
			try {
				const EXPRESS::ENTITY& e = shell->To<IFC::ENTITY>();
				const IFC::IfcConnectedFaceSet& fs = conv.db.MustGetObject(e).To<IFC::IfcConnectedFaceSet>(); 

				ProcessConnectedFaceSet(fs,meshtmp,conv);
			}
			catch(std::bad_cast&) {
				IFCImporter::LogWarn("unexpected type error, IfcShell ought to inherit from IfcConnectedFaceSet");
			}
		}
	}
	else if(const IFC::IfcSweptAreaSolid* swept = geo.ToPtr<IFC::IfcSweptAreaSolid>()) {
		ProcessSweptAreaSolid(*swept,meshtmp,conv);
	}
	else if(const IFC::IfcManifoldSolidBrep* brep = geo.ToPtr<IFC::IfcManifoldSolidBrep>()) {
		ProcessConnectedFaceSet(brep->Outer,meshtmp,conv);
	}
	else if(const IFC::IfcFaceBasedSurfaceModel* surf = geo.ToPtr<IFC::IfcFaceBasedSurfaceModel>()) {
		BOOST_FOREACH(const IFC::IfcConnectedFaceSet& fc, surf->FbsmFaces) {
			ProcessConnectedFaceSet(fc,meshtmp,conv);
		}
	}
	else if(const IFC::IfcBooleanResult* boolean = geo.ToPtr<IFC::IfcBooleanResult>()) {
		ProcessBoolean(*boolean,meshtmp,conv);
	}
	else if(const IFC::IfcBoundingBox* bb = geo.ToPtr<IFC::IfcBoundingBox>()) {
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
void AssignAddedMeshes(std::vector<unsigned int>& mesh_indices,aiNode* nd,ConversionData& conv)
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
bool TryQueryMeshCache(const IFC::IfcRepresentationItem& item, std::vector<unsigned int>& mesh_indices, ConversionData& conv) 
{
	ConversionData::MeshCache::const_iterator it = conv.cached_meshes.find(&item);
	if (it != conv.cached_meshes.end()) {
		std::copy((*it).second.begin(),(*it).second.end(),std::back_inserter(mesh_indices));
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void PopulateMeshCache(const IFC::IfcRepresentationItem& item, const std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	conv.cached_meshes[&item] = mesh_indices;
}

// ------------------------------------------------------------------------------------------------
bool ProcessRepresentationItem(const IFC::IfcRepresentationItem& item, std::vector<unsigned int>& mesh_indices, ConversionData& conv)
{
	if(const IFC::IfcTopologicalRepresentationItem* const topo = item.ToPtr<IFC::IfcTopologicalRepresentationItem>()) {
		if (!TryQueryMeshCache(item,mesh_indices,conv)) {
			if(ProcessTopologicalItem(*topo,mesh_indices,conv)) {
				if(mesh_indices.size()) {
					PopulateMeshCache(item,mesh_indices,conv);
				}
			}
			else return false;
		}
		return true;
	}
	else if(const IFC::IfcGeometricRepresentationItem* const geo = item.ToPtr<IFC::IfcGeometricRepresentationItem>()) {
		if (!TryQueryMeshCache(item,mesh_indices,conv)) {
			if(ProcessGeometricItem(*geo,mesh_indices,conv)) {
				if(mesh_indices.size()) {
					PopulateMeshCache(item,mesh_indices,conv);
				}
			} 
			else return false;
		}
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void ResolveObjectPlacement(aiMatrix4x4& m, const IFC::IfcObjectPlacement& place, ConversionData& conv)
{
	if (const IFC::IfcLocalPlacement* const local = place.ToPtr<IFC::IfcLocalPlacement>()){
		ConvertAxisPlacement(m, *local->RelativePlacement, conv);

		if (local->PlacementRelTo) {
			aiMatrix4x4 tmp;
			ResolveObjectPlacement(tmp,local->PlacementRelTo.Get(),conv);
			m = tmp * m;
		}
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcObjectPlacement entity, type is " + place.GetClassName());
	}
}

// ------------------------------------------------------------------------------------------------
void GetAbsTransform(aiMatrix4x4& out, const aiNode* nd, ConversionData& conv)
{
	aiMatrix4x4 t;
	if (nd->mParent) {
		GetAbsTransform(t,nd->mParent,conv);
	}
	out = t*nd->mTransformation;
}

// ------------------------------------------------------------------------------------------------
void ProcessMappedItem(const IFC::IfcMappedItem& mapped, aiNode* nd_src, std::vector< aiNode* >& subnodes_src, ConversionData& conv)
{
	// insert a custom node here, the cartesian transform operator is simply a conventional transformation matrix
	std::auto_ptr<aiNode> nd(new aiNode());
	nd->mName.Set("IfcMappedItem");
		
	// handle the cartesian operator
	aiMatrix4x4 m;
	ConvertTransformOperator(m, *mapped.MappingTarget);

	aiMatrix4x4 msrc;
	ConvertAxisPlacement(msrc,*mapped.MappingSource->MappingOrigin,conv);

	msrc = m*msrc;

	std::vector<unsigned int> meshes;
	const size_t old_openings = conv.collect_openings ? conv.collect_openings->size() : 0;
	if (conv.apply_openings) {
		aiMatrix4x4 minv = msrc;
		minv.Inverse();
		BOOST_FOREACH(TempOpening& open,*conv.apply_openings){
			open.Transform(minv);
		}
	}

	const IFC::IfcRepresentation& repr = mapped.MappingSource->MappedRepresentation;
	BOOST_FOREACH(const IFC::IfcRepresentationItem& item, repr.Items) {
		if(!ProcessRepresentationItem(item,meshes,conv)) {
			IFCImporter::LogWarn("skipping unknown mapped entity, type is " + item.GetClassName());
		}
	}

	AssignAddedMeshes(meshes,nd.get(),conv);
	if (conv.collect_openings) {

		// if this pass serves us only to collect opening geometry,
		// make sure we transform the TempMesh's which we need to
		// preserve as well.
		if(const size_t diff = conv.collect_openings->size() - old_openings) {
			for(size_t i = 0; i < diff; ++i) {
				(*conv.collect_openings)[old_openings+i].Transform(msrc);
			}
		}
	}

	nd->mTransformation =  nd_src->mTransformation * msrc;
	subnodes_src.push_back(nd.release());
}

// ------------------------------------------------------------------------------------------------
void ProcessProductRepresentation(const IFC::IfcProduct& el, aiNode* nd, std::vector< aiNode* >& subnodes, ConversionData& conv)
{
	if(!el.Representation) {
		return;
	}

	if(conv.settings.skipSpaceRepresentations) {
		if(const IFC::IfcSpace* const space = el.ToPtr<IFC::IfcSpace>()) {
			IFCImporter::LogWarn("skipping IfcSpace entity due to importer settings");
			return;
		}
	}

	std::vector<unsigned int> meshes;
	
	BOOST_FOREACH(const IFC::IfcRepresentation& repr, el.Representation.Get()->Representations) {
		if (conv.settings.skipCurveRepresentations && repr.RepresentationType && repr.RepresentationType.Get() == "Curve2D") {
			IFCImporter::LogWarn("skipping Curve2D representation item due to importer settings");
			continue;
		}
		BOOST_FOREACH(const IFC::IfcRepresentationItem& item, repr.Items) {
			if(const IFC::IfcMappedItem* const geo = item.ToPtr<IFC::IfcMappedItem>()) {
				ProcessMappedItem(*geo,nd,subnodes,conv);		
			}
			else {
				ProcessRepresentationItem(item,meshes,conv);
			}
		}
	}

	AssignAddedMeshes(meshes,nd,conv);
}

// ------------------------------------------------------------------------------------------------
aiNode* ProcessSpatialStructure(aiNode* parent, const IFC::IfcProduct& el, ConversionData& conv, std::vector<TempOpening>* collect_openings = NULL)
{
	const STEP::DB::RefMap& refs = conv.db.GetRefs();

	// add an output node for this spatial structure
	std::auto_ptr<aiNode> nd(new aiNode());
	nd->mName.Set(el.GetClassName()+"_"+(el.Name?el.Name:el.GlobalId));
	nd->mParent = parent;

	if(el.ObjectPlacement) {
		ResolveObjectPlacement(nd->mTransformation,el.ObjectPlacement.Get(),conv);
	}

	std::vector<TempOpening> openings;

	aiMatrix4x4 myInv;
	bool didinv = false;

	// convert everything contained directly within this structure,
	// this may result in more nodes.
	std::vector< aiNode* > subnodes;
	try {
		// locate aggregates and 'contained-in-here'-elements of this spatial structure and add them in recursively
		// on our way, collect openings in *this* element
		STEP::DB::RefMapRange range = refs.equal_range(el.GetID());

		for(STEP::DB::RefMapRange range2 = range; range2.first != range.second; ++range2.first) {
			const STEP::LazyObject& obj = conv.db.MustGetObject((*range2.first).second);

			// handle regularly-contained elements
			if(const IFC::IfcRelContainedInSpatialStructure* const cont = obj->ToPtr<IFC::IfcRelContainedInSpatialStructure>()) {
				BOOST_FOREACH(const IFC::IfcProduct& pro, cont->RelatedElements) {		
					if(const IFC::IfcOpeningElement* const open = pro.ToPtr<IFC::IfcOpeningElement>()) {
						// IfcOpeningElement is handled below. Sadly we can't use it here as is:
						// The docs say that opening elements are USUALLY attached to building storeys
						// but we want them for the building elements to which they belong to.
						continue;
					}
					
					subnodes.push_back( ProcessSpatialStructure(nd.get(),pro,conv,NULL) );
				}
			}
			// handle openings, which we collect in a list rather than adding them to the node graph
			else if(const IFC::IfcRelVoidsElement* const fills = obj->ToPtr<IFC::IfcRelVoidsElement>()) {
				if(fills->RelatingBuildingElement->GetID() == el.GetID()) {
					const IFC::IfcFeatureElementSubtraction& open = fills->RelatedOpeningElement;

					// move opening elements to a separate node since they are semantically different than elements that are just 'contained'
					std::auto_ptr<aiNode> nd_aggr(new aiNode());
					nd_aggr->mName.Set("$RelVoidsElement");
					nd_aggr->mParent = nd.get();

					nd_aggr->mTransformation = nd->mTransformation;

					nd_aggr->mNumChildren = 1;
					nd_aggr->mChildren = new aiNode*[1]();

					std::vector<TempOpening> openings_local;
					nd_aggr->mChildren[0] = ProcessSpatialStructure( nd_aggr.get(),open, conv,&openings_local);
					

					if(openings_local.size()) {
						if (!didinv) {
							myInv = aiMatrix4x4(nd->mTransformation ).Inverse();
							didinv = true;
						}

						// we need all openings to be in the local space of *this* node, so transform them
						BOOST_FOREACH(TempOpening& op,openings_local) {
							op.Transform( myInv*nd_aggr->mChildren[0]->mTransformation);
							openings.push_back(op);
						}
					}

					subnodes.push_back( nd_aggr.release() );
				}
			}
		}

		for(;range.first != range.second; ++range.first) {
			if(const IFC::IfcRelAggregates* const aggr = conv.db.GetObject((*range.first).second)->ToPtr<IFC::IfcRelAggregates>()) {

				// move aggregate elements to a separate node since they are semantically different than elements that are just 'contained'
				std::auto_ptr<aiNode> nd_aggr(new aiNode());
				nd_aggr->mName.Set("$RelAggregates");
				nd_aggr->mParent = nd.get();

				nd_aggr->mTransformation = nd->mTransformation;

				nd_aggr->mChildren = new aiNode*[aggr->RelatedObjects.size()]();
				BOOST_FOREACH(const IFC::IfcObjectDefinition& def, aggr->RelatedObjects) {
					if(const IFC::IfcProduct* const prod = def.ToPtr<IFC::IfcProduct>()) {
						nd_aggr->mChildren[nd_aggr->mNumChildren++] = ProcessSpatialStructure(nd_aggr.get(),*prod,conv,NULL);
					}
				}
			
				subnodes.push_back( nd_aggr.release() );
			}
		}

		conv.collect_openings = collect_openings;
		if(!conv.collect_openings) {
			conv.apply_openings = &openings;
		}

		ProcessProductRepresentation(el,nd.get(),subnodes,conv);
		conv.apply_openings = conv.collect_openings = NULL;

		if (subnodes.size()) {
			nd->mChildren = new aiNode*[subnodes.size()]();
			BOOST_FOREACH(aiNode* nd2, subnodes) {
				nd->mChildren[nd->mNumChildren++] = nd2;
				nd2->mParent = nd.get();
			}
		}
	}
	catch(...) {
		// it hurts, but I don't want to pull boost::ptr_vector into -noboost only for these few spots here
		std::for_each(subnodes.begin(),subnodes.end(),delete_fun<aiNode>());
		throw;
	}

	return nd.release();
}

// ------------------------------------------------------------------------------------------------
void ProcessSpatialStructures(ConversionData& conv)
{
	// XXX add support for multiple sites (i.e. IfcSpatialStructureElements with composition == COMPLEX)


	// process all products in the file. it is reasonable to assume that a
	// file that is relevant for us contains at least a site or a building.
	const STEP::DB::ObjectMapByType& map = conv.db.GetObjectsByType();

	ai_assert(map.find("ifcsite") != map.end());
	const STEP::DB::ObjectSet* range = &map.find("ifcsite")->second;

	if (range->empty()) {
		ai_assert(map.find("ifcbuilding") != map.end());
		range = &map.find("ifcbuilding")->second;
		if (range->empty()) {
			// no site, no building -  fail;
			IFCImporter::ThrowException("no root element found (expected IfcBuilding or preferably IfcSite)");
		}
	}

	
	BOOST_FOREACH(const STEP::LazyObject* lz, *range) {
		const IFC::IfcSpatialStructureElement* const prod = lz->ToPtr<IFC::IfcSpatialStructureElement>();
		if(!prod) {
			continue;
		}
		IFCImporter::LogDebug("looking at spatial structure `" + (prod->Name ? prod->Name.Get() : "unnamed") + "`" + (prod->ObjectType? " which is of type " + prod->ObjectType.Get():""));
	
		// the primary site is referenced by an IFCRELAGGREGATES element which assigns it to the IFCPRODUCT
		const STEP::DB::RefMap& refs = conv.db.GetRefs();
		STEP::DB::RefMapRange range = refs.equal_range(conv.proj.GetID());
		for(;range.first != range.second; ++range.first) {
			if(const IFC::IfcRelAggregates* const aggr = conv.db.GetObject((*range.first).second)->ToPtr<IFC::IfcRelAggregates>()) {
			
				BOOST_FOREACH(const IFC::IfcObjectDefinition& def, aggr->RelatedObjects) {
					// comparing pointer values is not sufficient, we would need to cast them to the same type first
					// as there is multiple inheritance in the game.
					if (def.GetID() == prod->GetID()) { 
						IFCImporter::LogDebug("selecting this spatial structure as root structure");
						// got it, this is the primary site.
						conv.out->mRootNode = ProcessSpatialStructure(NULL,*prod,conv,NULL);
						return;
					}
				}

			}
		}
	}

	
	IFCImporter::LogWarn("failed to determine primary site element, taking the first IfcSite");
	BOOST_FOREACH(const STEP::LazyObject* lz, *range) {
		const IFC::IfcSpatialStructureElement* const prod = lz->ToPtr<IFC::IfcSpatialStructureElement>();
		if(!prod) {
			continue;
		}

		conv.out->mRootNode = ProcessSpatialStructure(NULL,*prod,conv,NULL);
		return;
	}

	IFCImporter::ThrowException("failed to determine primary site element");
}

// ------------------------------------------------------------------------------------------------
void MakeTreeRelative(aiNode* start, const aiMatrix4x4& combined)
{
	// combined is the parent's absolute transformation matrix
	aiMatrix4x4 old = start->mTransformation;

	if (!combined.IsIdentity()) {
		start->mTransformation = aiMatrix4x4(combined).Inverse() * start->mTransformation;
	}

	// All nodes store absolute transformations right now, so we need to make them relative
	for (unsigned int i = 0; i < start->mNumChildren; ++i) {
		MakeTreeRelative(start->mChildren[i],old);
	}
}

// ------------------------------------------------------------------------------------------------
void MakeTreeRelative(ConversionData& conv)
{
	MakeTreeRelative(conv.out->mRootNode,aiMatrix4x4());
}

} // !anon



#endif
