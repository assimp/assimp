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

import java.awt.*;

/**
 * Represents an embedded texture. Sometimes textures are not referenced
 * with a path, instead they are directly embedded into the model file.
 * Example file formats doing this include MDL3, MDL5 and MDL7 (3D GameStudio).
 * Embedded textures are converted to an array of color values (RGBA).
 * <p/>
 * Compressed textures (textures that are stored in a format like png or jpg)
 * are represented by the <code><CompressedTexture/code> class.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Texture extends Mappable {

    protected int width = 0;
    protected int height = 0;

    protected Object data = null;

    /**
     * Construction from a given parent object and array index
     *
     * @param parent Must be valid, null is not allowed
     * @param index  Valied index in the parent's list
     */
    public Texture(Object parent, int index) throws NativeError {
        super(parent, index);

        long lTemp;
        if (0x0 == (lTemp = this._NativeGetTextureInfo(((Scene) this.getParent()).
                getImporter().getContext(), this.getArrayIndex()))) {
            throw new NativeError("Unable to get the width and height of the texture");
        }
        this.width = (int) (lTemp);
        this.height = (int) (lTemp >> 32);
    }


    /**
     * Retrieve the height of the texture image
     *
     * @return Height, in pixels
     */
    public int getHeight() {
        return height;
    }

    /**
     * Retrieve the width of the texture image
     *
     * @return Width, in pixels
     */
    public int getWidth() {
        return width;
    }

    /**
     * Get the color at a given position of the texture
     *
     * @param x X coordinate, zero based
     * @param y Y coordinate, zero based
     * @return Color at this position
     */
    public Color getPixel(int x, int y) {

        assert(x < width && y < height);

        // map the color data in memory if required ...
        if (null == data) {
            try {
                this.onMap();
            } catch (NativeError nativeError) {
                return Color.black;
            }
        }
        return ((Color[]) data)[y * width + x];
    }

    /**
     * Get a pointer to the color buffer of the texture
     *
     * @return Array of <code>java.awt.Color</code>, size: width * height
     */
    public Color[] getColorArray() {

        // map the color data in memory if required ...
        if (null == data) {
            try {
                this.onMap();
            } catch (NativeError nativeError) {
                return null;
            }
        }
        return (Color[]) data;
    }

    /**
     * Internal helper function to map the native texture data into
     * a <code>java.awt.Color</code> array
     */
    @Override
    protected void onMap() throws NativeError {
        final int iNumPixels = width * height;

        // first allocate the output array
        data = new Color[iNumPixels];

        // now allocate a temporary output array
        byte[] temp = new byte[(iNumPixels) << 2];

        // and copy the native color data to it
        if (0xffffffff == this._NativeMapColorData(((Scene) this.getParent()).getImporter().getContext(),
                this.getArrayIndex(), temp)) {
            throw new NativeError("Unable to map aiTexture into the Java-VM");
        }

        DefaultLogger.get().debug("Texture.onMap successful");

        // now convert the temporary representation to a Color array
        // (data is given in BGRA order, we need RGBA)
        for (int i = 0, iBase = 0; i < iNumPixels; ++i, iBase += 4) {
            ((Color[]) data)[i] = new Color(temp[iBase + 2], temp[iBase + 1], temp[iBase], temp[iBase + 3]);
        }
        return;
    }

    /**
     * JNI bridge call. For internal use only
     * The method maps the contents of the native aiTexture object into memory
     * the native memory area will be deleted afterwards.
     *
     * @param context Current importer context (imp.hashCode)
     * @param index   Index of the texture in the scene
     * @param temp    Output array. Assumed to be width * height * 4 in size
     * @return 0xffffffff if an error occured
     */
    protected native int _NativeMapColorData(long context, long index, byte[] temp);

    /**
     * JNI bridge call. For internal use only
     * The method retrieves information on the underlying texture
     *
     * @param context Current importer context (imp.hashCode)
     * @param index   Index of the texture in the scene
     * @return 0x0 if an error occured, otherwise the width in the lower 32 bits
     *         and the height in the higher 32 bits
     */
    private native long _NativeGetTextureInfo(long context, long index);
}
