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

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;

/**
 * IOSystem based on the Java classloader.<p>
 * 
 * This IOSystem allows loading models directly from the 
 * classpath. No extraction to the file system is 
 * necessary.
 * 
 * @author Jesper Smith
 *
 */
public class AiClassLoaderIOSystem implements AiIOSystem<AiInputStreamIOStream>
{
   private final Class<?> clazz;
   private final ClassLoader classLoader;
  
   /**
    * Construct a new AiClassLoaderIOSystem.<p>
    * 
    * This constructor uses a ClassLoader to resolve
    * resources.
    * 
    * @param classLoader classLoader to resolve resources.
    */
   public AiClassLoaderIOSystem(ClassLoader classLoader) {
      this.clazz = null;
      this.classLoader = classLoader;
   }

   /**
    * Construct a new AiClassLoaderIOSystem.<p>
    * 
    * This constructor uses a Class to resolve
    * resources.
    * 
    * @param class<?> class to resolve resources.
    */
   public AiClassLoaderIOSystem(Class<?> clazz) {
      this.clazz = clazz;
      this.classLoader = null;
   }
   

   @Override
   public AiInputStreamIOStream open(String filename, String ioMode) {
      try {
         
         InputStream is;
         
         if(clazz != null) {
            is = clazz.getResourceAsStream(filename);
         }
         else if (classLoader != null) {
            is = classLoader.getResourceAsStream(filename);
         }
         else {
            System.err.println("[" + getClass().getSimpleName() + 
                "] No class or classLoader provided to resolve " + filename);
            return null;
         }
         
         if(is != null) {
            return new AiInputStreamIOStream(is);
         }
         else {
            System.err.println("[" + getClass().getSimpleName() + 
                               "] Cannot find " + filename);
            return null;
         }
      }
      catch (IOException e) {
         e.printStackTrace();
         return null;
      }
   }

   @Override
   public void close(AiInputStreamIOStream file) {
   }

   @Override
   public boolean exists(String path)
   {
      URL url = null;
      if(clazz != null) {
         url = clazz.getResource(path);
      }
      else if (classLoader != null) {
         url = classLoader.getResource(path);
      }

      
      if(url == null)
      {
         return false;
      }
      else
      {
         return true;
      }
      
   }

   @Override
   public char getOsSeparator()
   {
      return '/';
   }

}
