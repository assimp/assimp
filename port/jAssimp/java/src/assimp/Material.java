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
 * Class to wrap materials. Materials are represented in ASSIMP as a list of
 * key/value pairs, the key being a <code>String</code> and the value being a
 * binary buffer. The class provides several get methods to access material
 * properties easily.
 *
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public class Material {

	/**
	 * Internal representation of a material property
	 */
	private class Property {
		String key;
		Object value;
	}

	/**
	 * List of all properties for this material
	 */
	private Property[] properties;

	/**
	 * Special exception class which is thrown if a material property could not
	 * be found.
	 */
	@SuppressWarnings("serial")
	public class PropertyNotFoundException extends Exception {
		public final String property_key;

		/**
		 * Constructs a new exception
		 * 
		 * @param message
		 *            Error message
		 * @param property_key
		 *            Name of the property that wasn't found
		 */
		public PropertyNotFoundException(String message, String property_key) {
			super(message);
			this.property_key = property_key;
		}
	}

	/**
	 * Get the value for a specific key from the material.
	 * 
	 * @param key
	 *            Raw name of the property to be queried
	 * @return null if the property wasn't there or hasn't the desired output
	 *         type. The returned <code>Object</code> can be casted to the
	 *         expected data type for the property. Primitive types are
	 *         represented by their boxed variants.
	 */
	public Object getProperty(String key) throws PropertyNotFoundException {

		for (Property prop : properties) {
			if (prop.key.equals(key)) {
				return prop.value;
			}
		}
		throw new PropertyNotFoundException(
				"Unable to find material property: ", key);
	}

	/**
	 * Get the floating-point value for a specific key from the material.
	 * 
	 * @param key
	 *            One of the constant key values defined in <code>MatKey</code>
	 * @return the value of the property
	 * @throws PropertyNotFoundException
	 *             If the property isn't set.
	 */
	public Float getProperty(MatKey.Any<Float> key)
			throws PropertyNotFoundException {

		return (Float) getProperty(key.name);
	}
	
	/**
	 * Get the integer value for a specific key from the material.
	 * 
	 * @param key
	 *            One of the constant key values defined in <code>MatKey</code>
	 * @return the value of the property
	 * @throws PropertyNotFoundException
	 *             If the property isn't set.
	 */
	public Integer getProperty(MatKey.Any<Integer> key)
			throws PropertyNotFoundException {

		return (Integer) getProperty(key.name);
	}
	
	/**
	 * Get the floating-point-array value for a specific key from the material.
	 * 
	 * @param key
	 *            One of the constant key values defined in <code>MatKey</code>
	 * @return the value of the property
	 * @throws PropertyNotFoundException
	 *             If the property isn't set.
	 */
	public float[] getProperty(MatKey.Any<float[]> key)
			throws PropertyNotFoundException {

		return (float[]) getProperty(key.name);
	}
	
	/**
	 * Get the integer-array value for a specific key from the material.
	 * 
	 * @param key
	 *            One of the constant key values defined in <code>MatKey</code>
	 * @return the value of the property
	 * @throws PropertyNotFoundException
	 *             If the property isn't set.
	 */
	public int[] getProperty(MatKey.Any<int[]> key)
			throws PropertyNotFoundException {

		return (int[]) getProperty(key.name);
	}
	
	/**
	 * Get the string value for a specific key from the material.
	 * 
	 * @param key
	 *            One of the constant key values defined in <code>MatKey</code>
	 * @return the value of the property
	 * @throws PropertyNotFoundException
	 *             If the property isn't set.
	 */
	public String getProperty(MatKey.Any<String> key)
			throws PropertyNotFoundException {

		return (String) getProperty(key.name);
	}

}
