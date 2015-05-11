Open Asset Import Library (assimp) 
========

Open Asset Import Library is a library to load various 3d file formats into a shared, in-memory format. It supports more than __40 file formats__ for import and a growing selection of file formats for export.

APIs are provided for C and C++. There are various bindings to other languages (C#, Java, Python, Delphi, D). Assimp also runs on Android and iOS.

Additionally, assimp features various __mesh post processing tools__: normals and tangent space generation, triangulation, vertex cache locality optimization, removal of degenerate primitives and duplicate vertices, sorting by primitive type, merging of redundant materials and many more.

This is the development trunk containing the latest features and bugfixes. For productive use though, we recommend one of the stable releases available from [assimp.sf.net](http://assimp.sf.net) or from *nix package repositories. According to [Travis-CI] (https://travis-ci.org/), the current build status is:

[![Build Status](https://travis-ci.org/assimp/assimp.png)](https://travis-ci.org/assimp/assimp)

__[open3mod](https://github.com/acgessler/open3mod) is a powerful 3D model viewer based on Assimp's import and export abilities.__

#### Supported file formats ####

A full list [is here](http://assimp.sourceforge.net/main_features_formats.html).
__Importers__:

- 3DS
- BLEND (Blender)
- DAE/Collada
- FBX
- IFC-STEP 
- ASE
- DXF
- HMP
- MD2
- MD3 
- MD5
- MDC
- MDL
- NFF
- PLY
- STL
- X 
- OBJ 
- SMD
- LWO 
- LXO 
- LWS  
- TER 
- AC3D 
- MS3D 
- COB
- Q3BSP
- XGL
- CSM
- BVH
- B3D
- NDO
- Ogre Binary
- Ogre XML
- Q3D
- ASSBIN (Assimp custom format)
 
Additionally, some formats are supported by dependency on non-free code or external SDKs (not built by default):

- C4D (https://github.com/acgessler/assimp-cinema4d)

__Exporters__:

- DAE (Collada)
- STL
- OBJ
- PLY
- X
- 3DS
- JSON (for WebGl, via https://github.com/acgessler/assimp2json)
- ASSBIN
	
### Building ###


Take a look into the `INSTALL` file. Our build system is CMake, if you used CMake before there is a good chance you know what to do.


#### Repository structure ####


Open Asset Import Library is implemented in C++. The directory structure is:

	/code		Source code
	/contrib	Third-party libraries
	/doc		Documentation (doxysource and pre-compiled docs)
	/include	Public header C and C++ header files
	/scripts 	Scripts used to generate the loading code for some formats
	/port		Ports to other languages and scripts to maintain those.
	/test		Unit- and regression tests, test suite of models
	/tools		Tools (old assimp viewer, command line `assimp`)
	/samples	A small number of samples to illustrate possible 
                        use cases for Assimp
	/workspaces	Build enviroments for vc,xcode,... (deprecated,
			CMake has superseeded all legacy build options!)


### Where to get help ###


For more information, visit [our website](http://assimp.sourceforge.net/). Or check out the `./doc`- folder, which contains the official documentation in HTML format.
(CHMs for Windows are included in some release packages and should be located right here in the root folder).

If the docs don't solve your problem, ask on [StackOverflow](http://stackoverflow.com/questions/tagged/assimp?sort=newest). If you think you found a bug, please open an issue on Github.

For development discussions, there is also a (very low-volume) mailing list, _assimp-discussions_
  [(subscribe here)]( https://lists.sourceforge.net/lists/listinfo/assimp-discussions) 

### Contributing ###

Contributions to assimp are highly appreciated. The easiest way to get involved is to submit 
a pull request with your changes against the main repository's `master` branch.

### License ###

Our license is based on the modified, __3-clause BSD__-License. 

An _informal_ summary is: do whatever you want, but include Assimp's license text with your product - 
and don't sue us if our code doesn't work. Note that, unlike LGPLed code, you may link statically to Assimp.
For the legal details, see the `LICENSE` file. 

