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

using System;
using System.Collections.Generic;
using System.Text;

namespace Assimp.NET
{
    public class Mesh
    {
        public static int MAX_NUMBER_OF_COLOR_SETS;
        public static int MAX_NUMBER_OF_TEXTURECOORDS;

        public Mesh()
        {
            throw new System.NotImplementedException();
        }

        public void getBitangent(int Index, float[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getBitangent(int Index, float[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public float[] getBitangetArray()
        {
            throw new System.NotImplementedException();
        }

        public Bone getBone(int i)
        {
            throw new System.NotImplementedException();
        }

        public Bone[] getBonesArray()
        {
            throw new System.NotImplementedException();
        }

        public void getFace(int Index, int[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getFace(int Index, int[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public int[] getFaceArray()
        {
            throw new System.NotImplementedException();
        }

        public int getMaterialIndex()
        {
            throw new System.NotImplementedException();
        }

        public void getNormal(int Index, float[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getNormal(int Index, float[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public float[] getNormalArray()
        {
            throw new System.NotImplementedException();
        }

        public int getNumBones()
        {
            throw new System.NotImplementedException();
        }

        public int getNumFaces()
        {
            throw new System.NotImplementedException();
        }

        public int getNumVertices()
        {
            throw new System.NotImplementedException();
        }

        public void getPosition(int Index, float[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getPosition(int Index, float[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public float[] getPositionArray()
        {
            throw new System.NotImplementedException();
        }

        public void getTangent(int Index, float[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getTangent(int Index, float[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public float[] getTangentArray()
        {
            throw new System.NotImplementedException();
        }

        public void getTexCoord(int channel, int Index, float[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getTexCoord(int channel, int Index, float[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public float[] getTexCoordArray(int channel)
        {
            throw new System.NotImplementedException();
        }

        public System.Drawing.Color getVertexColor(int channel, int Index)
        {
            throw new System.NotImplementedException();
        }

        public void getVertexColor(int channel, int Index, float[] Out)
        {
            throw new System.NotImplementedException();
        }

        public void getVertexColor(int channel, int Index, float[] Out, int OutBase)
        {
            throw new System.NotImplementedException();
        }

        public float[] getVertexColorArray(int channel)
        {
            throw new System.NotImplementedException();
        }

        public bool hasBones
        {
            get { throw new System.NotImplementedException(); }
        }

        public bool hasNormals
        {
            get { throw new System.NotImplementedException(); }
        }

        public bool hasPositions
        {
            get { throw new System.NotImplementedException(); }
        }

        public bool hasTangentsAndBitangets
        {
            get { throw new System.NotImplementedException(); }
        }

        public bool hasUVCoords
        {
            get { throw new System.NotImplementedException(); }
        }

        public bool hasVertexColors
        {
            get { throw new System.NotImplementedException(); }
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public override bool Equals(object obj)
        {
            return (Mesh)obj == this;
        }

        public override string ToString()
        {
            throw new System.NotImplementedException();
        }

    }
}
