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
    public class PostProcessStep
    {
        private PostProcessStep(String name)
        {
            throw new System.NotImplementedException();
        }

        public static readonly PostProcessStep CalcTangetSpace = new PostProcessStep("CalcTangentSpace");
        public static readonly PostProcessStep ConvertToLeftHanded = new PostProcessStep("ConvertToLeftHanded");
        public static readonly PostProcessStep FixInfacingNormals = new PostProcessStep("FixInfacingNormals");
        public static readonly PostProcessStep GenFaceNormals = new PostProcessStep("GenFaceNormals");
        public static readonly PostProcessStep GenSmoothNormals = new PostProcessStep("GenSmoothNormals");
        public static readonly PostProcessStep ImproveVertexLocality = new PostProcessStep("ImproveVertexLocality");
        public static readonly PostProcessStep JoinIdenticalVertices = new PostProcessStep("JoinIdenticalVertices");
        public static readonly PostProcessStep KillNormals = new PostProcessStep("KillNormals");
        public static readonly PostProcessStep LimitBoneWeights = new PostProcessStep("LimitBoneWeights");
        public static readonly PostProcessStep PreTransformVertices = new PostProcessStep("PreTransformVertices");
        public static readonly PostProcessStep SplitLargeMeshes = new PostProcessStep("SplitLargeMeshes");
        public static readonly PostProcessStep VladiateDataStructure = new PostProcessStep("VladiateDataStructure");

        public static readonly int DEFAULT_VERTEX_SPLIT_LIMIT = 1000000;
        public static readonly int DEFAULT_TRIANGLE_SPLIT_LIMIT = 1000000;
        public static readonly int DEFAULT_BONE_WEIGHT_LIMIT = 4;

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public override bool Equals(object obj)
        {
            return (PostProcessStep)obj == this;
        }

        public override string ToString()
        {
            throw new System.NotImplementedException();
        }
    }
}
