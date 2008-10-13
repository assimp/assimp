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
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.File;

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
     * Represents a property (key-value)
     */
    private class Property<Type> {
        String key;
        Type value;
    }


    /**
     * Represents a property list
     */
    private class PropertyList<Type> extends Vector<Property<Type>> {

        public void setProperty(final String prop, final Type val) {

            for (Property<Type> i : this) {
                if (i.key.equals(prop)) {
                    i.value = val;
                    return;
                }
            }

            Property<Type> propNew = new Property<Type>();
            propNew.key = prop;
            propNew.value = val;
            this.add(propNew);
        }

        public Type getProperty(final String prop) {

            for (Property<Type> i : this) {
                if (i.key.equals(prop)) {
                    return i.value;
                }
            }
            return null;
        }
    }

    /**
     * Default implementation of <code>IOStream</code>.
     * <br>
     * This might become a performance bottleneck: The application
     * needs to map the data read by this interface into a C-style
     * array. For every single read operation! Therefore it is a good
     * optimization to use the default C IOStream handler if no custom
     * java handler was specified. Assuming that the Java Runtime is using
     * the fXXX-family of functions internally too, the result should be
     * the same. The problem is: we can't be sure we'll be able to open
     * the file for reading from both Java and native code. Therefore we
     * need to close our Java <code>FileReader</code> handle before
     * control is given to native code.
     */
    private class DefaultIOStream implements IOStream {

        private FileReader reader = null;

        /**
         * Construction with a given path
         *
         * @param file Path to the file to be opened
         * @throws FileNotFoundException If the file isn't accessible at all
         */
        public DefaultIOStream(final String file) throws FileNotFoundException {
            reader = new FileReader(file);
        }

    }

    /**
     * Default implementation of <code>IOSystem</code>.
     */
    private class DefaultIOSystem implements IOSystem {

        /**
         * Called to check whether a file is existing
         *
         * @param file Filename
         * @return true if the file is existing and accessible
         */
        public boolean Exists(String file) {
            File f = new File(file);
            return f.exists();
        }

        /**
         * Open a file and return an <code> IOStream </code> interface
         * to access it.
         *
         * @param file File name of the file to be opened
         * @return A valid IOStream interface
         * @throws FileNotFoundException if the file can't be accessed
         */
        public IOStream Open(String file) throws FileNotFoundException {
            return new DefaultIOStream(file);
        }
    }


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
     * I/O system to be used
     */
    private IOSystem ioSystem = new DefaultIOSystem();

    /**
     * List of config properties for all supported types: int, float and string
     */
    private PropertyList<Integer> properties = new PropertyList<Integer>();
    private PropertyList<Float>   propertiesFloat = new PropertyList<Float>();
    private PropertyList<String>  propertiesString = new PropertyList<String>();


    /**
     * Specifies whether the native jAssimp library is currently
     * in a loaded state.
     */
    private static boolean bLibInitialized = false;


    private static final String JASSIMP_RUNTIME_NAME_X64 = "jAssimp64";
    private static final String JASSIMP_RUNTIME_NAME_X86 = "jAssimp32";

    public static final int PROPERTY_WAS_NOT_EXISTING = 0xffffffff;

    /**
     * Public constructor. Initialises the JNI bridge to the native
     * ASSIMP library. A native Assimp::Importer object is constructed and
     * initialized. The flag list is set to zero, a default I/O handler
     * is initialized.
     *
     * @param iVersion Version of the JNI interface to be used.
     * @throws NativeException Thrown if the jassimp library could not be loaded
     *                         or if the entry point to the module wasn't found. if this exception
     *                         is not thrown you can assume that jAssimp is fully available.
     */
    public Importer(int iVersion) throws NativeException {

        if (!bLibInitialized) {

            /** try to load the jassimp library. First try to load the
             * x64 version, in case of failure the x86 version
             */
            try {
                System.loadLibrary(JASSIMP_RUNTIME_NAME_X64);
            }
            catch (UnsatisfiedLinkError exc) {
                try {
                    System.loadLibrary(JASSIMP_RUNTIME_NAME_X86);
                }
                catch (UnsatisfiedLinkError exc2) {
                    throw new NativeException("Unable to load the jAssimp library");
                }
            }
            bLibInitialized = true;
        }
        // now create the native Importer class and setup our internal
        // data structures outside the VM.
        try {
            if (0xffffffffffffffffl == (this.m_iNativeHandle = _NativeInitContext(iVersion))) {
                throw new NativeException(
                        "Unable to initialize the native library context." +
                                "The initialization routine has failed");
            }
        }
        catch (UnsatisfiedLinkError exc) {
            throw new NativeException(
                    "Unable to initialize the native library context." +
                            "The initialization routine has not been found");
        }
    }

    public Importer() throws NativeException {
        this(0);
    }

    /**
     * Get the I/O system (<code>IOSystem</code>) to be used for loading
     * assets. If no custom implementation was provided via <code>setIoSystem()</code>
     * a default implementation will be used. Use <code>isDefaultIoSystem()</code>
     * to check this.
     *
     * @return Always a valid <code>IOSystem</code> object, never null.
     */
    public final IOSystem getIoSystem() {
        return ioSystem;
    }


    /**
     * Checks whether a default IO system is currently being used to load
     * assets. Using the default IO system has many performance benefits,
     * but it is possible to provide a custom IO system (<code>setIoSystem()</code>).
     * This allows applications to add support for archives like ZIP.
     *
     * @return true if a default <code>IOSystem</code> is active,
     */
    public final boolean isDefaultIoSystem() {
        return ioSystem instanceof DefaultIOSystem;
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
    public final boolean addPostProcessStep(PostProcessStep p_Step) {

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
    public final boolean isPostProcessStepActive(PostProcessStep p_Step) {

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
    public final boolean removePostProcessStep(PostProcessStep p_Step) {

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
     * @throws NativeException This exception is thrown when an unknown error
     *                         occurs in the JNI bridge module.
     */
    public final Scene readFile(String path) throws NativeException {
        this.scene = new Scene(this);
        this.path = path;

        // we need to build a path that is valid for the current OS
        char sep = System.getProperty("file.separator").charAt(0);
        if (sep != '\\') this.path = this.path.replace('\\', sep);
        if (sep != '/') this.path = this.path.replace('/', sep);

        // need to build a list of postprocess step as bitflag combination
        // Of course, this could have been implemented directly. However,
        // I've used the PostProcessStep enumeration to make debugging easier.
        int flags = 0x8; // always apply the triangulation step

        for (PostProcessStep step : m_vPPSteps) {
            if (step.equals(PostProcessStep.CalcTangentSpace)) flags |= 0x1;
            else if (step.equals(PostProcessStep.JoinIdenticalVertices)) flags |= 0x2;
            else if (step.equals(PostProcessStep.ConvertToLeftHanded)) flags |= 0x4;
            else if (step.equals(PostProcessStep.KillNormals)) flags |= 0x10;
            else if (step.equals(PostProcessStep.GenFaceNormals)) flags |= 0x20;
            else if (step.equals(PostProcessStep.GenSmoothNormals)) flags |= 0x40;
            else if (step.equals(PostProcessStep.SplitLargeMeshes)) flags |= 0x80;
            else if (step.equals(PostProcessStep.PreTransformVertices)) flags |= 0x100;
            else if (step.equals(PostProcessStep.LimitBoneWeights)) flags |= 0x200;
            else if (step.equals(PostProcessStep.ValidateDataStructure)) flags |= 0x400;
            else if (step.equals(PostProcessStep.ImproveVertexLocality)) flags |= 0x800;
            else if (step.equals(PostProcessStep.RemoveRedundantMaterials)) flags |= 0x1000;
            else if (step.equals(PostProcessStep.FixInfacingNormals)) flags |= 0x2000;
            else if (step.equals(PostProcessStep.OptimizeGraph)) flags |= 0x4000;
        }

        // now load the mesh
        if (0xffffffff == this._NativeLoad(this.path, flags, this.m_iNativeHandle)) {
            this.scene = null;
            this.path = null;
            throw new NativeException("Failed to load the mesh");
        }
        if (null == this.scene) {
            throw new NativeException("Failed to copy the data to Java");
        }
        return this.scene;
    }


    /**
     * Get the current scene or <code>null</code> if none is loaded
     * @return Hello Amanda, I want to play a game ...
     */
    public final Scene getScene()   {
        return scene;
    }


    /**
     * Get the source path of the current scene or <code>null</code> if none is loaded
     * @return Game Over.
     */
    public final String getScenePath() {
        return path;
    }


    /**
     * Implementation of <code>java.lang.Object.equals()</code>
     *
     * @param o Object to be compred with *this*
     * @return true if *this* is equal to o
     */
    public final boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        final Importer importer = (Importer) o;

        return m_iNativeHandle == importer.m_iNativeHandle;

    }

    /**
     * Implementation of <code>java.lang.Object.finalize()</code>
     * We override this to make sure that all native resources are
     * deleted properly. This will free the native Assimp::Importer object
     * and its associated aiScene instance. A NativeException is thrown
     * if the destruction failed. This means that not all resources have
     * been deallocated and memory leaks are remaining.
     */
    @Override
    protected void finalize() throws Throwable {
        super.finalize();

        // be sure that native resources are deallocated properly
        if (0xffffffff == _NativeFreeContext(this.m_iNativeHandle)) {
            throw new NativeException("Unable to destroy the native library context");
        }
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
     * Set an integer property. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @param prop Name of the config property
     * @param val  New value for the config property
     */
    public final void setPropertyInt(final String prop, int val) {

        this.properties.setProperty(prop, val);
        this._NativeSetPropertyInt(prop, val, this.getContext());
    }


    /**
     * Set a floating-point property. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @param prop Name of the config property
     * @param val  New value for the config property
     */
    public final void setPropertyFloat(final String prop, float val) {

        this.propertiesFloat.setProperty(prop, val);
        this._NativeSetPropertyFloat(prop, val, this.getContext());
    }


    /**
     * Set a string property. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @param prop Name of the config property
     * @param val  New value for the config property
     */
    public final void setPropertyString(final String prop, String val) {

        this.propertiesString.setProperty(prop, val);
        this._NativeSetPropertyString(prop, val, this.getContext());
    }


    /**
     * Gets an integer config property  that has been set using
     * <code>setPropertyInt</code>. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @param prop         Name of the config property
     * @param error_return Default return value if the property isn't there
     * @return Current value of the config property or
     *         error_return if the property has not yet been set
     */
    public final int getPropertyInt(final String prop, int error_return) {

        Integer i = this.properties.getProperty(prop);
        return i != null ? i : error_return;
    }


    /**
     * Gets a floating-point config property that has been set using
     * <code>setPropertyFloat</code>. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @see <code>getPropertyInt</code>
     */
    public final float getPropertyFloat(final String prop, float error_return) {

        Float i = this.propertiesFloat.getProperty(prop);
        return i != null ? i : error_return;
    }


    /**
     * Gets a string config property that has been set using
     * <code>setPropertyString</code>. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @see <code>getPropertyInt</code>
     */
    public final String getPropertyString(final String prop, String error_return) {

        String i = this.propertiesString.getProperty(prop);
        return i != null ? i : error_return;
    }

    /**
     * Gets an integer config property  that has been set using
     * <code>setPropertyInt</code>. All supported config properties are
     * defined as constants in the <code>ConfigProperty</code> class
     *
     * @param prop Name of the config property
     * @return Current of the property or <code>PROPERTY_WAS_NOT_EXISTING</code>
     *         if the property has not yet been set.
     */
    public final int getPropertyInt(final String prop) {
        return getPropertyInt(prop, PROPERTY_WAS_NOT_EXISTING);
    }

    /**
     * Retrieves the native context of the class. This is normally the
     * address of the native Importer object.
     *
     * @return Native context
     */
    public final long getContext() {
        return m_iNativeHandle;
    }


    // *********************************************************************************
    // JNI INTERNALS
    // *********************************************************************************

    /**
     * JNI bridge call. For internal use only
     * The method initializes the ASSIMP-JNI bridge for use. No native
     * function call to assimp will be successful unless this function has
     * been called. If the function is found by the Java VM (and no <code>
     * UnsatisfiedLinkError</code> is thrown), jAssimp assumes that all other
     * library functions are available, too. If they are not, an <code>
     * UnsatisfiedLinkError</code> will be thrown during model loading.
     *
     * @param version Version of the JNI bridge requested
     * @return Unique handle for the class or 0xffffffff if an error occured
     */
    private native int _NativeInitContext(int version);

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

    /**
     * JNI bridge call. For internal use only
     * The method sets a property
     *
     * @param name Name of the property
     * @param prop New value for the property
     * @return 0xffffffff if an error occured
     */
    private native int _NativeSetPropertyInt(String name,
 	int prop, long iContext);

    // float-version
    private native int _NativeSetPropertyFloat(String name,
       float prop, long iContext);

    // String-version
    private native int _NativeSetPropertyString(String name,
       String prop, long iContext);
}
