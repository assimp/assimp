Open Asset Import Library (assimp) 
========

Open Asset Import Library is a Open Source library designed to load various __3d file formats and convert them into a single, in-memory format__. It supports more than 30 file formats. It also supports exporting files to a growing selection of file formats.

Its abbreviated name under which it is commonly known is __assimp__. 

This is the development trunk of assimp containing the latest features and bugfixes. For productive use though, we recommend one of the stable releases available from [assimp.sf.net](http://assimp.sf.net) or from *nix package repositories. According to [Travis-CI] (https://travis-ci.org/), the current build status of the trunk is [![Build Status](https://travis-ci.org/assimp/assimp.png)](https://travis-ci.org/assimp/assimp)

#### 1.1 Supported file formats ####

The library provides importers for a lot of file formats, including:

- 3DS
- BLEND (Blender 3D)
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
- Ogre XML
- Q3D
 
Additionally, the following formats are also supported, but not part of the core library as they depend on proprietary libraries.

- C4D (https://github.com/acgessler/assimp-cinema4d)

Exporters include:

- DAE (Collada)
- STL
- OBJ
- PLY
	
See [the full list here](http://assimp.sourceforge.net/main_features_formats.html).



#### Repository structure ####


Open Asset Import Library is implemented in C++ (but provides both a C and a 
C++ish interface). The directory structure is:

	/bin		Folder for binaries, only used on Windows
	/code		Source code
	/contrib	Third-party libraries
	/doc		Documentation (doxysource and pre-compiled docs)
	/include	Public header C and C++ header files
	/lib		Static library location for Windows
	/obj		Object file location for Windows
	/scripts 	Scripts used to generate the loading code for some formats
	/port		Ports to other languages and scripts to maintain those.
	/test		Unit- and regression tests, test suite of models
	/tools		Tools (viewer, command line `assimp`)
	/samples	A small number of samples to illustrate possible 
                        use cases for Assimp
	/workspaces	Build enviroments for vc,xcode,... (deprecated,
			CMake has superseeded all legacy build options!)



### Building ###


Take a look into the `INSTALL` file. Or fire up CMake with the usual steps.


### Where to get help ###


For more information, visit [our website](http://assimp.sourceforge.net/). Or check out the `./doc`- folder, which contains the official documentation in HTML format.
(CHMs for Windows are included in some release packages and should be located right here in the root folder).

If the documentation doesn't solve your problems, 
[try our forums at SF.net](http://sourceforge.net/p/assimp/discussion/817654) or ask on
[StackOverflow](http://stackoverflow.com/questions/tagged/assimp?sort=newest).

For development discussions, there is also a mailing list, _assimp-discussions_
  [(subscribe here)]( https://lists.sourceforge.net/lists/listinfo/assimp-discussions) 

### Contributing ###

Contributions to assimp are highly appreciated. The easiest way to get involved is to submit 
a pull request with your changes against the main repository's `master` branch.


### License ###

Our license is based on the modified, __3-clause BSD__-License, which is very liberal. 

An _informal_ summary is: do whatever you want, but include Assimp's license text with your product - 
and don't sue us if our code doesn't work. Note that, unlike LGPLed code, you may link statically to Assimp.
For the legal details, see the `LICENSE` file. 

