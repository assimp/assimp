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
 * A material key represents a single material property, accessed through its
 * key. Most material properties are predefined for simplicity.
 * 
 * Material properties can be of several data types, including integers, floats
 * strings and arrays of them.
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public final class MatKey {
	private MatKey() {}

	/**
	 * Enumerates all supported texture layer blending operations.
	 * 
	 * @see TEXOP()
	 */
	public static final class TextureOp {
		/** T = T1 * T2 */
		public static final int Multiply = 0x0;

		/** T = T1 + T2 */
		public static final int Add = 0x1;

		/** T = T1 - T2 */
		public static final int Subtract = 0x2;

		/** T = T1 / T2 */
		public static final int Divide = 0x3;

		/** T = (T1 + T2) - (T1 * T2) */
		public static final int SmoothAdd = 0x4;

		/** T = T1 + (T2-0.5) */
		public static final int SignedAdd = 0x5;
	}

	/**
	 * Enumerates all supported texture wrapping modes. They define how mapping
	 * coordinates outside the 0...1 range are handled.
	 * 
	 * @see TEXWRAP_U()
	 * @see TEXWRAP_V()
	 */
	public static final class TextureWrapMode {
		/**
		 * A texture coordinate u|v is translated to u%1|v%1
		 */
		public static final int Wrap = 0x0;

		/**
		 * Texture coordinates outside [0...1] are clamped to the nearest valid
		 * value.
		 */
		public static final int Clamp = 0x1;

		/**
		 * If the texture coordinates for a pixel are outside [0...1], the
		 * texture is not applied to that pixel.
		 */
		public static final int Decal = 0x3;

		/**
		 * A texture coordinate u|v becomes u%1|v%1 if (u-(u%1))%2 is zero and
		 * 1-(u%1)|1-(v%1) otherwise
		 */
		public static final int Mirror = 0x2;
	}

	/**
	 * Enumerates all supported texture mapping modes. They define how the
	 * texture is applied to the surface.
	 * 
	 * @see TEXMAP()
	 */
	public static final class TextureMapping {
		/**
		 * Explicit mapping coordinates are given. The source mapping channel to
		 * be used is defined in the <code>UVSRC</code> material property.
		 */
		public static final int UV = 0x0;

		/** Spherical mapping */
		public static final int SPHERE = 0x1;

		/** Cylindrical mapping */
		public static final int CYLINDER = 0x2;

		/** Cubic mapping */
		public static final int BOX = 0x3;

		/** Planar mapping */
		public static final int PLANE = 0x4;

		/** Undefined mapping. Have fun. */
		public static final int OTHER = 0x5;
	}

	/**
	 * Enumerates all recognized purposes of texture maps. Textures are layered
	 * in stacks, one stack for each kind of texture map.
	 */
	public static final class TextureType {
		/**
		 * Dummy value.
		 * 
		 * Value for <code>Any.textureType</code> for properties not related to
		 * a specific texture layer.
		 */
		public static final int NONE = 0x0;

		/**
		 * The texture is combined with the result of the diffuse lighting
		 * equation.
		 */
		public static final int DIFFUSE = 0x1;

		/**
		 * The texture is combined with the result of the specular lighting
		 * equation.
		 */
		public static final int SPECULAR = 0x2;

		/**
		 * The texture is combined with the result of the ambient lighting
		 * equation, if any.
		 */
		public static final int AMBIENT = 0x3;

		/**
		 * The texture is added to the final result of the lighting calculation.
		 * It isn't influenced by incoming or ambient light.
		 */
		public static final int EMISSIVE = 0x4;

		/**
		 * The texture is a height map.
		 * 
		 * Usually higher grey-scale values map to higher elevations.
		 * Applications will typically want to convert height maps to tangent
		 * space normal maps.
		 */
		public static final int HEIGHT = 0x5;

		/**
		 * The texture is a tangent space normal-map.
		 * 
		 * There are many conventions, but usually you can expect the data to be
		 * given in the r,g channels of the texture where the b or a channel is
		 * probably containing the original height map.
		 */
		public static final int NORMALS = 0x6;

		/**
		 * The texture defines the glossiness of the material.
		 * 
		 * The glossiness is nothing else than the exponent of the specular
		 * (phong) lighting equation. Usually there is a conversion function
		 * defined to map the linear color values in the texture to a suitable
		 * exponent. Have fun.
		 */
		public static final int SHININESS = 0x7;

		/**
		 * The texture defines per-pixel opacity.
		 * 
		 * Usually 'white' means opaque and 'black' means 'transparency'. Or
		 * quite the opposite. Have fun.
		 */
		public static final int OPACITY = 0x8;

		/**
		 * Displacement texture
		 * 
		 * The exact purpose and format is application-dependent. Higher color
		 * values usually map to higher displacements.
		 */
		public static final int DISPLACEMENT = 0x9;

		/**
		 * Lightmap texture (or Ambient Occlusion Map)
		 * 
		 * Both 'lightmaps' in the classic sense and dedicated 'ambient
		 * occlusion maps' are covered by this material property. The texture
		 * contains a scaling value for the final color value of a pixel. It's
		 * intensity is not affected by incoming or ambient light.
		 */
		public static final int LIGHTMAP = 0xA;

		/**
		 * Reflection texture
		 * 
		 * Defines the color of a perfect mirror reflection at a particular
		 * pixel.
		 */
		public static final int REFLECTION = 0xB;

		/**
		 * Unknown texture for your amusement.
		 * 
		 * A texture reference that does not match any of the definitions above
		 * is considered to be 'unknown'. It is still imported, but is excluded
		 * from any further post processing.
		 */
		public static final int UNKNOWN = 0xC;
	};

	/**
	 * Defines some standard shading hints provided by Assimp. In order to match
	 * the intended visual appearance of a model as closely as possible,
	 * applications will need to apply the right shader to the model or at least
	 * find a similar one.
	 * 
	 * The list of shaders comes from Blender, btw.
	 */
	public static final class ShadingMode {
		/**
		 * Flat shading. Shading is done on per-face base, diffuse only. Also
		 * known as 'faceted shading'.
		 */
		public static final int Flat = 0x1;

		/**
		 * Simple Gouraud Shader
		 */
		public static final int Gouraud = 0x2;

		/**
		 * Phong Shader
		 */
		public static final int Phong = 0x3;

		/**
		 * Phong-Blinn Shader
		 */
		public static final int Blinn = 0x4;

		/**
		 * Toon Shader
		 * 
		 * Also known as 'comic' shader.
		 */
		public static final int Toon = 0x5;

		/**
		 * OrenNayar Shader
		 * 
		 * Extension to standard Gouraud shading taking the roughness of the
		 * material into account
		 */
		public static final int OrenNayar = 0x6;

		/**
		 * Minnaert Shader
		 * 
		 * Extension to standard Gouraud shading taking the "darkness" of the
		 * material into account
		 */
		public static final int Minnaert = 0x7;

		/**
		 * CookTorrance Shader.
		 * 
		 * Special shader for metal surfaces.
		 */
		public static final int CookTorrance = 0x8;

		/**
		 * No shading at all. Constant light influence of 1.0.
		 */
		public static final int NoShading = 0x9;

		/**
		 * Fresnel Shader
		 */
		public static final int Fresnel = 0xa;
	};

	/**
	 * Some mixed texture flags. They are only present in a material if Assimp
	 * gets very, very detailed information about the characteristics of a
	 * texture from the source file. In order to display completely generic
	 * models properly, you'll need to query them.
	 * 
	 * @see TEXFLAGS()
	 */
	public static final class TextureFlags {
		/**
		 * The texture's color values have to be inverted (component wise 1-n)
		 */
		public static final int Invert = 0x1;

		/**
		 * Explicit request to the application to process the alpha channel of
		 * the texture.
		 * 
		 * Mutually exclusive with <code>IgnoreAlpha</code>. These flags are set
		 * if the library can say for sure that the alpha channel is used/is not
		 * used. If the model format does not define this, it is left to the
		 * application to decide whether the texture alpha channel - if any - is
		 * evaluated or not.
		 */
		public static final int UseAlpha = 0x2;

		/**
		 * Explicit request to the application to ignore the alpha channel of
		 * the texture.
		 * 
		 * Mutually exclusive with <code>UseAlpha</code>.
		 */
		public static final int IgnoreAlpha = 0x4;
	}

	/**
	 * Defines how the computed value for a particular pixel is combined with
	 * the previous value of the backbuffer at that position.
	 * 
	 * @see #BLEND_FUNC()
	 */
	public static final class BlendFunc {

		/** SourceColor*SourceAlpha + DestColor*(1-SourceAlpha) */
		public static final int Default = 0x0;

		/** SourceColor*1 + DestColor*1 */
		public static final int Additive = 0x1;
	}

	/**
	 * Defines the name of the material.
	 */
	public static final Any<String> NAME = new Any<String>("?mat.name");

	/**
	 * Defines whether the material must be rendered two-sided. This is a
	 * boolean property. n != 0 is <code>true</code>.
	 */
	public static final Any<Integer> TWOSIDED = new Any<Integer>(
			"$mat.twosided");

	/**
	 * Defines whether the material must be rendered in wireframe. This is a
	 * boolean property. n != 0 is <code>true</code>.
	 */
	public static final Any<Integer> WIREFRAME = new Any<Integer>(
			"$mat.wireframe");

	/**
	 * Defines the shading model to be used to compute the lighting for the
	 * material. This is one of the values defined in the
	 * <code>ShadingModel</code> 'enum'.
	 */
	public static final Any<Integer> SHADING_MODEL = new Any<Integer>(
			"$mat.shadingm");

	/**
	 * Defines the blend function to be used to combine the computed color value
	 * of a pixel with the previous value in the backbuffer. This is one of the
	 * values defined in the <code>BlendFunc</code> 'enum'.
	 */
	public static final Any<Integer> BLEND_FUNC = new Any<Integer>("$mat.blend");

	/**
	 * Defines the basic opacity of the material in range 0..1 where 0 is fully
	 * transparent and 1 is fully opaque. The default value to be taken if this
	 * property is not defined is naturally <code>1.0f</code>.
	 */
	public static final Any<Float> OPACITY = new Any<Float>("$mat.opacity");

	/**
	 * Defines the height scaling of bumpmaps/parallax maps on this material.
	 * The default value to be taken if this property is not defined is
	 * naturally <code>1.0f</code>.
	 */
	public static final Any<Float> BUMPHEIGHT = new Any<Float>(
			"$mat.bumpscaling");

	/**
	 * Defines the shininess of the material. This is simply the exponent of the
	 * phong and blinn-phong shading equations. If the property is not defined,
	 * no specular lighting must be computed for the material.
	 */
	public static final Any<Float> SHININESS = new Any<Float>("$mat.shininess");

	/**
	 * Dummy base for all predefined material keys.
	 */
	public static class Any<Type> {
		public Any(String str) {
			this(str, 0, 0);
		}

		public Any(String str, int tType, int tIndex) {
			name = str;
			textureType = tType;
			textureIndex = tIndex;
		}

		String name;
		int textureType, textureIndex;
	}

}
