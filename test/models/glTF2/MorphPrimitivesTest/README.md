# Morph-Primitives Test

## Tags

[core](../../Models-core.md), [testing](../../Models-testing.md)

## Summary

Tests a morph target on multiple primitives.

## Operations

* [Display](https://github.khronos.org/glTF-Sample-Viewer-Release/?model=https://raw.GithubUserContent.com/KhronosGroup/glTF-Sample-Assets/main/./Models/MorphPrimitivesTest/glTF-Binary/MorphPrimitivesTest.glb) in SampleViewer
* [Download GLB](https://raw.GithubUserContent.com/KhronosGroup/glTF-Sample-Assets/main/./Models/MorphPrimitivesTest/glTF-Binary/MorphPrimitivesTest.glb)
* [Model Directory](./)

## Screenshot

![screenshot](screenshot/screenshot.jpg)

## Description

This model contains a simple mesh with two primitives:  A larger red primitive displays a grid covering 3 of the 4 quadrants of the model's area, followed by a smaller green primitive covering the last quadrant.

Each primitive has a morph target that creates an elevated area within these quadrants.  The model's only mesh contains a `weights: [0.5]` instruction that should cause these morph targets to be applied at half strength, raising the center of the model as shown in the screenshot above.

## Common Problems

If the entire model appears perfectly flat, it is likely that the morph targets have not been applied as requested.

If the red area or green area is missing, particularly in the Draco-compressed version of this model, it could indicate a problem with decompression or with support of multiple primitives within a single mesh.

## Author

Model by [@ft-lab](https://github.com/ft-lab).




## Legal

&copy; 2018, ft-lab. [CC BY 4.0 International](https://creativecommons.org/licenses/by/4.0/legalcode)

 - ft-lab for Everything

&copy; 2020, Frank Galligan. [CC BY 4.0 International](https://creativecommons.org/licenses/by/4.0/legalcode)

 - Frank Galligan for DRACO compression

#### Assembled by modelmetadata