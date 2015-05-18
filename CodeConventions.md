
Open Asset Import Library Coding Conventions
==

If you want to participate as a developer in the **Open Asset Import Library** please read and respect the following coding conventions. This will ensure consistency throughout the codebase and help all the Open Asset Import Library users.

Spacing
==

* No spaces between parentheses and arguments - i.e. ```foo(bar)```, not ```foo( bar )```

Tabs
--

The tab width shall be 4 spaces.  Use spaces instead of tabs.

Class/Struct Initialization
==
Constructors shall use initializer lists as follows:
```cpp
SomeClass()
    : mExists(false)
    , mCounter()
    , mPtr()
{}
```

* Initializations are one-per-line
* Commas go at the beginning of the line rather than the end
* The order of the list must match the order of declaration in the class
* Any member with a default value should leave out the optional *NULL* or *0* - e.g. ```foo()```, not ```foo(NULL)```
