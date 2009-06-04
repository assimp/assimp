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
 * Main class of jAssimp. Just a simple wrapper around the native Assimp API
 * (Assimp::Importer).
 * 
 * The java port exposes the full functionality of the library but differs in
 * some documented details. In order to use Assimp from multiple threads, each
 * thread should maintain its own <code>Importer</code> instance.
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public class Importer {

	/**
	 * Indicates that a requested property has not yet a defined value
	 */
	public static final int PROPERTY_WAS_NOT_EXISTING = 0xffffffff;
	
	/**
	 * Current interface version. Increment this if a change on one side of the
	 * language bridge will also require to change the other.
	 */
	public static final int ABI_VERSION = 1;

	/**
	 * Public constructor. Initializes the JNI bridge to the native Assimp
	 * library. The post processing flag list is set to zero and a default I/O
	 * handler is prepared for use.
	 * 
	 * @throws NativeException
	 *             Thrown if the required native jAssimp library could not be
	 *             located and mapped into address space. If the c'tor succeeds
	 *             with no exception, you can safely assume that jAssimp is
	 *             fully available for use.
	 */
	public Importer() throws NativeException {
		this(ABI_VERSION);
	}

	/**
	 * Get the I/O system (<code>IOSystem</code>) to be used for loading assets.
	 * If no custom implementation was provided via <code>setIoSystem()</code> a
	 * default implementation will be used. Use <code>isDefaultIoSystem()</code>
	 * to check this.
	 * 
	 * @return Always a valid <code>IOSystem</code> object, never null.
	 */
	public final IOSystem getIoSystem() {
		return ioSystem;
	}

	/**
	 * Checks whether a default IO system is currently being used to load
	 * assets. Using the default IO system has many performance benefits, but it
	 * is possible to provide a custom IO system (<code>setIoSystem()</code>).
	 * This allows applications to add support for archives like ZIP.
	 * 
	 * @return true if a default <code>IOSystem</code> is active,
	 */
	public final boolean isDefaultIoSystem() {
		return ioSystem instanceof DefaultIOSystem;
	}

	/**
	 * Load a model from a file using the current list of post processing steps
	 * and the current I/O handler. If no custom I/O handler was provided a
	 * default implementation is used. This implementation uses basic OS file
	 * handling and provides no support for direct reading from archives
	 * formats, such as ZIP or PAK.
	 * 
	 * @param path
	 *            Path to the file to be read
	 * @param flags
	 *            List of post processing steps to be applied. Any values from
	 *            the <code>PostProcessing</code> 'enum', or'ed together.
	 * @return <code>null</code> if the import failed, otherwise a valid
	 *         <code>Scene</code> instance. The <code>Importer</code> keeps a
	 *         reference to it, use <code>getScene()</code> to query it.
	 * @throws NativeException
	 *             Thrown if an unknown error occurs somewhere in the magic JNI
	 *             bridge module.
	 */
	public final Scene readFile(String path, long flags) throws NativeException {
		this.scene = new Scene(this);
		this.path = path;

		// now load the scene and hope and pray for success
		if (0xffffffff == this._NativeLoad(this.path, flags,
				this.m_iNativeHandle)) {
			this.scene = null;
			this.path = null;
			throw new NativeException("Failed to load the scene");
		}
		if (null == this.scene) {
			throw new NativeException("Failed to transfer the scene data to VM");
		}
		return this.scene;
	}

	/**
	 * Get the current scene or <code>null</code> if none is loaded
	 * 
	 * @return Current scene. The Importer keeps a permanent reference to it
	 *         until it is replaced by a new scene. Nevertheless you may access
	 *         if even after the Importer has been finalized.
	 */
	public final Scene getScene() {
		return scene;
	}

	/**
	 * Get the source path of the current scene or <code>null</code> if none is
	 * in a loaded state.
	 * 
	 * @return Game Over.
	 */
	public final String getScenePath() {
		return path;
	}

	/**
	 * Implementation of <code>java.lang.Object.equals()</code>
	 * 
	 * @param o
	 *            Object to be compared with *this*
	 * @return true if *this* is equal to o
	 */
	public final boolean equals(Object o) {
		if (this == o)
			return true;
		if (o == null || getClass() != o.getClass())
			return false;

		final Importer importer = (Importer) o;

		// comparing native handles is unique
		return m_iNativeHandle == importer.m_iNativeHandle;

	}

	/**
	 * Implementation of <code>java.lang.Object.finalize()</code> We override
	 * this to make sure that all native resources are deleted properly. This
	 * will free the native Assimp::Importer object and its associated aiScene
	 * instance. A NativeException is thrown if the destruction fails. This can
	 * mean that not all resources have been deallocated and memory leaks are
	 * remaining but it will not affect the stability of the VM (hopefully).
	 */
	@Override
	protected void finalize() throws Throwable {
		super.finalize();

		// be sure that native resources are deallocated properly
		if (0xffffffff == _NativeFreeContext(this.m_iNativeHandle)) {
			throw new NativeException(
					"Unable to destroy the native library context");
		}
	}

	/**
	 * Implementation of <code>java.lang.Object.hashCode()</code>
	 * <p/>
	 * The native handle obtained from the JNI bridge is used as hash code. It
	 * is always unique.
	 * 
	 * @return An unique value representing the object
	 */
	@Override
	public int hashCode() {
		return (int) (m_iNativeHandle >> 32) ^ (int) (m_iNativeHandle);
	}

	/**
	 * Set an integer property. All supported configuration properties are
	 * defined as constants in the <code>ConfigProperty</code> class
	 * 
	 * @param prop
	 *            Name of the configuration property
	 * @param val
	 *            New value for the configuration property
	 */
	public final void setPropertyInt(final String prop, int val) {

		this.properties.setProperty(prop, val);
		this._NativeSetPropertyInt(prop, val, this.getContext());
	}

	/**
	 * Set a floating-point property. All supported import properties are
	 * defined as constants in the <code>ConfigProperty</code> class
	 * 
	 * @param prop
	 *            Name of the configuration property
	 * @param val
	 *            New value for the configuration property
	 */
	public final void setPropertyFloat(final String prop, float val) {

		this.propertiesFloat.setProperty(prop, val);
		this._NativeSetPropertyFloat(prop, val, this.getContext());
	}

	/**
	 * Set a string property. All supported import properties are defined as
	 * constants in the <code>ConfigProperty</code> class
	 * 
	 * @param prop
	 *            Name of the configuration property
	 * @param val
	 *            New value for the configuration property
	 */
	public final void setPropertyString(final String prop, String val) {

		this.propertiesString.setProperty(prop, val);
		this._NativeSetPropertyString(prop, val, this.getContext());
	}

	/**
	 * Gets an integer configuration property that has been set using
	 * <code>setPropertyInt</code>. All supported import properties are defined
	 * as constants in the <code>ConfigProperty</code> class
	 * 
	 * @param prop
	 *            Name of the configuration property
	 * @param error_return
	 *            Default return value if the property isn't there
	 * @return Current value of the configuration property or error_return if
	 *         the property has not yet been set
	 */
	public final int getPropertyInt(final String prop, int error_return) {

		Integer i = this.properties.getProperty(prop);
		return i != null ? i : error_return;
	}

	/**
	 * Gets a floating-point configuration property that has been set using
	 * <code>setPropertyFloat</code>. All supported import properties are
	 * defined as constants in the <code>ConfigProperty</code> class
	 * 
	 * @see <code>getPropertyInt</code>
	 */
	public final float getPropertyFloat(final String prop, float error_return) {

		Float i = this.propertiesFloat.getProperty(prop);
		return i != null ? i : error_return;
	}

	/**
	 * Gets a string configuration property that has been set using
	 * <code>setPropertyString</code>. All supported import properties are
	 * defined as constants in the <code>ConfigProperty</code> class
	 * 
	 * @see <code>getPropertyInt</code>
	 */
	public final String getPropertyString(final String prop, String error_return) {

		String i = this.propertiesString.getProperty(prop);
		return i != null ? i : error_return;
	}

	/**
	 * Gets an integer configuration property that has been set using
	 * <code>setPropertyInt</code>. All supported import properties are defined
	 * as constants in the <code>ConfigProperty</code> class
	 * 
	 * @param prop
	 *            Name of the configuration property
	 * @return Current of the property or <code>PROPERTY_WAS_NOT_EXISTING</code>
	 *         if the property has not yet been set.
	 */
	public final int getPropertyInt(final String prop) {
		return getPropertyInt(prop, PROPERTY_WAS_NOT_EXISTING);
	}

	/**
	 * Retrieves the native context of the class. This is normally the address
	 * of the native Importer object.
	 * 
	 * @return Native context
	 */
	public final long getContext() {
		return m_iNativeHandle;
	}

	/**
	 * Represents a property (key-value)
	 */
	private class Property<Type> {
		String key;
		Type value;
	}

	/**
	 * Represents a property list. This exposes Assimp's configuration interface
	 * from Java.
	 */
	private class PropertyList<Type> extends Vector<Property<Type>> {

		/**
		 * 
		 */
		private static final long serialVersionUID = -990406536792129089L;

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
	 * Default implementation of <code>IOStream</code>. <br>
	 * This might become a performance bottleneck: The application needs to map
	 * the data read via this interface into a C-style array. For every single
	 * read operation! Therefore it's a good optimization to use the default C
	 * IOStream handler if no custom java handler was specified. Assuming that
	 * the Java Runtime is using the fXXX-family of functions internally too,
	 * the result should be the same.
	 */
	private class DefaultIOStream implements IOStream {

		@SuppressWarnings("unused")
		private FileReader reader = null;

		/**
		 * Construction from a given path
		 * 
		 * @param file
		 *            Path to the file to be opened
		 * @throws FileNotFoundException
		 *             If the file isn't accessible at all
		 */
		public DefaultIOStream(final String file) throws FileNotFoundException {
			reader = new FileReader(file);
		}

	}

	/**
	 * Default implementation of <code>IOSystem</code>.
	 * 
	 * Again, we're assuming that the Java runtime accesses the file system
	 * similar to our own native default implementation. This allows this piece
	 * of code to reside in Java.
	 */
	private class DefaultIOSystem implements IOSystem {

		/**
		 * Called to check whether a file is existing
		 * 
		 * @param file
		 *            Filename
		 * @return true if the file is existing and accessible
		 */
		public boolean Exists(String file) {
			File f = new File(file);
			return f.exists();
		}

		/**
		 * Open a file and return an <code> IOStream </code> interface to access
		 * it.
		 * 
		 * @param file
		 *            File name of the file to be opened
		 * @return A valid IOStream interface
		 * @throws FileNotFoundException
		 *             if the file can't be accessed
		 */
		public IOStream Open(String file) throws FileNotFoundException {
			return new DefaultIOStream(file);
		}
	}

	/**
	 * Unique number to identify the internal Assimp::Importer object which
	 * belongs to a Java Importer. This value is opaque and may not be changed
	 * from within Java.
	 */
	private long m_iNativeHandle = 0xffffffffffffffffl;

	/**
	 * Loaded scene. It is - unlike in native Assimp - not bound to its father
	 * Importer.
	 */
	private Scene scene = null;

	/**
	 * Path to the scene to be loaded
	 */
	private String path = null;

	/**
	 * I/O system to be used for importing
	 */
	private IOSystem ioSystem = new DefaultIOSystem();

	/**
	 * List of configuration properties for the three supported types integer,
	 * float and string.
	 */
	private PropertyList<Integer> properties = new PropertyList<Integer>();
	private PropertyList<Float> propertiesFloat = new PropertyList<Float>();
	private PropertyList<String> propertiesString = new PropertyList<String>();

	/**
	 * Specifies whether the native jAssimp library is currently in a properly
	 * initialized and responding state.
	 */
	private static boolean bLibInitialized = false;

	/**
	 * Expected names for native runtime libraries.
	 */
	private static final String JASSIMP_RUNTIME_NAME_X64 = "assimp-jbridge64";
	private static final String JASSIMP_RUNTIME_NAME_X86 = "assimp-jbridge32";

	/**
	 * Private constructor for internal use.
	 * 
	 * @param Interface
	 *            version. Increment this if a change has global effect.
	 * @throws NativeException
	 */
	private Importer(int iVersion) throws NativeException {

		if (!bLibInitialized) {

			/**
			 * Try to load the jAssimp library. First attempt to load the x64
			 * version, in case of failure the x86 version
			 */
			try {
				System.loadLibrary(JASSIMP_RUNTIME_NAME_X64);
			} catch (UnsatisfiedLinkError exc) {
				try {
					System.loadLibrary(JASSIMP_RUNTIME_NAME_X86);
				} catch (UnsatisfiedLinkError exc2) {
					throw new NativeException(
							"Unable to load the jAssimp library");
				}
			}
			bLibInitialized = true;
		}
		// Now create the native Importer class and setup our internal
		// data structures outside the VM.
		try {
			if (0xffffffffffffffffl == (m_iNativeHandle = _NativeInitContext(iVersion))) {
				throw new NativeException(
						"Failed to initialize jAssimp: interface version not matching");
			}
		} catch (UnsatisfiedLinkError exc) {
			throw new NativeException(
					"Failed to initialize jAssimp: entry point not found");
		}
	}

	// --------------------------------------------------------------------------
	// JNI INTERNALS
	// --------------------------------------------------------------------------

	/**
	 * JNI interface call
	 */
	private native int _NativeInitContext(int version);

	/**
	 * JNI interface call
	 */
	private native int _NativeFreeContext(long iContext);

	/**
	 * JNI interface call
	 */
	private native int _NativeLoad(String path, long flags, long iContext);

	/**
	 * JNI interface call
	 */
	private native int _NativeSetPropertyInt(String name, int prop,
			long iContext);

	/**
	 * JNI interface call
	 */
	private native int _NativeSetPropertyFloat(String name, float prop,
			long iContext);

	/**
	 * JNI interface call
	 */
	private native int _NativeSetPropertyString(String name, String prop,
			long iContext);
}
