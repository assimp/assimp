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
 * Defines all shading models supported by the library
 * 
 * NOTE: The list of shading modes has been taken from Blender3D.
 * See Blender3D documentation for more information. The API does
 * not distinguish between "specular" and "diffuse" shaders (thus the
 * specular term for diffuse shading models like Oren-Nayar remains
 * undefined)
 */
public class ShadingMode {

    private ShadingMode() {}

    /**
     * Flat shading. Shading is done on per-face base,
     * diffuse only.
     */
    public static final int Flat = 0x1;

    /**
     * Diffuse gouraud shading. Shading on per-vertex base
     */
    public static final int Gouraud = 0x2;

    /**
     * Diffuse/Specular Phong-Shading
     * <p/>
     * Shading is applied on per-pixel base. This is the
     * slowest algorithm, but generates the best results.
     */
    public static final int Phong = 0x3;

    /**
     * Diffuse/Specular Phong-Blinn-Shading
     * <p/>
     * Shading is applied on per-pixel base. This is a little
     * bit faster than phong and in some cases even
     * more realistic
     */
    public static final int Blinn = 0x4;

    /**
     * Toon-Shading per pixel
     * <p/>
     * Shading is applied on per-pixel base. The output looks
     * like a comic. Often combined with edge detection.
     */
    public static final int Toon = 0x5;

    /**
     * OrenNayar-Shading per pixel
     * <p/>
     * Extension to standard lambertian shading, taking the
     * roughness of the material into account
     */
    public static final int OrenNayar = 0x6;

    /**
     * Minnaert-Shading per pixel
     * <p/>
     * Extension to standard lambertian shading, taking the
     * "darkness" of the material into account
     */
    public static final int Minnaert = 0x7;

    /**
     * CookTorrance-Shading per pixel
     */
    public static final int CookTorrance = 0x8;

    /**
     * No shading at all
     */
    public static final int NoShading = 0x9;
}
