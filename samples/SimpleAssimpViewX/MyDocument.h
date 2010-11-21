//
//  MyDocument.h
//  DisplayLinkAsyncMoviePlayer
//
//  Created by vade on 10/26/10.
//  Copyright __MyCompanyName__ 2010 . All rights reserved.
//


#import "ModelLoaderHelperClasses.h"

// assimp include files. These three are usually needed.
#import "assimp.h"
#import "aiPostProcess.h"
#import "aiScene.h"

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <Quartz/Quartz.h>


@interface MyDocument : NSPersistentDocument 
{
    CVDisplayLinkRef _displayLink;
    NSOpenGLContext* _glContext;
    NSOpenGLPixelFormat* _glPixelFormat;
    
    NSView* _view;
    
    // Assimp Stuff
    aiScene* _scene;
    struct aiVector3D scene_min, scene_max, scene_center;
    double normalizedScale;    
    
    // Our array of textures.
    GLuint *textureIds;
    
    // only used if we use
    NSMutableArray* modelMeshes;   
    BOOL builtBuffers;
    
    NSMutableDictionary* textureDictionary;	// Array of Dicionaries that map image filenames to textureIds      
}

@property (retain) IBOutlet NSView* _view;


- (CVReturn)displayLinkRenderCallback:(const CVTimeStamp *)timeStamp;
- (void) render;

- (void) drawMeshesInContext:(CGLContextObj)cgl_ctx;
- (void) createGLResourcesInContext:(CGLContextObj)cgl_ctx;
- (void) deleteGLResourcesInContext:(CGLContextObj)cgl_ctx;

- (void) loadTexturesInContext:(CGLContextObj)cgl_ctx withModelPath:(NSString*) modelPath;
- (void) getBoundingBoxWithMinVector:(struct aiVector3D*) min maxVectr:(struct aiVector3D*) max;
- (void) getBoundingBoxForNode:(const struct aiNode*)nd  minVector:(struct aiVector3D*) min maxVector:(struct aiVector3D*) max matrix:(struct aiMatrix4x4*) trafo;

@end
