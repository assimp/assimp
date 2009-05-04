/** @file dox.h
 *  @brief General documentation built from a doxygen comment
 */

/**
@mainpage ASSIMP - Open Asset Import Library

<img src="dragonsplash.png"></img>

@section intro Introduction

ASSIMP is a library to load and process geometric scenes from various data formats. It is tailored at typical game 
scenarios by supporting a node hierarchy, static or skinned meshes, materials, bone animations and potential texture data.
The library is *not* designed for speed, it is primarily useful for importing assets from various sources once and 
storing it in a engine-specific format for easy and fast every-day-loading. ASSIMP is also able to apply various post
processing steps to the imported data such as conversion to indexed meshes, calculation of normals or tangents/bitangents
or conversion from right-handed to left-handed coordinate systems.

ASSIMP currently supports the following file formats (note that some loaders lack some features of their formats because 
some file formats contain data not supported by ASSIMP, some stuff would require so much conversion work
that it has not yet been implemented, and some formats have not completely been documented by their inventors):
<hr>
<br><tt>
<b>Collada</b> ( <i>*.dae;*.xml</i> ) <sup>3</sup><br>
<b>Biovision BVH </b> ( <i>*.bvh</i> ) <br>
<b>3D Studio Max 3DS</b> ( <i>*.3ds</i> ) <br>
<b>3D Studio Max ASE</b> ( <i>*.ase</i> ) <br>
<b>Wavefront Object</b> ( <i>*.obj</i> ) <br>
<b>Stanford Polygon Library</b> ( <i>*.ply</i> ) <br>
<b>AutoCAD DXF</b> ( <i>*.dxf</i> ) <sup>2</sup><br>
<b>Neutral File Format</b> ( <i>*.nff</i> ) <br>
<b>Sense8 WorldToolkit</b> ( <i>*.nff</i> ) <br>
<b>LightWave Model</b> ( <i>*.lwo</i> ) <br>
<b>MODO model</b> ( <i>*.lxo</i> ) <br>
<b>Valve Model</b> ( <i>*.smd,*.vta</i> ) <sup>3</sup> <br>
<b>Quake I</b> ( <i>*.mdl</i> ) <br>
<b>Quake II</b> ( <i>*.md2</i> ) <br>
<b>Quake III</b> ( <i>*.md3</i> ) <br>
<b>RtCW</b> ( <i>*.mdc</i> )<br>
<b>Doom 3</b> ( <i>*.md5mesh;*.md5anim;*.md5camera</i> ) <br>
<b>DirectX X </b> ( <i>*.x</i> ). <br>		
<b>Quick3D </b> ( <i>*.q3o;*q3s</i> ). <br>	
<b>Raw Triangles </b> ( <i>*.raw</i> ). <br>	
<b>AC3D </b> ( <i>*.ac</i> ). <br>
<b>Stereolithography </b> ( <i>*.stl</i> ). <br>
<b>Autodesk DXF </b> ( <i>*.dxf</i> ). <br>
<b>Irrlicht Mesh </b> ( <i>*.irrmesh;*.xml</i> ). <br>
<b>Irrlicht Scene </b> ( <i>*.irr;*.xml</i> ). <br>
<b>Object File Format </b> ( <i>*.off</i> ). <br>	
<b>Terragen Terrain </b> ( <i>*.ter</i> ) <br>
<b>3D GameStudio Model </b> ( <i>*.mdl</i> ) <br>
<b>3D GameStudio Terrain</b> ( <i>*.hmp</i> )<br><br><br>
</tt>
<sup>3</sup>: These formats support animations, but ASSIMP doesn't yet support them (or they're buggy)
<br>
<hr>

ASSIMP is independent of the Operating System by nature, providing a C++ interface for easy integration 
with game engines and a C interface to allow bindings to other programming languages. At the moment the library runs 
on any little-endian platform including X86/Windows/Linux/Mac and X64/Windows/Linux/Mac. Special attention 
was paid to keep the library as free as possible from dependencies. 

Big endian systems such as PPC-Macs or PPC-Linux systems are not officially supported at the moment. However, most 
formats handle the required endian conversion correctly, so large parts of the library should work.

The ASSIMP linker library and viewer application are provided under the BSD 3-clause license. This basically means
that you are free to use it in open- or closed-source projects, for commercial or non-commercial purposes as you like
as long as you retain the license informations and take own responsibility for what you do with it. For details see
the LICENSE file.

You can find test models for almost all formats in the <assimp_root>/test/models directory. Beware, they're *free*,
but not all of them are *open-source*. If there's an accompagning '<file>\source.txt' file don't forget to read it.

@section main_install Installation

ASSIMP can be used in two ways: linking against the pre-built libraries or building the library on your own. The former 
option is the easiest, but the ASSIMP distribution contains pre-built libraries only for Visual C++ 2005 and 2008. For other
compilers you'll have to build ASSIMP for yourself. Which is hopefully as hassle-free as the other way, but needs a bit 
more work. Both ways are described at the @link install Installation page. @endlink

@section main_usage Usage

When you're done integrating the library into your IDE / project, you can now start using it. There are two separate 
interfaces by which you can access the library: a C++ interface and a C interface using flat functions. While the former
is easier to handle, the latter also forms a point where other programming languages can connect to. Upto the moment, though,
there are no bindings for any other language provided. Have a look at the @link usage Usage page @endlink for a detailed explanation and code examples.

@section main_data Data Structures

When the importer successfully completed its job, the imported data is returned in an aiScene structure. This is the root
point from where you can access all the various data types that a scene/model file can possibly contain. The 
@link data Data Structures page @endlink describes how to interpret this data.

@section main_viewer The Viewer

The ASSIMP viewer is a standalone Windows/DirectX application that was developed along with the library. It is very useful 
for quickly examining the contents of a scene file and test the suitability of its contents for realtime rendering. 
The viewer offers a lot of additional features to view, interact with or export bits of the data. See the
@link viewer Viewer page @endlink for a detailed description of its capabilities.


@section main_support Support & Feedback

If you have any questions/comments/suggestions/bug reports you're welcome to post them in our 
<a href="https://sourceforge.net/forum/forum.php?forum_id=817653">forums</a>. Alternatively there's
a mailing list, <a href="https://sourceforge.net/mailarchive/forum.php?forum_name=assimp-discussions">
assimp-discussions</a>.


*/

