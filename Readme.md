Open Asset Import Library (assimp)
==================================
A library to import and export various 3d-model-formats including scene-post-processing to generate missing render data.
### Current project status ###
[![Financial Contributors on Open Collective](https://opencollective.com/assimp/all/badge.svg?label=financial+contributors)](https://opencollective.com/assimp) 
![C/C++ CI](https://github.com/assimp/assimp/workflows/C/C++%20CI/badge.svg)
<a href="https://scan.coverity.com/projects/5607">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/5607/badge.svg"/>
</a>
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/9973693b7bdd4543b07084d5d9cf4745)](https://www.codacy.com/gh/assimp/assimp/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=assimp/assimp&amp;utm_campaign=Badge_Grade)
[![Coverage Status](https://coveralls.io/repos/github/assimp/assimp/badge.svg?branch=master)](https://coveralls.io/github/assimp/assimp?branch=master)
[![Join the chat at https://gitter.im/assimp/assimp](https://badges.gitter.im/assimp/assimp.svg)](https://gitter.im/assimp/assimp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/assimp/assimp.svg)](http://isitmaintained.com/project/assimp/assimp "Average time to resolve an issue")
[![Total alerts](https://img.shields.io/lgtm/alerts/g/assimp/assimp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/assimp/assimp/alerts/)
<br>

APIs are provided for C and C++. There are various bindings to other languages (C#, Java, Python, Delphi, D). Assimp also runs on Android and iOS.
Additionally, assimp features various __mesh post processing tools__: normals and tangent space generation, triangulation, vertex cache locality optimization, removal of degenerate primitives and duplicate vertices, sorting by primitive type, merging of redundant materials and many more.

### Latest Doc's ###
Please check the latest documents at [Asset-Importer-Lib-Doc](https://assimp-docs.readthedocs.io/en/latest/). 

### Get involved ###
This is the development repo containing the latest features and bugfixes. For productive use though, we recommend one of the stable releases available from [Github Assimp Releases](https://github.com/assimp/assimp/releases).
<br>
You find a bug in the docs? Use [Doc-Repo](https://github.com/assimp/assimp-docs).
<br>
Please check our Wiki as well: https://github.com/assimp/assimp/wiki

If you want to check our Model-Database, use the following repo: https://github.com/assimp/assimp-mdb

#### Supported file formats ####
You can find the complete list of supported file-formats [here](https://github.com/assimp/assimp/blob/master/doc/Fileformats.md)

### Building ###
Take a look into the https://github.com/assimp/assimp/blob/master/Build.md file. We are available in vcpkg, and our build system is CMake; if you used CMake before there is a good chance you know what to do.

### Ports ###
* [Android](port/AndroidJNI/README.md)
* [Python](port/PyAssimp/README.md)
* [.NET](https://bitbucket.org/Starnick/assimpnet/src/master/)
* [Pascal](port/AssimpPascal/Readme.md)
* [Javascript (Alpha)](https://github.com/makc/assimp2json)
* [Javascript/Node.js Interface](https://github.com/kovacsv/assimpjs)
* [Unity 3d Plugin](https://ricardoreis.net/trilib-2/)
* [Unreal Engine Plugin](https://github.com/irajsb/UE4_Assimp/)
* [JVM](https://github.com/kotlin-graphics/assimp) Full jvm port (current [status](https://github.com/kotlin-graphics/assimp/wiki/Status))
* [HAXE-Port](https://github.com/longde123/assimp-haxe) The Assimp-HAXE-port.
* [Rust](https://github.com/jkvargas/russimp)

### Other tools ###
[open3mod](https://github.com/acgessler/open3mod) is a powerful 3D model viewer based on Assimp's import and export abilities.

#### Repository structure ####
Open Asset Import Library is implemented in C++. The directory structure looks like:

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

The source code is organized in the following way:

	code/Common			The base implementation for importers and the infrastructure
	code/PostProcessing		The post-processing steps
	code/AssetLib/<FormatName>	Implementation for import and export for the format

### Where to get help ###
For more information, visit [our website](http://assimp.org/). Or check out the `./doc`- folder, which contains the official documentation in HTML format.
(CHMs for Windows are included in some release packages and should be located right here in the root folder).

If the docs don't solve your problem, ask on [StackOverflow with the assimp-tag](http://stackoverflow.com/questions/tagged/assimp?sort=newest). If you think you found a bug, please open an issue on Github.

Open Asset Import Library is a library to load various 3d file formats into a shared, in-memory format. It supports more than __40 file formats__ for import and a growing selection of file formats for export.

And we also have a Gitter-channel:Gitter [![Join the chat at https://gitter.im/assimp/assimp](https://badges.gitter.im/assimp/assimp.svg)](https://gitter.im/assimp/assimp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)<br>

### Contributing ###
Contributions to assimp are highly appreciated. The easiest way to get involved is to submit
a pull request with your changes against the main repository's `master` branch.

## Contributors

### Code Contributors

This project exists thanks to all the people who contribute. [[Contribute](CONTRIBUTING.md)].

<a href="https://github.com/assimp/assimp/graphs/contributors"><img src="https://opencollective.com/assimp/contributors.svg?width=890&button=false" /></a>

### Financial Contributors

Become a financial contributor and help us sustain our community. [[Contribute](https://opencollective.com/assimp/contribute)]

#### Individuals

<a href="https://opencollective.com/assimp"><img src="https://opencollective.com/assimp/individuals.svg?width=890"></a>


#### Organizations

Support this project with your organization. Your logo will show up here with a link to your website. [[Contribute](https://opencollective.com/assimp/contribute)]

<a href="https://opencollective.com/assimp/organization/0/website"><img src="https://opencollective.com/assimp/organization/0/avatar.svg"></a>

### License ###
Our license is based on the modified, __3-clause BSD__-License.

An _informal_ summary is: do whatever you want, but include Assimp's license text with your product -
and don't sue us if our code doesn't work. Note that, unlike LGPLed code, you may link statically to Assimp.
For the legal details, see the `LICENSE` file.

### Why this name ###
Sorry, we're germans :-), no english native speakers ...
