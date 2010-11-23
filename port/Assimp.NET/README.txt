Assimp.NET
----------

This part of the assimp project provides a .NET wrapper for the main assimp 
classes and a test viewer written in C# that shows how to use the API.

NOTE: This has only been tested on 32-bit Windows 7 in .NET 3.5SP1 compiled
      under Visual Studio 2008 with the 'Debug' and 'Release' targets.



How To Build
------------

For Windows:

You don't need to build the main assimp projects first, that project is
referenced in the "Assimp.NET.sln" solution file so you can test C++ changes
to the main assimp source from the C# viewer application.


1) Download SWIG 2.0.0 or later (http://sourceforge.net/projects/swig/)
   Install it somewhere like (C:\Program Files\swigwin-2.0.0).

   NOTE: you can leave this step out, provided the SWIG-generated
   files in the repository are up-to-date. This will usually be
   the case in release versions, but not necessarily in TRUNK.


2) Download Boost 1.43.0 or later (http://www.boost.org/)
   Install it somewhere like (C:\Program Files\Boost\boost_1_43_0).

2) In Visual Studio 2008 go to:
   Tools->Options->Projects and Solutions->VC++ Directories
   Set "Platform:" to "Win32"
   Set "Show directories for:" to "Executable Files"
   - Add the swig directory to this list (C:\Program Files\swigwin-2.0.0).
   Set "Show directories for:" to "Include Files"
   - Add the boost directory to this list (C:\Program Files\Boost\boost_1_43_0).

3) Open the "Assimp.NET.sln" solution file in Visual Studio

4) Build and run Assimp.NET_DEMO.


By default, the viewer application loads a pre-defined 3DS test file from
the /test/models/3DS folder. To load another file, pass it on the
command line.

The viewer is very minimalistic, don't expect all files to be displayed 
properly. Feel free to extend it ;-)



License
-------

The license for Assimp.NET is the same as the main Assimp license.
