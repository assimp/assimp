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
 * Represents a 3x3 row major matrix
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Matrix3x3  {


    /**
     * Default constructor. Initializes the matrix with its identity
     */
    public  Matrix3x3() {
        coeff[0] = coeff[4] = coeff[8] = 1.0f;
    }

    /**
     * Copy constructor
     * @param other Matrix to be copied
     */
    public  Matrix3x3 ( Matrix3x3 other) {
        System.arraycopy(other.coeff, 0, this.coeff, 0, 9);
    }

    /**
     * Construction from nine given coefficents
     * @param a1
     * @param a2
     * @param a3
     * @param b1
     * @param b2
     * @param b3
     * @param c1
     * @param c2
     * @param c3
     */
    public Matrix3x3 (float a1, float a2, float a3,
                      float b1, float b2, float b3,
                      float c1, float c2, float c3) {

        coeff[0] = a1;
        coeff[1] = a2;
        coeff[2] = a3;
        coeff[3] = b1;
        coeff[4] = b2;
        coeff[5] = b3;
        coeff[6] = c1;
        coeff[7] = c2;
        coeff[8] = c3;
    }


    /**
     * Copy constructor (construction from a 4x4 matrix)
     * @param other Matrix to be copied
     */
    public  Matrix3x3 ( Matrix4x4 other) {
        coeff[0] = other.coeff[0];
        coeff[1] = other.coeff[1];
        coeff[2] = other.coeff[2];
        coeff[3] = other.coeff[4];
        coeff[4] = other.coeff[5];
        coeff[5] = other.coeff[6];
        coeff[6] = other.coeff[8];
        coeff[7] = other.coeff[9];
        coeff[8] = other.coeff[10];
    }


    /**
     * The coefficients of the matrix
     */
    public float[] coeff = new float[9];


    /**
     * Returns a field in the matrix
     * @param row Row index
     * @param column Column index
     * @return The corresponding field value
     */
    public final float get(int row, int column) {
        assert(row <= 2 && column <= 2);
        return coeff[row*3+column];
    }


    /**
     * Multiplies *this* matrix with another matrix
     * @param m Matrix to multiply with
     * @return Output matrix
     */
    public final Matrix3x3 Mul(Matrix3x3 m) {

        return new Matrix3x3(
            m.coeff[0] * coeff[0] + m.coeff[3] * coeff[1] + m.coeff[6] * coeff[2],
		    m.coeff[1] * coeff[0] + m.coeff[4] * coeff[1] + m.coeff[7] * coeff[2],
		    m.coeff[2] * coeff[0] + m.coeff[5] * coeff[1] + m.coeff[8] * coeff[2],
		    m.coeff[0] * coeff[3] + m.coeff[3] * coeff[4] + m.coeff[6] * coeff[5],
		    m.coeff[1] * coeff[3] + m.coeff[4] * coeff[4] + m.coeff[7] * coeff[5],
		    m.coeff[2] * coeff[3] + m.coeff[5] * coeff[4] + m.coeff[8] * coeff[5],
		    m.coeff[0] * coeff[6] + m.coeff[3] * coeff[7] + m.coeff[6] * coeff[8],
		    m.coeff[1] * coeff[6] + m.coeff[4] * coeff[7] + m.coeff[7] * coeff[8],
		    m.coeff[2] * coeff[6] + m.coeff[5] * coeff[7] + m.coeff[8] * coeff[8]);
    }
}
