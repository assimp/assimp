/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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
using Microsoft.DirectX;

namespace Assimp.Viewer {
    public class Camera {
	    public Camera () {
		    vPos = new Vector3(0.0f,0.0f,-10.0f);
            vLookAt = new Vector3(0.0f, 0.0f, 1.0f);
		    vUp = new Vector3(0.0f,1.0f,0.0f);
		    vRight = new Vector3(0.0f,1.0f,0.0f);
		}

	    // position of the camera
	    public Vector3 vPos;

	    // up-vector of the camera
        public Vector3 vUp;

	    // camera's looking point is vPos + vLookAt
        public Vector3 vLookAt;

	    // right vector of the camera
        public Vector3 vRight;


	    // Equation
	    // (vRight ^ vUp) - vLookAt == 0  
	    // needn't apply

        public Matrix GetMatrix() {
            vLookAt.Normalize();
            vRight = Vector3.Cross(vUp, vLookAt);
            vRight.Normalize();
            vUp = Vector3.Cross(vLookAt, vRight);
            vUp.Normalize();

            var view = Matrix.Identity;
            view.M11 = vRight.X;
            view.M12 = vUp.X;
            view.M13 = vLookAt.X;
            view.M14 = 0.0f;

            view.M21 = vRight.Y;
            view.M22 = vUp.Y;
            view.M23 = vLookAt.Y;
            view.M24 = 0.0f;

            view.M31 = vRight.Z;
            view.M32 = vUp.Z;
            view.M33 = vLookAt.Z;
            view.M34 = 0.0f;

            view.M41 = -Vector3.Dot(vPos, vRight);
            view.M42 = -Vector3.Dot(vPos, vUp);
            view.M43 = -Vector3.Dot(vPos, vLookAt);
            view.M44 = 1.0f;

            return view;
        }
    }
}
