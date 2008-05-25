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
 * Represents an embedded compressed texture that is stored in a format
 * like JPEG or PNG. See the documentation of <code>Texture</code>
 * for more details on this class. Use <code>instanceof</code> to
 * determine whether a particular <code>Texture</code> in the list
 * returned by <code>Scene.getTextures()</code> is a compressed texture.
 * <p/>
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class CompressedTexture extends Texture {

    private String m_format = "";
    private byte[] m_data;
    private int m_length = 0;


    /**
     * Construction from a given parent object and array index
     *
     * @param parent Must be valid, null is not allowed
     * @param index  Valied index in the parent's list
     */
    public CompressedTexture(Object parent, int index) throws NativeError {
        super(parent, index);
    }

    /**
     * Retrieves the format of the texture data. This is
     * the most common file extension of the format (without a
     * dot at the beginning). Examples include dds, png, jpg ...
     * @return Extension string or null if the format of the texture
     * data is not known to ASSIMP.
     */
    public String getFormat() {
        return m_format;
    }

    public byte[] getData() {
        return m_data;
    }

    /**
     * Retrieve the height of the texture image
     *
     * @return Height, in pixels
     */
    public int getHeight() {
        DefaultLogger.get()
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
     * Internal helper function to map the native texture data into
     * a <code>byte</code> array in the memory of the JVM
     */
    @Override
    protected void OnMap() throws NativeError {

        // first allocate the output array
        data = new Color[iNumPixels];

        // now allocate a temporary output array
        byte[] temp = new byte[(iNumPixels) << 2];

        // and copy the native color data to it
        if (0xffffffff == this._NativeMapColorData(((Scene)this.getParent()).getImporter().getContext(),
                this.getArrayIndex(),temp)) {
           throw new NativeError("Unable to map aiTexture into the Java-VM");
        }

        // now convert the temporary representation to a Color array
        // (data is given in BGRA order, we need RGBA)
        for (int i = 0, iBase = 0; i < iNumPixels; ++i, iBase += 4) {
            data[i] = new Color(temp[iBase + 2], temp[iBase + 1], temp[iBase], temp[iBase + 3]);
        }
        return;
    }
}
