### A portable (OSX/Linux/Windows), simple zip library written in C
This is done by hacking awesome [miniz](https://code.google.com/p/miniz) library and layering functions on top of the miniz v1.15 API.

[![Build](https://github.com/kuba--/zip/workflows/build/badge.svg)](https://github.com/kuba--/zip/actions?query=workflow%3Abuild)


# The Idea
<img src="zip.png" name="zip" />
... Some day, I was looking for zip library written in C for my project, but I could not find anything simple enough and lightweight.
Everything what I tried required 'crazy mental gymnastics' to integrate or had some limitations or was too heavy.
I hate frameworks, factories and adding new dependencies. If I must to install all those dependencies and link new library, I'm getting almost sick.
I wanted something powerfull and small enough, so I could add just a few files and compile them into my project.
And finally I found miniz.
Miniz is a lossless, high performance data compression library in a single source file. I only needed simple interface to append buffers or files to the current zip-entry. Thanks to this feature I'm able to merge many files/buffers and compress them on-the-fly.

It was the reason, why I decided to write zip module on top of the miniz. It required a little bit hacking and wrapping some functions, but I kept simplicity. So, you can grab these 3 files and compile them into your project. I hope that interface is also extremely simple, so you will not have any problems to understand it.

# Examples

* Create a new zip archive with default compression level.
```c
struct zip_t *zip = zip_open("foo.zip", ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
{
    zip_entry_open(zip, "foo-1.txt");
    {
        const char *buf = "Some data here...\0";
        zip_entry_write(zip, buf, strlen(buf));
    }
    zip_entry_close(zip);

    zip_entry_open(zip, "foo-2.txt");
    {
        // merge 3 files into one entry and compress them on-the-fly.
        zip_entry_fwrite(zip, "foo-2.1.txt");
        zip_entry_fwrite(zip, "foo-2.2.txt");
        zip_entry_fwrite(zip, "foo-2.3.txt");
    }
    zip_entry_close(zip);
}
zip_close(zip);
```

* Append to the existing zip archive.
```c
struct zip_t *zip = zip_open("foo.zip", ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
{
    zip_entry_open(zip, "foo-3.txt");
    {
        const char *buf = "Append some data here...\0";
        zip_entry_write(zip, buf, strlen(buf));
    }
    zip_entry_close(zip);
}
zip_close(zip);
```

* Extract a zip archive into a folder.
```c
int on_extract_entry(const char *filename, void *arg) {
    static int i = 0;
    int n = *(int *)arg;
    printf("Extracted: %s (%d of %d)\n", filename, ++i, n);

    return 0;
}

int arg = 2;
zip_extract("foo.zip", "/tmp", on_extract_entry, &arg);
```

* Extract a zip entry into memory.
```c
void *buf = NULL;
size_t bufsize;

struct zip_t *zip = zip_open("foo.zip", 0, 'r');
{
    zip_entry_open(zip, "foo-1.txt");
    {
        zip_entry_read(zip, &buf, &bufsize);
    }
    zip_entry_close(zip);
}
zip_close(zip);

free(buf);
```

* Extract a zip entry into memory (no internal allocation).
```c
unsigned char *buf;
size_t bufsize;

struct zip_t *zip = zip_open("foo.zip", 0, 'r');
{
    zip_entry_open(zip, "foo-1.txt");
    {
        bufsize = zip_entry_size(zip);
        buf = calloc(sizeof(unsigned char), bufsize);

        zip_entry_noallocread(zip, (void *)buf, bufsize);
    }
    zip_entry_close(zip);
}
zip_close(zip);

free(buf);
```

* Extract a zip entry into memory using callback.
```c
struct buffer_t {
    char *data;
    size_t size;
};

static size_t on_extract(void *arg, unsigned long long offset, const void *data, size_t size) {
    struct buffer_t *buf = (struct buffer_t *)arg;
    buf->data = realloc(buf->data, buf->size + size + 1);
    assert(NULL != buf->data);

    memcpy(&(buf->data[buf->size]), data, size);
    buf->size += size;
    buf->data[buf->size] = 0;

    return size;
}

struct buffer_t buf = {0};
struct zip_t *zip = zip_open("foo.zip", 0, 'r');
{
    zip_entry_open(zip, "foo-1.txt");
    {
        zip_entry_extract(zip, on_extract, &buf);
    }
    zip_entry_close(zip);
}
zip_close(zip);

free(buf.data);
```


* Extract a zip entry into a file.
```c
struct zip_t *zip = zip_open("foo.zip", 0, 'r');
{
    zip_entry_open(zip, "foo-2.txt");
    {
        zip_entry_fread(zip, "foo-2.txt");
    }
    zip_entry_close(zip);
}
zip_close(zip);
```

* Create a new zip archive in memory (stream API).

```c
char *outbuf = NULL;
size_t outbufsize = 0;

const char *inbuf = "Append some data here...\0";
struct zip_t *zip = zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
{
    zip_entry_open(zip, "foo-1.txt");
    {
        zip_entry_write(zip, inbuf, strlen(inbuf));
    }
    zip_entry_close(zip);

    /* copy compressed stream into outbuf */
    zip_stream_copy(zip, (void **)&outbuf, &outbufsize);
}
zip_stream_close(zip);

free(outbuf);
```

* Extract a zip entry into a memory (stream API).

```c
char *buf = NULL;
ssize_t bufsize = 0;

struct zip_t *zip = zip_stream_open(zipstream, zipstreamsize, 0, 'r');
{
    zip_entry_open(zip, "foo-1.txt");
    {
        zip_entry_read(zip, (void **)&buf, &bufsize);
    }
    zip_entry_close(zip);
}
zip_stream_close(zip);

free(buf);
```

* List of all zip entries
```c
struct zip_t *zip = zip_open("foo.zip", 0, 'r');
int i, n = zip_entries_total(zip);
for (i = 0; i < n; ++i) {
    zip_entry_openbyindex(zip, i);
    {
        const char *name = zip_entry_name(zip);
        int isdir = zip_entry_isdir(zip);
        unsigned long long size = zip_entry_size(zip);
        unsigned int crc32 = zip_entry_crc32(zip);
    }
    zip_entry_close(zip);
}
zip_close(zip);
```

* Compress folder (recursively)
```c
void zip_walk(struct zip_t *zip, const char *path) {
    DIR *dir;
    struct dirent *entry;
    char fullpath[MAX_PATH];
    struct stat s;

    memset(fullpath, 0, MAX_PATH);
    dir = opendir(path);
    assert(dir);

    while ((entry = readdir(dir))) {
      // skip "." and ".."
      if (!strcmp(entry->d_name, ".\0") || !strcmp(entry->d_name, "..\0"))
        continue;

      snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
      stat(fullpath, &s);
      if (S_ISDIR(s.st_mode))
        zip_walk(zip, fullpath);
      else {
        zip_entry_open(zip, fullpath);
        zip_entry_fwrite(zip, fullpath);
        zip_entry_close(zip);
      }
    }

    closedir(dir);
}
```

* Deletes zip archive entries.
```c
char *entries[] = {"unused.txt", "remove.ini", "delete.me"};

struct zip_t *zip = zip_open("foo.zip", 0, 'd');
{
    zip_entries_delete(zip, entries, 3);
}
zip_close(zip);
```

# Bindings
Compile zip library as a dynamic library.
```shell
$ mkdir build
$ cd build
$ cmake -DBUILD_SHARED_LIBS=true ..
$ make
```

### [Go](https://golang.org) (cgo)
```go
package main

/*
#cgo CFLAGS: -I../src
#cgo LDFLAGS: -L. -lzip
#include <zip.h>
*/
import "C"
import "unsafe"

func main() {
	path := C.CString("/tmp/go.zip")
	zip := C.zip_open(path, 6, 'w')

	entryname := C.CString("test")
	C.zip_entry_open(zip, entryname)

	content := "test content"
	buf := unsafe.Pointer(C.CString(content))
	bufsize := C.size_t(len(content))
	C.zip_entry_write(zip, buf, bufsize)

	C.zip_entry_close(zip)

	C.zip_close(zip)
}
```

### [Rust](https://www.rust-lang.org) (ffi)
```rust
extern crate libc;
use std::ffi::CString;

#[repr(C)]
pub struct Zip {
    _private: [u8; 0],
}

#[link(name = "zip")]
extern "C" {
    fn zip_open(path: *const libc::c_char, level: libc::c_int, mode: libc::c_char) -> *mut Zip;
    fn zip_close(zip: *mut Zip) -> libc::c_void;

    fn zip_entry_open(zip: *mut Zip, entryname: *const libc::c_char) -> libc::c_int;
    fn zip_entry_close(zip: *mut Zip) -> libc::c_int;
    fn zip_entry_write(
        zip: *mut Zip,
        buf: *const libc::c_void,
        bufsize: libc::size_t,
    ) -> libc::c_int;
}

fn main() {
    let path = CString::new("/tmp/rust.zip").unwrap();
    let mode: libc::c_char = 'w' as libc::c_char;

    let entryname = CString::new("test.txt").unwrap();
    let content = "test content\0";

    unsafe {
        let zip: *mut Zip = zip_open(path.as_ptr(), 5, mode);
        {
            zip_entry_open(zip, entryname.as_ptr());
            {
                let buf = content.as_ptr() as *const libc::c_void;
                let bufsize = content.len() as libc::size_t;
                zip_entry_write(zip, buf, bufsize);
            }
            zip_entry_close(zip);
        }
        zip_close(zip);
    }
}
```

### [Ruby](http://www.ruby-lang.org) (ffi)
Install _ffi_ gem.
```shell
$ gem install ffi
```

Bind in your module.
```ruby
require 'ffi'

module Zip
  extend FFI::Library
  ffi_lib "./libzip.#{::FFI::Platform::LIBSUFFIX}"

  attach_function :zip_open, [:string, :int, :char], :pointer
  attach_function :zip_close, [:pointer], :void

  attach_function :zip_entry_open, [:pointer, :string], :int
  attach_function :zip_entry_close, [:pointer], :void
  attach_function :zip_entry_write, [:pointer, :string, :int], :int
end

ptr = Zip.zip_open("/tmp/ruby.zip", 6, "w".bytes()[0])

status = Zip.zip_entry_open(ptr, "test")

content = "test content"
status = Zip.zip_entry_write(ptr, content, content.size())

Zip.zip_entry_close(ptr)
Zip.zip_close(ptr)
```

### [Python](https://www.python.org) (cffi)
Install _cffi_ package
```shell
$ pip install cffi
```

Bind in your package.
```python
import ctypes.util
from cffi import FFI

ffi = FFI()
ffi.cdef("""
    struct zip_t *zip_open(const char *zipname, int level, char mode);
    void zip_close(struct zip_t *zip);

    int zip_entry_open(struct zip_t *zip, const char *entryname);
    int zip_entry_close(struct zip_t *zip);
    int zip_entry_write(struct zip_t *zip, const void *buf, size_t bufsize);
""")

Zip = ffi.dlopen(ctypes.util.find_library("zip"))

ptr = Zip.zip_open("/tmp/python.zip", 6, 'w')

status = Zip.zip_entry_open(ptr, "test")

content = "test content"
status = Zip.zip_entry_write(ptr, content, len(content))

Zip.zip_entry_close(ptr)
Zip.zip_close(ptr)
```

### [Never](https://never-lang.readthedocs.io/) (ffi)
```never
extern "libzip.so" func zip_open(zipname: string, level: int, mode: char) -> c_ptr
extern "libzip.so" func zip_close(zip: c_ptr) -> void

extern "libzip.so" func zip_entry_open(zip: c_ptr, entryname: string) -> int
extern "libzip.so" func zip_entry_close(zip: c_ptr) -> int
extern "libzip.so" func zip_entry_write(zip: c_ptr, buf: string, bufsize: int) -> int
extern "libzip.so" func zip_entry_fwrite(zip: c_ptr, filename: string) -> int

func main() -> int
{
    let content = "Test content"

    let zip = zip_open("/tmp/never.zip", 6, 'w');

    zip_entry_open(zip, "test.file");
    zip_entry_fwrite(zip, "/tmp/test.txt");
    zip_entry_close(zip);

    zip_entry_open(zip, "test.content");
    zip_entry_write(zip, content, length(content));
    zip_entry_close(zip);

    zip_close(zip);
    0
}
```

### [Ring](http://ring-lang.net)
The language comes with RingZip based on this library
```ring
load "ziplib.ring"

new Zip {
    setFileName("myfile.zip")
    open("w")
    newEntry() {
        open("test.c")
        writefile("test.c")
        close()
    }
    close()
}
```

# Check out more cool projects which use this library:
- [Filament](https://github.com/google/filament): Filament is a real-time physically based rendering engine for Android, iOS, Linux, macOS, Windows, and WebGL. It is designed to be as small as possible and as efficient as possible on Android.
- [Hermes JS Engine](https://github.com/facebook/hermes): Hermes is a JavaScript engine optimized for fast start-up of React Native apps on Android. It features ahead-of-time static optimization and compact bytecode.
- [Open Asset Import Library](https://github.com/assimp/assimp): A library to import and export various 3d-model-formats including scene-post-processing to generate missing render data.
- [PowerToys](https://github.com/microsoft/PowerToys): Set of utilities for power users to tune and streamline their Windows 10 experience for greater productivity.
- [The Ring Programming Language](https://ring-lang.github.io): Innovative and practical general-purpose multi-paradigm language.
- [The V Programming Language](https://github.com/vlang/v): Simple, fast, safe, compiled. For developing maintainable software.
- [TIC-80](https://github.com/nesbox/TIC-80): TIC-80 is a FREE and OPEN SOURCE fantasy computer for making, playing and sharing tiny games.
- [Urho3D](https://github.com/urho3d/Urho3D): Urho3D is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE and Horde3D.
- [Vcpkg](https://github.com/microsoft/vcpkg): Vcpkg helps you manage C and C++ libraries on Windows, Linux and MacOS.
- [and more...](https://grep.app/search?q=kuba--/zip)

