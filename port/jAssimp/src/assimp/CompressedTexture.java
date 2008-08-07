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

import javax.imageio.ImageIO;
import javax.imageio.stream.ImageInputStream;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.ByteArrayInputStream;

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

    /**
     * Retrieves the format of the texture data. This is
     * the most common file extension of the format (without a
     * dot at the beginning). Examples include dds, png, jpg ...
     *
     * @return Extension string or null if the format of the texture
     *         data is not known to ASSIMP.
     */
    public final String getFormat() {
        return m_format;
    }

    /**
     * Get a pointer to the data of the compressed texture
     *
     * @return Data poiner
     */
    public final byte[] getData() {
        return (byte[]) data;
    }

    /**
     * Get the length of the data of the compressed texture
     *
     * @return Data poiner
     */
    public final int getLength() {
        return width;
    }

    /**
     * Returns 0 for compressed textures
     *
     * @return n/a
     */
    @Override
    public final int getHeight() {
        return 0;
    }

    /**
     * Returns 0 for compressed textures
     *
     * @return n/a
     */
    @Override
    public final int getWidth() {
        return 0;
    }

    /**
     * Returns null for compressed textures
     *
     * @return n/a
     */
    @Override
    public Color getPixel(int x, int y) {
        return null;
    }

    /**
     * Returns null for compressed textures
     *
     * @return n/a
     */
    @Override
    public final Color[] getColorArray() {
        return null;
    }


    /**
     * @return The return value is <code>true</code> of the
     *         file format can't be recognized.
     * @see <code>Texture.hasAlphaChannel()</code>
     */
    public boolean hasAlphaChannel() {

        // try to determine it from the file format sequence
        if (m_format.equals("bmp") || m_format.equals("dib")) return false;
        if (m_format.equals("tif") || m_format.equals("tiff")) return false;
        if (m_format.equals("jpg") || m_format.equals("jpeg")) return false;

        // todo: add more

        return true;
    }


    /**
     * Convert the texture into a <code>java.awt.BufferedImage</code>
     *
     * @return <code>java.awt.BufferedImage</code> object containing
     *         a copy of the texture image. The return value is <code>null</code>
     *         if the file format is not known.
     */
    public BufferedImage convertToImage() {

        BufferedImage img = null;
        try {

            // create an input stream and attach it to an image input stream
            ImageInputStream stream = ImageIO.createImageInputStream(
                    new ByteArrayInputStream((byte[])data));

            // and use the stream to decode the file
            img = ImageIO.read(stream);

        } catch (IOException e) {

            DefaultLogger.get().error("Unable to decode compressed embedded texture +" +
                    "(Format hint: " + m_format + ")" );

        }
        // return the created image to the caller
        return img;
    }
}
