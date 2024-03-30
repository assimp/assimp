string id
=========

![Project Status](https://img.shields.io/endpoint?url=https%3A%2F%2Fwww.jonathanmueller.dev%2Fproject%2Fstring_id%2Findex.json)

Motivation
----------
It is often useful for logging purposes in real-time applications like games to give each entity a name. This makes tracking down errors easier because finding entities via unique names is easier than looking at unique numbers or other forms of identifiers. But strings are huge and copying and comparing them is slow, so they often can't be used in performance critical code.

One solution are hashed strings which are only integers and thus small, fast to copy and compare. But hashes don't allow retrieving the original string value which is exactly what is needed for logging and debugging purposes! In addition, there is a chance of collisions, so equal hash code doesn't necessarily mean equal strings. This chance is small, but a source for hard to find bugs.

Another solution is string interning. String interning uses a global look-up table where each string is stored only once and is referenced via an index or something similar. Copying and comparing is fast, too, but they are still not perfect: You can only access them at runtime. Getting the value at compile-time e.g. for a switch is not possible.

So on the one hand we want fast and lightweight identifiers but on the other hand also methods to get the name back. 


string_id
---------
This open source library provides a mix between the two solutions in the form of the class *string_id*. Each object stores a hashed string value and a pointer to the database in which the original string value is stored. This allows retrieving the string value when needed while also getting the performance benefits from hashed strings. In addition, the database can detect collisions which can be handled via a custom collision handler. There is a user-defined literal to create a compile-time hashed string value to use it as a constant expression.

The database can be any user-defined type derived from a certain interface class. There are several pre-defined databases. This includes a dummy database that does not store anything, a adapter for other databases to make them threadsafe and a highly optimized database for efficient storing and retrieving of strings. A typedef *default_database* is one of those databases and can be set via the following CMake options:

* *FOONATHAN_STRING_ID_DATABASE* - if *OFF*, the database is disabled completely, e.g. the dummy database is used. This does not allow retrieving strings or collision checking but does not need that much memory. It is *ON* by default.

* *FOONATHAN_STRING_ID_MULTITHREADED* - if *ON*, database access will be synchronized via a mutex, e.g. the thread safe adapter will be used. It has no effect if database is disabled. Default value is *ON*.

There are special generator classes. They have a similar interface to the random number generators in the standard libraries, but generate string identifiers. This is used to generate a bunch of identifiers in an automated fashion. The generators also take care that there are always new identifiers generated. This can be controlled via a handler similar to the collision handling, too.

See example/main.cpp for an example.

Hashing and Databases
---------------------
It currently uses a FNV-1a 64bit hash. Collisions are really rare, I have tested 219,606 English words (in lowercase) mixed with a bunch of numbers and didn't encounter a single collision. Since this is the normal use case for identifiers, the hash function is pretty good. In addition, there is a good distribution of the hashed values and it is easy to calculate.

The database uses a specialized hash table. Collisions of the bucket index are resolved via separate chaining with single linked list. Each node contains the string directly without additional memory allocation. The nodes on the linked list are sorted using the hash value. This allows efficient retrieving and checking whether there is already a string with the same hash value stored. This makes it very efficient and faster than the std::unordered_map that was used before (at least faster than libstdc++ implementation I have used for the benchmarks).

Compiler Support
----------------
This library has been compiled under the following compilers:
* GCC 4.6-4.9 on Linux
* clang 3.4-3.5 on Linux
* Visual Studio 12 on Windows

There are the compatibility options and replacement marcos for constexpr, noexcept, override and literal operators. Atomic handler functions can be disabled optionally and are off for GCC 4.6 by default since it does not support them.
