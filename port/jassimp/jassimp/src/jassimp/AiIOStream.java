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

import java.nio.ByteBuffer;


/**
 * Interface to allow custom resource loaders for jassimp.<p>
 *
 * The design is based on passing the file wholly in memory, 
 * because Java inputstreams do not have to support seek. <p>
 * 
 * Writing files from Java is unsupported.
 * 
 * 
 * @author Jesper Smith
 *
 */
public interface AiIOStream
{

   /**
    * Read all data into buffer. <p>
    * 
    * The whole stream should be read into the buffer. 
    * No support is provided for partial reads. 
    * 
    * @param buffer Target buffer for the model data
    * 
    * @return true if successful, false if an error occurred.
    */
   boolean read(ByteBuffer buffer);

   /**
    * The total size of this stream. <p>
    *  
    * @return total size of this stream
    */
   int getFileSize();

}
