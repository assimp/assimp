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
 * a binary buffer.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Material extends Mappable {


    public static final String MATKEY_NAME = "$mat.name";


    /**
     * Specifies the blend operation to be used to combine the Nth
     * diffuse texture with the (N-1)th diffuse texture (or the diffuse
     * base color for the first diffuse texture)
     * <br>
     * <b>Type:</b> int (TextureOp)<br>
     * <b>Default value:</b> 0<br>
     * <b>Requires:</b> MATKEY_TEXTURE_DIFFUSE(0)<br>
     */
    public static String MATKEY_TEXOP_DIFFUSE(int N) {
        return "$tex.op.diffuse[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_SPECULAR(int N) {
        return "$tex.op.specular[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_AMBIENT(int N) {
        return "$tex.op.ambient[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_EMISSIVE(int N) {
        return "$tex.op.emissive[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_NORMALS(int N) {
        return "$tex.op.normals[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_HEIGHT(int N) {
        return "$tex.op.height[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_SHININESS(int N) {
        return "$tex.op.shininess[" + N + "]";
    }

    /**
     * @see MATKEY_TEXOP_DIFFUSE()
     */
    public static String MATKEY_TEXOP_OPACITY(int N) {
        return "$tex.op.opacity[" + N + "]";
    }


    /**
     * Construction from a given parent object and array index
     *
     * @param parent Must be valid, null is not allowed
     * @param index  Valied index in the parent's list
     */
    public Material(Object parent, int index) {
        super(parent, index);
    }

    protected void onMap() throws NativeError {

    }
}
