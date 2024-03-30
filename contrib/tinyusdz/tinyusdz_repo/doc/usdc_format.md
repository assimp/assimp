## Bootstrap(header)

* magic header: "PXR-USDC" : 8 bytes
* version number: uint8 x 8(0 = major, 1 = minor, 2 = patch, rest unused) :8 bytes
  * Current(as of pxrUSD 23.05: default 0.8.0) 
* int64_t tocOffset
  * Byte offset to ToC(Table of Contents) 
* int64_t _reserved[8]

=> total 88 bytes

## Version

From pxr/usd/usd/crateFile.cpp

```
// Version history:
// 0.9.0: Added support for the timecode and timecode[] value types.
// 0.8.0: Added support for SdfPayloadListOp values and SdfPayload values with
//        layer offsets.
// 0.7.0: Array sizes written as 64 bit ints.
```

* TinyUSDZ should support at least v0.8.0(USD v20.11).
* TinyUSDZ has some support to load USDC version v0.7.0 or v0.4.0
  * Moana island v2.1 USD file
     
## Compression

version 0.4.0 or later uses LZ4 for compression and interger compression(pxr original? compression. its backend still uses LZ4) for index arrays.

## TOC(table of contents)

List of Sections.

## Sections

There are 6 known sections.

### TOKENS

List of strings(tokens). tokens are unique.

### STRINGS

List of StringIndices(index to `tokens`)

### FIELDS

Data field info. This is a bit tricky data format.
See the source code for details.

Field is consist of

* index to token
* 8 byte info. 2 byte for type an so on, 6 byte for primitive value or offset to the value(e.g. for array data)

### FIELDSETS

List of indices

### PATHS

List of path indices.

Paths are encoded in child-sibling tree.
                          
### SPECS

List of specs

##### Spec

* path index
* fieldsets index 
* SdfSpecType
