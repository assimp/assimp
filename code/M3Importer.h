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
#ifndef AI_M3LOADER_H_INCLUDED
#define AI_M3LOADER_H_INCLUDED

#include <vector>

namespace Assimp {

namespace M3 {

// ------------------------------------------------------------------------------------------------
// The following data definitions are from http://code.google.com/p/libm3/, many thanks for that 
// help.
// ------------------------------------------------------------------------------------------------
typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned int uint32;
typedef int int32;

class Vec3D 
{
public:
    float x,y,z;

    Vec3D(float x0 = 0.0f, float y0 = 0.0f, float z0 = 0.0f) : x(x0), y(y0), z(z0) {}

    Vec3D(const Vec3D& v) : x(v.x), y(v.y), z(v.z) {}

    void reset() 
	{
		x = y = z = 0.0f;
    }

    Vec3D& operator= (const Vec3D &v) 
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
    }

    Vec3D operator+ (const Vec3D &v) const
    {
		Vec3D r(x+v.x,y+v.y,z+v.z);
		return r;
    }

	Vec3D operator- (const Vec3D &v) const
	{
		Vec3D r(x-v.x,y-v.y,z-v.z);
		return r;
	}

    float operator* (const Vec3D &v) const
    {
		return x*v.x + y*v.y + z*v.z;
    }

    Vec3D operator* (float d) const
    {
            Vec3D r(x*d,y*d,z*d);
    return r;
    }

    Vec3D operator/ (float d) const
    {
            Vec3D r(x/d,y/d,z/d);
    return r;
    }

    friend Vec3D operator* (float d, const Vec3D& v)
    {
		return v * d;
    }

    // Cross Product
    Vec3D operator% (const Vec3D &v) const
    {
		Vec3D r(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);
				return r;
    }

    Vec3D& operator+= (const Vec3D &v)
    {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
    }

    Vec3D& operator-= (const Vec3D &v)
    {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
    }

    Vec3D& operator*= (float d)
    {
		x *= d;
		y *= d;
		z *= d;
		return *this;
    }

    float lengthSquared() const
    {
		return x*x+y*y+z*z;
    }

    float length() const
    {
	    return sqrtf(x*x+y*y+z*z);
    }

    Vec3D& normalize()
    {
        this->operator*= (1.0f/length());
        return *this;
    }

    Vec3D operator~ () const
    {
		Vec3D r(*this);
		r.normalize();
		return r;
    }

    operator float*()
    {
		return (float*)this;
    }
};


class Vec2D 
{
public:
    float x,y;
    
    Vec2D(float x0 = 0.0f, float y0 = 0.0f) : x(x0), y(y0) {}

    Vec2D(const Vec2D& v) : x(v.x), y(v.y) {}

    Vec2D& operator= (const Vec2D &v) 
	{
		x = v.x;
		y = v.y;
		return *this;
    }

    Vec2D operator+ (const Vec2D &v) const
    {
		Vec2D r(x+v.x,y+v.y);
		return r;
    }

    Vec2D operator- (const Vec2D &v) const
    {
		Vec2D r(x-v.x,y-v.y);
		return r;
    }

    float operator* (const Vec2D &v) const
    {
	    return x*v.x + y*v.y;
    }

    Vec2D operator* (float d) const
    {
		Vec2D r(x*d,y*d);
		return r;
    }

    friend Vec2D operator* (float d, const Vec2D& v)
    {
		return v * d;
    }

    Vec2D& operator+= (const Vec2D &v)
    {
		x += v.x;
		y += v.y;
		return *this;
    }

    Vec2D& operator-= (const Vec2D &v)
    {
		x -= v.x;
		y -= v.y;
		return *this;
    }

    Vec2D& operator*= (float d)
    {
		x *= d;
		y *= d;
		return *this;
    }

    float lengthSquared() const
    {
		return x*x+y*y;
    }

    float length() const
    {
		return sqrtf(x*x+y*y);
    }

    Vec2D& normalize()
    {
		this->operator*= (1.0f/length());
		return *this;
    }

    Vec2D operator~ () const
    {
		Vec2D r(*this);
		r.normalize();
		return r;
    }

