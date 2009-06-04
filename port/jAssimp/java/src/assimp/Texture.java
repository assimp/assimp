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
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.Iterator;

import javax.imageio.ImageIO;
import javax.imageio.ImageWriter;
import javax.imageio.stream.ImageOutputStream;

/**
 * Represents an embedded texture. Sometimes textures are not referenced with a
 * path, instead they are directly embedded into the model file. Example file
 * formats doing this include MDL3, MDL5 and MDL7 (3D GameStudio). Embedded
 * textures are converted to an array of color values (RGBA).
 * <p/>
 * Compressed textures (textures that are stored in a format like png or jpg)
 * are represented by the <code><CompressedTexture/code> class.
 * 
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Texture {

	protected int width = 0;
	protected int height = 0;
	protected int needAlpha = 0xffffffff;

	protected Object data = null;

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
	 * Returns whether the texture uses its alpha channel
	 * 
	 * @return <code>true</code> if at least one pixel has an alpha value below
	 *         0xFF.
	 */
	public boolean hasAlphaChannel() {

		// already computed?
		if (0xffffffff == needAlpha && null != data) {

			Color[] clr = getColorArray();
			for (Color c : clr) {
				if (c.getAlpha() < 255) {
					needAlpha = 1;
					return true;
				}
			}
			needAlpha = 0;
			return false;
		}
		return 0x1 == needAlpha;
	}

	/**
	 * Get the color at a given position of the texture
	 * 
	 * @param x
	 *            X coordinate, zero based
	 * @param y
	 *            Y coordinate, zero based
	 * @return Color at this position
	 */
	public Color getPixel(int x, int y) {

		assert (x < width && y < height);
		return ((Color[]) data)[y * width + x];
	}

	/**
	 * Get a pointer to the color buffer of the texture
	 * 
	 * @return Array of <code>java.awt.Color</code>, size: width * height
	 */
	public Color[] getColorArray() {
		return (Color[]) data;
	}

	/**
	 * Convert the texture into a <code>java.awt.BufferedImage</code>
	 * 
	 * @return Valid <code>java.awt.BufferedImage</code> object containing a
	 *         copy of the texture image. The texture is a ARGB texture if an
	 *         alpha channel is really required, otherwise RGB is used as pixel
	 *         format.
	 * @throws IOException
	 *             If the conversion fails.
	 */
	public BufferedImage convertToImage() throws IOException {

		BufferedImage buf = new BufferedImage(width, height,
				hasAlphaChannel() ? BufferedImage.TYPE_INT_ARGB
						: BufferedImage.TYPE_INT_RGB);

		int[] aiColorBuffer = new int[width * height];
		Color[] clr = getColorArray();

		for (int i = 0; i < width * height; ++i) {
			aiColorBuffer[i] = clr[i].getRGB();
		}

		buf.setRGB(0, 0, width, height, aiColorBuffer, 0, width * 4);
		return buf;
	}

	/**
	 * Saves a texture as TGA file. This is a debugging helper.
	 * 
	 * @param texture
	 *            Texture to be exported
	 * @param path
	 *            Output path. Output file format is always TGA, regardless of
	 *            the file extension.
	 */
	public static void SaveTextureToTGA(Texture texture, String path)
			throws IOException {
		BufferedImage bImg = texture.convertToImage();

		Iterator<ImageWriter> writers = ImageIO.getImageWritersBySuffix("tga");
		if (!(writers.hasNext())) {
			
			final String err = "No writer for TGA file format available";
			
			DefaultLogger.get().error(err);
			throw new IOException(err);
		}
		ImageWriter w = (ImageWriter) (writers.next());
		File fo = new File(path);

		ImageOutputStream ios = ImageIO.createImageOutputStream(fo);
		w.setOutput(ios);
		w.write(bImg);
	}
}
