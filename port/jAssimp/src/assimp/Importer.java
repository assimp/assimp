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

import java.util.Vector;

/**
 * Main class of jAssimp. The class is a simple wrapper for the native
 * Assimp::Importer and aiScene classes.
 * If multiple threads are used to load assets, each thread should manage its
 * own instance of this class to avoid threading issues. The class requires
 * the native jAssimp library to work. It must be named "jassimpNN.EXT", where
 * NN is the platform's default int size, e.g. 32 for the x86 architecture.
 * EXT is the default extension for program libraries on the system, .DLL for
 * Windows, .SO for Linux derivates.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Importer {

    /**
     * List of all postprocess steps to apply to the model
     * Empty by default.
     */
    private Vector<PostProcessStep> m_vPPSteps = new Vector<PostProcessStep>();

    /**
     * Unique number representing the address of the internal
     * Assimp::Importer object. For 64 bit platforms it is something else ..
     * at least it is guaranted to be unique ;-)
     */
    private long m_iNativeHandle = 0xffffffffffffffffl;

    /**
     * Loaded scene. It can't be used after the Importer class instance
     * has been finalized!
     */
    private Scene scene = null;

    /**
     * Path to the scene loaded
     */
    private String path = null;

    /**
     * Public constructor. Initialises the JNI bridge to the native
     * ASSIMP library. A native Assimp::Importer object is constructed and
     * initialized. The flag list is set to zero, a default I/O handler
     * is constructed.
     *
     * @throws NativeError Thrown if the jassimp library could not be loaded
     *                     or if the entry point to the module wasn't found. if this exception
     *                     is not thrown, you can assume that jAssimp is fully available.
     */
    public Importer() throws NativeError {
        /** try to load the jassimp library. First try to load the
         * x64 version, in case of failure the x86 version
         */
        try {
            System.loadLibrary("jassimp64");
        }
        catch (UnsatisfiedLinkError exc) {
            try {
                System.loadLibrary("jassimp32");
            }
            catch (UnsatisfiedLinkError exc2) {
                throw new NativeError("Unable to load the jassimp library");
            }
        }
        // now create the native Importer class and setup our internal
        // data structures outside the VM.
        try {
            if (0xffffffffffffffffl == (this.m_iNativeHandle = _NativeInitContext())) {
                throw new NativeError(
                        "Unable to initialize the native library context." +
                                "The initialization routine has failed");
            }
        }
        catch (UnsatisfiedLinkError exc) {
            throw new NativeError(
                    "Unable to initialize the native library context." +
                            "The initialization routine has not been found");
        }
        return;
    }


    /**
     * Add a postprocess step to the list of steps to be executed on
     * the model. Postprocess steps are applied to the model after it
     * has been loaded. They are implemented in C/C++, this is only a wrapper.
     *
     * @param p_Step Postprocess step to be added
     * @return true if the step has been added successfully
     * @see PostProcessStep
     */
    public boolean addPostProcessStep(PostProcessStep p_Step) {

        if (isPostProcessStepActive(p_Step)) return false;
        this.m_vPPSteps.add(p_Step);
        return true;
    }


    /**
     * Check whether a given postprocess step is existing in the list
     * of all steps to be executed on the model. Postprocess steps are
     * applied to the model after it has been loaded. They are implemented
     * in C/C++, this is only a wrapper.
     *
     * @param p_Step Postprocess step to be queried
     * @return true if the step is active
     * @see PostProcessStep
     */
    public boolean isPostProcessStepActive(PostProcessStep p_Step) {

        for (PostProcessStep step : m_vPPSteps) {
            if (step.equals(p_Step)) return true;
        }
        return false;
    }


    /**
     * Remove a postprocess step from the list of steps to be executed
     * on the model. Postprocess steps are applied to the model after it
     * has been loaded. They are implemented in C/C++, this is only a wrapper.
     *
     * @param p_Step Postprocess step to be removed
     * @return true if the step has been removed successfully, false if
     *         it was not existing
     * @see PostProcessStep
     */
    public boolean removePostProcessStep(PostProcessStep p_Step) {

        return this.m_vPPSteps.remove(p_Step);
    }


    /**
     * Load a model from a file using the current list of postprocess steps
     * and the current I/O handler. If no custom I/O handler was provided,
     * a default implementation is used. This implementation uses fopen()/
     * fread()/fwrite()/fclose()/ftell()/fseek() and provides no support
     * for archives like ZIP or PAK.
     *
     * @param path Path to the file to be read
     * @return null if the import failed, otherwise a valid Scene instance
     * @throws NativeError This exception is thrown when an unknown error
     * occurs in the JNI bridge module.
     */
    public Scene readFile(String path) throws NativeError {
        this.scene = new Scene(this);
        this.path = path;

        // we need to build a path that is valid for the current OS
        char sep = System.getProperty("file.separator").charAt(0);
        if (sep != '\\') this.path = this.path.replace('\\', sep);
        if (sep != '/') this.path = this.path.replace('/', sep);

        // need to build a list of postprocess step as bitflag combination
        // Of course, this could have been implemented directly. However,
        // I've used the PostProcessStep enumeration to make debugging easier.
        int flags = 0;

        for (PostProcessStep step : m_vPPSteps) {
            if (step.equals(PostProcessStep.CalcTangentSpace)) flags |= 0x1;
            else if (step.equals(PostProcessStep.JoinIdenticalVertices)) flags |= 0x2;
            else if (step.equals(PostProcessStep.ConvertToLeftHanded)) flags |= 0x4;
            else if (step.equals(PostProcessStep.Triangulate)) flags |= 0x8;
            else if (step.equals(PostProcessStep.KillNormals)) flags |= 0x10;
            else if (step.equals(PostProcessStep.GenFaceNormals)) flags |= 0x20;
            else if (step.equals(PostProcessStep.GenSmoothNormals)) flags |= 0x40;
            else if (step.equals(PostProcessStep.SplitLargeMeshes)) flags |= 0x80;
        }

        // now load the mesh
        if (0xffffffff == this._NativeLoad(this.path, flags, this.m_iNativeHandle)) {
            this.scene = null;
            this.path = null;
            throw new NativeError("Failed to load the mesh");
        }
        // and setup our Scene object
        try {
          this.scene.construct();
        }
        catch (NativeError exc) {

            this.scene = null;
            this.path = null;
            throw exc;
        }
        return this.scene;
    }


    /**
     * Implementation of <code>java.lang.Object.equals()</code>
     *
     * @param o Object to be compred with *this*
     * @return true if *this* is equal to o
     */
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        final Importer importer = (Importer) o;

        if (m_iNativeHandle != importer.m_iNativeHandle) return false;

        return true;
    }

    /**
     * Implementation of <code>java.lang.Object.finalize()</code>
     * We override this to make sure that all native resources are
     * deleted properly. This will free the native Assimp::Importer object
     * and its associated aiScene instance. A NativeError is thrown
     * if the destruction failed. This means that not all resources have
     * been deallocated and memory leaks are remaining.
     */
    @Override
    protected void finalize() throws Throwable {
        super.finalize();

        // be sure that native resources are deallocated properly
        if (0xffffffff == _NativeFreeContext(this.m_iNativeHandle)) {
            throw new NativeError("Unable to destroy the native library context");
        }
        return;
    }

    /**
     * Implementation of <code>java.lang.Object.hashCode()</code>
     * <p/>
     * The native handle obtained from the JNI bridge is used as hash code.
     * It is assumed to be unique, in fact it is normally the address of
     * the native Assimp::Importer object.
     *
     * @return An unique value representing the object
     */
    @Override
    public int hashCode() {
        return (int) (m_iNativeHandle >> 32) ^ (int) (m_iNativeHandle);
    }


    /**
     * Retrieves the native context of the class. This is normally the
     * address of the native Importer object.
     * @return Native context
     */
    public long getContext() {
        return m_iNativeHandle;
    }


    /**
     * JNI bridge call. For internal use only
     * The method initializes the ASSIMP-JNI bridge for use. No native
     * function call to assimp will be successful unless this function has
     * been called. If the function is found by the Java VM (and no <code>
     * UnsatisfiedLinkError</code> is thrown), jAssimp assumes that all other
     * library functions are available, too. If they are not, an <code>
     * UnsatisfiedLinkError</code> will be thrown during model loading.
     *
     * @return Unique handle for the class or 0xffffffff if an error occured
     */
    private native int _NativeInitContext();

    /**
     * JNI bridge call. For internal use only
     * The method destroys the ASSIMP-JNI bridge. No native function call
     * to assimp will be successful after this method has been called.
     *
     * @return 0xffffffff if an error occured
     */
    private native int _NativeFreeContext(long iContext);

    /**
     * JNI bridge call. For internal use only
     * The method loads the model into memory, but does not map it into the VM
     *
     * @param path  Path (valid separators for the OS) to the model to be loaded
     * @param flags List of postprocess steps to be executed
     * @return 0xffffffff if an error occured
     */
    private native int _NativeLoad(String path, int flags, long iContext);
}