    operator float*()
    {
		return (float*)this;
    }
};

inline 
void rotate(float x0, float y0, float *x, float *y, float angle)
{
    float xa = *x - x0, ya = *y - y0;
    *x = xa*cosf(angle) - ya*sinf(angle) + x0;
    *y = xa*sinf(angle) + ya*cosf(angle) + y0;
}

struct Reference 
{
	uint32 nEntries;	// Code 0x00
	uint32 ref;			// Code 0x04
};

struct ReferenceEntry
{
	char id[ 4 ];		// Code 0x00
	uint32 offset;		// Code 0x04
	uint32 nEntries;	// Code 0x08
	uint32 type;		// Code 0x0C
};

struct MD33
{
	char id[4];			// Code 0x00
	uint32 ofsRefs;		// Code 0x04
	uint32 nRefs;		// Code 0x08
	Reference MODL;		// Code 0x0C
};

enum ModelType
{
	Type1 = 20,
	Type2 = 23
};

enum VertexFormat
{
	Vertex_Standard,
	Vertex_Extended
};

struct MODL23
{
	Reference name;				// Code 0x00
	uint32 version;				// Code 0x08
	Reference sequenceHeader;	// Code 0x0C
	Reference sequenceData;		// Code 0x14
	Reference sequenceLookup;	// Code 0x1C
	uint32 d2;					// Code 0x24
	uint32 d3;					// Code 0x28
	uint32 d4;					// Code 0x2C
	Reference STS;				// Code 0x30
	Reference bones;			// Code 0x38
	uint32 d5;					// Code 0x40
	uint32 flags;				// Code 0x44
	Reference vertexData;		// Code 0x48
	Reference views;			// Code 0x50
	Reference B;				// Code 0x58

	Vec3D extents[2];			// Code 0x60
	float radius;				// Code 0x78

	uint32 d7;					// Code 0x7C
	uint32 d8;					// Code 0x80
	uint32 d9;					// Code 0x84
	uint32 d10;					// Code 0x88
	uint32 d11;					// Code 0x8C
	uint32 d12;					// Code 0x90
	uint32 d13;					// Code 0x94
	uint32 d14;					// Code 0x98
	uint32 d15;					// Code 0x9C
	uint32 d16;					// Code 0xA0
	uint32 d17;					// Code 0xA4
	uint32 d18;					// Code 0xA8
	uint32 d19;					// Code 0xAC

	Reference attachments;		// Code 0xB0
	Reference attachmentLookup;	// Code 0xB8
	Reference lights;			// Code 0xC0
	Reference SHBX;				// Code 0xC8
	Reference cameras;			// Code 0xD0
	Reference D;				// Code 0xD8
	Reference materialLookup;	// Code 0xE0
	Reference materials;		// Code 0xE8
	Reference DIS;				// Code 0xF0
	Reference CMP;				// Code 0xF8

	Reference TER;				// Code 0x10
	Reference VOL;				// Code 0x10
	uint32 d21;					// Code 0x11
	uint32 d22;					// Code 0x11
	Reference CREP;				// Code 0x11
	Reference PAR;				// Code 0x12
	Reference PARC;				// Code 0x12
	Reference RIB;				// Code 0x13
	Reference PROJ;				// Code 0x13
	Reference FOR;				// Code 0x14
	Reference WRP;				// Code 0x14
	uint32 d24;					// Code 0x15
	uint32 d25;					// Code 0x15
	Reference PHRB;				// Code 0x15
	uint32 d27;					// Code 0x16
	uint32 d28;					// Code 0x16
	uint32 d29;					// Code 0x16
	uint32 d30;					// Code 0x16
	uint32 d32;					// Code 0x17
	uint32 d33;					// Code 0x17
	Reference IKJT;				// Code 0x17
	uint32 d35;					// Code 0x18
	uint32 d36;					// Code 0x18
	Reference PATU;				// Code 0x18
	Reference TRGD;				// Code 0x19
	Reference IREF;				// Code 0x19
	Reference E;				// Code 0x1A
	float matrix[4][4];			// Code 0x1A
	Vec3D extent[2];			// Code 0x1E
	float rad;					// Code 0x20
	Reference SSGS;				// Code 0x20
	Reference ATVL;				// Code 0x20
	uint32 d61;					// Code 0x21
	Reference F;				// uint16, Code6 0x21
	Reference G;				// uint16, Code 0x21
	Reference BBSC;				// Code 0x22
	Reference TMD;				// Code 0x22
	uint32 d62;					// Code 0x23
	uint32 d63;					// Code 0x23
	uint32 d64;					// Code 0x23
};

struct MODL20
{
	Reference name;				// Code 0x00
	uint32 version;				// Code 0x08
	Reference sequenceHeader;	// Code 0x0C
	Reference sequenceData;		// Code 0x14
	Reference sequenceLookup;	// Code 0x1C
	uint32 d2;					// Code 0x24
	uint32 d3;					// Code 0x28
	uint32 d4;					// Code 0x2C
	Reference STS;				// Code 0x30
	Reference bones;			// Code 0x38
	uint32 d5;					// Code 0x44
	uint32 flags;				// Code 0x44
	Reference vertexData;		// uint8, Code 0x48
	Reference views;			// Code 0x50
	Reference B;				// uint16, Code 0x58

