/** @file General documentation built from a doxygen comment */

/** @mainpage ASSIMP - The open asset import library
@section intro Introduction

ASSIMP is a library to load and process geometric scenes from various data formats. It is taylored at typical game 
scenarios by supporting a node hierarchy, static or skinned meshes, materials, bone animations and potential texture data.
The library is *not* designed for speed, it is primarily useful for importing assets from various sources once and 
storing it in a engine-specific format for easy and fast every-day-loading. ASSIMP is also able to apply various post
processing steps to the imported data such as conversion to indexed meshes, calculation of normals or tangents/bitangents
or conversion from right-handed to left-handed coordinate systems.

ASSIMP is able to import the following file formats into your application:

<b>AutoDesk 3D Studio 4/5 (.3ds).</b> The old native format of 3DS max, now still supported and
widely used.
<br>
<b>AutoDesk 3D Studio ASCII Export (.ase).</b> Text format used by 3DS max. Supports bone animations
and highly complex materials.
<br>
<b>DirectX (.x)</b> A file format that can easily be read by D3DX and that is supported 
as export format by most 3D modellers. ASSIMP supports both binary and ASCII X-Files.
<br>
<b>Stanford Polygon (.ply)</b> File format developed by the university of
stanford. Thanks to its flexibility it often used for scientific purposes. Supported by ASSIMP
are ASCII and binary PLY files, both LittleEndian and BigEndian.
<br>
<b>WaveFront Object (.obj)</b> File format that is widely used to exchange asset data
between different applications.
<br>
<b>Milkshape 3D (.ms3d)</b> Native file format of the well-known modeller Milkshape 3D. 
ASSIMP provides full support for bone animations contained in ms3d files.
<br>
<b>Quake I (.mdl)</b> The file format that was once used by the first part of the
quake series. ASSIMP provides full support for loading embedded textures from Quake models.
<br>
<b>3D GameStudio (.mdl)</b> The file format of Conitecs 3D GameStudio tool suite.
Used by the freeware modelling tool MED. ASSIMP provides full support for all types of
3D GameStudio MDL files: <i>MDL3, MDL4, MDL5, MDL6, MDL7</i>. Bone animations are supported.
<br>
<b>Half-Life/CS:S (.mdl, .smd)</b> The file formats used in half life.
<b>Quake II (.md2)/ Quake III (.md3)</b> Used by many free models on the web, support for
Quake 2's and Quake 3's file formats is a must-have for each game engine. Quake 4 is
existing but not widely used. However, it is supported by ASSIMP (and RavenSoft .mdr is supported, too)
<br>
<b>Doom 3 (.md5)</b> The well-known native file format of the Doom 3 engine, used in
many games. Supports bone animation and advanced material settings.



ASSIMP is independent of the Operating System by nature, providing a C++ interface for easy integration 
with game engines and a C interface to allow bindings to other programming languages. At the moment the library runs 
on any little-endian platform including X86/Windows/Linux/Mac and X64/Windows/Linux/Mac. Big endian systems such as 
PPC-Macs or PPC-Linux systems are not supported at the moment, but this might change later on. Special attention 
was paid to keep the library as free as possible from dependencies. 

The ASSIMP linker library and viewer application are provided under the BSD 3-clause license. This basically means
that you are free to use it in open- or closed-source projects, for commercial or non-commercial purposes as you like
as long as you retain the license informations and take own responsibility for what you do with it. For details see
the <link>License file</link>.

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
*/

