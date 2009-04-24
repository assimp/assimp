/** @file dox_cmd.h
 *  @brief General documentation for assimp_cmd
 */


//----------------------------------------------------------------------------------------------
// ASSIMP CMD MAINPAGE
/**
@mainpage ASSIMP Command-line tools

<img src="dragonsplash.png"></img>

@section intro Introduction

This document describes the usage of assimp's command line tools. 
This is *not* the SDK reference and programming-related stuff is not covered here.
<br><br>
<b>NOTE</b>: For simplicity, the following sections are written with Windows in mind. However
it's not different for Linux/Mac at all, except there's probably no assimp.exe ...

@section basic_use Basic use

Open a command prompt and navigate to the directory where assimp.exe resides. The basic command line is:

@code
assimp [command] [parameters]
@endcode

The following commands are available:

<table border="1">
 
  <tr>
    <td><b>@link version version @endlink</b></td>
    <td>Retrieve the current version of assimp</td>
  </tr>
  <tr>
    <td><b>@link help help @endlink</b></td>
    <td>Get a list of all commands (yes, it's this list ...)</td>
  </tr>
   <tr>
    <td><b>@link dump dump @endlink</b></td>
    <td>Generate a human-readable text dump of a model</td>
  </tr>
   <tr>
    <td><b>@link extract extract @endlink</b></td>
    <td>Extract an embedded texture image</td>
  </tr>
</table>

If you use assimp's command line frequently, consider adding assimp to your PATH
environment.

 */


/**
@page version 'version'-Command


 */

/**
@page help 'help'-Command


 */

//----------------------------------------------------------------------------------------------
// ASSIMP DUMP

/**
@page dump 'dump'-Command

Generate a text or binary dump of a model. This is the core component of Assimp's internal
regression test suite but it could also be useful for other developers to quickly
examine the contents of a model. Note that text dumps are not intended to be used as
intermediate format, Assimp is not able to read them again, nor is the file format
stable or well-defined. It may change with every revision without notice. 
Binary dumps (*.assfile) are backward- and forward-compatible.

<h3>Syntax:</h3>

@code
assimp dump <model> [<out>] [-b] [-s] [common parameters]
@endcode


<h3>Parameters:</h3>

<p>
<tt>
model<br></tt><br>
Required. Relative or absolute path to the input model. A wildcard may be specified.
See the @link wildcard wildcards page @endlink for more information.
</p>
<p>
<tt>
out<br></tt><br>
Optional. Relative or absolute path to write the output dump to. If it is ommitted,
the dump is written to <tt><model>-dump.txt</tt>
</p>

<p>
<tt>-b<br>
</tt><br>
Optional. If this switch is specified, the dumb is written in binary format.
The long form of this parameter is <tt>--binary</tt>.
</p>

<p>
<tt>-s<n><br>
</tt><br>
Optional. If this switch is specified, the dumb is shortened to include only
min/max values for all vertex components and animation channels. The resulting
file is much smaller, but the original model can't be reconstructed from it. This is 
used by Assimp's regression test suite,comapring those minidumps provides
a fast way to verify whether a loader works correctly or not.
The long form of this parameter is <tt>--short</tt>.
</p>

<p>
<tt>
common parameters<br></tt><br>
Optional. Import configuration & postprocessing. 
See the @link common common parameters page @endlink for more information.
</p>

<hr>

<h3>Sample:</h3>

@code
assimp dump test.3ds test.txt -l -cfull
assimp dump test.3ds test.txt -include-log -config=full
@endcode

Dumps 'test.3ds' to 'test.txt' after executing full post-processing on tehe imported data.
The log output is included with the dump.


@code
assimp dump files\*.*
assimp dump files\*.* 
@endcode

Dumps all loadable model files in the 'files' subdir. The output dumps are named
<tt><mode-file>-dump.txt</tt>. The log is not included.
 */

//----------------------------------------------------------------------------------------------
// ASSIMP EXTRACT

