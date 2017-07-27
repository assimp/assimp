/*
 * Copyright 2017 Florida Institute for Human and Machine Cognition (IHMC)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *     
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */
package jassimp;

public interface AiIOSystem <T extends AiIOStream>
{
   /**
    * 
    * Open a new file with a given path.
    * When the access to the file is finished, call close() to release all associated resources
    * 
    * @param path Path to the file
    * @param ioMode file I/O mode. Required are: "wb", "w", "wt", "rb", "r", "rt".
    * 
    * @return AiIOStream or null if an error occurred
    */
   public T open(String path, String ioMode);
   
   
   /**
    * Tests for the existence of a file at the given path.
    *  
    * @param path path to the file
    * @return true if there is a file with this path, else false.
    */
   public boolean exists(String path);

   /**
    * Returns the system specific directory separator.<p>
    * 
    * @return System specific directory separator
    */
   public char getOsSeparator();
   
   /**
    * Closes the given file and releases all resources associated with it.
    * 
    * @param file The file instance previously created by Open().
    */
   public void close(T file);
}