/**
@page install Installation

@section install_prebuilt Using the prebuilt libraries

If you develop at Visual Studio 2005 or 2008, you can simply use the prebuilt linker libraries provided in the distribution.
Extract all files to a place of your choice. A directory called "Assimp" will be created there. Add the Assimp/include path
to your include paths (Menu->Extras->Options->Projects and Solutions->VC++ Directories->Include files)
and the Assimp/lib/<Compiler> path to your linker paths (Menu->Extras->Options->Projects and Solutions->VC++ Directories->Library files).
This is neccessary only once to setup all paths inside you IDE.

To use the library in your C++ project you have to include either <assimp.hpp> or <assimp.h> plus some others starting with <aiTypes.h>.
If you set up your IDE correctly the compiler should be able to find the files. Then you have to add the linker library to your
project dependencies. Depending on your runtime of choice you either link against assimp_Debug.lib / assimp_Release.lib 
(static runtime) or assimp_Debug_DLL.lib / assimp_Release_DLL.lib. If done correctly you should now be able to compile, link,
run and use the application. If the linker complains about some integral functions being defined twice you propably have
mixed the runtimes. Recheck the project configuration (project properties -> C++ -> Code generation -> Runtime) if you use 
static runtimes (Multithreaded / Multithreaded Debug) or dynamic runtimes (Multithreaded DLL / Multithreaded Debug DLL). Choose
the ASSIMP linker lib accordingly.

@section install_own Building the library from scratch

To build the library on your own you first have to get hold of the dependencies. Fortunately, special attention was paid to 
keep the list of dependencies short. Unfortunately, the only dependency is <a href="http://www.boost.org">boost</a> which 
can be a bit painful to set up for certain development environments. Boost is a widely used collection of classes and 
functions for various purposes. Chances are that it was already installed along with your compiler. If not, you have to install 
it for yourself. Read the "Getting Started" section of the Boost documentation for how to setup boost. VisualStudio users 
can use a comfortable installer from <a href="http://www.boost-consulting.com/products/free">
http://www.boost-consulting.com/products/free</a>. Choose the appropriate version of boost for your runtime of choice.

Once boost is working, you have to set up a project for the ASSIMP library in your favourite IDE. If you use VC2005 or
VC2008, you can simply load the solution or project files in the workspaces/ folder, otherwise you have to create a new 
package and add all the headers and source files from the include/ and code/ directories. Set the temporary output folder
to obj/, for example, and redirect the output folder to bin/. Then build the library - it should compile and link fine.

The last step is to integrate the library into your project. This is basically the same task as described in the 
"Using prebuild libs" section above: add the include/ and bin/ directories to your IDE's paths so that the compiler can find
the library files. Alternatively you can simply add the ASSIMP project to your project's overall solution and build it inside
your solution.

*/