/**
@page extract 'extract'-Command

Extracts one or more embedded texture images from models.

<h3>Syntax:</h3>

@code
assimp extract <model> [<out>] [-t<n>] [-f<fmt>] [-ba] [-s] [common parameters]
@endcode


<h3>Parameters:</h3>

<p>
<tt>
model<br></tt><br>
Required. Relative or absolute path to the input model. A wildcard may be specified.
See the @link wildcard wildcards page @endlink for more information.
</p>
<p>
<tt>
out<br></tt><br>
Optional. Relative or absolute path to write the output images to. If the file name is
ommitted the output images are named <tt><model-filename></tt><br>
The suffix <tt>_img<n></tt> is appended to the file name if the -s switch is not specified 
(where <tt><n></tt> is the zero-based index of the texture in the model file).<br>

The output file format is determined from the given file extension. Supported
formats are BMP and TGA. If the file format can't be determined,
the value specified with the -f switch is taken.
<br>
Format settings are ignored for compressed embedded textures. They're always
written in their native file format (e.g. jpg).
</p>

<p>
<tt>-t<n><br>
</tt><br>
Optional. Specifies the (zero-based) index of the embedded texture to be extracted from 
the model. If this option is *not* specified all textures found are exported.
The long form of this parameter is <tt>--texture=<n></tt>.
</p>

<p>
<tt>-t<n><br>
</tt><br>
Optional. Specifies whether output BMPs contain an alpha channel or not.
The long form of this parameter is <tt>--bmp-with-alpha=<n></tt>.
</p>


<p>
<tt>-f<n><br>
</tt><br>
Optional. Specifies the output file format. Supported
formats are BMP and TGA. The default value is BMP (if a full output filename is
specified, the output file format is taken from its extension, not from here).
The long form of this parameter is <tt>--format=<n></tt>.
</p>

<p>
<tt>-s<n><br>
</tt><br>
Optional. Prevents the tool from adding the <tt>_img<n></tt> suffix to all filenames. This option
must be specified together with -t to ensure that just one image is written.
The long form of this parameter is <tt>--nosuffix</tt>.
</p>

<p>
<tt>
common parameters<br></tt><br>
Optional. Import configuration & postprocessing. Most postprocessing-steps don't affect
embedded texture images, configuring too much is probably senseless here.
See the @link common common parameters page @endlink for more information.
</p>

<hr>

<h3>Sample:</h3>

@code
assimp extract test.mdl test.bmp --texture=0 --validate-data-structure
assimp extract test.mdl test.bmp -t=0 -vds
@endcode

Extracts the first embedded texture (if any) from test.mdl after validating the
imported data structure and writes it to <tt>test_img0.bmp</tt>.


@code
assimp extract files\*.mdl *.bmp 
assimp extract files\*.mdl *.bmp 
@endcode

Extracts all embedded textures from all loadable .mdl files in the 'files' subdirectory
and writes them to bitmaps which are named <tt><model-file>_img<image-index>.bmp</tt>
 */