	Vec3D extents[2];			// Code 0x60
	float radius;				// Code 0x78

	uint32 d7;					// Code 0x7C
	uint32 d8;					// Code 0x80
	uint32 d9;					// Code 0x84
	uint32 d10;					// Code 0x88
	uint32 d11;					// Code 0x8C
	uint32 d12;					// Code 0x90
	uint32 d13;					// Code 0x94
	uint32 d14;					// Code 0x98
	uint32 d15;					// Code 0x9C
	uint32 d16;					// Code 0xA0
	uint32 d17;					// Code 0xA4
	uint32 d18;					// Code 0xA8
	uint32 d19;					// Code 0xAC

	Reference attachments;		// Code 0xB0
	Reference attachmentLookup; // uint16, Code 0xB8
	Reference lights;			// Code 0xC0
	Reference cameras;			// Code 0xC8
	Reference D;				// uint16, Code 0xD0
	Reference materialLookup;	// Code 0xD8
	Reference materials;		// Code 0xE0
	Reference DIS;				// Code 0xE8
	Reference CMP;				// Code 0xF0
	Reference TER;				// Code 0xF8

	uint32 d20;					// Code 0x10
	uint32 d21;					// Code 0x10
	uint32 d22;					// Code 0x10
	uint32 d23;					// Code 0x10
	Reference CREP;				// Code 0x11
	Reference PAR;				// Code 0x11
	Reference PARC;				// Code 0x12
	Reference RIB;				// Code 0x12
	Reference PROJ;				// Code 0x13
	Reference FOR;				// Code 0x13
	uint32 d25;					// Code 0x14
	uint32 d26;					// Code 0x14
	uint32 d27;					// Code 0x14
	uint32 d28;					// Code 0x14
	Reference PHRB;				// Code 0x15
	uint32 d30;					// Code 0x15
	uint32 d31;					// Code 0x15
	uint32 d32;					// Code 0x16
	uint32 d33;					// Code 0x16
	uint32 d34;					// Code 0x16
	uint32 d35;					// Code 0x16
	Reference IKJT;				// Code 0x17
	uint32 d36;					// Code 0x17
	uint32 d37;					// Code 0x17
	Reference PATU;				// Code 0x18
	Reference TRGD;				// Code 0x18
	Reference IREF;				// Code 0x19
	Reference E;				// int32, Code 0x19

	 float matrix[4][4];		// Code 0x1A
	 Vec3D extent[2];			// Code 0x1E
	 float rad;					// Code 0x1F

	 Reference SSGS;			// Code 0x1F
	 uint32 d38;				// Code 0x20
	 uint32 d39;				// Code 0x20
	 Reference BBSC;			// Code 0x20

	 uint32 d40;				// Code 0x21
	 uint32 d41;				// Code 0x21
	 uint32 d42;				// Code 0x21
	 uint32 d43;				// Code 0x22
	 uint32 d44;				// Code 0x22
};

struct BONE
{
	int32 d1; // Keybone?
	Reference name;
	uint32 flags;
	int16 parent;
	int16 s1;