/** 
@page usage Usage

@section access_cpp Access by class interface

The ASSIMP library can be accessed by both a class or flat function interface. The C++ class
interface is the preferred way of interaction: you create an instance of class Assimp::Importer, 
maybe adjust some settings of it and then call Assimp::Importer::ReadFile(). The class will
read the files and process its data, handing back the imported data as a pointer to an aiScene 
to you. You can now extract the data you need from the file. The importer manages all the resources
for itsself. If the importer is destroyed, all the data that was created/read by it will be 
destroyed, too. So the easiest way to use the Importer is to create an instance locally, use its
results and then simply let it go out of scope. 

C++ example:
@code
#include <assimp.hpp>  // C++ importer interface
#include <aiScene.h>   // root structure of the imported data
#include <aiMesh.h>    // example: mesh data structures. you'll propably need other includes, too

bool DoTheImportThing( const std::string& pFile)
{
  // create an instance of the Importer class
  Assimp::Importer importer;

  // and have it read the given file with some example postprocessing
  aiScene* scene = importer.ReadFile( pFile, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
  
  // if the import failed, report it
  if( !scene)
  {
    DoTheErrorLogging( importer.GetErrorText());
    return false;
  }

  // now we can access the file's contents
  DoTheSceneProcessing( scene);

  // we're done. Everything will be cleaned up by the importer destructor
  return true;
}
@endcode

What exactly is read from the files and how you interpret it is described at the @link data Data 
Structures page. @endlink The post processing steps that the ASSIMP library can apply to the
imported data are listed at #aiPostProcessSteps.

@section access_c Access by function interface

The plain function interface is just as simple, but requires you to manually call the clean-up
after you're done with the imported data. To start the import process, call aiImportFile()
with the filename in question and the desired postprocessing flags like above. If the call
is successful, an aiScene pointer with the imported data is handed back to you. When you're
done with the extraction of the data you're interested in, call aiReleaseImport() on the
imported scene to clean up all resources associated with the import.

C example:
@code
#include <assimp.h>  // Plain C importer interface
#include <aiScene.h> // Root structure of the imported data
#include <aiMesh.h>  // Example: mesh data structures. you'll propably need other includes, too

bool DoTheImportThing( const char* pFile)
{
  // Start the import on the given file with some example postprocessing
  aiScene* scene = aiImportFile( pFile, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

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
class MyIOStream : public Assimp::IOStream
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
};

// Fisher Price - My First Filesystem
class MyIOSystem : public Assimp::IOSystem
{
  MyIOSystem() { ... }
  ~MyIOSystem() { ... }

  bool Exists( const std::string& pFile) const { ... }
  std::string getOsSeparator() const { return "/"; }
  IOStream* Open( const std::string& pFile, const std::string& pMode = std::string("rb")) { return new MyIOStream( ... ); }
  void Close( IOStream* pFile) { delete pFile; }
};
@endcode

Now that your IO system is implemented, supply an instance of it to the Importer object by calling 
Assimp::Importer::SetIOHandler(). 

@code
void DoTheImportThing( const std::string& pFile)
{
  Assimp::Importer importer;
  // put my custom IO handling in place
  importer.SetIOHandler( new MyIOSystem());

  // the import process will now use this implementation to access any file
  importer.ReadFile( pFile, SomeFlag | SomeOtherFlag);
}
@endcode

@section  logging Logging in the AssetImporter

The ASSIMP library provides an easy mechanism to log messages. For instance if you want to check the state of your 
import and you just want to see, after which preprocessing step the import-process was aborted you can take a look 
into the log. 
Per default the ASSIMP-library provides a default log implementation, where you can log your user specific message
by calling it as a singleton with the requested logging-type. To see how this works take a look to this:

@code

// Create a logger instance 
Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);

// Now I am ready for logging my stuff
Assimp::DefaultLogger::get()->info("this is my info-call");

// Kill it after the work is done
Assimp::DefaultLogger::kill();
@endcode

At first you have to create the default-logger-instance (create). Now you are ready to rock and can log a 
little bit around. After that you should kill it to release the singleton instance.

If you want to integrate the ASSIMP-log into your own GUI it my be helpful to have a mechanism writing
the logs into your own log windows. The logger interface provides this by implementing an interface called LogStream.
You can attach and detach this logstream to the default-logger instance or any implementation derived from Logger. 
Just derivate your own logger from the abstract baseclass LogStream and overwrite the write-method:

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
	void write(const std::string &message)
	{
		printf("%s\n", message.c_str();
	}
};

// Attaching it to the default logger instance:
unsigned int severity = 0;
severity |= Logger::DEBUGGING;
severity |= Logger::INFO;
severity |= Logger::WARN;
severity |= Logger::ERR;

// Attaching it to the default logger
Assimp::DefaultLogger::get()->attachStream( new myStream(), severity );

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
Assimp::DefaultLogger::get()->attachStream( new myStream(), severity );

@endcode

If you want to implement your own logger just build a derivate from the abstract base class 
Logger and overwrite the methods debug, info, warn and error. 

If you ust want to see the debug-messages in a debug-configured build the Logger-interface 
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
Assimp::Importer::ReadFile(), aiImportFile() or aiImportFileEx() - see the @link usage Usage page @endlink
for further information on how to use the library.

By default, all 3D data is provided in a right-handed coordinate system such as OpenGL uses. In
this coordinate system, +X points to the right, +Y points away from the viewer into the screen and
+Z points upwards. Several modelling packages such as 3D Studio Max use this coordinate system as well.
By contrast, some other environments use left-handed coordinate systems, a prominent example being
DirectX. If you need the imported data to be in a left-handed coordinate system, supply the
aiProcess_ConvertToLeftHanded flag to the ReadFile() function call.

All matrices in the library are row-major. That means that the matrices are stored row by row in memory,
which is similar to the OpenGL matrix layout. A typical 4x4 matrix including a translational part looks like this:
@code
X1  Y1  Z1  T1
X2  Y2  Z2  T2
X3  Y3  Z3  T3
0   0   0   1
@endcode

... with (X1, X2, X3) being the X base vector, (Y1, Y2, Y3) being the Y base vector, (Z1, Z2, Z3) 
being the Z base vector and (T1, T2, T3) being the translation part. If you want to use thess matrices
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
that form the bone hierarchy for another mesh/node, but don't have any mesh themselfes.

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

All materials are stored in an array of aiMaterial inside the aiScene. Each aiMesh refers to one 
material by its index in the array. Due to the vastly diverging definitions and usages of material
parameters there is no hard definition of a material structure. Instead a material is defined by
a set of properties accessible by their names. Have a look at aiMaterial.h to see what types of 
properties are defined. In this file there are also various functions defined to test for the
presence of certain properties in a material and retrieve their values.

Example to convert from an Assimp material to a Direct3D 9 material for use with the fixed 
function pipeline. Textures are not handled, only colors and the specular power, sometimes
also refered to as "shininess":
@code

void ConvertColor ( const aiColor4D& clrIn, D3DCOLORVALUE& clrOut )
{
   clrOut.r = clrIn.r;
   clrOut.g = clrIn.g;
   clrOut.b = clrIn.b;
   clrOut.a = clrIn.a;
}

void ConvertMaterial( aiMaterial* matIn, D3DMATERIAL9* matOut )
{ 
   // ***** DIFFUSE MATERIAL COLOR
   aiColor4D clr(0.0f,0.0f,0.0f,1.0f);
   // if the material property is not existing, the passed color pointer
   // won't be modified, therefore the diffuse color would be BLACK in this case
   aiGetMaterialColor(matIn,AI_MATKEY_COLOR_DIFFUSE,&clr);
   ConvertColor ( clr, matOut.Diffuse ); 

   // ***** SPECULAR MATERIAL COLOR
   clr = aiColor4D(1.0f,1.0f,1.0f,1.0f);
   aiGetMaterialColor(matIn,AI_MATKEY_COLOR_SPECULAR,&clr);
   ConvertColor ( clr, matOut.Specular ); 

   // ***** AMBIENT MATERIAL COLOR
   clr = aiColor4D(0.0f,0.0f,0.0f,1.0f);
   aiGetMaterialColor(matIn,AI_MATKEY_COLOR_AMBIENT,&clr);
   ConvertColor ( clr, matOut.Ambient ); 

   // ***** EMISIVE MATERIAL COLOR (Self illumination)
   clr = aiColor4D(0.0f,0.0f,0.0f,1.0f);
   aiGetMaterialColor(matIn,AI_MATKEY_COLOR_EMISSIVE,&clr);
   ConvertColor ( clr, matOut.Emissive ); 

   // ***** SHININESS (Phong power)  
   matOut.Power = 0.0f;
   aiGetMaterialFloat(matIn,AI_MATKEY_COLOR_EMISSIVE,&matOut.Power);
}
@endcode

Textures:

Textures can have various types and purposes. Sometimes ASSIMP is not able to
determine the exact purpose of a texture. Normally it will assume diffuse as default
purpose. Possible purposes for a texture:

<b>1. Diffuse textures.</b> Diffuse textures are combined with the result of the diffuse lighting term.
<br>
<b>2. Specular textures.</b> Specular textures are combined with the result of the specular lighting term.
Generally speaking, they can be used to map a texture onto specular highlights.
<br>
<b>3. Ambient textures.</b> Ambient textures are combined with the result of the ambient lighting term. 
<br>
<b>4. Emissive textures.</b> Emissive textures are combined with the emissive base color of the material. 
The result is then added to the final pixel color. Emissive textures are sometimes called
"Self ilumination maps".
<br>
<b>5. Opacity textures.</b> Opacity textures specify the opacity of a texel. They are 
normally grayscale images, black stands for fully transparent, white for fully opaque.
<br>
<b>6. Height maps.</b> Height maps specify the relative height of a point on a triangle on a
per-texel base. Normally height maps (sometimes called "Bump maps") are converted to normal
maps before rendering. Height maps are normally grayscale textures. Height maps could also
be used as displacement maps on a highly tesselated surface.
<br>
<b>7. Normal maps.</b> Normal maps contain normal vectors for a single texel, in tangent space.
They are not bound to an object. However, all lighting omputations must be done in tangent space. 
There are many resources on Normal Mapping on the internet.
<br>
<b>8. Shininess maps</b> Shininess maps (sometimes called "Gloss" or "SpecularMap") specify
the shininess of a texel mapped on a surface. They are normally used together with normal maps
to make flat surfaces look as if they were real 3d objects.
<br>

Textures are generally defined by a set of parameters, including
<br>
<b>1. The path to the texture.</b>  This property is always set. If it is not set, a texture 
is not existing. This can either be a valid path (beware, sometimes
it could be a 8.3 file name!) or an asterisk (*) suceeded by a zero-based index, the latter being
a reference to an embedded texture (@link textures more details @endlink). I suggest using code
like this to find out whether a texture is embedded:
@code
aiString path; // contains the path obtained via aiGetMaterialString()
const aiScene* scene; // valid aiScene instance

const char* szData = path.data;
if ('*' == *szData)
{
   int index = atoi(szData+1);
   ai_assert(index < scene->mNumTextures);

   // your loading code for loading from aiTexture's ...
}
else // your loading code to load from a path ...
@endcode
<br>
<b>2. An UV coordinate index.</b> This is an index into the UV coordinate set list of the
corresponding mesh. Note: Some formats don't define this, so beware, it could be that
a second diffuse texture in a mesh was originally intended to use a second UV channel although
ASSIMP states it uses the first one. UV coordinate source indices are defined by the
<i>AI_MATKEY_UVWSRC_<textype>(<texindex>)</i> material property. Assume 0 as default value if
this property is not set.
<br>
<b>3. A blend factor.</b> This is used if multiple textures are assigned to a slot, e.g. two
or more textures on the diffuse channel. A texture's color value is multiplied with its
blend factor before it is combined with the previous color value (from the last texture) using
a specific blend operation (see 4.). Blend factor are defined by the
<i>AI_MATKEY_TEXBLEND_<textype>(<texindex>)</i> material property. Assume 1.0f as default value 
if this property is not set.
<br>
<b>4. A blend operation.</b> This is used if multiple textures are assigned to a slot, e.g. two
or more textures on the diffuse channel. After a texture's color value has been multiplied
with its blend factor, the blend operation is used to combine it with the previous color value.
Blend operations are stored as integer property, however their type is aiTextureOp.
Blend factor are defined by the <i>AI_TEXOP_BLEND_<textype>(<texindex>)</i> material property. Assume
aiTextureOp_Multiply as default value if this property is not set. The blend operation for
the first texture in a texture slot (e.g. diffuse 0) specifies how the diffuse base color/
vertex color have to be combined with the texture color value.
<br>

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
Preinitialise it for all nodes with a "no". <br>
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

The aiAnimation consists of a series of aiBoneAnims. Each bone animation affects a single node in
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

*/

/** 
@page viewer The Viewer
Sinn: StandAlone-Test für die Importlib
Benutzung: was kann er und wie löst man es aus
Build: alles von #CustomBuild + DirectX + MFC?
*/
