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
 * Class to wrap materials. Materials are represented in ASSIMP as a list of
 * key/value pairs, the key being a <code>String</code> and the value being
 * a binary buffer. The class provides several get methods to access
 * material properties easily.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Material {


    /**
     * Internal representation of a material property
     */
    private class Property {
        String key;
        Object value;
    }

    /**
     * List of all properties for this material
     */
    private Property[] properties;


    /**
     * Special exception class which is thrown if a material property
     * could not be found.
     */
    public class PropertyNotFoundException extends Exception {

        public final String property_key;

        /**
         * Constructs a new exception
         *
         * @param message      Error message
         * @param property_key Name of the property that wasn't found
         */
        public PropertyNotFoundException(String message, String property_key) {
            super(message);
            this.property_key = property_key;
        }
    }


    /**
     * Get a property with a specific name as generic <code>Object</code>
     *
     * @param key MATKEY_XXX key constant
     * @return null if the property wasn't there or hasn't
     *         the desired output type. The returned <code>Object</code> can be
     *         casted to the expected data type for the property. Primitive
     *         types are represented by their boxed variants.
     */
    public Object getProperty(String key) throws PropertyNotFoundException {

        for (Property prop : properties) {
            if (prop.key.equals(key)) {
                return prop.value;
            }
        }
        throw new PropertyNotFoundException("Unable to find material property: ", key);
    }

    /**
     * Get a material property as float array
     *
     * @param key MATKEY_XXX key constant
     * @throws PropertyNotFoundException - if the property can't be found
     *                                   or if it has the wrong data type.
     */
    public float[] getPropertyAsFloatArray(String key) throws PropertyNotFoundException {

        Object obj = getProperty(key);
        if (obj instanceof float[]) {
            return (float[]) obj;
        }
        String msg = "The data type requested (float[]) doesn't match the " +
                "real data type of the material property";
        DefaultLogger.get().error(msg);
        throw new PropertyNotFoundException(msg, key);
    }


    /**
     * Get a floating-point material property
     *
     * @param key MATKEY_XXX key constant
     * @return The value of the property.
     * @throws PropertyNotFoundException - if the property can't be found
     *                                   or if it has the wrong data type.
     */
    public float getPropertyAsFloat(String key) throws PropertyNotFoundException {

        Object obj = getProperty(key);
        if (obj instanceof Float) {
            return (Float) obj;
        }
        String msg = "The data type requested (Float) doesn't match the " +
                "real data type of the material property";
        DefaultLogger.get().error(msg);
        throw new PropertyNotFoundException(msg, key);
    }


    /**
     * Get an integer material property
     *
     * @param key MATKEY_XXX key constant
     * @return The value of the property.
     * @throws PropertyNotFoundException - if the property can't be found
     *                                   or if it has the wrong data type.
     */
    public int getPropertyAsInt(String key) throws PropertyNotFoundException {

        Object obj = getProperty(key);
        if (obj instanceof Integer) {
            return (Integer) obj;
        }
        String msg = "The data type requested (Integer) doesn't match the " +
                "real data type of the material property";
        DefaultLogger.get().error(msg);
        throw new PropertyNotFoundException(msg, key);
    }


    /**
     * Get a material property string
     *
     * @param key MATKEY_XXX key constant
     * @return The value of the property.
     * @throws PropertyNotFoundException - if the property can't be found
     *                                   or if it has the wrong data type.
     */
    public String getPropertyAsString(String key) throws PropertyNotFoundException {

        Object obj = getProperty(key);
        if (obj instanceof String) {
            return (String) obj;
        }
        String msg = "The data type requested (java.lang.String) doesn't match the " +
                "real data type of the material property";
        DefaultLogger.get().error(msg);
        throw new PropertyNotFoundException(msg, key);
    }


    /**
     * Material key: defines the name of the material
     * The type of this material property is <code>String</code>
     */
    public static final String MATKEY_NAME = "$mat.name";

    /**
     * Material key: defines the diffuse base color of the material
     * The type of this material property is <code>float[]</code>.
     * The array has 4 or 3 components in RGB(A) order.
     */
    public static final String MATKEY_COLOR_DIFFUSE = "$clr.diffuse";

    /**
     * Material key: defines the specular base color of the material
     * The type of this material property is <code>float[]</code>.
     * The array has 4 or 3 components in RGB(A) order.
     */
    public static final String MATKEY_COLOR_SPECULAR = "$clr.specular";

    /**
     * Material key: defines the ambient base color of the material
     * The type of this material property is <code>float[]</code>.
     * The array has 4 or 3 components in RGB(A) order.
     */
    public static final String MATKEY_COLOR_AMBIENT = "$clr.ambient";

    /**
     * Material key: defines the emissive base color of the material
     * The type of this material property is <code>float[]</code>.
     * The array has 4 or 3 components in RGB(A) order.
     */
    public static final String MATKEY_COLOR_EMISSIVE = "$clr.emissive";

    /**
     * Specifies the blend operation to be used to combine the Nth
     * diffuse texture with the (N-1)th diffuse texture (or the diffuse
     * base color for the first diffuse texture)
     * <br>
     * <b>Type:</b> int (TextureOp)<br>
     * <b>Default value:</b> 0<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(N)<br>
     *
     * @param N Index of the texture
     */
    public static String MATKEY_TEXOP_DIFFUSE(int N) {
        return "$tex.op.diffuse[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_SPECULAR(int N) {
        return "$tex.op.specular[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_AMBIENT(int N) {
        return "$tex.op.ambient[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_EMISSIVE(int N) {
        return "$tex.op.emissive[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_NORMALS(int N) {
        return "$tex.op.normals[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_HEIGHT(int N) {
        return "$tex.op.height[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_SHININESS(int N) {
        return "$tex.op.shininess[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXOP_DIFFUSE</code>
     */
    public static String MATKEY_TEXOP_OPACITY(int N) {
        return "$tex.op.opacity[" + N + "]";
    }


    /**
     * Specifies the blend factor to be multiplied with the value of
     * the n'th texture layer before it is combined with the previous
     * layer using a specific blend operation.
     * <p/>
     * <br>
     * <b>Type:</b> float<br>
     * <b>Default value:</b> 1.0f<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(N)<br>
     *
     * @param N Index of the texture
     */
    public static String MATKEY_TEXBLEND_DIFFUSE(int N) {
        return "$tex.blend.diffuse[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_SPECULAR(int N) {
        return "$tex.blend.specular[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_AMBIENT(int N) {
        return "$tex.blend.ambient[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_EMISSIVE(int N) {
        return "$tex.blend.emissive[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_SHININESS(int N) {
        return "$tex.blend.shininess[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_OPACITY(int N) {
        return "$tex.blend.opacity[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_HEIGHT(int N) {
        return "$tex.blend.height[" + N + "]";
    }

    /**
     * @see <code>MATKEY_TEXBLEND_DIFFUSE</code>
     */
    public static String MATKEY_TEXBLEND_NORMALS(int N) {
        return "$tex.blend.normals[" + N + "]";
    }


    /**
     * Specifies the index of the UV channel to be used for a texture
     * <br>
     * <b>Type:</b>int<br>
     * <b>Default value:</b>0<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(N)<br>
     *
     * @param N Index of the texture
     */
    public static String MATKEY_UVWSRC_DIFFUSE(int N) {
        return "$tex.uvw.diffuse[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_SPECULAR(int N) {
        return "$tex.uvw.specular[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_AMBIENT(int N) {
        return "$tex.uvw.ambient[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_EMISSIVE(int N) {
        return "$tex.uvw.emissive[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_SHININESS(int N) {
        return "$tex.uvw.shininess[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_OPACITY(int N) {
        return "$tex.uvw.opacity[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_HEIGHT(int N) {
        return "$tex.uvw.height[" + N + "]";
    }

    /**
     * @see <code>MATKEY_UVWSRC_DIFFUSE</code>
     */
    public static String MATKEY_UVWSRC_NORMALS(int N) {
        return "$tex.uvw.normals[" + N + "]";
    }


    /**
     * Specifies the texture mapping mode in the v (y) direction.
     * <br>
     * <b>Type:</b>int<br>
     * <b>Default value:</b>TextureMapMode.Wrap<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(N)<br>
     *
     * @param N Index of the texture
     */
    public static String MATKEY_MAPPINGMODE_U_DIFFUSE(int N) {
        return "$tex.mapmodeu.diffuse[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_SPECULAR(int N) {
        return "$tex.mapmodeu.specular[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_AMBIENT(int N) {
        return "$tex.mapmodeu.ambient[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_EMISSIVE(int N) {
        return "$tex.mapmodeu.emissive[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_SHININESS(int N) {
        return "$tex.mapmodeu.shininess[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_OPACITY(int N) {
        return "$tex.mapmodeu.opacity[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_HEIGHT(int N) {
        return "$tex.mapmodeu.height[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_U_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_U_NORMALS(int N) {
        return "$tex.mapmodeu.normals[" + N + "]";
    }


    /**
     * Specifies the texture mapping mode in the v (y) direction.
     * <br>
     * <b>Type:</b>int<br>
     * <b>Default value:</b>TextureMapMode.Wrap<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(N)<br>
     *
     * @param N Index of the texture
     */
    public static String MATKEY_MAPPINGMODE_V_DIFFUSE(int N) {
        return "$tex.mapmodev.diffuse[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_SPECULAR(int N) {
        return "$tex.mapmodev.specular[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_AMBIENT(int N) {
        return "$tex.mapmodev.ambient[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_EMISSIVE(int N) {
        return "$tex.mapmodev.emissive[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_SHININESS(int N) {
        return "$tex.mapmodev.shininess[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_OPACITY(int N) {
        return "$tex.mapmodev.opacity[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_HEIGHT(int N) {
        return "$tex.mapmodev.height[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_V_NORMALS(int N) {
        return "$tex.mapmodev.normals[" + N + "]";
    }


    /**
     * Specifies the texture mapping mode in the w (z) direction.
     * <br>
     * <b>Type:</b>int<br>
     * <b>Default value:</b>TextureMapMode.Wrap<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(N)<br>
     *
     * @param N Index of the texture
     */
    public static String MATKEY_MAPPINGMODE_W_DIFFUSE(int N) {
        return "$tex.mapmodew.diffuse[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_SPECULAR(int N) {
        return "$tex.mapmodew.specular[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_AMBIENT(int N) {
        return "$tex.mapmodew.ambient[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_EMISSIVE(int N) {
        return "$tex.mapmodew.emissive[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_SHININESS(int N) {
        return "$tex.mapmodew.shininess[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_OPACITY(int N) {
        return "$tex.mapmodew.opacity[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_HEIGHT(int N) {
        return "$tex.mapmodew.height[" + N + "]";
    }

    /**
     * @see <code>MATKEY_MAPPINGMODE_V_DIFFUSE</code>
     */
    public static String MATKEY_MAPPINGMODE_W_NORMALS(int N) {
        return "$tex.mapmodew.normals[" + N + "]";
    }
}