	float floats[ 34 ];
};

struct VertexExt // 36 byte
{
	Vec3D pos;
	uint8 boneWeight[ 4 ];
	uint8 boneIndex[ 4 ];
	uint8 normal[ 4 ];  //normal_x = (float)normal[0]/255.0f...
	int16 uv[ 2 ];
	uint32 d1;
	uint8 tangent[ 4 ];
};

struct Vertex // 32 byte
{
	Vec3D pos;
	uint8 boneWeight[4];
	uint8 boneIndex[4];
	uint8 normal[4];  //normal_x = (float)normal[0]/255.0f...
	int16 uv[2];
	uint8 tangent[4];
};

struct MATM
{
	uint32 d1;
	uint32 d2; // Index into MAT-table?
};

struct MAT
{
	Reference name;
	int ukn1[ 8 ];
	float x, y;  //always 1.0f
	Reference layers[13];
	int ukn2[15];
};

struct LAYR
{
	int unk;
	Reference name;
	float unk2[85];
};

struct DIV
{
	Reference faces;	// Code 0x00
	Reference regions;	// Code 0x08
	Reference BAT;		// Code 0x10
	Reference MSEC;		// Code 0x18
};

struct Region 
{
	uint32 unk;
	uint16 ofsVertices;
	uint16 nVertices;
	uint32 ofsIndices;
	uint32 nIndices; // reference into DIV.faces
	uint8 unknown[12];
};

struct CAM
{
	int32 d1;		// Code 0x00
	Reference name;	// Code 0x04
	uint16 flags1;	// Code 0x0C
	uint16 flags2;	// Code 0x0E
};

struct EVNT
{
	Reference name;		// Code 0x00
	int16 unk1[4];		// Code 0x08
	float matrix[4][4];	// Code 0x10
	int32 unk2[4];		// Code 0x50
};

struct ATT
{
	int32 unk;		// Code 0x00
	Reference name;	// Code 0x04
	int32 bone;		// Code 0x0C
};

struct PHSH
{
	float m[ 4 ][ 4 ];
	float f1;
	float f2;
	Reference refs[ 5 ];
	float f3;
};

struct SEQS
{
	int32 d1;	// Code 0x00
	int32 d2;	// Code 0x04
	Reference name;	// Code 0x08
	int32 d3;	// Code 0x10
	uint32 length;	// Code 0x14
	int32 d4;	// Code 0x18
	uint32 flags;	// Code 0x1C
	int32 unk[5];	// Code 0x20
	Vec3D extents[2];	// Code 0x34
	float radius;	// Code 0x4C
	int32 d5;	// Code 0x50
	int32 d6;	// Code 0x54
};

struct STC
{
	Reference name;		// Code 0x00
	uint16 s1;			// Code 0x08
	uint16 s2;			// Code 0x0A
	uint16 s3;			// Code 0x0C
	uint16 s4;			// Code 0x0E
	Reference unk2;		// uint32	// Code 0x12
	Reference unk3;		// uint32	// Code 0x1A
	uint32 d3;			// Code 0x22
	Reference evt;		// Code 0x24
	Reference unk4[11]; // Seems to be transformation data	// Code 0x2C
	Reference bnds;		// Code 0x84
};

struct STS
{
	Reference unk1; // uint32	// Code 0x00
	int32 unk[3];	// Code 0x08
	int16 s1;		// Code 0x14
	int16 s2;		// Code 0x16
};

struct STG
{
	Reference name;		// Code 0x00
	Reference stcID;	// Code 0x08
};

struct SD
{
	Reference timeline;	// Code 0x00
	uint32 flags;		// Code 0x08
	uint32 length;		// Code 0x0C
	Reference data;		// Code 0x10
};

struct BNDS
{
	Vec3D extents1[2];		// Code 0x00
	float radius1;	// Code 0x18
	Vec3D extents2[2];	// Code 0x1C
	float radius2;	// Code 0x34
};

struct VEC2
{
	float x, y;
};

struct VEC3
{
	float x, y, z;
};

struct VEC4
{
	float x, y, z, w;
};

struct QUAT
{
	float x, y, z, w;
};

// ------------------------------------------------------------------------------------------------
/**	Loader to import M3-models.
 */
// ------------------------------------------------------------------------------------------------
class M3Importer : public BaseImporter
{
	friend class Importer;

public:
	///	@brief	The default constructor.
	M3Importer();

	///	@brief	The destructor.
	~M3Importer();

	/// @brief	Returns whether the class can handle the format of the given file. 
	/// @remark	See BaseImporter::CanRead() for details.
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig ) const;

private:
	const aiImporterDesc* GetInfo () const;
	void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler );
	void convertToAssimp( const std::string& pFile, aiScene* pScene, DIV *pViews, Region *pRegions, uint16 *pFaces, 
		const std::vector<aiVector3D> &vertices, const std::vector<aiVector3D> &uvCoords, const std::vector<aiVector3D> &normals );
	void createVertexData( aiMesh *pMesh, const std::vector<aiVector3D> &vertices, const std::vector<aiVector3D> &uvCoords,
		const std::vector<aiVector3D> &normals  );
	aiNode *createNode( aiNode *pParent );
	template<typename T>
	T* GetEntries( Reference ref );

private:
	MD33 *m_pHead;
	ReferenceEntry *m_pRefs;
	std::vector<unsigned char> m_Buffer;
};

// ------------------------------------------------------------------------------------------------
template<typename T>
inline
T* M3Importer::GetEntries( Reference ref )
{
	return (T*) ( &m_Buffer[ 0 ] + m_pRefs[ ref.ref ].offset );
}

// ------------------------------------------------------------------------------------------------

} // Namespace M3
} // Namespace Assimp

#endif // AI_M3LOADER_H_INCLUDED
