//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#ifndef OPENSUBDIV3_OSD_OPENGL_H
#define OPENSUBDIV3_OSD_OPENGL_H

#if defined(__APPLE__)
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #include <OpenGLES/ES2/gl.h>
    #else
        #if defined(OSD_USES_GLEW)
            #include <GL/glew.h>
        #else
            #include <OpenGL/gl3.h>
        #endif
    #endif
#elif defined(ANDROID)
    #include <GLES2/gl2.h>
#else
    #if defined(_WIN32)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
    #endif
    #if defined(OSD_USES_GLEW)
        #include <GL/glew.h>
    #else
        #include <GL/gl.h>
    #endif
#endif

#endif  // OPENSUBDIV3_OSD_OPENGL_H
