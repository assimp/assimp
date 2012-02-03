Open Asset Import Library (_assimp_) 
========


    Table of contents

	1.		Overview
	 1.1		Supported file formats
	 1.2		File structure
	2.		Build the library
	3. 		Where to get help
	4.		License




### 1. Overview ###


Open Asset Import Library is a Open Source library designed to load various 3d file formats and convert them into a shared, in-memory format. It supports more than 30 file formats. It also supports exporting files to a few selected file formats.

Its short name is _assimp_, which is an unintended joke (the abbreviation is derived from _Asset Importer_). 

__Note__: this `README` refers to the file structure used by release packages, which differs in some points from the development trunk.


#### 1.1 Supported file formats ####

The library provides importers for a lot of file formats, including:

- 3DS
- BLEND 
- DAE (Collada)
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
- XML 
- TER 
- AC3D 
- MS3D 

Exporters include:

- DAE (Collada)
- STL
- OBJ
	
See [the full list here](http://assimp.sourceforge.net/main_features_formats.html).



#### 1.2 Repository structure ####


Open Asset Import Library is implemented in C++ (but provides both a C and a 
C++ish interface). The directory structure is:

	/bin		Folder for binaries, only used on Windows
	/code		Source code
	/contrib	Third-party libraries
	/doc		Documentation (doxysource and pre-compiled docs)
	/include	Public header C and C++ header files.
	/lib		Static library location for Windows.
	/obj		Object file location for Windows.
	/port		Ports to other languages and scripts to maintain those. 
	/test		Unit- and regression tests, test suite of models.
	/tools		Tools (viewer, command line `assimp`).
	/samples	A small number of samples to illustrate possible 
                        use cases for Assimp.
	/workspaces	Build enviroments for vc,xcode,... (deprecated,
			CMake has superseeded all legacy build options!)



### 2. Build the library ###


Take a look into the `INSTALL` file. Or fire up CMake with the usual steps.



### 3. Where to get help ###


For more information, visit [our website](http://assimp.sourceforge.net/). Or check out the `./doc`- folder, which contains the official documentation in HTML format.
(CHMs for Windows are included in some release packages and should be located right here in the root folder).

If the documentation doesn't solve your problems, try our forums at SF.net 


- [Open Discussion](http://sourceforge.net/projects/assimp/forums/forum/817653) 
- [General Help](http://sourceforge.net/projects/assimp/forums/forum/817654)


For development stuff, there is also a mailing list, _assimp-discussions_
  [(subscribe here)]( https://lists.sourceforge.net/lists/listinfo/assimp-discussions) 



### 4. License ###

The license of the Asset Import Library is based on the modified, __3-clause BSD__-License, which is a very liberal license. An _informal_ summary is: do whatever you want, but include Assimp's license text with your product - and don't sue us if our code doesn't work.

Note that, unlike LGPLed code, you may link statically to Assimp.
For the formal details, see the `LICENSE` file. 


------------------------------

(This repository is a mirror of the SVN repository located [here](https://assimp.svn.sourceforge.net/svnroot/assimp). Thanks to [klickverbot](https://github.com/klickverbot) for setting this up!)