/**
@page install Installation

@section install_prebuilt Using the pre-built libraries with Visual C++ 8/9

If you develop at Visual Studio 2005 or 2008, you can simply use the pre-built linker libraries provided in the distribution.
Extract all files to a place of your choice. A directory called "ASSIMP" will be created there. Add the ASSIMP/include path
to your include paths (Menu-&gt;Extras-&gt;Options-&gt;Projects and Solutions-&gt;VC++ Directories-&gt;Include files)
and the ASSIMP/lib/&lt;Compiler&gt; path to your linker paths (Menu-&gt;Extras-&gt;Options-&gt;Projects and Solutions-&gt;VC++ Directories-&gt;Library files).
This is neccessary only once to setup all paths inside you IDE.

To use the library in your C++ project you have to include either &lt;assimp.hpp&gt; or &lt;assimp.h&gt; plus some others starting with &lt;aiTypes.h&gt;.
If you set up your IDE correctly the compiler should be able to find the files. Then you have to add the linker library to your
project dependencies. Link to <assimp_root>/lib/<config-name>/assimp.lib. config-name is one of the predefined
project configs. For static linking, use release/debug. See the sections below on this page for more information on the
other build configs.
If done correctly you should now be able to compile, link,
run and use the application. If the linker complains about some integral functions being defined twice you propably have
mixed the runtimes. Recheck the project configuration (project properties -&gt; C++ -&gt; Code generation -&gt; Runtime) if you use 
static runtimes (Multithreaded / Multithreaded Debug) or dynamic runtimes (Multithreaded DLL / Multithreaded Debug DLL).
Choose the ASSIMP linker lib accordingly. 
<br>
Please don't forget to also read the @link assimp_stl section on MSVC and the STL @endlink 

@section assimp_stl Microsoft Compilers & STL

In VC8 and VC9 Microsoft has introduced some STL debugging features. A good example are improved iterator checks and
various useful debug checks. Actually they are really helpful for debugging, but they're extremely slow. They're
so extremely slow that they can make the STL up to 100 times slower (imagine a <i>std::vector<T>::operator[] </i>
performing 3 or 4 single checks! scary ...).

These security enhancements are - thanks MS! - also active in release builds, rendering ASSIMP several times 
slower. However, it is possible to disable them by defining

@code
_HAS_ITERATOR_DEBUGGING=0
_SECURE_SCL=0
@endcode

in the preprocessor options (or alternatively in the source code, just before the STL is included for the first time).
<b>ASSIMP's vc8 and vc9 configs enable these flags by default</b>.

<i>If you're linking statically against ASSIMP:</i> Make sure your applications uses the same STl settings! 
If you do not, there are two binary incompatible STL versions mangled together and you'll crash. 
Alternatively you can disable the fast STL settings for ASSIMP by removing the 'FastSTL' property sheet from
the vc project file.

<i>If you're using ASSIMP in a DLL:</i> It's ok. There's no STL used in the DLL interface, so it doesn't care whether
your application uses the same STL settings or not.
<br><br>
Another option is to build against a different STL implementation, for example STlport. There's a special 
@link assimp_stlport section @endlink describing how to achieve this.


@section install_own Building the library from scratch

To build the library on your own you first have to get hold of the dependencies. Fortunately, special attention was paid to 
keep the list of dependencies short. Unfortunately, the only dependency is <a href="http://www.boost.org">boost</a> which 
can be a bit painful to set up for certain development environments. Boost is a widely used collection of classes and 
functions for various purposes. Chances are that it was already installed along with your compiler. If not, you have to install 
it for yourself. Read the "Getting Started" section of the Boost documentation for how to setup boost. VisualStudio users 
can use a comfortable installer from <a href="http://www.boost-consulting.com/products/free">
http://www.boost-consulting.com/products/free</a>. Choose the appropriate version of boost for your runtime of choice.

<b>If you don't want to use boost</b>, you can build against our <i>"Boost-Workaround"</i>. It consists of very small (dummy)
implementations of the various boost utility classes used. However, you'll loose functionality (e.g. threading) by doing this. 
So, if it is possible to use boost, you should use boost. See the @link use_noboost NoBoost @endlink for more details.

Once boost is working, you have to set up a project for the ASSIMP library in your favourite IDE. If you use VC2005 or
VC2008, you can simply load the solution or project files in the workspaces/ folder, otherwise you have to create a new 
package and add all the headers and source files from the include/ and code/ directories. Set the temporary output folder
to obj/, for example, and redirect the output folder to bin/. Then build the library - it should compile and link fine.

The last step is to integrate the library into your project. This is basically the same task as described in the 
"Using the pre-built libraries" section above: add the include/ and bin/ directories to your IDE's paths so that the compiler can find
the library files. Alternatively you can simply add the ASSIMP project to your project's overall solution and build it inside
your solution.


@section use_noboost Building without boost.

The Boost-Workaround consists of dummy replacements for some boost utility templates. Currently there are replacements for
<ul>
<li><i>boost.scoped_ptr</i></li>
<li><i>boost.scoped_array</i></li>
<li><i>boost.format</i> </li>
<li><i>boost.random</i> </li>
<li><i>boost.common_factor</i> </li>
<li><i>boost.foreach</i> </li>
<li><i>boost.tuple</i></li>
</ul>
These implementations are very limited and are not intended for use outside ASSIMP. A compiler
with full support for partial template specializations is required. To enable the workaround, put the following in
your compiler's list of predefined macros: 
@code
#define ASSIMP_BUILD_BOOST_WORKAROUND
@endcode
<br>
If you're working with the provided solutions for Visual Studio use the <i>-noboost</i> build configs. <br>

<b>ASSIMP_BUILD_BOOST_WORKAROUND</b> implies <b>ASSIMP_BUILD_SINGLETHREADED</b>. <br>
See the @link assimp_st next @endlink section
for more details.

@section assimp_st Single-threaded build

-- currently there is no difference between single-thread and normal builds --


@section assimp_dll DLL build

ASSIMP can be built as DLL. You just need to select a -dll config from the list of project
configs and you're fine. Don't forget to copy the DLL to the directory of your executable :-)

<b>NOTE:</b> Theoretically ASSIMP-dll can be used with multithreaded (non-dll) runtime libraries, 
as long as you don't utilize any non-public stuff from the code dir. However, if you happen
to encounter *very* strange problems try changing the runtime to multithreaded (Debug) DLL.

@section assimp_stlport Building against STLport

If your compiler's default implementation of the STL is too slow, lacks some features,
contains bugs or if you just want to tweak ASSIMP's performance a little try a build
against STLport. STLport is a free, fast and secure STL replacement that works with
all major compilers and platforms. To get it visit their website at
<a href="http://www.stlport.org"/><stlport.org></a> and download the latest STLport release.
Usually you'll just need to run 'configure' + a makefile (see the README for more details).
Don't miss to add <stlport_root>/stlport to your compiler's default include paths - <b>prior</b>
to the directory where the compiler vendor's STL lies. Do the same for  <stlport_root>/lib and
recompile ASSIMP. To ensure you're really building against STLport see aiGetCompileFlags().
<br>
Usually building ASSIMP against STLport yields a better overall performance so it might be
worth a try if the library is too slow for you.

*/


