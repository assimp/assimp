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
 * A mesh represents a geometry or model with a single material.
 * <p/>
 * It usually consists of a number of vertices and a series of primitives/faces
 * referencing the vertices. In addition there might be a series of bones, each
 * of them addressing a number of vertices with a certain weight. Vertex data
 * is presented in channels with each channel containing a single per-vertex
 * information such as a set of texture coords or a normal vector.
 * <p/>
 * Note that not all mesh data channels must be there. E.g. most models
 * don't contain vertex colors, so this data channel is often not filled.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Mesh extends IMappable {

    /**
     * Defines the maximum number of UV(W) channels that are available
     * for a mesh. If a loader finds more channels in a file, some
     * will be skipped
     */
    private static final int MAX_NUMBER_OF_TEXTURECOORDS = 0x4;

    /**
     * Defines the maximum number of vertex color channels that are
     * available for a mesh. If a loader finds more channels in a file,
     * some will be skipped
     */
    private static final int MAX_NUMBER_OF_COLOR_SETS = 0x4;

    /**
     * Construction from a given parent object and array index
     *
     * @param parent Parent object
     * @param index  Valied index in the parent's list
     */
    public Mesh(Object parent, int index) {
        super(parent, index);
    }


    protected void OnMap() throws NativeError {

    }
}
