
/****************************************************************************
* MeshLab                                                           o o     *
* An extendible mesh processor                                    o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005, 2006                                          \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
 History
 $Log$
 Revision 1.2  2008/02/21 13:04:09  gianpaolopalma
 Fixed compile error

 Revision 1.1  2008/02/20 22:02:00  gianpaolopalma
 First working version of Vrml to X3D translator

*****************************************************************************/

#if !defined(VRML_PARSER_H__)
#define VRML_PARSER_H__

#include <QtXml>
#include <set>


#include "Scanner.h"

namespace VrmlTranslator {


class Errors {
public:
	int count;			// number of errors detected
	wchar_t* stringError;
	
	Errors();
	~Errors();
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Warning(const wchar_t *s);
	void Exception(const wchar_t *s);

}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_id=1,
		_intCont=2,
		_realCont=3,
		_string=4,
		_x3dVersion=5,
		_vrmlVersion=6,
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

QDomDocument *doc;
	
	std::map<QString, QString> defNode;
	
	std::set<QString> proto;
	
	std::set<QString> x3dNode;
	
	void InitX3dNode()
	{
	  x3dNode.insert("Arc2D"); x3dNode.insert("ArcClose2D"); x3dNode.insert("BallJoint");
	  x3dNode.insert("BooleanFilter"); x3dNode.insert("BooleanSequencer"); x3dNode.insert("BooleanToggle");
	  x3dNode.insert("BooleanTrigger"); x3dNode.insert("BoundedPhysicsModel"); x3dNode.insert("CADAssembly");
	  x3dNode.insert("CADFace"); x3dNode.insert("CADLayer"); x3dNode.insert("CADPart");
	  x3dNode.insert("Circle2D"); x3dNode.insert("ClipPlane"); x3dNode.insert("CollidableOffset");
	  x3dNode.insert("CollidableShape"); x3dNode.insert("CollisionCollection"); x3dNode.insert("CollisionSensor");
	  x3dNode.insert("CollisionSpace"); x3dNode.insert("ColorDamper");x3dNode.insert("ColorRGBA");
	  x3dNode.insert("ComposedCubeMapTexture"); x3dNode.insert("ComposedShader");
	  x3dNode.insert("ComposedTexture3D"); x3dNode.insert("ConeEmitter"); x3dNode.insert("Contact");
	  x3dNode.insert("Contour2D"); x3dNode.insert("ContourPolyline2D"); x3dNode.insert("CoordinateDamper");
	  x3dNode.insert("CoordinateDouble"); x3dNode.insert("CoordinateInterpolator2D");
	  x3dNode.insert("DISEntityManager"); x3dNode.insert("DISEntityTypeMapping");
	  x3dNode.insert("Disk2D"); x3dNode.insert("DoubleAxisHingeJoint"); x3dNode.insert("EaseInEaseOut");
	  x3dNode.insert("EspduTransform"); x3dNode.insert("ExplosionEmitter");
	  x3dNode.insert("FillProperties"); x3dNode.insert("FloatVertexAttribute");
	  x3dNode.insert("FogCoordinate"); x3dNode.insert(" GeneratedCubeMapTexture");
	  x3dNode.insert("GeoCoordinate"); x3dNode.insert("GeoElevationGrid"); x3dNode.insert("GeoLocation"); 
	  x3dNode.insert("GeoLOD"); x3dNode.insert("GeoMetadata"); x3dNode.insert("GeoOrigin");
	  x3dNode.insert("GeoPositionInterpolator"); x3dNode.insert("GeoProximitySensor");
	  x3dNode.insert("GeoTouchSensor"); x3dNode.insert("GeoViewpoint");	x3dNode.insert("GravityPhysicsModel");
	  x3dNode.insert("HAnimDisplacer"); x3dNode.insert("HAnimHumanoid"); x3dNode.insert("HAnimJoint");
	  x3dNode.insert("HAnimSegment"); x3dNode.insert("HAnimSite"); x3dNode.insert("ImageCubeMapTexture");
	  x3dNode.insert("ImageTexture3D"); x3dNode.insert("IndexedQuadSet"); x3dNode.insert("IndexedTriangleFanSet");
	  x3dNode.insert("IndexedTriangleSet"); x3dNode.insert("IndexedTriangleStripSet");
	  x3dNode.insert("IntegerSequencer"); x3dNode.insert("IntegerTrigger"); x3dNode.insert("KeySensor");
	  x3dNode.insert("Layer"); x3dNode.insert("LayerSet"); x3dNode.insert("Layout");
	  x3dNode.insert("LayoutGroup"); x3dNode.insert("LayoutLayer"); x3dNode.insert("LinePicker");
	  x3dNode.insert("LineProperties"); x3dNode.insert("LineSet"); x3dNode.insert("LoadSensor");
	  x3dNode.insert("LocalFog"); x3dNode.insert("Material"); x3dNode.insert("Matrix3VertexAttribute"); 
	  x3dNode.insert("Matrix4VertexAttribute"); x3dNode.insert("MetadataDouble");
	  x3dNode.insert("MetadataFloat"); x3dNode.insert("MetadataInteger"); x3dNode.insert("MetadataSet");
	  x3dNode.insert("MetadataString"); x3dNode.insert("MotorJoint"); x3dNode.insert("MultiTexture");
	  x3dNode.insert("MultiTextureCoordinate"); x3dNode.insert("MultiTextureTransform");
	  x3dNode.insert("NurbsCurve"); x3dNode.insert("NurbsCurve2D"); x3dNode.insert("NurbsOrientationInterpolator");
	  x3dNode.insert("NurbsPatchSurface"); x3dNode.insert("NurbsPositionInterpolator"); x3dNode.insert("NurbsSet");
	  x3dNode.insert("NurbsSurfaceInterpolator"); x3dNode.insert("NurbsSweptSurface");
	  x3dNode.insert("NurbsSwungSurface"); x3dNode.insert("NurbsTextureCoordinate");
	  x3dNode.insert("NurbsTrimmedSurface"); x3dNode.insert("OrientationChaser");
	  x3dNode.insert("OrientationDamper"); x3dNode.insert("OrthoViewpoint"); x3dNode.insert("PackagedShader");
	  x3dNode.insert("ParticleSystem"); x3dNode.insert("PickableGroup"); x3dNode.insert("PixelTexture3D");
	  x3dNode.insert("PointEmitter"); x3dNode.insert("PointPicker"); x3dNode.insert("PointSet");
	  x3dNode.insert("Polyline2D"); x3dNode.insert("PolylineEmitter"); x3dNode.insert("Polypoint2D");
	  x3dNode.insert("PositionChaser"); x3dNode.insert("PositionChaser2D"); x3dNode.insert("PositionDamper");
	  x3dNode.insert("PositionDamper2D"); x3dNode.insert("PositionInterpolator2D");
	  x3dNode.insert("PrimitivePicker"); x3dNode.insert("ProgramShader"); x3dNode.insert("QuadSet");
	  x3dNode.insert("ReceiverPdu"); x3dNode.insert("Rectangle2D"); x3dNode.insert("RigidBody");
	  x3dNode.insert("RigidBodyCollection"); x3dNode.insert("ScalarChaser"); x3dNode.insert("ScreenFontStyle");
	  x3dNode.insert("ScreenGroup"); x3dNode.insert("ShaderPart"); x3dNode.insert("ShaderProgram");
	  x3dNode.insert("SignalPdu"); x3dNode.insert("SingleAxisHingeJoint"); x3dNode.insert("SliderJoint");
	  x3dNode.insert("SplinePositionInterpolator");	x3dNode.insert("SplinePositionInterpolator2D");
	  x3dNode.insert("SplineScalarInterpolator"); x3dNode.insert("SquadOrientationInterpolator");
	  x3dNode.insert("StaticGroup"); x3dNode.insert("StringSensor"); x3dNode.insert("SurfaceEmitter");
	  x3dNode.insert("TexCoordDamper"); x3dNode.insert("TextureBackground");
	  x3dNode.insert("TextureCoordiante3D"); x3dNode.insert("TextureCoordinate4D");
	  x3dNode.insert("TextureCoordinateGenerator"); x3dNode.insert("TextureProperties");
	  x3dNode.insert("TextureTransformMatrix3D"); x3dNode.insert("TextureTransform3D");
	  x3dNode.insert("TimeTrigger"); x3dNode.insert("TransformSensor"); x3dNode.insert("TransmitterPdu");
	  x3dNode.insert("TriangleFanSet"); x3dNode.insert("TriangleSet"); x3dNode.insert("TriangleSet2D");
	  x3dNode.insert("TriangleStripSet"); x3dNode.insert("TwoSidedMaterial"); x3dNode.insert("UniversalJoint");
	  x3dNode.insert(" Viewpoint"); x3dNode.insert("ViewpointGroup"); x3dNode.insert("VolumeEmitter");
	  x3dNode.insert("VolumePicker"); x3dNode.insert("WindPhysicsModel"); x3dNode.insert("Cylinder"); x3dNode.insert("Sphere");
	}
	


