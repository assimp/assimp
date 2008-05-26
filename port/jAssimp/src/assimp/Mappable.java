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
 * Defines base behaviour for all sub objects of <code>Mesh</code>.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public abstract class Mappable {

    /**
     * Index of the mapped object in the parent Mesh
     */
    private int m_iArrayIndex = 0;

    /**
     * Reference to the parent of the object
     */
    private Object m_parent = null;


    /**
     * Construction from a given parent object and array index
     * @param parent Must be valid, null is not allowed
     * @param index Valied index in the parent's list
     */
    public Mappable(Object parent, int index) {
        m_parent = parent;
        m_iArrayIndex = index;
    }

    /**
     * Called as a request to the object to map all of its
     * data into the address space of the Java virtual machine.
     * After this method has been called the class instance must
     * be ready to be used without an underyling native aiScene
     * @throws NativeError
     */
    protected abstract void onMap() throws NativeError;


    /**
     * Retrieve the index ofthe mappable object in the parent mesh
     * @return Value between 0 and n-1
     */
    public int getArrayIndex() {
        return m_iArrayIndex;
    }

    /**
     * Provide access to the parent
     * @return Never null ...
     */
    public Object getParent() {
        return m_parent;
    }
}
