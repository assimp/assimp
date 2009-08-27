/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2009, ASSIMP Development Team

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

/**
 * Provides facilities for dynamically loading the Assimp library.
 *
 * Currently requires Tango, but there is no reason why Phobos could not be
 * supported too.
 */
module assimp.loader;

import assimp.api;
import tango.io.Stdout;
import tango.sys.SharedLib;

const uint ASSIMP_BINDINGS_MAJOR = 0;
const uint ASSIMP_BINDINGS_MINOR = 5;

/**
 * Loader class for dynamically loading the Assimp library.
 *
 * The library is »reference-counted«, meaning that the library is not
 * unloaded on a call to <code>unload()</code> if there are still other
 * references to it.
 */
struct Assimp {
public:
   /**
    * Loads the library if it is not already loaded and increases the
    * reference counter.
    *
    * The library file (<code>libassimp.so</code> on POSIX systems,
    * <code>Assimp32.dll</code> on Win32) is loaded via Tango's SharedLib
    * class.
    */
   static void load() {
      if ( m_sRefCount == 0 ) {
         version ( Posix ) {
            m_sLibrary = SharedLib.load( "libassimp.so" );
         }
         version ( Win32 ) {
            m_sLibrary = SharedLib.load( "Assimp32.dll" );
         }

         // Versioning
         bind( aiGetLegalString )( "aiGetLegalString" );
         bind( aiGetVersionMinor )( "aiGetVersionMinor" );
         bind( aiGetVersionMajor )( "aiGetVersionMajor" );
         bind( aiGetVersionRevision )( "aiGetVersionRevision" );
         bind( aiGetCompileFlags )( "aiGetCompileFlags" );

         // Check for version mismatch between the external, dynamically loaded
         // library and the version the bindings were created against.
         uint libMajor = aiGetVersionMajor();
         uint libMinor = aiGetVersionMinor();

         if ( ( libMajor < ASSIMP_BINDINGS_MAJOR ) ||
            ( libMinor < ASSIMP_BINDINGS_MINOR ) ) {
            Stdout.format(
               "WARNING: Assimp version too old (loaded library: {}.{}, " ~
                  "bindings: {}.{})!",
               libMajor,
               libMinor,
               ASSIMP_BINDINGS_MAJOR,
               ASSIMP_BINDINGS_MINOR
            ).newline;
         }

         if ( libMajor > ASSIMP_BINDINGS_MAJOR ) {
            Stdout.format(
               "WARNING: Assimp version too new (loaded library: {}.{}, " ~
                  "bindings: {}.{})!",
               libMajor,
               libMinor,
               ASSIMP_BINDINGS_MAJOR,
               ASSIMP_BINDINGS_MINOR
            ).newline;
         }

         // General API
         bind( aiImportFile )( "aiImportFile" );
         bind( aiImportFileEx )( "aiImportFileEx" );
         bind( aiImportFileFromMemory )( "aiImportFileFromMemory" );
         bind( aiApplyPostProcessing )( "aiApplyPostProcessing" );
         bind( aiGetPredefinedLogStream )( "aiGetPredefinedLogStream" );
         bind( aiAttachLogStream )( "aiAttachLogStream" );
         bind( aiEnableVerboseLogging )( "aiEnableVerboseLogging" );
         bind( aiDetachLogStream )( "aiDetachLogStream" );
         bind( aiDetachAllLogStreams )( "aiDetachAllLogStreams" );
         bind( aiReleaseImport )( "aiReleaseImport" );
         bind( aiGetErrorString )( "aiGetErrorString" );
         bind( aiIsExtensionSupported )( "aiIsExtensionSupported" );
         bind( aiGetExtensionList )( "aiGetExtensionList" );
         bind( aiGetMemoryRequirements )( "aiGetMemoryRequirements" );
         bind( aiSetImportPropertyInteger )( "aiSetImportPropertyInteger" );
         bind( aiSetImportPropertyFloat )( "aiSetImportPropertyFloat" );
         bind( aiSetImportPropertyString )( "aiSetImportPropertyString" );

         // Mathematical functions
         bind( aiCreateQuaternionFromMatrix )( "aiCreateQuaternionFromMatrix" );
         bind( aiDecomposeMatrix )( "aiDecomposeMatrix" );
         bind( aiTransposeMatrix4 )( "aiTransposeMatrix4" );
         bind( aiTransposeMatrix3 )( "aiTransposeMatrix3" );
         bind( aiTransformVecByMatrix3 )( "aiTransformVecByMatrix3" );
         bind( aiTransformVecByMatrix4 )( "aiTransformVecByMatrix4" );
         bind( aiMultiplyMatrix4 )( "aiMultiplyMatrix4" );
         bind( aiMultiplyMatrix3 )( "aiMultiplyMatrix3" );
         bind( aiIdentityMatrix3 )( "aiIdentityMatrix3" );
         bind( aiIdentityMatrix4 )( "aiIdentityMatrix4" );

         // Material system
         //bind( aiGetMaterialProperty )( "aiGetMaterialProperty" );
         bind( aiGetMaterialFloatArray )( "aiGetMaterialFloatArray" );
         bind( aiGetMaterialIntegerArray )( "aiGetMaterialIntegerArray" );
         bind( aiGetMaterialColor )( "aiGetMaterialColor" );
         bind( aiGetMaterialString )( "aiGetMaterialString" );
         bind( aiGetMaterialTextureCount )( "aiGetMaterialTextureCount" );
         bind( aiGetMaterialTexture )( "aiGetMaterialTexture" );
      }
      ++m_sRefCount;
   }

   /**
    * Decreases the reference counter and unloads the library if this was the
    * last reference.
    */
   static void unload() {
      assert( m_sRefCount > 0 );
      --m_sRefCount;

      if ( m_sRefCount == 0 ) {
         m_sLibrary.unload();
      }
   }

private:
   // The binding magic is heavily inspired by the Derelict loading code.
   struct Binder {
   public:
      static Binder opCall( void** functionPointerAddress ) {
         Binder binder;
         binder.m_functionPointerAddress = functionPointerAddress;
         return binder;
      }

      void opCall( char* name ) {
         *m_functionPointerAddress = m_sLibrary.getSymbol( name );
      }

   private:
       void** m_functionPointerAddress;
   }

   template bind( Function ) {
      static Binder bind( inout Function a ) {
         Binder binder = Binder( cast( void** ) &a );
         return binder;
      }
   }

   /// Current number of references to the library.
   static uint m_sRefCount;

   /// Library handle.
   static SharedLib m_sLibrary;
}
