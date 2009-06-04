/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
 */

package assimp;

/**
 * Describes a virtual light source in the scene
 * 
 * Lights have a representation in the node graph and can be animated
 * 
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Light {

	/**
	 * Enumerates all supported types of light sources
	 */
	public class Type {
		// public static final int UNDEFINED = 0x0;

		/**
		 * A directional light source has a well-defined direction but is
		 * infinitely far away. That's quite a good approximation for sun light.
		 * */
		public static final int DIRECTIONAL = 0x1;

		/**
		 * A point light source has a well-defined position in space but no
		 * direction - it emmits light in all directions. A normal bulb is a
		 * point light.
		 * */
		public static final int POINT = 0x2;

		/**
		 * A spot light source emmits light in a specific angle. It has a
		 * position and a direction it is pointing to. A good example for a spot
		 * light is a light spot in sport arenas.
		 * */
		public static final int SPOT = 0x3;
	};


	/**
	 * Get the name of the light source.
	 * 
	 * There must be a node in the scenegraph with the same name. This node
	 * specifies the position of the light in the scene hierarchy and can be
	 * animated.
	 */
	public final String GetName() {
		return mName;
	}

	/**
	 * Get the type of the light source.
	 * 
	 */
	public final Type GetType() {
		return mType;
	}

	/**
	 * Get the position of the light source in space. Relative to the
	 * transformation of the node corresponding to the light.
	 * 
	 * The position is undefined for directional lights.
	 * 
	 * @return Component order: x,y,z
	 */
	public final float[] GetPosition() {
		return mPosition;
	}

	/**
	 * Get the direction of the light source in space. Relative to the
	 * transformation of the node corresponding to the light.
	 * 
	 * The direction is undefined for point lights. The vector may be
	 * normalized, but it needn't.
	 * 
	 * @return Component order: x,y,z
	 */
	public final float[] GetDirection() {
		return mDirection;
	}

	/**
	 * Get the constant light attenuation factor.
	 * 
	 * The intensity of the light source at a given distance 'd' from the
	 * light's position is
	 * 
	 * @code Atten = 1/( att0 + att1 * d + att2 * d*d)
	 * @endcode This member corresponds to the att0 variable in the equation.
	 */
	public final float GetAttenuationConstant() {
		return mAttenuationConstant;
	}

	/**
	 * Get the linear light attenuation factor.
	 * 
	 * The intensity of the light source at a given distance 'd' from the
	 * light's position is
	 * 
	 * @code Atten = 1/( att0 + att1 * d + att2 * d*d)
	 * @endcode This member corresponds to the att1 variable in the equation.
	 */
	public final float GetAttenuationLinear() {
		return mAttenuationLinear;
	}

	/**
	 * Get the quadratic light attenuation factor.
	 * 
	 * The intensity of the light source at a given distance 'd' from the
	 * light's position is
	 * 
	 * @code Atten = 1/( att0 + att1 * d + att2 * d*d)
	 * @endcode This member corresponds to the att2 variable in the equation.
	 */
	public final float GetAttenuationQuadratic() {
		return mAttenuationQuadratic;
	}

	/**
	 * Get the diffuse color of the light source
	 * 
	 * 
	 */
	public final float[] GetColorDiffuse() {
		return mColorDiffuse;
	}

	/**
	 * Get the specular color of the light source
	 * 
	 * The specular light color is multiplied with the specular material color
	 * to obtain the final color that contributes to the specular shading term.
	 */
	public final float[] GetColorSpecular() {
		return mColorSpecular;
	}

	/**
	 * Get the ambient color of the light source
	 * 
	 * The ambient light color is multiplied with the ambient material color to
	 * obtain the final color that contributes to the ambient shading term. Most
	 * renderers will ignore this value it, is just a remaining of the
	 * fixed-function pipeline that is still supported by quite many file
	 * formats.
	 */
	public final float[] GetColorAmbient() {
		return mColorAmbient;
	}

	/**
	 * Get the inner angle of a spot light's light cone.
	 * 
	 * The spot light has maximum influence on objects inside this angle. The
	 * angle is given in radians. It is 2PI for point lights and undefined for
	 * directional lights.
	 */
	public final float GetAngleInnerCone() {
		return mAngleInnerCone;
	}

	/**
	 * Get the outer angle of a spot light's light cone.
	 * 
	 * The spot light does not affect objects outside this angle. The angle is
	 * given in radians. It is 2PI for point lights and undefined for
	 * directional lights. The outer angle must be greater than or equal to the
	 * inner angle. It is assumed that the application uses a smooth
	 * interpolation between the inner and the outer cone of the spot light.
	 */
	public final float GetAngleOuterCone() {
		return mAngleOuterCone;
	}
	
	/**
	 * The name of the light source.
	 */
	private String mName;

	/**
	 * The type of the light source.
	 */
	private Type mType;

	/**
	 * Position of the light source in space.
	 */
	private float[] mPosition;

	/**
	 * Direction of the light source in space.
	 */
	private float[] mDirection;

	/**
	 * Constant light attenuation factor.
	 */
	private float mAttenuationConstant;

	/**
	 * Linear light attenuation factor.
	 */
	private float mAttenuationLinear;

	/**
	 * Quadratic light attenuation factor.
	 */
	private float mAttenuationQuadratic;

	/**
	 * Diffuse color of the light source
	 */
	private float[] mColorDiffuse;

	/**
	 * Specular color of the light source
	 */
	private float[] mColorSpecular;

	/**
	 * Ambient color of the light source
	 */
	private float[] mColorAmbient;

	/**
	 * Inner angle of a spot light's light cone.
	 */
	private float mAngleInnerCone;

	/**
	 * Outer angle of a spot light's light cone.
	 */
	private float mAngleOuterCone;
};