/** 
@page usage Usage

@section access_cpp Access by C++ class interface

The ASSIMP library can be accessed by both a class or flat function interface. The C++ class
interface is the preferred way of interaction: you create an instance of class ASSIMP::Importer, 
maybe adjust some settings of it and then call ASSIMP::Importer::ReadFile(). The class will
read the files and process its data, handing back the imported data as a pointer to an aiScene 
to you. You can now extract the data you need from the file. The importer manages all the resources
for itsself. If the importer is destroyed, all the data that was created/read by it will be 
destroyed, too. So the easiest way to use the Importer is to create an instance locally, use its
results and then simply let it go out of scope. 

C++ example:
@code
#include <assimp.hpp>      // C++ importer interface
#include <aiScene.h>       // Outptu data structure
#include <aiPostProcess.h> // Post processing flags


bool DoTheImportThing( const std::string& pFile)
{
  // Create an instance of the Importer class
  ASSIMP::Importer importer;

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll 
  // propably to request more postprocessing than we do in this example.
  const aiScene* scene = importer.ReadFile( pFile, 
	aiProcess_CalcTangentSpace       | 
	aiProcess_Triangulate            |
	aiProcess_JoinIdenticalVertices  |
	aiProcess_SortByPType);
  
  // If the import failed, report it
  if( !scene)
  {
    DoTheErrorLogging( importer.GetErrorString());
    return false;
  }

  // Now we can access the file's contents. 
  DoTheSceneProcessing( scene);

  // We're done. Everything will be cleaned up by the importer destructor
  return true;
}
@endcode

What exactly is read from the files and how you interpret it is described at the @link data Data 
Structures page. @endlink The post processing steps that the ASSIMP library can apply to the
imported data are listed at #aiPostProcessSteps. See the @link pp Post proccessing page @endlink for more details.

Note that the aiScene data structure returned is declared 'const'. Yes, you can get rid of 
these 5 letters with a simple cast. Yes, you may do that. No, it's not recommended (and it's 
suicide in DLL builds ...). 

@section access_c Access by plain-c function interface

The plain function interface is just as simple, but requires you to manually call the clean-up
after you're done with the imported data. To start the import process, call aiImportFile()
with the filename in question and the desired postprocessing flags like above. If the call
is successful, an aiScene pointer with the imported data is handed back to you. When you're
done with the extraction of the data you're interested in, call aiReleaseImport() on the
imported scene to clean up all resources associated with the import.

C example:
@code
#include <assimp.h>        // Plain-C interface
#include <aiScene.h>       // Output data structure
#include <aiPostProcess.h> // Post processing flags

bool DoTheImportThing( const char* pFile)
{
  // Start the import on the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll t
  // probably to request more postprocessing than we do in this example.
  const aiScene* scene = aiImportFile( pFile, 
    aiProcess_CalcTangentSpace       | 
	aiProcess_Triangulate            |
	aiProcess_JoinIdenticalVertices  |
	aiProcess_SortByPType);

  // If the import failed, report it
  if( !scene)
  {
    DoTheErrorLogging( aiGetErrorString());
    return false;
  }

  // Now we can access the file's contents
  DoTheSceneProcessing( scene);

  // We're done. Release all resources associated with this import
  aiReleaseImport( scene);
  return true;
}
@endcode

@section custom_io Using custom IO logic

The ASSIMP library needs to access files internally. This of course applies to the file you want
to read, but also to additional files in the same folder for certain file formats. By default,
standard C/C++ IO logic is used to access these files. If your application works in a special
environment where custom logic is needed to access the specified files, you have to supply 
custom implementations of IOStream and IOSystem. A shortened example might look like this:

@code
#include <IOStream.h>
#include <IOSystem.h>

// My own implementation of IOStream
class MyIOStream : public ASSIMP::IOStream
{
  friend class MyIOSystem;

protected:
  // Constructor protected for private usage by MyIOSystem
  MyIOStream(void);

public:
  ~MyIOStream(void);
  size_t Read( void* pvBuffer, size_t pSize, size_t pCount) { ... }
  size_t Write( const void* pvBuffer, size_t pSize, size_t pCount) { ... }
  aiReturn Seek( size_t pOffset, aiOrigin pOrigin) { ... }
  size_t Tell() const { ... }
  size_t FileSize() const { ... }
  void Flush () { ... }
};

// Fisher Price - My First Filesystem
class MyIOSystem : public ASSIMP::IOSystem
{
  MyIOSystem() { ... }
  ~MyIOSystem() { ... }

  // Check whether a specific file exists
  bool Exists( const std::string& pFile) const {
    .. 
  }

  // Get the path delimiter character we'd like to get
  char GetOsSeparator() const { 
    return '/'; 
  }

  // ... and finally a method to open a custom stream
  IOStream* Open( const std::string& pFile, const std::string& pMode) {
	return new MyIOStream( ... ); 
  }

  void Close( IOStream* pFile) { delete pFile; }
};
@endcode

Now that your IO system is implemented, supply an instance of it to the Importer object by calling 
ASSIMP::Importer::SetIOHandler(). 

@code
void DoTheImportThing( const std::string& pFile)
{
  ASSIMP::Importer importer;
  // put my custom IO handling in place
  importer.SetIOHandler( new MyIOSystem());

  // the import process will now use this implementation to access any file
  importer.ReadFile( pFile, SomeFlag | SomeOtherFlag);
}
@endcode


@section custom_io_c Using custom IO logic with the plain-c function interface

// TODO

@section threadsafety Thread-safety and internal multi-threading

The ASSIMP library can be accessed by multiple threads simultaneously, as long as the
following prerequisites are fulfilled: 
<ul>
<li> When using the C++-API make sure you create a new Importer instance for each thread.
   Constructing instances of Importer is expensive, so it might be a good idea to
   let every thread maintain its own thread-local instance (use it to 
   load as many models as you want).</li>
<li> The C-API is threadsafe as long as AI_C_THREADSAFE is defined. That's the default. </li>
<li> When supplying custom IO logic, make sure your underyling implementation is thead-safe.</li>
</ul>

See the @link assimp_st Single-threaded build section @endlink to learn how to build a lightweight variant
of ASSIMP which is not thread-safe and does not utilize multiple threads for loading.

@section  logging Logging in the AssetImporter

The ASSIMP library provides an easy mechanism to log messages. For instance if you want to check the state of your 
import and you just want to see, after which preprocessing step the import-process was aborted you can take a look 
into the log. 
Per default the ASSIMP-library provides a default log implementation, where you can log your user specific message
by calling it as a singleton with the requested logging-type. To see how this works take a look to this:

@code

// Create a logger instance 
ASSIMP::DefaultLogger::create("",ASSIMP::Logger::VERBOSE);

// Now I am ready for logging my stuff
ASSIMP::DefaultLogger::get()->info("this is my info-call");

// Kill it after the work is done
ASSIMP::DefaultLogger::kill();
@endcode

At first you have to create the default-logger-instance (create). Now you are ready to rock and can log a 
little bit around. After that you should kill it to release the singleton instance.

If you want to integrate the ASSIMP-log into your own GUI it my be helpful to have a mechanism writing
the logs into your own log windows. The logger interface provides this by implementing an interface called LogStream.
You can attach and detach this log stream to the default-logger instance or any implementation derived from Logger. 
Just derivate your own logger from the abstract base class LogStream and overwrite the write-method:

@code
// Example stream
class myStream :
	public LogStream
{
public:
	// Constructor
	myStream()
	{
		// empty
	}
	
	// Destructor
	~myStream()
	{
		// empty
	}

	// Write womethink using your own functionality
	void write(const char* message)
	{
		::printf("%s\n", message);
	}
};

// Attaching it to the default logger instance:
unsigned int severity = 0;
severity |= Logger::DEBUGGING;
severity |= Logger::INFO;
severity |= Logger::WARN;
severity |= Logger::ERR;

// Attaching it to the default logger
ASSIMP::DefaultLogger::get()->attachStream( new myStream(), severity );

@endcode

The severity level controls the kind of message which will be written into
the attached stream. If you just want to log errors and warnings set the warn 
and error severity flag for those severities. It is also possible to remove 
a self defined logstream from an error severity by detaching it with the severity 
flag set:

@code

unsigned int severity = 0;
severity |= Logger::DEBUGGING;

// Detach debug messages from you self defined stream
ASSIMP::DefaultLogger::get()->attachStream( new myStream(), severity );

@endcode

If you want to implement your own logger just build a derivate from the abstract base class 
Logger and overwrite the methods debug, info, warn and error. 

If you want to see the debug-messages in a debug-configured build, the Logger-interface 
provides a logging-severity. You can set it calling the following method:

@code

Logger::setLogSeverity( LogSeverity log_severity );

@endcode

The normal logging severity supports just the basic stuff like, info, warnings and errors. 
In the verbose level debug messages will be logged, too.

*/

