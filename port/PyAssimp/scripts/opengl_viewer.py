#!/usr/bin/env python
#-*- coding: UTF-8 -*-

""" This program demonstrate the use of pyassimp to render
objects in OpenGL.

It loads a 3D model with ASSIMP and display it.

Textures are currently ignored.

This sample is based on several sources, including:
 - http://www.lighthouse3d.com/tutorials
 - http://code.activestate.com/recipes/325391/
 - ASSIMP's C++ SimpleOpenGL viewer
"""

import os, sys
from OpenGL.GLUT import *
from OpenGL.GLU import *
from OpenGL.GL import *
from OpenGL.arrays import ArrayDatatype

import logging;logger = logging.getLogger("assimp_opengl")
logging.basicConfig(level=logging.INFO)
import math
import numpy

from pyassimp import core as pyassimp
from pyassimp.postprocess import *
from pyassimp.helper import *


name = 'pyassimp OpenGL viewer'
height = 400
width = 708

zup = numpy.matrix([[1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1]])

class GLRenderer():
    def __init__(self):
        self.scene = None

        self.auto_rotate = False
        self.viewangle_h = self.viewangle_v = 0.0
        self.current_cam_index = 0

        # for FPS calculation
        self.prev_time = 0
        self.prev_fps_time = 0
        self.frames = 0

    def onkeypress(self, key, x, y):
        if key == 'a':
             self.auto_rotate = not self.auto_rotate
             self.viewangle_h = 0.
             self.viewangle_v = 0.
        if key == 'z':
            self.viewangle_v = -0.04
        if key == 's':
            self.viewangle_v = 0.04
        if key == 'q':
            self.viewangle_h = -0.04
        if key == 'd':
            self.viewangle_h = 0.04
        if key == 'c':
            self.fit_scene(restore = True)
            self.set_camera(self.cycle_cameras())
        if key == 'x':
            sys.exit(0)



    def prepare_gl_buffers(self, mesh):

        mesh.gl = {}

        # Fill the buffer for vertex positions
        mesh.gl["vertices"] = glGenBuffers(1)
        glBindBuffer(GL_ARRAY_BUFFER, mesh.gl["vertices"])
        glBufferData(GL_ARRAY_BUFFER, 
                    mesh.vertices,
                    GL_STATIC_DRAW)

        # Fill the buffer for normals
        mesh.gl["normals"] = glGenBuffers(1)
        glBindBuffer(GL_ARRAY_BUFFER, mesh.gl["normals"])
        glBufferData(GL_ARRAY_BUFFER, 
                    mesh.normals,
                    GL_STATIC_DRAW)


        # Fill the buffer for vertex positions
        mesh.gl["triangles"] = glGenBuffers(1)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.gl["triangles"])
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                    mesh.faces,
                    GL_STATIC_DRAW)

        # Unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER,0)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0)

    
    def load_dae(self, path, postprocess = None):
        #scene = pyassimp.load(path, aiProcessPreset_TargetRealtime_Quality)
        logger.info("Loading model:" + path + "...")

        if postprocess:
            self.scene = pyassimp.load(path, postprocess)
        else:
            self.scene = pyassimp.load(path)
        logger.info("Done.")

        scene = self.scene
        #log some statistics
        logger.info("  meshes: %d" % len(scene.meshes))
        logger.info("  total faces: %d" % sum([len(mesh.faces) for mesh in scene.meshes]))
        logger.info("  materials: %d" % len(scene.materials))
        self.bb_min, self.bb_max = get_bounding_box(self.scene)
        logger.info("  bounding box:" + str(self.bb_min) + " - " + str(self.bb_max))

        self.scene_center = [(a + b) / 2. for a, b in zip(self.bb_min, self.bb_max)]

        for index, mesh in enumerate(scene.meshes):
            self.prepare_gl_buffers(mesh)

        # Finally release the model
        pyassimp.release(scene)

    def cycle_cameras(self):
        self.current_cam_index
        if not self.scene.cameras:
            return None
        self.current_cam_index = (self.current_cam_index + 1) % len(self.scene.cameras)
        cam = self.scene.cameras[self.current_cam_index]
        logger.info("Switched to camera " + str(cam))
        return cam
    
    def set_camera(self, camera):
    
        znear = camera.clipplanenear
        zfar = camera.clipplanefar
        aspect = camera.aspect
        fov = camera.horizontalfov
    
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
    
    
        ### Compute gl frustrum
        # taken from http://www.songho.ca/opengl/gl_transform.html
    
        tangent = math.tan(fov/2.)
        height = znear * tangent
        width = height * aspect
    
        # params: left, right, bottom, top, near, far
        glFrustum(-width, width, -height, height, znear, zfar)
    
        # equivalent to:
        #gluPerspective(fov * 180/math.pi, aspect, znear, zfar)
    
    
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
    
        #cam_in_obj_coords = numpy.linalg.inv(camera.transformation)
        #glMultMatrixf(cam_in_obj_coords)
    
        #import pdb;pdb.set_trace()    
        
        cam = transform(camera.position, camera.transformation)
        at = transform(camera.lookat, camera.transformation)
        gluLookAt(cam[0], cam[2], -cam[1],
                  at[0], at[2], -at[1],
                  0,1,0)
    
    
    
    def fit_scene(self, restore = False):
        x_max = self.bb_max[0] - self.bb_min[0]
        y_max = self.bb_max[1] - self.bb_min[1]
        tmp = max(x_max, y_max)
        z_max = self.bb_max[2] - self.bb_min[2]
        tmp = max(z_max, tmp)
        
        if not restore:
            tmp = 1. / tmp

        logger.info("Scaling the scene by %.02f" % tmp)
        glScalef(tmp, tmp, tmp)
    
        # center the model
        direction = -1 if not restore else 1
        glTranslatef( direction * self.scene_center[0], 
                      direction * self.scene_center[1], 
                      direction * self.scene_center[2] )

        return x_max, y_max, z_max


    def apply_material(self, mat):
    
        if not hasattr(mat, "gl_mat"): # evaluate once the mat properties, and cache the values in a glDisplayList.
    
            diffuse = mat.properties.get("$clr.diffuse", numpy.array([0.8, 0.8, 0.8, 1.0]))
            specular = mat.properties.get("$clr.specular", numpy.array([0., 0., 0., 1.0]))
            ambient = mat.properties.get("$clr.ambient", numpy.array([0.2, 0.2, 0.2, 1.0]))
            emissive = mat.properties.get("$clr.emissive", numpy.array([0., 0., 0., 1.0]))
            shininess = mat.properties.get("$mat.shininess", 1.0)
            wireframe = mat.properties.get("$mat.wireframe", 0)
            twosided = mat.properties.get("$mat.twosided", 1)
    
            from OpenGL.raw import GL
            setattr(mat, "gl_mat", GL.GLuint(0))
            mat.gl_mat = glGenLists(1)
            glNewList(mat.gl_mat, GL_COMPILE)
    
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse)
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular)
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient)
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissive)
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE if wireframe else GL_FILL)
            glDisable(GL_CULL_FACE) if twosided else glEnable(GL_CULL_FACE)
    
            glEndList()
    
        glCallList(mat.gl_mat)
    
    def apply_transformation(self, m):
            m = m.transpose() # OpenGL row major
            glMultMatrixf(m)
    
    
    def recursive_render(self, node):
    
        # save model matrix and apply node transformation
        glPushMatrix()
        self.apply_transformation(node.transformation)
    
        for mesh in node.meshes:
            self.apply_material(mesh.material)
            
    
            glBindBuffer(GL_ARRAY_BUFFER, mesh.gl["vertices"])
            glEnableClientState(GL_VERTEX_ARRAY)
            glVertexPointer(3, GL_FLOAT, 0, None)
    
            glBindBuffer(GL_ARRAY_BUFFER, mesh.gl["normals"])
            glEnableClientState(GL_NORMAL_ARRAY)
            glNormalPointer(GL_FLOAT, 0, None)
    
            glDrawArrays(GL_TRIANGLES,0, len(mesh.faces)*3)
            #glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.gl["triangles"])
            #glDrawElements(GL_TRIANGLES,len(mesh.faces) * 3, GL_UNSIGNED_INT, 0)
    
            glDisableClientState(GL_VERTEX_ARRAY)
    
            glBindBuffer(GL_ARRAY_BUFFER, 0)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)
    
        for child in node.children:
            self.recursive_render(child)
    
        glPopMatrix()
    
   
    def do_motion(self):
    
        gl_time = glutGet(GLUT_ELAPSED_TIME)
    
        self.viewangle_h = (gl_time-self.prev_time)*0.03 if self.auto_rotate else self.viewangle_h
    
        self.prev_time = gl_time
    
        # Compute FPS
        self.frames += 1
        if gl_time - self.prev_fps_time >= 1000:
            current_fps = self.frames * 1000 / (gl_time - self.prev_fps_time)
            logger.info('%.0f fps' % current_fps)
            self.frames = 0
            self.prev_fps_time = gl_time
        
        glutPostRedisplay()
    
    def display(self):
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
    
        # rotate it around the y axis
        glRotatef(self.viewangle_h,0.,0.,1.)
        glRotatef(self.viewangle_v,0.,1.,0.)
    
   
        self.recursive_render(self.scene.rootnode)
    
        glutSwapBuffers()
        self.do_motion()
        return

    def render(self, filename=None, fullscreen = False, autofit = True):
        """

        :param autofit: if true, scale the scene to fit the whole geometry
        in the viewport.
        """
    
        # First initialize the openGL context
        glutInit(sys.argv)
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH)
        if not fullscreen:
            glutInitWindowSize(width, height)
            glutCreateWindow(name)
        else:
            glutGameModeString("1024x768")
            if glutGameModeGet(GLUT_GAME_MODE_POSSIBLE):
                glutEnterGameMode()
            else:
                print("Fullscreen mode not available!")
                sys.exit(1)
    
    
        self.load_dae(filename)
    
        glClearColor(0.,0.,0.,1.)
        glShadeModel(GL_SMOOTH)
        glEnable(GL_CULL_FACE)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_LIGHTING)
        lightZeroPosition = [10.,4.,10.,1.]
        lightZeroColor = [0.8,1.0,0.8,1.0] #green tinged
        glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition)
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor)
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1)
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05)
        glEnable(GL_LIGHT0)
    
        glutDisplayFunc(self.display)


        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(35.0, width/float(height) , 0.10, 100.0)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        gluLookAt(0.,0.,3., # pos
                  0.,0.,0., # look at
                  0.,1.,0.) # up vector


        if autofit:
            # scale the whole asset to fit into our view frustum·
            self.fit_scene()

        glPushMatrix()
    
        glutKeyboardFunc(self.onkeypress)
    
        glutMainLoop()
        return


if __name__ == '__main__':
    if not len(sys.argv) > 1:
        print("Usage: " + __file__ + " <model>")
        sys.exit(0)

    glrender = GLRenderer()
    glrender.render(sys.argv[1], fullscreen = False)