//----------------------------------------------------------------------------------------------
// ASSIMP COMMON PARAMETERS
/**
@page common Common parameters

The parameters described on this page are commonly used by almost every assimp command. They 
specify how the library will postprocess the imported data. This is done by several
configurable pipeline stages, called 'post processing steps'. Below you can find a list
of all supported steps along with short descriptions of what they're doing. <br><b>Programmers</b>: 
more information can be found in the <tt>aiPostProcess.h</tt> header.

<table border="1">
 
  <tr>
    <th>Parameter</th>
    <th>Long parameter</th>
	<th>Description</th>
  </tr>
  <tr>
    <td><tt>-ptv</tt></td>
    <td><tt>--pretransform-vertices</tt></td>
	<td>Move all vertices into worldspace and collapse the scene graph. Animation data is lost. 
	This is intended for applications which don't support scenegraph-oriented rendering.</td>
  </tr>
  <tr>
    <td><tt>-gsn</tt></td>
    <td><tt>--gen-smooth-normals</tt></td>
	<td>Computes 'smooth' per-vertex normal vectors if necessary. Mutually exclusive with -gn</td>
  </tr>
  <tr>
    <td><tt>-gn</tt></td>
    <td><tt>--gen-normals</tt></td>
	<td>Computes 'hard' per-face normal vectors if necessary. Mutually exclusive with -gsn</td>
  </tr>
  <tr>
    <td><tt>-cts</tt></td>
    <td><tt>--calc-tangent-space</tt></td>
	<td>If one UV channel and normal vectors are given, compute tangents and bitangents</td>
  </tr>
  <tr>
    <td><tt>-jiv</tt></td>
    <td><tt>--join-identical-vertices</tt></td>
	<td>Optimize the index buffer. If this flag is not specified all vertices are referenced once.</td>
  </tr>
  <tr>
    <td><tt>-rrm</tt></td>
    <td><tt>--remove-redundant-materials</tt></td>
	<td>Remove redundant materials from the imported data.</td>
  </tr>
  <tr>
    <td><tt>-fd</tt></td>
    <td><tt>--find-degenerates</tt></td>
	<td>Find and process degenerates primitives.</td>
  </tr>
  <tr>
    <td><tt>-slm</tt></td>
    <td>--split-large-meshes</tt></td>
	<td>Split large meshes over a specific treshold in smaller sub meshes. The default vertex & face limit is 1000000</td>
  </tr>
  <tr>
    <td><tt>-lbw</tt></td>
    <td><tt>--limit-bone-weights</tt></td>
	<td>Limit the number of bones influencing a single vertex. The default limit is 4.</td>
  </tr>
  <tr>
    <td><tt>-vds</tt></td>
    <td><tt>--validate-data-structure</tt></td>
	<td>Performs a full validation of the imported data structure. Recommended to avoid crashes if
	an import plugin produces rubbish</td>
  </tr>
   <tr>
    <td><tt>-icl</tt></td>
    <td><tt>--improve-cache-locality</tt></td>
	<td>Improve the cache locality of the vertex buffer by reordering the index buffer 
	to achieve a lower ACMR (average post-transform vertex cache miss ratio)</td>
  </tr>
   <tr>
    <td><tt>-sbpt</tt></td>
    <td><tt>--sort-by-ptype</tt></td>
	<td>Splits meshes which consist of more than one kind of primitives (e.g. lines and triangles mixed up)
	in 'clean' submeshes. </td>
  </tr>
   <tr>
    <td><tt>-lh</tt></td>
    <td><tt>--convert-to-lh</tt></td>
	<td>Converts the imported data to left-handed coordinate space</td>
  </tr>
   <tr>
    <td><tt>-fuv</tt></td>
    <td><tt>--flip-uv</tt></td>
	<td>Flip UV coordinates from upper-left origin to lower-left origin</td>
  </tr>
   <tr>
    <td><tt>-fwo</tt></td>
    <td><tt>--flip-winding-order</tt></td>
	<td>Flip face winding order from CCW to CW</td>
  </tr>
  <tr>
    <td><tt>-ett</tt></td>
    <td><tt>--evaluate-texture-transform</tt></td>
	<td>Evaluate per-texture UV transformations (e.g scaling, offset) and build pretransformed UV channels</td>
  </tr>
   <tr>
    <td><tt>-guv</tt></td>
    <td><tt>--gen-uvcoords</tt></td>
	<td>Replace abstract mapping descriptions, such as 'spherical' or 'cylindrical' with proper UV channels</td>
  </tr>
    <tr>
    <td><tt>-fixn</tt></td>
    <td><tt>--fix-normals</tt></td>
	<td>Run a heuristic algorithm to detect meshes with wrong face winding order/normals. </td>
  </tr>
   <tr>
    <td><tt>-tri</tt></td>
    <td><tt>--triangulate</tt></td>
	<td>Triangulate poylgons with 4 and more points. Lines, points and triangles are not affected. </td>
  </tr>
   <tr>
    <td><tt>-fi</tt></td>
    <td><tt>--find-instances</tt></td>
	<td>Search the data structure for instanced meshes and replace them by references. This can
	reduce vertex/face counts but the postprocessing-step takes some time.</td>
  </tr>

    <tr>
    <td><tt>-og</tt></td>
    <td><tt>--optimize-graph</tt></td>
	<td>Simplify and optimize the scenegraph. Use it with care, all hierarchy information could be lost.
	Animations remain untouched. </td>
  </tr>

    <tr>
    <td><tt>-om</tt></td>
    <td><tt>--optimize-mesh</tt></td>
	<td>Optimize mesh usage. Meshes are merged, if possible. Very effective in combination with <tt>--optimize-graph</tt></td>
  </tr>
</table>

For convenience some default postprocessing configurations are provided.
The corresponding command line parameter is <tt>-c<name></tt> (or <tt>--config=<name></tt>).

<table border="1">
 
  <tr>
    <th>Name</th>
    <th>Description</th>
	<th>List of steps executed</th>
  </tr>
  <tr>
    <td>fast</td>
    <td>Fast post processing config, performs some essential optimizations and computes tangents</td>
	<td><tt>-cts, -gn, -jiv, -tri, -guv, -sbpt</tt></td>
  </tr>
    <tr>
    <td>default</td>
    <td>Balanced post processing config; performs most optimizations</td>
	<td><tt>-cts, -gsn, -jiv, -icl, -lbw, -rrm, -slm, -tri, -guv, -sbpt, -fd, -fiv</tt></td>
  </tr>
    <tr>
    <td>full</td>
    <td>Full post processing. May take a while but results in best output quality for most purposes</td>
	<td><tt>-cts, -gsn, -jiv, -icl, -lbw, -rrm, -slm, -tri, -guv, -sbpt, -fd, -fiv, -fi, -vds -om</tt></td>
  </tr>
 </table>

 The <tt>-tuv, -ptv, -og</tt> flags always need to be enabled manually.

There are also some common flags to customize Assimp's logging behaviour:

<table border="1">
 
  <tr>
    <th>Name</th>
    <th>Description</th>
  </tr>
  <tr>
    <td><tt>-l</tt> or <tt>--show-log</tt></td>
    <td>Show log file on console window (stderr)</td>
  </tr>
    <tr>
    <td><tt>-lo<file></tt> or <tt>--log-out=<file></tt></td>
    <td>Streams the log to <file></td>
  </tr>
    <tr>
    <td><tt>-v</tt> or <tt>--verbose</tt></td>
    <td>Enables verbose logging. Debug messages will be produced too. This might 
	decrease loading performance and result in *very* long logs ... use with caution if you experience strange issues.</td>
  </tr>
 </table>
 */
