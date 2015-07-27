The OpenDDL-Parser
==================

A simple and fast OpenDDL Parser
Current build status: [![Build Status](https://travis-ci.org/kimkulling/openddl-parser.png)](https://travis-ci.org/kimkulling/openddl-parser)

Get the source code
===================
You can get the code from our git repository, which is located at GitHub. You can clone the repository like:

> git clone https://github.com/kimkulling/openddl-parser.git

Build from repo
===============
To build the library you need to install cmake first ( see http://www.cmake.org/ for more information ). Make also sure that a compiler toolchain is installed on your machine.
After installing it you can open a console and type:

> cmake CMakeLists.txt

This command will generate a build environment for your installed build enrironment ( for Visual Studio the project files will be generated, for gcc the makefiles will be generated ).
When using an IDE open the IDE and run the build. When using GNU-make type in your console:

> make

and that's all.

Use the library
===============
To use the OpenDDL-parser you need to build the lib first. Now add the 
> <Repo-folder>/include 

to your include-path and the 

> <Repo-folder>/lib

to your lib-folder. Link the openddl.lib to your application. 

Here is a small example how to use the lib:

```cpp

#include <iostream>
#include <cassert>
#include <openddlparser/OpenDDLParser.h>

USE_ODDLPARSER_NS;

int main( int argc, char *argv[] ) {
    if( argc < 3 ) {
        return 1;
    }

    char *filename( nullptr );
    if( 0 == strncmp( FileOption, argv[ 1 ], strlen( FileOption ) ) ) {
        filename = argv[ 2 ];
    }
    std::cout << "file to import: " << filename << std::endl;   
    if( nullptr == filename ) {
        std::cerr << "Invalid filename." << std::endl;
        return Error;
    }

    FILE *fileStream = fopen( filename, "r+" );
    if( NULL == filename ) {
        std::cerr << "Cannot open file " << filename << std::endl;
        return 1;
    }

    // obtain file size:
    fseek( fileStream, 0, SEEK_END );
    const size_t size( ftell( fileStream ) );   
    rewind( fileStream );   
    if( size > 0 ) {
        char *buffer = new char[ size ];
        const size_t readSize( fread( buffer, sizeof( char ), size, fileStream ) );
        assert( readSize == size );
        OpenDDLParser theParser;
        theParser.setBuffer( buffer, size );
        const bool result( theParser.parse() );
        if( !result ) {
            std::cerr << "Error while parsing file " << filename << "." << std::endl;
        }
    }
    return 0;
}

```

How to access the imported data
===============================
The data is organized as a tree. You can get the root tree with the following code:

```
OpenDDLParser theParser;
theParser.setBuffer( buffer, size );
const bool result( theParser.parse() );
if ( result ) {
    DDLNode *root = theParser.getRoot();

    DDLNode::DllNodeList childs = root->getChildNodeList();
    for ( size_t i=0; i<childs.size(); i++ ) {
        DDLNode *child = childs[ i ];
        Property *prop = child->getProperty(); // to get properties
        std:.string type = child->getType();   // to get the node type
        Value *values = child->getValue();     // to get the data;
    }
}

```

The instance called root contains the data.