/** 
@page data Data Structures

The ASSIMP library returns the imported data in a collection of structures. aiScene forms the root
of the data, from here you gain access to all the nodes, meshes, materials, animations or textures
that were read from the imported file. The aiScene is returned from a successful call to 
ASSIMP::Importer::ReadFile(), aiImportFile() or aiImportFileEx() - see the @link usage Usage page @endlink
for further information on how to use the library.

By default, all 3D data is provided in a right-handed coordinate system such as OpenGL uses. In
this coordinate system, +X points to the right, +Y points away from the viewer into the screen and
+Z points upwards. Several modeling packages such as 3D Studio Max use this coordinate system as well.
By contrast, some other environments use left-handed coordinate systems, a prominent example being
DirectX. If you need the imported data to be in a left-handed coordinate system, supply the
#aiProcess_MakeLeftHanded flag to the ReadFile() function call.

The output face winding is counter clockwise. Use #aiProcess_FlipWindingOrder to get CW data.
@code
x2
  
            x1
	x0
@endcode

Outputted polygons can be literally everything: they're probably concave, self-intersecting or non-planar,
although our built-in triangulation (#aiProcess_Triangulate postprocessing step) doesn't handle the two latter. 

The output UV coordinate system has its origin in the lower-left corner:
@code
0y|1y ---------- 1x|1y 
 |                |
 |                |
 |                |
0x|0y ---------- 1x|0y
@endcode
Use the #aiProcess_FlipUVs flag to get UV coordinates with the upper-left corner als origin.

All matrices in the library are row-major. That means that the matrices are stored row by row in memory,
which is similar to the OpenGL matrix layout. A typical 4x4 matrix including a translational part looks like this:
@code
X1  Y1  Z1  T1
X2  Y2  Z2  T2
X3  Y3  Z3  T3
0   0   0   1
@endcode

... with (X1, X2, X3) being the X base vector, (Y1, Y2, Y3) being the Y base vector, (Z1, Z2, Z3) 
being the Z base vector and (T1, T2, T3) being the translation part. If you want to use these matrices
in DirectX functions, you have to transpose them.

@section hierarchy The Node Hierarchy

Nodes are little named entities in the scene that have a place and orientation relative to their parents.
Starting from the scene's root node all nodes can have 0 to x child nodes, thus forming a hierarchy. 
They form the base on which the scene is built on: a node can refer to 0..x meshes, can be referred to
by a bone of a mesh or can be animated by a key sequence of an animation. DirectX calls them "frames",
others call them "objects", we call them aiNode. 

A node can potentially refer to single or multiple meshes. The meshes are not stored inside the node, but
instead in an array of aiMesh inside the aiScene. A node only refers to them by their array index. This also means
that multiple nodes can refer to the same mesh, which provides a simple form of instancing. A mesh referred to
by this way lives in the node's local coordinate system. If you want the mesh's orientation in global
space, you'd have to concatenate the transformations from the referring node and all of its parents. 

Most of the file formats don't really support complex scenes, though, but a single model only. But there are
more complex formats such as .3ds, .x or .collada scenes which may contain an arbitrary complex
hierarchy of nodes and meshes. I for myself would suggest a recursive filter function such as the
following pseudocode:

@code
void CopyNodesWithMeshes( aiNode node, SceneObject targetParent, Matrix4x4 accTransform)
{
  SceneObject parent;
  Matrix4x4 transform;

  // if node has meshes, create a new scene object for it
  if( node.mNumMeshes > 0)
  {
    SceneObjekt newObject = new SceneObject;
    targetParent.addChild( newObject);
    // copy the meshes
    CopyMeshes( node, newObject);

    // the new object is the parent for all child nodes
    parent = newObject;
    transform.SetUnity();
  } else
  {
    // if no meshes, skip the node, but keep its transformation
    parent = targetParent;
    transform = node.mTransformation * accTransform;
  }

  // continue for all child nodes
  for( all node.mChildren)
    CopyNodesWithMeshes( node.mChildren[a], parent, transform);
}
@endcode

This function copies a node into the scene graph if it has children. If yes, a new scene object
is created for the import node and the node's meshes are copied over. If not, no object is created.
Potential child objects will be added to the old targetParent, but there transformation will be correct
in respect to the global space. This function also works great in filtering the bone nodes - nodes 
that form the bone hierarchy for another mesh/node, but don't have any mesh themselves.

@section meshes Meshes

All meshes of an imported scene are stored in an array of aiMesh* inside the aiScene. Nodes refer
to them by their index in the array and providing the coordinate system for them, too. One mesh uses
only a single material everywhere - if parts of the model use a different material, this part is
moved to a separate mesh at the same node. The mesh refers to its material in the same way as the
node refers to its meshes: materials are stored in an array inside aiScene, the mesh stores only
an index into this array.

An aiMesh is defined by a series of data channels. The presence of these data channels is defined
by the contents of the imported file: by default there are only those data channels present in the mesh
that were also found in the file. The only channels guarenteed to be always present are aiMesh::mVertices
and aiMesh::mFaces. You can test for the presence of other data by testing the pointers against NULL
or use the helper functions provided by aiMesh. You may also specify several post processing flags 
at Importer::ReadFile() to let ASSIMP calculate or recalculate additional data channels for you.

At the moment, a single aiMesh may contain a set of triangles and polygons. A single vertex does always
have a position. In addition it may have one normal, one tangent and bitangent, zero to AI_MAX_NUMBER_OF_TEXTURECOORDS
(4 at the moment) texture coords and zero to AI_MAX_NUMBER_OF_COLOR_SETS (4) vertex colors. In addition
a mesh may or may not have a set of bones described by an array of aiBone structures. How to interpret
the bone information is described later on.

@section material Materials

See the @link materials Material System Page. @endlink

@section bones Bones

A mesh may have a set of bones in the form of aiBone structures.. Bones are a means to deform a mesh 
according to the movement of a skeleton. Each bone has a name and a set of vertices on which it has influence. 
Its offset matrix declares the transformation needed to transform from mesh space to the local space of this bone. 

Using the bones name you can find the corresponding node in the node hierarchy. This node in relation
to the other bones' nodes defines the skeleton of the mesh. Unfortunately there might also be
nodes which are not used by a bone in the mesh, but still affect the pose of the skeleton because
they have child nodes which are bones. So when creating the skeleton hierarchy for a mesh I
suggest the following method:

a) Create a map or a similar container to store which nodes are necessary for the skeleton. 
Pre-initialise it for all nodes with a "no". <br>
b) For each bone in the mesh: <br>
b1) Find the corresponding node in the scene's hierarchy by comparing their names. <br>
b2) Mark this node as "yes" in the necessityMap. <br>
b3) Mark all of its parents the same way until you 1) find the mesh's node or 2) the parent of the mesh's node. <br>
c) Recursively iterate over the node hierarchy <br>
c1) If the node is marked as necessary, copy it into the skeleton and check its children <br>
c2) If the node is marked as not necessary, skip it and do not iterate over its children. <br>

Reasons: you need all the parent nodes to keep the transformation chain intact. Depending on the
file format and the modelling package the node hierarchy of the skeleton is either a child
of the mesh node or a sibling of the mesh node. Therefore b3) stops at both the mesh's node and
the mesh's node's parent. The node closest to the root node is your skeleton root, from there you
start copying the hierarchy. You can skip every branch without a node being a bone in the mesh - 
that's why the algorithm skips the whole branch if the node is marked as "not necessary".

You should now have a mesh in your engine with a skeleton that is a subset of the imported hierarchy.

@section anims Animations

An imported scene may contain zero to x aiAnimation entries. An animation in this context is a set
of keyframe sequences where each sequence describes the orientation of a single node in the hierarchy
over a limited time span. Animations of this kind are usually used to animate the skeleton of
a skinned mesh, but there are other uses as well.

An aiAnimation has a duration. The duration as well as all time stamps are given in ticks. To get
the correct timing, all time stamp thus have to be divided by aiAnimation::mTicksPerSecond. Beware,
though, that certain combinations of file format and exporter don't always store this information
in the exported file. In this case, mTicksPerSecond is set to 0 to indicate the lack of knowledge.

The aiAnimation consists of a series of aiNodeAnim's. Each bone animation affects a single node in
the node hierarchy only, the name specifying which node is affected. For this node the structure 
stores three separate key sequences: a vector key sequence for the position, a quaternion key sequence
for the rotation and another vector key sequence for the scaling. All 3d data is local to the
coordinate space of the node's parent, that means in the same space as the node's transformation matrix.
There might be cases where animation tracks refer to a non-existent node by their name, but this
should not be the case in your every-day data. 

To apply such an animation you need to identify the animation tracks that refer to actual bones
in your mesh. Then for every track: <br>
a) Find the keys that lay right before the current anim time. <br>
b) Optional: interpolate between these and the following keys. <br>
c) Combine the calculated position, rotation and scaling to a tranformation matrix <br>
d) Set the affected node's transformation to the calculated matrix. <br>

If you need hints on how to convert to or from quaternions, have a look at the 
<a href="http://www.j3d.org/matrix_faq/matrfaq_latest.html">Matrix&Quaternion FAQ</a>. I suggest
using logarithmic interpolation for the scaling keys if you happen to need them - usually you don't
need them at all.

@section textures Textures

Normally textures used by assets are stored in separate files, however,
there are file formats embedding their textures directly into the model file.
Such textures are loaded into an aiTexture structure. 
<br>
There are two cases:
<br>
<b>1)</b> The texture is NOT compressed. Its color data is directly stored
in the aiTexture structure as an array of aiTexture::mWidth * aiTexture::mHeight aiTexel structures. Each aiTexel represents a pixel (or "texel") of the texture
image. The color data is stored in an unsigned RGBA8888 format, which can be easily used for
both Direct3D and OpenGL (swizzling the order of the color components might be necessary).
RGBA8888 has been chosen because it is well-known, easy to use and natively
supported by nearly all graphics APIs.
<br>
<b>2)</b> This is the case for aiTexture::mHeight == 0. The texture is stored in a
"compressed" format such as DDS or PNG. The term "compressed" does not mean that the
texture data must actually be compressed, however the texture was stored in the
model file as if it was stored in a separate file on the harddisk. Appropriate
decoders (such as libjpeg, libpng, D3DX, DevIL) are required to load theses textures.
aiTexture::mWidth specifies the size of the texture data in bytes, aiTexture::pcData is
a pointer to the raw image data and aiTexture::achFormatHint is either zeroed or
containing the file extension of the format of the embedded texture. This is only
set if ASSIMP is able to determine the file format.
*/

