# WRL/X3DV to X3D file format converter

## VRML and X3D 3D model formats background
`.wrl` and `.x3dv` 3D model files use the "classic" VRML file format.

Later the X3D model specification was introduced, which was a superset of both WRL and X3DV.
While X3D can understand the _content_ of WRL/X3DV files, it can't directly parse them because
X3D uses `.xml` files, rather than the "classic" VRML file format.

But, if a converter is available to migrate just the file format (preserving the content), so that
the `.wrl`/`.x3dv` files can be converted to an X3D-compatible `.xml` file, then the X3D importer
will be able to load the resulting model file.

## How this code is used
The sole purpose of `Parser`/`Scanner` is to take a "classic" VRML (`.wrl` or `.x3dv`) file as input,
and convert to an X3D `.xml` file.  That's it.

By passing the converted in-memory `.xml` file content to the `X3DImporter`, the `.wrl` or `x3dv`
model can be loaded via assimp.
