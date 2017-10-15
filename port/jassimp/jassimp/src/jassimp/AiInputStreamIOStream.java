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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URI;
import java.net.URL;
import java.nio.ByteBuffer;


/**
 * Implementation of AiIOStream reading from a InputStream
 * 
 * @author Jesper Smith
 *
 */
public class AiInputStreamIOStream implements AiIOStream
{
   private final ByteArrayOutputStream os = new ByteArrayOutputStream(); 
   
   
   public AiInputStreamIOStream(URI uri) throws IOException {
      this(uri.toURL());
   }
   
   public AiInputStreamIOStream(URL url) throws IOException {
      this(url.openStream());
   }
   
   public AiInputStreamIOStream(InputStream is) throws IOException {
      int read;
      byte[] data = new byte[1024];
      while((read = is.read(data, 0, data.length)) != -1) {
         os.write(data, 0, read);
      }
      os.flush();
      
      is.close();
   }
   
   @Override
   public int getFileSize() {
      return os.size();
   }
   
   @Override
   public boolean read(ByteBuffer buffer) {
     ByteBufferOutputStream bos = new ByteBufferOutputStream(buffer);
     try
     {
        os.writeTo(bos);
     }
     catch (IOException e)
     {
        e.printStackTrace();
        return false;
     }
     return true;
   }
   
   /**
    * Internal helper class to copy the contents of an OutputStream
    * into a ByteBuffer. This avoids a copy.
    *
    */
   private static class ByteBufferOutputStream extends OutputStream {

      private final ByteBuffer buffer;
      
      public ByteBufferOutputStream(ByteBuffer buffer) {
         this.buffer = buffer;
      }
      
      @Override
      public void write(int b) throws IOException
      {
         buffer.put((byte) b);
      }
    
      @Override
      public void write(byte b[], int off, int len) throws IOException {
         buffer.put(b, off, len);
      }
   }
}