	Parser(Scanner *scanner);
	~Parser();
	void SemErr(const wchar_t* msg);

	void VrmlTranslator();
	void HeaderStatement();
	void ProfileStatement();
	void ComponentStatements();
	void MetaStatements();
	void Statements(QDomElement& parent);
	void ProfileNameId();
	void ComponentStatement();
	void ComponentNameId();
	void ComponentSupportLevel();
	void ExportStatement();
	void NodeNameId(QString& str);
	void ExportedNodeNameId();
	void ImportStatement();
	void InlineNodeNameId();
	void MetaStatement();
	void Metakey();
	void Metavalue();
	void Statement(QDomElement& parent);
	void NodeStatement(QDomElement& parent);
	void ProtoStatement(QDomElement& parent);
	void RouteStatement();
	void Node(QDomElement& parent, QString& tagName, const QString defValue);
	void RootNodeStatement(QDomElement& parent);
	void Proto(QDomElement& parent);
	void Externproto(QDomElement& parent);
	void ProtoStatements(QDomElement& parent);
	void NodeTypeId(QString& str);
	void InterfaceDeclarations(QDomElement& parent);
	void ProtoBody(QDomElement& parent);
	void InterfaceDeclaration(QDomElement& parent);
	void RestrictedInterfaceDeclaration(QDomElement& parent);
	void FieldType(QString& str);
	void InputOnlyId(QString& str);
	void OutputOnlyId(QString& str);
	void InitializeOnlyId(QString& str);
	void FieldValue(QDomElement& parent, QString fieldName, bool flag);
	void FieldId(QString& str);
	void ExternInterfaceDeclarations(QDomElement& parent);
	void URLList(QString& url);
	void ExternInterfaceDeclaration(QDomElement& parent);
	void NodeBody(QDomElement& parent, bool flag);
	void ScriptBody();
	void NodeBodyElement(QDomElement& parent, bool flag);
	void ScriptBodyElement();
	void InputOutputId(QString& str);
	void SingleValue(QDomElement& parent, QString fieldName, bool flag);
	void MultiValue(QDomElement& parent, QString fieldName, bool flag);
	void MultiNumber(QString& value);
	void MultiString(QString& value);
	void MultiBool(QString& value);

	void Parse();

}; // end Parser

} // namespace


#endif // !defined(VRML_PARSER_H__)

