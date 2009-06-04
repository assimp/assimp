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
 * Represents a rotation quaternion
 * 
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Quaternion {

	public float x, y, z, w;

	/**
	 * Construction from euler angles
	 * 
	 * @param fPitch
	 *            Rotation around the x axis
	 * @param fYaw
	 *            Rotation around the y axis
	 * @param fRoll
	 *            Rotation around the z axis
	 */
	public Quaternion(float fPitch, float fYaw, float fRoll) {
		float fSinPitch = (float) Math.sin(fPitch * 0.5F);
		float fCosPitch = (float) Math.cos(fPitch * 0.5F);
		float fSinYaw = (float) Math.sin(fYaw * 0.5F);
		float fCosYaw = (float) Math.cos(fYaw * 0.5F);
		float fSinRoll = (float) Math.sin(fRoll * 0.5F);
		float fCosRoll = (float) Math.cos(fRoll * 0.5F);
		float fCosPitchCosYaw = (fCosPitch * fCosYaw);
		float fSinPitchSinYaw = (fSinPitch * fSinYaw);
		x = fSinRoll * fCosPitchCosYaw - fCosRoll * fSinPitchSinYaw;
		y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
		z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
		w = fCosRoll * fCosPitchCosYaw + fSinRoll * fSinPitchSinYaw;
	}

	/**
	 * Construction from an existing rotation matrix
	 * 
	 * @param pRotMatrix
	 *            Matrix to be converted to a quaternion
	 */
	public Quaternion(Matrix3x3 pRotMatrix) {

		float t = 1 + pRotMatrix.coeff[0] + pRotMatrix.coeff[4]
				+ pRotMatrix.coeff[8];

		// large enough
		if (t > 0.00001f) {
			float s = (float) Math.sqrt(t) * 2.0f;
			x = (pRotMatrix.coeff[8] - pRotMatrix.coeff[7]) / s;
			y = (pRotMatrix.coeff[6] - pRotMatrix.coeff[2]) / s;
			z = (pRotMatrix.coeff[1] - pRotMatrix.coeff[3]) / s;
			w = 0.25f * s;
		} // else we have to check several cases
		else if (pRotMatrix.coeff[0] > pRotMatrix.coeff[4]
				&& pRotMatrix.coeff[0] > pRotMatrix.coeff[8]) {
			// Column 0:
			float s = (float) Math.sqrt(1.0f + pRotMatrix.coeff[0]
					- pRotMatrix.coeff[4] - pRotMatrix.coeff[8]) * 2.0f;
			x = -0.25f * s;
			y = (pRotMatrix.coeff[1] + pRotMatrix.coeff[3]) / s;
			z = (pRotMatrix.coeff[6] + pRotMatrix.coeff[2]) / s;
			w = (pRotMatrix.coeff[7] - pRotMatrix.coeff[5]) / s;
		} else if (pRotMatrix.coeff[4] > pRotMatrix.coeff[8]) {
			// Column 1:
			float s = (float) Math.sqrt(1.0f + pRotMatrix.coeff[4]
					- pRotMatrix.coeff[0] - pRotMatrix.coeff[8]) * 2.0f;
			x = (pRotMatrix.coeff[1] + pRotMatrix.coeff[3]) / s;
			y = -0.25f * s;
			z = (pRotMatrix.coeff[5] + pRotMatrix.coeff[7]) / s;
			w = (pRotMatrix.coeff[2] - pRotMatrix.coeff[6]) / s;
		} else {
			// Column 2:
			float s = (float) Math.sqrt(1.0f + pRotMatrix.coeff[8]
					- pRotMatrix.coeff[0] - pRotMatrix.coeff[4]) * 2.0f;
			x = (pRotMatrix.coeff[6] + pRotMatrix.coeff[2]) / s;
			y = (pRotMatrix.coeff[5] + pRotMatrix.coeff[7]) / s;
			z = -0.25f * s;
			w = (pRotMatrix.coeff[3] - pRotMatrix.coeff[1]) / s;
		}
	}

	/**
	 * Convert the quaternion to a rotation matrix
	 * 
	 * @return 3x3 rotation matrix
	 */
	public Matrix3x3 getMatrix() {

		Matrix3x3 resMatrix = new Matrix3x3();
		resMatrix.coeff[0] = 1.0f - 2.0f * (y * y + z * z);
		resMatrix.coeff[1] = 2.0f * (x * y + z * w);
		resMatrix.coeff[2] = 2.0f * (x * z - y * w);
		resMatrix.coeff[3] = 2.0f * (x * y - z * w);
		resMatrix.coeff[4] = 1.0f - 2.0f * (x * x + z * z);
		resMatrix.coeff[5] = 2.0f * (y * z + x * w);
		resMatrix.coeff[6] = 2.0f * (x * z + y * w);
		resMatrix.coeff[7] = 2.0f * (y * z - x * w);
		resMatrix.coeff[8] = 1.0f - 2.0f * (x * x + y * y);

		return resMatrix;
	}

}
