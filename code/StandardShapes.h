/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

/** @file Declares a helper class, "StandardShapes" which generates
 *  vertices for standard shapes, such as cylnders, cones, spheres ..
 */
#ifndef AI_STANDARD_SHAPES_H_INC
#define AI_STANDARD_SHAPES_H_INC

#include <vector>

namespace Assimp	{

// ---------------------------------------------------------------------------
/** \brief Helper class to generate vertex buffers for standard geometric
 *  shapes, such as cylinders, cones, boxes, spheres, elipsoids ... .
 */
class ASSIMP_API StandardShapes
{
	// class cannot be instanced
	StandardShapes() {}

public:

	/** @brief Generates a hexahedron (cube)
	 *
	 *  Hexahedrons can be scaled on all axes.
	 *  @param center Center point of the hexahedron
	 *  @param length Radius of the hexahedron 
	 *  @param positions Receives output triangles.
	 */
	static void MakeHexahedron(aiVector3D& center,const aiVector3D& length,
		std::vector<aiVector3D>& positions);


	/** @brief Generates an icosahedron
	 *
	 *  @param center Center point of the icosahedron
	 *  @param length Radius of the icosahedron
	 *  @param positions Receives output triangles.
	 */
	static void MakeIcosahedron(aiVector3D& center,const aiVector3D& length,
		std::vector<aiVector3D>& positions);


	/** @brief Generates a dodecahedron
	 *
	 *  @param center Center point of the dodecahedron
	 *  @param length Radius of the dodecahedron
	 *  @param positions Receives output triangles
	 */
	static void MakeDodecahedron(aiVector3D& center,const aiVector3D& length,
		std::vector<aiVector3D>& positions);

	/** @brief Generates an octahedron
	 *
	 *  @param center Center point of the octahedron
	 *  @param length Radius of the octahedron
	 *  @param positions Receives output triangles.
	 */
	static void MakeOctahedron(aiVector3D& center,const aiVector3D& length,
		std::vector<aiVector3D>& positions);

	/** @brief Generates a tetrahedron
	 *
	 *  @param center Center point of the tetrahedron
	 *  @param length Radius of the octahedron
	 *  @param positions Receives output triangles.
	 */
	static void MakeTetrahedron(aiVector3D& center,const aiVector3D& length,
		std::vector<aiVector3D>& positions);


	/** @brief Generates a sphere
	 *
	 *  @param center Center point of the sphere
	 *  @param radius Radius of the sphere
	 *  @param tess Number of subdivions - 0 generates a octahedron
	 *  @param positions Receives output triangles.
	 */
	static void MakeSphere(aiVector3D& center,float length,unsigned int tess,
		std::vector<aiVector3D>& positions);

	/** @brief Generates a cone or a cylinder, either opened or closed.
	 *
	 *  @code
	 *
	 *       |-----|       <- radius 1
	 *
	 *        __x__        <- center 1
	 *       /     \  
	 *      /       \
	 *     /         \
	 *    /	          \
	 *   /______x______\   <- center 2
	 *
	 *   |-------------|   <- radius 2
	 *
	 *  @endcode
	 *
	 *  @param center1 First center point
	 *  @param radius1 First radius
	 *  @param center2 Second center point
	 *  @param radius2 Second radius
	 *  @param tess Number of subdivisions
	 *  @param bOpened true for an open cone/cylinder.
	 *  @param positions Receives output triangles.
	 */
	static void MakeCone(aiVector3D& center1,float radius1,
		aiVector3D& center2,float radius2,unsigned int tess, 
		std::vector<aiVector3D>& positions,bool bOpened = false);

	/** @brief Generates a flat circle
	 *
	 *  @param center Center point of the circle
	 *  @param radius Radius of the circle
	 *  @param normal Normal vector of the circle.
	 *    This is also the normal vector of all triangles generated by
	 *    this function.
	 *  @param tess Number of triangles
	 *  @param positions Receives output triangles.
	 */
	static void MakeCircle(aiVector3D& center, aiVector3D& normal, 
		float radius, unsigned int tess,
		std::vector<aiVector3D>& positions);


	// simplified versions - the radius is a single float and applies
	// to all axes. These version of the functions must be used if ou 
	// REALLY want a platonic primitive :-)
	static void MakeHexahedron(aiVector3D& center,float length,
		std::vector<aiVector3D>& positions)
	{
		MakeHexahedron(center,aiVector3D(length),positions);
	}

	static void MakeDodecahedron(aiVector3D& center,float length,
		std::vector<aiVector3D>& positions)
	{
		MakeDodecahedron(center,aiVector3D(length),positions);
	}

	static void MakeOcathedron(aiVector3D& center,float length,
		std::vector<aiVector3D>& positions)
	{
		MakeOctahedron(center,aiVector3D(length),positions);
	}

	static void MakeTetrahedron(aiVector3D& center,float length,
		std::vector<aiVector3D>& positions)
	{
		MakeTetrahedron(center,aiVector3D(length),positions);
	}

	static void MakeIcosahedron(aiVector3D& center,float length,
		std::vector<aiVector3D>& positions)
	{
		MakeIcosahedron(center,aiVector3D(length),positions);
	}
	
};
} // ! Assimp

#endif // !! AI_STANDARD_SHAPES_H_INC