/**
@page materials Material System

@section General Overview

Warn, WIP. All materials are stored in an array of aiMaterial inside the aiScene. 

Each aiMesh refers to one 
material by its index in the array. Due to the vastly diverging definitions and usages of material
parameters there is no hard definition of a material structure. Instead a material is defined by
a set of properties accessible by their names. Have a look at aiMaterial.h to see what types of 
properties are defined. In this file there are also various functions defined to test for the
presence of certain properties in a material and retrieve their values.

@section mat_tex Textures

Textures are organized in stacks, each stack being evaluated independently. The final color value from a particular texture stack is used in the shading equation. For example, the computed color value of the diffuse texture stack (aiTextureType_DIFFUSE) is multipled with the amount of incoming diffuse light to obtain the final diffuse color of a pixel.

@code

 Stack                               Resulting equation

------------------------
| Constant base color  |             color
------------------------ 
| Blend operation 0    |             +
------------------------
| Strength factor 0    |             0.25*
------------------------
| Texture 0            |             texture_0
------------------------ 
| Blend operation 1    |             *
------------------------
| Strength factor 1    |             1.0*
------------------------
| Texture 1            |             texture_1
------------------------
  ...                                ...

@endcode

@section keys Constants

All material key constants start with 'AI_MATKEY' (it's an ugly macro for historical reasons, don't ask). 

<table border="1">
  <tr>
    <th>Name</th>
    <th>Data Type</th>
    <th>Default Value</th>
	<th>Meaning</th>
	<th>Notes</th>
  </tr>
  <tr>
    <td><tt>NAME</tt></td>
    <td>aiString</td>
    <td>n/a</td>
	<td>The name of the material, if available. </td>
	<td>Ignored by <tt>aiProcess_RemoveRedundantMaterials</tt>. Materials are considered equal even if their names are different.</td>
  </tr>
  <tr>
    <td><tt>COLOR_DIFFUSE</tt></td>
    <td>aiColor3D</td>
    <td>black (0,0,0)</td>
	<td>Diffuse color of the material. This is typically scaled by the amount of incoming diffuse light (e.g. using gouraud shading) </td>
	<td>---</td>
  </tr>
  <tr>
    <td><tt>COLOR_SPECULAR</tt></td>
    <td>aiColor3D</td>
    <td>black (0,0,0)</td>
	<td>Specular color of the material. This is typically scaled by the amount of incoming specular light (e.g. using phong shading) </td>
	<td>---</td>
  </tr>
  <tr>
    <td><tt>COLOR_AMBIENT</tt></td>
    <td>aiColor3D</td>
    <td>black (0,0,0)</td>
	<td>Ambient color of the material. This is typically scaled by the amount of ambient light </td>
	<td>---</td>
  </tr>
  <tr>
    <td><tt>COLOR_EMISSIVE</tt></td>
    <td>aiColor3D</td>
    <td>black (0,0,0)</td>
	<td>Emissive color of the material. This is the amount of light emitted by the object. In real time applications it will usually not affect surrounding objects, but raytracing applications may wish to treat emissive objects as light sources. </td>
	<td>---</tt></td>
  </tr>

  <tr>
    <td><tt>COLOR_TRANSPARENT</tt></td>
    <td>aiColor3D</td>
    <td>black (0,0,0)</td>
	<td>Defines the transparent color of the material, this is the color to be multiplied with the color of 
	translucent light to construct the final 'destination color' for a particular position in the screen buffer. T </td>
	<td>---</tt></td>
  </tr>

  <tr>
    <td><tt>WIREFRAME</tt></td>
    <td>int</td>
    <td>false</td>
	<td>Specifies whether wireframe rendering must be turned on for the material. 0 for false, !0 for true. </td>
	<td>---</tt></td>
  </tr>

  <tr>
    <td><tt>TWOSIDED</tt></td>
    <td>int</td>
    <td>false</td>
	<td>Specifies whether meshes using this material must be rendered without backface culling. 0 for false, !0 for true. </td>
	<td>Some importers set this property if they don't know whether the output face oder is right. As long as it is not set, you may safely enable backface culling.</tt></td>
  </tr>

  <tr>
    <td><tt>SHADING_MODEL</tt></td>
    <td>int</td>
    <td>gouraud</td>
	<td>One of the #aiShadingMode enumerated values. Defines the library shading model to use for (real time) rendering to approximate the original look of the material as closely as possible. </td>
	<td>The presence of this key might indicate a more complex material. If absent, assume phong shading only if a specular exponent is given.</tt></td>
  </tr>

  <tr>
    <td><tt>BLEND_FUNC</tt></td>
    <td>int</td>
    <td>false</td>
	<td>One of the #aiBlendMode enumerated values. Defines how the final color value in the screen buffer is computed from the given color at that position and the newly computed color from the material. Simply said, alpha blending settings.</td>
	<td>-</td>
  </tr>

  <tr>
    <td><tt>OPACITY</tt></td>
    <td>float</td>
    <td>1.0</td>
	<td>Defines the opacity of the material in a range between 0..1.</td>
	<td>Use this value to decide whether you have to activate alpha blending for rendering. <tt>OPACITY</tt> != 1 usually also implies TWOSIDED=1 to avoid cull artifacts.</td>
  </tr>

  <tr>
    <td><tt>SHININESS</tt></td>
    <td>float</td>
    <td>0.f</td>
	<td>Defines the shininess of a phong-shaded material. This is actually the exponent of the phong specular equation</td>
	<td><tt>SHININESS</tt>=0 is equivalent to <tt>SHADING_MODEL</tt>=<tt>aiShadingMode_Gouraud</tt>.</td>
  </tr>

  <tr>
    <td><tt>SHININESS_STRENGTH</tt></td>
    <td>float</td>
    <td>1.0</td>
	<td>Scales the specular color of the material.</td>
	<td>This value is kept separate from the specular color by most modelers, and so do we.</td>
  </tr>

  <tr>
    <td><tt>REFRACTI</tt></td>
    <td>float</td>
    <td>1.0</td>
	<td>Defines the Index Of Refraction for the material. That's not supported by most file formats.</td>
	<td>Might be of interest for raytracing.</td>
  </tr>

  <tr>
    <td><tt>TEXTURE(t,n)</tt></td>
    <td>aiString</td>
    <td>n/a</td>
	<td>Defines the path to the n'th texture on the stack 't', where 'n' is any value >= 0 and 't' is one of the #aiTextureType enumerated values.</td>
	<td>See the 'Textures' section above.</td>
  </tr>

  <tr>
    <td><tt>TEXBLEND(t,n)</tt></td>
    <td>float</td>
    <td>n/a</td>
	<td>Defines the strength the n'th texture on the stack 't'. All color components (rgb) are multipled with this factor *before* any further processing is done.</td>
	<td>-</td>
  </tr>

  <tr>
    <td><tt>TEXOP(t,n)</tt></td>
    <td>int</td>
    <td>n/a</td>
	<td>One of the #aiTextureOp enumerated values. Defines the arithmetic operation to be used to combine the n'th texture on the stack 't' with the n-1'th. <tt>TEXOP(t,0)</tt> refers to the blend operation between the base color for this stack (e.g. <tt>COLOR_DIFFUSE</tt> for the diffuse stack) and the first texture.</td>
	<td>-</td>
  </tr>

  <tr>
    <td><tt>MAPPING(t,n)</tt></td>
    <td>int</td>
    <td>n/a</td>
	<td>Defines how the input mapping coordinates for sampling the n'th texture on the stack 't' are computed. Usually explicit UV coordinates are provided, but some model file formats might also be using basic shapes, such as spheres or cylinders, to project textures onto meshes.</td>
	<td>See the 'Textures' section below. #aiProcess_GenUVCoords can be used to let Assimp compute proper UV coordinates from projective mappings.</td>
  </tr>

  <tr>
    <td><tt>UVWSRC(t,n)</tt></td>
    <td>int</td>
    <td>n/a</td>
	<td>Defines the UV channel to be used as input mapping coordinates for sampling the n'th texture on the stack 't'. All meshes assigned to this material share the same UV channel setup</td>
	<td>Presence of this key implies <tt>MAPPING(t,n)</tt> to be #aiTextureMapping_UV</td>
  </tr>

  <tr>
    <td><tt>MAPPINGMODE_U(t,n)</tt></td>
    <td>int</td>
    <td>n/a</td>
	<td>Any of the #aiTextureMapMode enumerated values. Defines the texture wrapping mode on the x axis for sampling the n'th texture on the stack 't'. 'Wrapping' occurs whenever UVs lie outside the 0..1 range. </td>
	<td>-</td>
  </tr>

  <tr>
    <td><tt>MAPPINGMODE_V(t,n)</tt></td>
    <td>int</td>
    <td>n/a</td>
	<td>Wrap mode on the v axis. See <tt>MAPPINGMODE_U</tt>. </td>
	<td>-</td>
  </tr>

   <tr>
    <td><tt>TEXMAP_AXIS(t,n)</tt></td>
    <td>aiVector3D</td>
    <td>n/a</td>
	<td></tt> Defines the base axis to to compute the mapping coordinates for the n'th texture on the stack 't' from. This is not required for UV-mapped textures. For instance, if <tt>MAPPING(t,n)</tt> is #aiTextureMapping_SPHERE, U and V would map to longitude and latitude of a sphere around the given axis. The axis is given in local mesh space.</td>
	<td>-</td>
  </tr>

  <tr>
    <td><tt>TEXFLAGS(t,n)</tt></td>
    <td>int</td>
    <td>n/a</td>
	<td></tt> Defines miscellaneous flag for the n'th texture on the stack 't'. This is a bitwise combination of the #aiTextureFlags enumerated values.</td>
	<td>-</td>
  </tr>

</table>

@section cpp C++-API

Retrieving a property from a material is done using various utility functions. For C++ it's simply calling aiMaterial::Get()

@code

aiMaterial* mat = .....

// The generic way
if(AI_SUCCESS != mat->Get(<material-key>,<where-to-store>)) {
   // handle epic failure here
}

@endcode

Simple, isn't it? To get the name of a material you would use

@code

aiString name;
mat->Get(AI_MATKEY_NAME,name);

@endcode

Or for the diffuse color ('color' won't be modified if the property is not set)

@code

aiColor3D color (0.f,0.f,0.f);
mat->Get(AI_MATKEY_COLOR_DIFFUSE,color);

@endcode

<b>Note:</b> Get() is actually a template with explicit specializations for aiColor3D, aiColor4D, aiString, float, int and some others.
Make sure that the type of the second parameter is matching the expected data type of the material property (no compile-time check yet!). 
Don't follow this advice if you wish to encounter very strange results.

@section C C-API

For good old C it's slightly different. Take a look at the aiGetMaterialGet<data-type> functions.

@code

aiMaterial* mat = .....

if(AI_SUCCESS != aiGetMaterialFloat(mat,<material-key>,<where-to-store>)) {
   // handle epic failure here
}

@endcode

To get the name of a material you would use

@code

aiString name;
aiGetMaterialString(mat,AI_MATKEY_NAME,&name);

@endcode

Or for the diffuse color ('color' won't be modified if the property is not set)

@code

aiColor3D color (0.f,0.f,0.f);
aiGetMaterialColor(mat,AI_MATKEY_COLOR_DIFFUSE,&color);

@endcode

@section pseudo Pseudo Code Listing

For completeness, the following is a very rough pseudo-code sample showing how to evaluate Assimp materials in your 
shading pipeline. You'll probably want to limit your handling of all those material keys to a reasonable subset suitable for your purposes 
(for example most 3d engines won't support highly complex multi-layer materials, but many 3d modellers do).

Also note that this sample is targeted at a (shader-based) rendering pipeline for real time graphics.

INCOMPLETE! WIP!

@code

// ---------------------------------------------------------------------------------------
// Evaluate multiple textures stacked on top of each other
float3 EvaluateStack(stack)
{
  // For the 'diffuse' stack stack.base_color would be COLOR_DIFFUSE
  // and TEXTURE(aiTextureType_DIFFUSE,n) the n'th texture.

  float3 base = stack.base_color;
  for (every texture in stack)
  {
    // assuming we have explicit & pretransformed UVs for this texture
    float3 color = SampleTexture(texture,uv); 

    // scale by texture blend factor
    color *= texture.blend;

    if (texture.op == add)
      base += color;
    else if (texture.op == multiply)
      base *= color;
    else // other blend ops go here
  }
  return base;
}

// ---------------------------------------------------------------------------------------
// Compute the diffuse contribution for a pixel
float3 ComputeDiffuseContribution()
{
  if (shading == none)
     return float3(1,1,1);

  float3 intensity (0,0,0);
  for (all lights in range)
  {
    float fac = 1.f;
    if (shading == gouraud)
      fac =  lambert-term ..
    else // other shading modes go here

    // handling of different types of lights, such as point or spot lights
    // ...

    // and finally sum the contribution of this single light ...
    intensity += light.diffuse_color * fac;
  }
  // ... and combine the final incoming light with the diffuse color
  return EvaluateStack(diffuse) * intensity;
}

// ---------------------------------------------------------------------------------------
// Compute the specular contribution for a pixel
float3 ComputeSpecularContribution()
{
  if (shading == gouraud || specular_strength == 0 || specular_exponent == 0)
    return float3(0,0,0);

  float3 intensity (0,0,0);
  for (all lights in range)
  {
    float fac = 1.f;
    if (shading == phong)
      fac =  phong-term ..
    else // other specular shading modes go here

    // handling of different types of lights, such as point or spot lights
    // ...

    // and finally sum the specular contribution of this single light ...
    intensity += light.specular_color * fac;
  }
  // ... and combine the final specular light with the specular color
  return EvaluateStack(specular) * intensity * specular_strength;
}

// ---------------------------------------------------------------------------------------
// Compute the ambient contribution for a pixel
float3 ComputeAmbientContribution()
{
  if (shading == none)
     return float3(0,0,0);

  float3 intensity (0,0,0);
  for (all lights in range)
  {
    float fac = 1.f;

    // handling of different types of lights, such as point or spot lights
    // ...

    // and finally sum the ambient contribution of this single light ...
    intensity += light.ambient_color * fac;
  }
  // ... and combine the final ambient light with the ambient color
  return EvaluateStack(ambient) * intensity;
}

// ---------------------------------------------------------------------------------------
// Compute the final color value for a pixel
// @param prev Previous color at that position in the framebuffer
float4 PimpMyPixel (float4 prev)
{
  // .. handle displacement mapping per vertex
  // .. handle bump/normal mapping

  // Get all single light contribution terms
  float3 diff = ComputeDiffuseContribution();
  float3 spec = ComputeSpecularContribution(); 
  float3 ambi = ComputeAmbientContribution();

  // .. and compute the final color value for this pixel
  float3 color = diff + spec + ambi;

  float3 opac  = EvaluateStack(opacity);

  if (blend_func == multiply)
       return prev
}

@endcode

*/

/** 
@page viewer The Viewer
Sinn: StandAlone-Test für die Importlib
Benutzung: was kann er und wie löst man es aus
Build: alles von CustomBuild + DirectX + MFC?
*/



