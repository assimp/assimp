/*
---------------------------------------------------------------------------
Open Asset Import Library - Java Binding (jassimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
---------------------------------------------------------------------------
*/
package jassimp;

/** 
 * Defines the purpose of a texture.<p> 
 *
 * This is a very difficult topic. Different 3D packages support different
 * kinds of textures. For very common texture types, such as bumpmaps, the
 * rendering results depend on implementation details in the rendering 
 * pipelines of these applications. Assimp loads all texture references from
 * the model file and tries to determine which of the predefined texture
 * types below is the best choice to match the original use of the texture
 * as closely as possible.<p>
 *  
 * In content pipelines you'll usually define how textures have to be handled,
 * and the artists working on models have to conform to this specification,
 * regardless which 3D tool they're using.
 */
public enum AiTextureType {
    /** Dummy value.
     *
     *  No texture, but the value to be used as 'texture semantic'
     *  (#aiMaterialProperty::mSemantic) for all material properties
     *  *not* related to textures.
     */
    NONE(0),

    /** LEGACY API MATERIALS 
     * Legacy refers to materials which 
     * Were originally implemented in the specifications around 2000.
     * These must never be removed, as most engines support them.
     */

    /** The texture is combined with the result of the diffuse
     *  lighting equation.
     */
    DIFFUSE(1),

    /** The texture is combined with the result of the specular
     *  lighting equation.
     */
    SPECULAR(2),

    /** The texture is combined with the result of the ambient
     *  lighting equation.
     */
    AMBIENT(3),

    /** The texture is added to the result of the lighting
     *  calculation. It isn't influenced by incoming light.
     */
    EMISSIVE(4),

    /** The texture is a height map.
     *
     *  By convention, higher gray-scale values stand for
     *  higher elevations from the base height.
     */
    HEIGHT(5),

    /** The texture is a (tangent space) normal-map.
     *
     *  Again, there are several conventions for tangent-space
     *  normal maps. Assimp does (intentionally) not
     *  distinguish here.
     */
    NORMALS(6),

    /** The texture defines the glossiness of the material.
     *
     *  The glossiness is in fact the exponent of the specular
     *  (phong) lighting equation. Usually there is a conversion
     *  function defined to map the linear color values in the
     *  texture to a suitable exponent. Have fun.
    */
    SHININESS(7),

    /** The texture defines per-pixel opacity.
     *
     *  Usually 'white' means opaque and 'black' means
     *  'transparency'. Or quite the opposite. Have fun.
    */
    OPACITY(8),

    /** Displacement texture
     *
     *  The exact purpose and format is application-dependent.
     *  Higher color values stand for higher vertex displacements.
    */
    DISPLACEMENT(9),

    /** Lightmap texture (aka Ambient Occlusion)
     *
     *  Both 'Lightmaps' and dedicated 'ambient occlusion maps' are
     *  covered by this material property. The texture contains a
     *  scaling value for the final color value of a pixel. Its
     *  intensity is not affected by incoming light.
    */
    LIGHTMAP(10),

    /** Reflection texture
     *
     * Contains the color of a perfect mirror reflection.
     * Rarely used, almost never for real-time applications.
    */
    REFLECTION(11),

    /** PBR Materials
     * PBR definitions from maya and other modelling packages now use this standard.
     * This was originally introduced around 2012.
     * Support for this is in game engines like Godot, Unreal or Unity3D.
     * Modelling packages which use this are very common now.
     */

    BASE_COLOR(12),
    NORMAL_CAMERA(13),
    EMISSION_COLOR(14),
    METALNESS(15),
    DIFFUSE_ROUGHNESS(16),
    AMBIENT_OCCLUSION(17),

    /** Unknown texture
     *
     *  A texture reference that does not match any of the definitions
     *  above is considered to be 'unknown'. It is still imported,
     *  but is excluded from any further post-processing.
    */
    UNKNOWN(18);
   
   
   /**
    * Utility method for converting from c/c++ based integer enums to java 
    * enums.<p>
    * 
    * This method is intended to be used from JNI and my change based on
    * implementation needs.
    * 
    * @param rawValue an integer based enum value (as defined by assimp) 
    * @return the enum value corresponding to rawValue
    */
   static AiTextureType fromRawValue(int rawValue) {
       for (AiTextureType type : AiTextureType.values()) {
           if (type.m_rawValue == rawValue) {
               return type;
           }
       }

       throw new IllegalArgumentException("unexptected raw value: " + 
               rawValue);
   }
   
   
   /**
    * Utility method for converting from java enums to c/c++ based integer 
    * enums.<p>
    * 
    * @param type the type to convert, may not be null
    * @return the rawValue corresponding to type
    */
   static int toRawValue(AiTextureType type) {
       return type.m_rawValue;
   }


   /**
    * Constructor.
    * 
    * @param rawValue maps java enum to c/c++ integer enum values
    */
   private AiTextureType(int rawValue) {
       m_rawValue = rawValue;
   }


   /**
    * The mapped c/c++ integer enum value.
    */
   private final int m_rawValue;
}
