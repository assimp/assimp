/** @file General documentation built from a doxygen comment */

/** @mainpage ASSIMP - The open asset import library
@section intro Introduction

ASSIMP is a library to load and process geometric scenes from various data formats. It is taylored at typical game 
scenarios by supporting a node hierarchy, static or skinned meshes, materials, bone animations and potential texture data.
The library is *not* designed for speed, it is primarily useful for importing assets from various sources once and 
storing it in a engine-specific format for easy and fast every-day-loading. ASSIMP is also able to apply various post
processing steps to the imported data such as conversion to indexed meshes, calculation of normals or tangents/bitangents
or conversion from right-handed to left-handed coordinate systems.

At the moment ASSIMP is able to read Lightwave Object files (.obj), Milkshape3D scene (.ms3d), DirectX scenes (.x), 
old 3D Studio Max scene files (.3ds), Doom/Quake model files (.md1 to .md7), 3D Game Studio models (.mdl) and 
PLY files (.ply). ASSIMP is independent of the Operating System by nature, providing a C++ interface for easy integration 
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
#include <aiScene.h> // root structure of the imported data
#include <aiMesh.h>  // example: mesh data structures. you'll propably need other includes, too

bool DoTheImportThing( const char* pFile)
{
  // start the import on the given file with some example postprocessing
  aiScene* scene = aiImportFile( pFile, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

  // if the import failed, report it
  if( !scene)
  {
    DoTheErrorLogging( aiGetErrorString());
    return false;
  }

  // now we can access the file's contents
  DoTheSceneProcessing( scene);

  // we're done. Release all resources associated with this import
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

Auch das Logging noch erklären?

*/

/** 
@page data Data Structures

Grundlegend: Koordinatensystem, aiScene (Link), Erklärung Hierarchie, Nodes, Verweis auf Meshes, Mesh-Sammlung, 
Einzel-Mesh, Mesh-Komponentenbauweise, Verweis auf Material, Material-Sammlung, Erklärung Material-Tags, 
Animations-Sammlung, Einzel-Animation, Interpretation der Keyframes.
Bones: Finden und Zuordnen der Bone-Hierarchie zu Meshes.
*/

/** 
@page viewer The Viewer
Sinn: StandAlone-Test für die Importlib
Benutzung: was kann er und wie löst man es aus
Build: alles von #CustomBuild + DirectX + MFC?
*/
