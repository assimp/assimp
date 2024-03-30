/*
    opengametools vox file reader/writer - v0.6 - MIT license - Justin Paver, Oct 2019

    This is a single-header-file library that provides easy-to-use
    support for reading MagicaVoxel .vox files into structures that
    are easy to dereference and extract information from. It also
    supports writing back out to .vox file from those structures.

    Please see the MIT license information at the end of this file.

    Also, please consider sharing any improvements you make.

    For more information and more tools, visit:
      https://github.com/jpaver/opengametools

    HOW TO COMPILE THIS LIBRARY

    1.  To compile this library, do this in *one* C or C++ file:
        #define OGT_VOX_IMPLEMENTATION
        #include "ogt_vox.h"

    2. From any other module, it is sufficient to just #include this as usual:
        #include "ogt_vox.h"

    HOW TO READ A VOX SCENE (See demo_vox.cpp)

    1. load a .vox file off disk into a memory buffer.

    2. construct a scene from the memory buffer:
       ogt_vox_scene* scene = ogt_vox_read_scene(buffer, buffer_size);

    3. use the scene members to extract the information you need. eg.
       printf("# of layers: %u\n", scene->num_layers );

    4. destroy the scene:
       ogt_vox_destroy_scene(scene);

    HOW TO MERGE MULTIPLE VOX SCENES (See merge_vox.cpp)

    1. construct multiple scenes from files you want to merge.

        // read buffer1/buffer_size1 from "test1.vox"
        // read buffer2/buffer_size2 from "test2.vox"
        // read buffer3/buffer_size3 from "test3.vox"
        ogt_vox_scene* scene1 = ogt_vox_read_scene(buffer1, buffer_size1);
        ogt_vox_scene* scene2 = ogt_vox_read_scene(buffer2, buffer_size2);
        ogt_vox_scene* scene3 = ogt_vox_read_scene(buffer3, buffer_size3);

    2. construct a merged scene

        const ogt_vox_scene* scenes[] = {scene1, scene2, scene3};
        ogt_vox_scene* merged_scene = ogt_vox_merge_scenes(scenes, 3, NULL, 0);

    3. save out the merged scene

        uint8_t* out_buffer = ogt_vox_write_scene(merged_scene, &out_buffer_size);
        // save out_buffer to disk as a .vox file (it has length out_buffer_size)

    4. destroy the merged scene:

        ogt_vox_destroy_scene(merged_scene);

    EXPLANATION OF SCENE ELEMENTS:

    A ogt_vox_scene comprises primarily a set of instances, models, layers and a palette.

    A ogt_vox_palette contains a set of 256 colors that is used for the scene.
    Each color is represented by a 4-tuple called an ogt_vox_rgba which contains red,
    green, blue and alpha values for the color.

    A ogt_vox_model is a 3-dimensional grid of voxels, where each of those voxels
    is represented by an 8-bit color index. Voxels are arranged in order of increasing
    X then increasing Y then increasing Z.

    Given the x,y,z values for a voxel within the model dimensions, the voxels index
    in the grid can be obtained as follows:

        voxel_index = x + (y * model->size_x) + (z * model->size_x * model->size_y)

    The index is only valid if the coordinate x,y,z satisfy the following conditions:
            0 <= x < model->size_x -AND-
            0 <= y < model->size_y -AND-
            0 <= z < model->size_z

    A voxels color index can be obtained as follows:

        uint8_t color_index = model->voxel_data[voxel_index];

    If color_index == 0, the voxel is not solid and can be skipped,
    If color_index != 0, the voxel is solid and can be used to lookup the color in the palette:

        ogt_vox_rgba color = scene->palette.color[ color_index]

    A ogt_vox_instance is an individual placement of a voxel model within the scene. Each
    instance has a transform that determines its position and orientation within the scene,
    but it also has an index that specifies which model the instance uses for its shape. It
    is expected that there is a many-to-one mapping of instances to models.

    An ogt_vox_layer is used to conceptually group instances. Each instance indexes the
    layer that it belongs to, but the layer itself has its own name and hidden/shown state.

    EXPLANATION OF MERGED SCENES:

    A merged scene contains all the models and all the scene instances from
    each of the scenes that were passed into it.

    The merged scene will have a combined palette of all the source scene
    palettes by trying to match existing colors exactly, and falling back
    to an RGB-distance matched color when all 256 colors in the merged
    scene palette has been allocated.

    You can explicitly control up to 255 merge palette colors by providing
    those colors to ogt_vox_merge_scenes in the required_colors parameters eg.

        const ogt_vox_palette palette;  // load this via .vox or procedurally or whatever
        const ogt_vox_scene* scenes[] = {scene1, scene2, scene3};
        // palette.color[0] is always the empty color which is why we pass 255 colors starting from index 1 only:
        ogt_vox_scene* merged_scene = ogt_vox_merge_scenes(scenes, 3, &palette.color[1], 255);

    EXPLANATION OF MODEL PIVOTS

    If a voxel model grid has dimension size.xyz in terms of number of voxels, the centre pivot
    for that model is located at floor( size.xyz / 2).

    eg. for a 3x4x1 voxel model, the pivot would be at (1,2,0), or the X in the below ascii art.

           4 +-----+-----+-----+
             |  .  |  .  |  .  |
           3 +-----+-----+-----+
             |  .  |  .  |  .  |
           2 +-----X-----+-----+
             |  .  |  .  |  .  |
           1 +-----+-----+-----+
             |  .  |  .  |  .  |
           0 +-----+-----+-----+
             0     1     2     3

     An example model in this grid form factor might look like this:

           4 +-----+-----+-----+
             |  .  |  .  |  .  |
           3 +-----+-----+-----+
                   |  .  |
           2       X-----+
                   |  .  |
           1       +-----+
                   |  .  |
           0       +-----+
             0     1     2     3

     If you were to generate a mesh from this, clearly each vertex and each face would be on an integer
     coordinate eg. 1, 2, 3 etc. while the centre of each grid location (ie. the . in the above diagram)
     will be on a coordinate that is halfway between integer coordinates. eg. 1.5, 2.5, 3.5 etc.

     To ensure your mesh is properly centered such that instance transforms are correctly applied, you
     want the pivot to be treated as if it were (0,0,0) in model space. To achieve this, simply
     subtract the pivot from any geometry that is generated (eg. vertices in a mesh).

     For the 3x4x1 voxel model above, doing this would look like this:

           2 +-----+-----+-----+
             |  .  |  .  |  .  |
           1 +-----+-----+-----+
                   |  .  |
           0       X-----+
                   |  .  |
          -1       +-----+
                   |  .  |
          -2       +-----+
            -1     0     1     2


*/


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#ifndef OGT_VOX_H__
#define OGT_VOX_H__

#if _MSC_VER == 1400
    // VS2005 doesn't have inttypes or stdint so we just define what we need here.
    typedef unsigned char uint8_t;
    typedef signed int    int32_t;
    typedef unsigned int  uint32_t;
	#ifndef UINT32_MAX
		#define UINT32_MAX	((uint32_t)0xFFFFFFFF)
	#endif
	#ifndef INT32_MAX
		#define INT32_MAX	((int32_t)0x7FFFFFFF)
	#endif
	#ifndef UINT8_MAX
		#define UINT8_MAX	((uint8_t)0xFF)
	#endif
#elif defined(_MSC_VER)
    // general VS*
    #include <inttypes.h>
#elif __APPLE__
    // general Apple compiler
#elif defined(__GNUC__)
    // any GCC*
    #include <inttypes.h>
    #include <stdlib.h> // for size_t
#else
    #error some fixup needed for this platform?
#endif

    // denotes an invalid group index. Usually this is only applicable to the scene's root group's parent.
    static const uint32_t k_invalid_group_index = UINT32_MAX;

    // color
    typedef struct ogt_vox_rgba
    {
        uint8_t r,g,b,a;            // red, green, blue and alpha components of a color.
    } ogt_vox_rgba;

    // column-major 4x4 matrix
    typedef struct ogt_vox_transform
    {
        float m00, m01, m02, m03;   // column 0 of 4x4 matrix, 1st three elements = x axis vector, last element always 0.0
        float m10, m11, m12, m13;   // column 1 of 4x4 matrix, 1st three elements = y axis vector, last element always 0.0
        float m20, m21, m22, m23;   // column 2 of 4x4 matrix, 1st three elements = z axis vector, last element always 0.0
        float m30, m31, m32, m33;   // column 3 of 4x4 matrix. 1st three elements = translation vector, last element always 1.0
    } ogt_vox_transform;

    // a palette of colors
    typedef struct ogt_vox_palette
    {
        ogt_vox_rgba color[256];      // palette of colors. use the voxel indices to lookup color from the palette.
    } ogt_vox_palette;

    // Extended Material Chunk MATL types
    enum ogt_matl_type
    {
        ogt_matl_type_diffuse = 0, // diffuse is default
        ogt_matl_type_metal   = 1,
        ogt_matl_type_glass   = 2,
        ogt_matl_type_emit    = 3,
        ogt_matl_type_blend   = 4,
        ogt_matl_type_media   = 5,
    };

    // Content Flags for ogt_vox_matl values for a given material
    static const uint32_t k_ogt_vox_matl_have_metal  = 1 << 0;
    static const uint32_t k_ogt_vox_matl_have_rough  = 1 << 1;
    static const uint32_t k_ogt_vox_matl_have_spec   = 1 << 2;
    static const uint32_t k_ogt_vox_matl_have_ior    = 1 << 3;
    static const uint32_t k_ogt_vox_matl_have_att    = 1 << 4;
    static const uint32_t k_ogt_vox_matl_have_flux   = 1 << 5;
    static const uint32_t k_ogt_vox_matl_have_emit   = 1 << 6;
    static const uint32_t k_ogt_vox_matl_have_ldr    = 1 << 7;
    static const uint32_t k_ogt_vox_matl_have_trans  = 1 << 8;
    static const uint32_t k_ogt_vox_matl_have_alpha  = 1 << 9;
    static const uint32_t k_ogt_vox_matl_have_d      = 1 << 10;
    static const uint32_t k_ogt_vox_matl_have_sp     = 1 << 11;
    static const uint32_t k_ogt_vox_matl_have_g      = 1 << 12;
    static const uint32_t k_ogt_vox_matl_have_media  = 1 << 13;

    // Extended Material Chunk MATL information
    typedef struct ogt_vox_matl
    {
        uint32_t      content_flags; // set of k_ogt_vox_matl_* OR together to denote contents available
        ogt_matl_type type;
        float         metal;
        float         rough;
        float         spec;
        float         ior;
        float         att;
        float         flux;
        float         emit;
        float         ldr;
        float         trans;
        float         alpha;
        float         d;
        float         sp;
        float         g;
        float         media;

    } ogt_vox_matl;

    // Extended Material Chunk MATL array of materials
    typedef struct ogt_vox_matl_array
    {
        ogt_vox_matl matl[256];      // extended material information from Material Chunk MATL
    } ogt_vox_matl_array;

    // a 3-dimensional model of voxels
    typedef struct ogt_vox_model
    {
        uint32_t       size_x;        // number of voxels in the local x dimension
        uint32_t       size_y;        // number of voxels in the local y dimension
        uint32_t       size_z;        // number of voxels in the local z dimension
        uint32_t       voxel_hash;    // hash of the content of the grid.
        const uint8_t* voxel_data;    // grid of voxel data comprising color indices in x -> y -> z order. a color index of 0 means empty, all other indices mean solid and can be used to index the scene's palette to obtain the color for the voxel.
    } ogt_vox_model;

    // an instance of a model within the scene
    typedef struct ogt_vox_instance
    {
        const char*       name;         // name of the instance if there is one, will be NULL otherwise.
        ogt_vox_transform transform;    // orientation and position of this instance within the scene. This is relative to its group local transform if group_index is not 0
        uint32_t          model_index;  // index of the model used by this instance. used to lookup the model in the scene's models[] array.
        uint32_t          layer_index;  // index of the layer used by this instance. used to lookup the layer in the scene's layers[] array.
        uint32_t          group_index;  // this will be the index of the group in the scene's groups[] array. If group is zero it will be the scene root group and the instance transform will be a world-space transform, otherwise the transform is relative to the group.
        bool              hidden;       // whether this instance is individually hidden or not. Note: the instance can also be hidden when its layer is hidden, or if it belongs to a group that is hidden.
    } ogt_vox_instance;

    // describes a layer within the scene
    typedef struct ogt_vox_layer
    {
        const char* name;               // name of this layer if there is one, will be NULL otherwise.
        bool        hidden;             // whether this layer is hidden or not.
    } ogt_vox_layer;

    // describes a group within the scene
    typedef struct ogt_vox_group
    {
        ogt_vox_transform transform;            // transform of this group relative to its parent group (if any), otherwise this will be relative to world-space.
        uint32_t          parent_group_index;   // if this group is parented to another group, this will be the index of its parent in the scene's groups[] array, otherwise this group will be the scene root group and this value will be k_invalid_group_index
        uint32_t          layer_index;          // which layer this group belongs to. used to lookup the layer in the scene's layers[] array.
        bool              hidden;               // whether this group is hidden or not.
    } ogt_vox_group;

    // the scene parsed from a .vox file.
    typedef struct ogt_vox_scene
    {
        uint32_t                num_models;     // number of models within the scene.
        uint32_t                num_instances;  // number of instances in the scene
        uint32_t                num_layers;     // number of layers in the scene
        uint32_t                num_groups;     // number of groups in the scene
        const ogt_vox_model**   models;         // array of models. size is num_models
        const ogt_vox_instance* instances;      // array of instances. size is num_instances
        const ogt_vox_layer*    layers;         // array of layers. size is num_layers
        const ogt_vox_group*    groups;         // array of groups. size is num_groups
        ogt_vox_palette         palette;        // the palette for this scene
        ogt_vox_matl_array      materials;      // the extended materials for this scene
    } ogt_vox_scene;

    // allocate memory function interface. pass in size, and get a pointer to memory with at least that size available.
    typedef void* (*ogt_vox_alloc_func)(size_t size);

    // free memory function interface. pass in a pointer previously allocated and it will be released back to the system managing memory.
    typedef void  (*ogt_vox_free_func)(void* ptr);

    // override the default scene memory allocator if you need to control memory precisely.
    void  ogt_vox_set_memory_allocator(ogt_vox_alloc_func alloc_func, ogt_vox_free_func free_func);
    void* ogt_vox_malloc(size_t size);
    void  ogt_vox_free(void* mem);

    // flags for ogt_vox_read_scene_with_flags
    static const uint32_t k_read_scene_flags_groups = 1 << 0; // if not specified, all instance transforms will be flattened into world space. If specified, will read group information and keep all transforms as local transform relative to the group they are in.

    // creates a scene from a vox file within a memory buffer of a given size.
    // you can destroy the input buffer once you have the scene as this function will allocate separate memory for the scene objecvt.
    const ogt_vox_scene* ogt_vox_read_scene(const uint8_t* buffer, uint32_t buffer_size);

    // just like ogt_vox_read_scene, but you can additionally pass a union of k_read_scene_flags
    const ogt_vox_scene* ogt_vox_read_scene_with_flags(const uint8_t* buffer, uint32_t buffer_size, uint32_t read_flags);

    // destroys a scene object to release its memory.
    void ogt_vox_destroy_scene(const ogt_vox_scene* scene);

    // writes the scene to a new buffer and returns the buffer size. free the buffer with ogt_vox_free
    uint8_t* ogt_vox_write_scene(const ogt_vox_scene* scene, uint32_t* buffer_size);

    // merges the specified scenes together to create a bigger scene. Merged scene can be destroyed using ogt_vox_destroy_scene
    // If you require specific colors in the merged scene palette, provide up to and including 255 of them via required_colors/required_color_count.
    ogt_vox_scene* ogt_vox_merge_scenes(const ogt_vox_scene** scenes, uint32_t scene_count, const ogt_vox_rgba* required_colors, const uint32_t required_color_count);

#endif // OGT_VOX_H__

//-----------------------------------------------------------------------------------------------------------------
//
// If you're only interested in using this library, everything you need is above this point.
// If you're interested in how this library works, everything you need is below this point.
//
//-----------------------------------------------------------------------------------------------------------------
#ifdef OGT_VOX_IMPLEMENTATION
    #include <assert.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>

    // MAKE_VOX_CHUNK_ID: used to construct a literal to describe a chunk in a .vox file.
    #define MAKE_VOX_CHUNK_ID(c0,c1,c2,c3)     ( (c0<<0) | (c1<<8) | (c2<<16) | (c3<<24) )

    static const uint32_t CHUNK_ID_VOX_ = MAKE_VOX_CHUNK_ID('V','O','X',' ');
    static const uint32_t CHUNK_ID_MAIN = MAKE_VOX_CHUNK_ID('M','A','I','N');
    static const uint32_t CHUNK_ID_SIZE = MAKE_VOX_CHUNK_ID('S','I','Z','E');
    static const uint32_t CHUNK_ID_XYZI = MAKE_VOX_CHUNK_ID('X','Y','Z','I');
    static const uint32_t CHUNK_ID_RGBA = MAKE_VOX_CHUNK_ID('R','G','B','A');
    static const uint32_t CHUNK_ID_nTRN = MAKE_VOX_CHUNK_ID('n','T','R','N');
    static const uint32_t CHUNK_ID_nGRP = MAKE_VOX_CHUNK_ID('n','G','R','P');
    static const uint32_t CHUNK_ID_nSHP = MAKE_VOX_CHUNK_ID('n','S','H','P');
    static const uint32_t CHUNK_ID_IMAP = MAKE_VOX_CHUNK_ID('I','M','A','P');
    static const uint32_t CHUNK_ID_LAYR = MAKE_VOX_CHUNK_ID('L','A','Y','R');
    static const uint32_t CHUNK_ID_MATL = MAKE_VOX_CHUNK_ID('M','A','T','L');
    static const uint32_t CHUNK_ID_MATT = MAKE_VOX_CHUNK_ID('M','A','T','T');
    static const uint32_t CHUNK_ID_rOBJ = MAKE_VOX_CHUNK_ID('r','O','B','J');

    // Some older .vox files will not store a palette, in which case the following palette will be used!
    static const uint8_t k_default_vox_palette[256 * 4] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcc, 0xff, 0xff, 0xff, 0x99, 0xff, 0xff, 0xff, 0x66, 0xff, 0xff, 0xff, 0x33, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xcc, 0xff, 0xff, 0xff, 0xcc, 0xcc, 0xff,
        0xff, 0xcc, 0x99, 0xff, 0xff, 0xcc, 0x66, 0xff, 0xff, 0xcc, 0x33, 0xff, 0xff, 0xcc, 0x00, 0xff, 0xff, 0x99, 0xff, 0xff, 0xff, 0x99, 0xcc, 0xff, 0xff, 0x99, 0x99, 0xff, 0xff, 0x99, 0x66, 0xff,
        0xff, 0x99, 0x33, 0xff, 0xff, 0x99, 0x00, 0xff, 0xff, 0x66, 0xff, 0xff, 0xff, 0x66, 0xcc, 0xff, 0xff, 0x66, 0x99, 0xff, 0xff, 0x66, 0x66, 0xff, 0xff, 0x66, 0x33, 0xff, 0xff, 0x66, 0x00, 0xff,
        0xff, 0x33, 0xff, 0xff, 0xff, 0x33, 0xcc, 0xff, 0xff, 0x33, 0x99, 0xff, 0xff, 0x33, 0x66, 0xff, 0xff, 0x33, 0x33, 0xff, 0xff, 0x33, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xcc, 0xff,
        0xff, 0x00, 0x99, 0xff, 0xff, 0x00, 0x66, 0xff, 0xff, 0x00, 0x33, 0xff, 0xff, 0x00, 0x00, 0xff, 0xcc, 0xff, 0xff, 0xff, 0xcc, 0xff, 0xcc, 0xff, 0xcc, 0xff, 0x99, 0xff, 0xcc, 0xff, 0x66, 0xff,
        0xcc, 0xff, 0x33, 0xff, 0xcc, 0xff, 0x00, 0xff, 0xcc, 0xcc, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0xff, 0xcc, 0xcc, 0x99, 0xff, 0xcc, 0xcc, 0x66, 0xff, 0xcc, 0xcc, 0x33, 0xff, 0xcc, 0xcc, 0x00, 0xff,
        0xcc, 0x99, 0xff, 0xff, 0xcc, 0x99, 0xcc, 0xff, 0xcc, 0x99, 0x99, 0xff, 0xcc, 0x99, 0x66, 0xff, 0xcc, 0x99, 0x33, 0xff, 0xcc, 0x99, 0x00, 0xff, 0xcc, 0x66, 0xff, 0xff, 0xcc, 0x66, 0xcc, 0xff,
        0xcc, 0x66, 0x99, 0xff, 0xcc, 0x66, 0x66, 0xff, 0xcc, 0x66, 0x33, 0xff, 0xcc, 0x66, 0x00, 0xff, 0xcc, 0x33, 0xff, 0xff, 0xcc, 0x33, 0xcc, 0xff, 0xcc, 0x33, 0x99, 0xff, 0xcc, 0x33, 0x66, 0xff,
        0xcc, 0x33, 0x33, 0xff, 0xcc, 0x33, 0x00, 0xff, 0xcc, 0x00, 0xff, 0xff, 0xcc, 0x00, 0xcc, 0xff, 0xcc, 0x00, 0x99, 0xff, 0xcc, 0x00, 0x66, 0xff, 0xcc, 0x00, 0x33, 0xff, 0xcc, 0x00, 0x00, 0xff,
        0x99, 0xff, 0xff, 0xff, 0x99, 0xff, 0xcc, 0xff, 0x99, 0xff, 0x99, 0xff, 0x99, 0xff, 0x66, 0xff, 0x99, 0xff, 0x33, 0xff, 0x99, 0xff, 0x00, 0xff, 0x99, 0xcc, 0xff, 0xff, 0x99, 0xcc, 0xcc, 0xff,
        0x99, 0xcc, 0x99, 0xff, 0x99, 0xcc, 0x66, 0xff, 0x99, 0xcc, 0x33, 0xff, 0x99, 0xcc, 0x00, 0xff, 0x99, 0x99, 0xff, 0xff, 0x99, 0x99, 0xcc, 0xff, 0x99, 0x99, 0x99, 0xff, 0x99, 0x99, 0x66, 0xff,
        0x99, 0x99, 0x33, 0xff, 0x99, 0x99, 0x00, 0xff, 0x99, 0x66, 0xff, 0xff, 0x99, 0x66, 0xcc, 0xff, 0x99, 0x66, 0x99, 0xff, 0x99, 0x66, 0x66, 0xff, 0x99, 0x66, 0x33, 0xff, 0x99, 0x66, 0x00, 0xff,
        0x99, 0x33, 0xff, 0xff, 0x99, 0x33, 0xcc, 0xff, 0x99, 0x33, 0x99, 0xff, 0x99, 0x33, 0x66, 0xff, 0x99, 0x33, 0x33, 0xff, 0x99, 0x33, 0x00, 0xff, 0x99, 0x00, 0xff, 0xff, 0x99, 0x00, 0xcc, 0xff,
        0x99, 0x00, 0x99, 0xff, 0x99, 0x00, 0x66, 0xff, 0x99, 0x00, 0x33, 0xff, 0x99, 0x00, 0x00, 0xff, 0x66, 0xff, 0xff, 0xff, 0x66, 0xff, 0xcc, 0xff, 0x66, 0xff, 0x99, 0xff, 0x66, 0xff, 0x66, 0xff,
        0x66, 0xff, 0x33, 0xff, 0x66, 0xff, 0x00, 0xff, 0x66, 0xcc, 0xff, 0xff, 0x66, 0xcc, 0xcc, 0xff, 0x66, 0xcc, 0x99, 0xff, 0x66, 0xcc, 0x66, 0xff, 0x66, 0xcc, 0x33, 0xff, 0x66, 0xcc, 0x00, 0xff,
        0x66, 0x99, 0xff, 0xff, 0x66, 0x99, 0xcc, 0xff, 0x66, 0x99, 0x99, 0xff, 0x66, 0x99, 0x66, 0xff, 0x66, 0x99, 0x33, 0xff, 0x66, 0x99, 0x00, 0xff, 0x66, 0x66, 0xff, 0xff, 0x66, 0x66, 0xcc, 0xff,
        0x66, 0x66, 0x99, 0xff, 0x66, 0x66, 0x66, 0xff, 0x66, 0x66, 0x33, 0xff, 0x66, 0x66, 0x00, 0xff, 0x66, 0x33, 0xff, 0xff, 0x66, 0x33, 0xcc, 0xff, 0x66, 0x33, 0x99, 0xff, 0x66, 0x33, 0x66, 0xff,
        0x66, 0x33, 0x33, 0xff, 0x66, 0x33, 0x00, 0xff, 0x66, 0x00, 0xff, 0xff, 0x66, 0x00, 0xcc, 0xff, 0x66, 0x00, 0x99, 0xff, 0x66, 0x00, 0x66, 0xff, 0x66, 0x00, 0x33, 0xff, 0x66, 0x00, 0x00, 0xff,
        0x33, 0xff, 0xff, 0xff, 0x33, 0xff, 0xcc, 0xff, 0x33, 0xff, 0x99, 0xff, 0x33, 0xff, 0x66, 0xff, 0x33, 0xff, 0x33, 0xff, 0x33, 0xff, 0x00, 0xff, 0x33, 0xcc, 0xff, 0xff, 0x33, 0xcc, 0xcc, 0xff,
        0x33, 0xcc, 0x99, 0xff, 0x33, 0xcc, 0x66, 0xff, 0x33, 0xcc, 0x33, 0xff, 0x33, 0xcc, 0x00, 0xff, 0x33, 0x99, 0xff, 0xff, 0x33, 0x99, 0xcc, 0xff, 0x33, 0x99, 0x99, 0xff, 0x33, 0x99, 0x66, 0xff,
        0x33, 0x99, 0x33, 0xff, 0x33, 0x99, 0x00, 0xff, 0x33, 0x66, 0xff, 0xff, 0x33, 0x66, 0xcc, 0xff, 0x33, 0x66, 0x99, 0xff, 0x33, 0x66, 0x66, 0xff, 0x33, 0x66, 0x33, 0xff, 0x33, 0x66, 0x00, 0xff,
        0x33, 0x33, 0xff, 0xff, 0x33, 0x33, 0xcc, 0xff, 0x33, 0x33, 0x99, 0xff, 0x33, 0x33, 0x66, 0xff, 0x33, 0x33, 0x33, 0xff, 0x33, 0x33, 0x00, 0xff, 0x33, 0x00, 0xff, 0xff, 0x33, 0x00, 0xcc, 0xff,
        0x33, 0x00, 0x99, 0xff, 0x33, 0x00, 0x66, 0xff, 0x33, 0x00, 0x33, 0xff, 0x33, 0x00, 0x00, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xcc, 0xff, 0x00, 0xff, 0x99, 0xff, 0x00, 0xff, 0x66, 0xff,
        0x00, 0xff, 0x33, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xcc, 0xff, 0xff, 0x00, 0xcc, 0xcc, 0xff, 0x00, 0xcc, 0x99, 0xff, 0x00, 0xcc, 0x66, 0xff, 0x00, 0xcc, 0x33, 0xff, 0x00, 0xcc, 0x00, 0xff,
        0x00, 0x99, 0xff, 0xff, 0x00, 0x99, 0xcc, 0xff, 0x00, 0x99, 0x99, 0xff, 0x00, 0x99, 0x66, 0xff, 0x00, 0x99, 0x33, 0xff, 0x00, 0x99, 0x00, 0xff, 0x00, 0x66, 0xff, 0xff, 0x00, 0x66, 0xcc, 0xff,
        0x00, 0x66, 0x99, 0xff, 0x00, 0x66, 0x66, 0xff, 0x00, 0x66, 0x33, 0xff, 0x00, 0x66, 0x00, 0xff, 0x00, 0x33, 0xff, 0xff, 0x00, 0x33, 0xcc, 0xff, 0x00, 0x33, 0x99, 0xff, 0x00, 0x33, 0x66, 0xff,
        0x00, 0x33, 0x33, 0xff, 0x00, 0x33, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xcc, 0xff, 0x00, 0x00, 0x99, 0xff, 0x00, 0x00, 0x66, 0xff, 0x00, 0x00, 0x33, 0xff, 0xee, 0x00, 0x00, 0xff,
        0xdd, 0x00, 0x00, 0xff, 0xbb, 0x00, 0x00, 0xff, 0xaa, 0x00, 0x00, 0xff, 0x88, 0x00, 0x00, 0xff, 0x77, 0x00, 0x00, 0xff, 0x55, 0x00, 0x00, 0xff, 0x44, 0x00, 0x00, 0xff, 0x22, 0x00, 0x00, 0xff,
        0x11, 0x00, 0x00, 0xff, 0x00, 0xee, 0x00, 0xff, 0x00, 0xdd, 0x00, 0xff, 0x00, 0xbb, 0x00, 0xff, 0x00, 0xaa, 0x00, 0xff, 0x00, 0x88, 0x00, 0xff, 0x00, 0x77, 0x00, 0xff, 0x00, 0x55, 0x00, 0xff,
        0x00, 0x44, 0x00, 0xff, 0x00, 0x22, 0x00, 0xff, 0x00, 0x11, 0x00, 0xff, 0x00, 0x00, 0xee, 0xff, 0x00, 0x00, 0xdd, 0xff, 0x00, 0x00, 0xbb, 0xff, 0x00, 0x00, 0xaa, 0xff, 0x00, 0x00, 0x88, 0xff,
        0x00, 0x00, 0x77, 0xff, 0x00, 0x00, 0x55, 0xff, 0x00, 0x00, 0x44, 0xff, 0x00, 0x00, 0x22, 0xff, 0x00, 0x00, 0x11, 0xff, 0xee, 0xee, 0xee, 0xff, 0xdd, 0xdd, 0xdd, 0xff, 0xbb, 0xbb, 0xbb, 0xff,
        0xaa, 0xaa, 0xaa, 0xff, 0x88, 0x88, 0x88, 0xff, 0x77, 0x77, 0x77, 0xff, 0x55, 0x55, 0x55, 0xff, 0x44, 0x44, 0x44, 0xff, 0x22, 0x22, 0x22, 0xff, 0x11, 0x11, 0x11, 0xff, 0x00, 0x00, 0x00, 0xff,
    };

    // internal math/helper utilities
    static inline uint32_t _vox_max(uint32_t a, uint32_t b) {
        return (a > b) ? a : b;
    }
    static inline uint32_t _vox_min(uint32_t a, uint32_t b) {
        return (a < b) ? a : b;
    }

    // string utilities
    #ifdef _MSC_VER
        #define _vox_str_scanf(str,...)      sscanf_s(str,__VA_ARGS__)
        #define _vox_strcpy_static(dst,src)  strcpy_s(dst,src)
        #define _vox_strcasecmp(a,b)         _stricmp(a,b)
        #define _vox_strcmp(a,b)             strcmp(a,b)
        #define _vox_strlen(a)               strlen(a)
        #define _vox_sprintf(str,str_max,fmt,...)    sprintf_s(str, str_max, fmt, __VA_ARGS__)
    #else
        #define _vox_str_scanf(str,...)      sscanf(str,__VA_ARGS__)
        #define _vox_strcpy_static(dst,src)  strcpy(dst,src)
        #define _vox_strcasecmp(a,b)         strcasecmp(a,b)
        #define _vox_strcmp(a,b)             strcmp(a,b)
        #define _vox_strlen(a)               strlen(a)
        #define _vox_sprintf(str,str_max,fmt,...)    snprintf(str, str_max, fmt, __VA_ARGS__)
    #endif

    // 3d vector utilities
    struct vec3 {
        float x, y, z;
    };
    static inline vec3 vec3_make(float x, float y, float z) { vec3 v; v.x = x; v.y = y; v.z = z; return v; }
    static inline vec3 vec3_negate(const vec3& v) { vec3 r; r.x = -v.x;  r.y = -v.y; r.z = -v.z; return r; }

    // API for emulating file transactions on an in-memory buffer of data.
    struct _vox_file {
        const  uint8_t* buffer;       // source buffer data
        const uint32_t  buffer_size;  // size of the data in the buffer
        uint32_t        offset;       // current offset in the buffer data.
    };

    static uint32_t _vox_file_bytes_remaining(const _vox_file* fp) {
        if (fp->offset < fp->buffer_size) {
            return fp->buffer_size - fp->offset;
        } else {
            return 0;
        }
    }

    static bool _vox_file_read(_vox_file* fp, void* data, uint32_t data_size) {
        size_t data_to_read = _vox_min(_vox_file_bytes_remaining(fp), data_size);
        memcpy(data, &fp->buffer[fp->offset], data_to_read);
        fp->offset += data_size;
        return data_to_read == data_size;
    }

    static void _vox_file_seek_forwards(_vox_file* fp, uint32_t offset) {
        fp->offset += _vox_min(offset, _vox_file_bytes_remaining(fp));
    }

    static const void* _vox_file_data_pointer(const _vox_file* fp) {
        return &fp->buffer[fp->offset];
    }

    // hash utilities
    static uint32_t _vox_hash(const uint8_t* data, uint32_t data_size) {
        uint32_t hash = 0;
        for (uint32_t i = 0; i < data_size; i++)
            hash = data[i] + (hash * 65559);
        return hash;
    }

    // memory allocation utils.
    static void* _ogt_priv_alloc_default(size_t size) { return malloc(size); }
    static void  _ogt_priv_free_default(void* ptr)    { free(ptr); }
    static ogt_vox_alloc_func g_alloc_func = _ogt_priv_alloc_default; // default function for allocating
    static ogt_vox_free_func  g_free_func = _ogt_priv_free_default;   // default  function for freeing.

    // set the provided allocate/free functions if they are non-null, otherwise reset to default allocate/free functions
    void ogt_vox_set_memory_allocator(ogt_vox_alloc_func alloc_func, ogt_vox_free_func free_func)
    {
        assert((alloc_func && free_func) ||      // both alloc/free must be non-NULL -OR-
            (!alloc_func && !free_func));    // both alloc/free must be NULL. No mixing 'n matching.
        if (alloc_func && free_func) {
            g_alloc_func = alloc_func;
            g_free_func = free_func;
        }
        else  {
            // reset to default allocate/free functions.
            g_alloc_func = _ogt_priv_alloc_default;
            g_free_func = _ogt_priv_free_default;
        }
    }

    static void* _vox_malloc(size_t size) {
        return size ? g_alloc_func(size) : NULL;
    }

    static void* _vox_calloc(size_t size) {
        void* pMem = _vox_malloc(size);
        if (pMem)
            memset(pMem, 0, size);
        return pMem;
    }

    static void _vox_free(void* old_ptr) {
        if (old_ptr)
            g_free_func(old_ptr);
    }

    static void* _vox_realloc(void* old_ptr, size_t old_size, size_t new_size) {
        // early out if new size is non-zero and no resize is required.
        if (new_size && old_size >= new_size)
            return old_ptr;

        // memcpy from the old ptr only if both sides are valid.
        void* new_ptr = _vox_malloc(new_size);
        if (new_ptr) {
            // copy any existing elements over
            if (old_ptr && old_size)
                memcpy(new_ptr, old_ptr, old_size);
            // zero out any new tail elements
            assert(new_size > old_size); // this should be guaranteed by the _vox_realloc early out case above.
            uintptr_t new_tail_ptr = (uintptr_t)new_ptr + old_size;
            memset((void*)new_tail_ptr, 0, new_size - old_size);
        }
        if (old_ptr)
            _vox_free(old_ptr);
        return new_ptr;
    }

    // std::vector-style allocator, which use client-provided allocation functions.
    template <class T> struct _vox_array {
        _vox_array() : data(NULL), capacity(0), count(0) { }
        ~_vox_array() {
            _vox_free(data);
            data = NULL;
            count = 0;
            capacity = 0;
        }
        void reserve(size_t new_capacity) {
            data = (T*)_vox_realloc(data, capacity * sizeof(T), new_capacity * sizeof(T));
            capacity = new_capacity;
        }
        void grow_to_fit_index(size_t index) {
            if (index >= count)
                resize(index + 1);
        }
        void resize(size_t new_count) {
            if (new_count > capacity)
                reserve(new_count);
            count = new_count;
        }
        void push_back(const T & new_element) {
            if (count == capacity) {
                size_t new_capacity = capacity ? (capacity * 3) >> 1 : 2;   // grow by 50% each time, otherwise start at 2 elements.
                reserve(new_capacity);
                assert(capacity > count);
            }
            data[count++] = new_element;
        }
        void push_back_many(const T * new_elements, size_t num_elements) {
            if (count + num_elements > capacity) {
                size_t new_capacity = capacity + num_elements;
                new_capacity = new_capacity ? (new_capacity * 3) >> 1 : 2;   // grow by 50% each time, otherwise start at 2 elements.
                reserve(new_capacity);
                assert(capacity >= (count + num_elements));
            }
            for (size_t i = 0; i < num_elements; i++)
                data[count + i] = new_elements[i];
            count += num_elements;
        }
        size_t size() const {
            return count;
        }
        T& operator[](size_t index) {
            assert(index < count);
            return data[index];
        }
        const T& operator[](size_t index) const {
            assert(index < count);
            return data[index];
        }
        T*     data;      // data for the array
        size_t capacity;  // capacity of the array
        size_t count;      // size of the array
    };

    // matrix utilities
    static ogt_vox_transform _vox_transform_identity() {
        ogt_vox_transform t;
        t.m00 = 1.0f; t.m01 = 0.0f; t.m02 = 0.0f; t.m03 = 0.0f;
        t.m10 = 0.0f; t.m11 = 1.0f; t.m12 = 0.0f; t.m13 = 0.0f;
        t.m20 = 0.0f; t.m21 = 0.0f; t.m22 = 1.0f; t.m23 = 0.0f;
        t.m30 = 0.0f; t.m31 = 0.0f; t.m32 = 0.0f; t.m33 = 1.0f;
        return t;
    }

    static ogt_vox_transform _vox_transform_multiply(const ogt_vox_transform& a, const ogt_vox_transform& b) {
        ogt_vox_transform r;
        r.m00 = (a.m00 * b.m00) + (a.m01 * b.m10) + (a.m02 * b.m20) + (a.m03 * b.m30);
        r.m01 = (a.m00 * b.m01) + (a.m01 * b.m11) + (a.m02 * b.m21) + (a.m03 * b.m31);
        r.m02 = (a.m00 * b.m02) + (a.m01 * b.m12) + (a.m02 * b.m22) + (a.m03 * b.m32);
        r.m03 = (a.m00 * b.m03) + (a.m01 * b.m13) + (a.m02 * b.m23) + (a.m03 * b.m33);
        r.m10 = (a.m10 * b.m00) + (a.m11 * b.m10) + (a.m12 * b.m20) + (a.m13 * b.m30);
        r.m11 = (a.m10 * b.m01) + (a.m11 * b.m11) + (a.m12 * b.m21) + (a.m13 * b.m31);
        r.m12 = (a.m10 * b.m02) + (a.m11 * b.m12) + (a.m12 * b.m22) + (a.m13 * b.m32);
        r.m13 = (a.m10 * b.m03) + (a.m11 * b.m13) + (a.m12 * b.m23) + (a.m13 * b.m33);
        r.m20 = (a.m20 * b.m00) + (a.m21 * b.m10) + (a.m22 * b.m20) + (a.m23 * b.m30);
        r.m21 = (a.m20 * b.m01) + (a.m21 * b.m11) + (a.m22 * b.m21) + (a.m23 * b.m31);
        r.m22 = (a.m20 * b.m02) + (a.m21 * b.m12) + (a.m22 * b.m22) + (a.m23 * b.m32);
        r.m23 = (a.m20 * b.m03) + (a.m21 * b.m13) + (a.m22 * b.m23) + (a.m23 * b.m33);
        r.m30 = (a.m30 * b.m00) + (a.m31 * b.m10) + (a.m32 * b.m20) + (a.m33 * b.m30);
        r.m31 = (a.m30 * b.m01) + (a.m31 * b.m11) + (a.m32 * b.m21) + (a.m33 * b.m31);
        r.m32 = (a.m30 * b.m02) + (a.m31 * b.m12) + (a.m32 * b.m22) + (a.m33 * b.m32);
        r.m33 = (a.m30 * b.m03) + (a.m31 * b.m13) + (a.m32 * b.m23) + (a.m33 * b.m33);
        return r;
    }

    // dictionary utilities
    static const uint32_t k_vox_max_dict_buffer_size = 4096;
    static const uint32_t k_vox_max_dict_key_value_pairs = 256;
    struct _vox_dictionary {
        const char* keys[k_vox_max_dict_key_value_pairs];
        const char* values[k_vox_max_dict_key_value_pairs];
        uint32_t    num_key_value_pairs;
        char        buffer[k_vox_max_dict_buffer_size + 4];    // max 4096, +4 for safety
        uint32_t    buffer_mem_used;
    };

    static bool _vox_file_read_dict(_vox_dictionary * dict, _vox_file * fp) {
        uint32_t num_pairs_to_read = 0;
        _vox_file_read(fp, &num_pairs_to_read, sizeof(uint32_t));
        assert(num_pairs_to_read <= k_vox_max_dict_key_value_pairs);

        dict->buffer_mem_used = 0;
        dict->num_key_value_pairs = 0;
        for (uint32_t i = 0; i < num_pairs_to_read; i++) {
            // get the size of the key string
            uint32_t key_string_size = 0;
            if (!_vox_file_read(fp, &key_string_size, sizeof(uint32_t)))
                return false;
            // allocate space for the key, and read it in.
            if (dict->buffer_mem_used + key_string_size > k_vox_max_dict_buffer_size)
                return false;
            char* key = &dict->buffer[dict->buffer_mem_used];
            dict->buffer_mem_used += key_string_size + 1;    // + 1 for zero terminator
            if (!_vox_file_read(fp, key, key_string_size))
                return false;
            key[key_string_size] = 0;    // zero-terminate
            assert(_vox_strlen(key) == key_string_size);    // sanity check

            // get the size of the value string
            uint32_t value_string_size = 0;
            if (!_vox_file_read(fp, &value_string_size, sizeof(uint32_t)))
                return false;
            // allocate space for the value, and read it in.
            if (dict->buffer_mem_used + value_string_size > k_vox_max_dict_buffer_size)
                return false;
            char* value = &dict->buffer[dict->buffer_mem_used];
            dict->buffer_mem_used += value_string_size + 1;    // + 1 for zero terminator
            if (!_vox_file_read(fp, value, value_string_size))
                return false;
            value[value_string_size] = 0;    // zero-terminate
            assert(_vox_strlen(value) == value_string_size);    // sanity check
            // now assign it in the dictionary
            dict->keys[dict->num_key_value_pairs] = key;
            dict->values[dict->num_key_value_pairs] = value;
            dict->num_key_value_pairs++;
        }

        return true;
    }

    // helper for looking up in the dictionary
    static const char* _vox_dict_get_value_as_string(const _vox_dictionary* dict, const char* key_to_find, const char* default_value = NULL) {
        for (uint32_t i = 0; i < dict->num_key_value_pairs; i++)
            if (_vox_strcasecmp(dict->keys[i], key_to_find) == 0)
                return dict->values[i];
        return default_value;
    }

    // lookup table for _vox_make_transform_from_dict_strings
    static const vec3 k_vectors[4] = {
	vec3_make(1.0f, 0.0f, 0.0f),
	vec3_make(0.0f, 1.0f, 0.0f),
	vec3_make(0.0f, 0.0f, 1.0f),
	vec3_make(0.0f, 0.0f, 0.0f)    // invalid!
    };

    // lookup table for _vox_make_transform_from_dict_strings
    static const uint32_t k_row2_index[] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, 2, UINT32_MAX, 1, 0, UINT32_MAX };


    static ogt_vox_transform _vox_make_transform_from_dict_strings(const char* rotation_string, const char* translation_string) {
        ogt_vox_transform transform = _vox_transform_identity();

        if (rotation_string != NULL) {
            // compute the per-row indexes into k_vectors[] array.
            // unpack rotation bits.
            //  bits  : meaning
            //  0 - 1 : index of the non-zero entry in the first row
            //  2 - 3 : index of the non-zero entry in the second row
            uint32_t packed_rotation_bits = atoi(rotation_string);
            uint32_t row0_vec_index = (packed_rotation_bits >> 0) & 3;
            uint32_t row1_vec_index = (packed_rotation_bits >> 2) & 3;
            uint32_t row2_vec_index = k_row2_index[(1 << row0_vec_index) | (1 << row1_vec_index)];    // process of elimination to determine row 2 index based on row0/row1 being one of {0,1,2} choose 2.
            assert(row2_vec_index != UINT32_MAX); // if you hit this, you probably have invalid indices for row0_vec_index/row1_vec_index.

            // unpack rotation bits for vector signs
            //  bits  : meaning
            //  4     : the sign in the first row  (0 : positive; 1 : negative)
            //  5     : the sign in the second row (0 : positive; 1 : negative)
            //  6     : the sign in the third row  (0 : positive; 1 : negative)
            vec3 row0 = k_vectors[row0_vec_index];
            vec3 row1 = k_vectors[row1_vec_index];
            vec3 row2 = k_vectors[row2_vec_index];
            if (packed_rotation_bits & (1 << 4))
                row0 = vec3_negate(row0);
            if (packed_rotation_bits & (1 << 5))
                row1 = vec3_negate(row1);
            if (packed_rotation_bits & (1 << 6))
                row2 = vec3_negate(row2);

            // magicavoxel stores rows, we need columns, so we do the swizzle here into columns
            transform.m00 = row0.x; transform.m01 = row1.x; transform.m02 = row2.x;
            transform.m10 = row0.y; transform.m11 = row1.y; transform.m12 = row2.y;
            transform.m20 = row0.z; transform.m21 = row1.z; transform.m22 = row2.z;
        }

        if (translation_string != NULL) {
            int32_t x = 0;
            int32_t y = 0;
            int32_t z = 0;
            _vox_str_scanf(translation_string, "%i %i %i", &x, &y, &z);
            transform.m30 = (float)x;
            transform.m31 = (float)y;
            transform.m32 = (float)z;
        }
        return transform;
    }

    enum _vox_scene_node_type
    {
        k_nodetype_invalid   = 0,    // has not been parsed yet.
        k_nodetype_group     = 1,
        k_nodetype_transform = 2,
        k_nodetype_shape     = 3,
    };

    struct _vox_scene_node_ {
        _vox_scene_node_type node_type;    // only gets assigned when this has been parsed, otherwise will be k_nodetype_invalid
        union {
            // used only when node_type == k_nodetype_transform
            struct {
                char              name[65];    // max name size is 64 plus 1 for null terminator
                ogt_vox_transform transform;
                uint32_t          child_node_id;
                uint32_t          layer_id;
                bool              hidden;
            } transform;
            // used only when node_type == k_nodetype_group
            struct {
                uint32_t first_child_node_id_index; // the index of the first child node ID within the ChildNodeID array
                uint32_t num_child_nodes;           // number of child node IDs starting at the first index
            } group;
            // used only when node_type == k_nodetype_shape
            struct {
                uint32_t model_id;                  // will be UINT32_MAX if there is no model. Unlikely, there should always be a model.
            } shape;
        } u;
    };

    static void generate_instances_for_node(
        const _vox_array<_vox_scene_node_> & nodes, uint32_t node_index, const _vox_array<uint32_t> & child_id_array, uint32_t layer_index,
        const ogt_vox_transform& transform, const _vox_array<ogt_vox_model*> & model_ptrs, const char* transform_last_name, bool transform_last_hidden,
        _vox_array<ogt_vox_instance> & instances, _vox_array<char> & string_data, _vox_array<ogt_vox_group>& groups, uint32_t group_index, bool generate_groups)
    {
        const _vox_scene_node_* node = &nodes[node_index];
        assert(node);
        switch (node->node_type)
        {
            case k_nodetype_transform:
            {
                ogt_vox_transform new_transform = (generate_groups) ? node->u.transform.transform  // don't multiply by the parent transform. caller wants the group-relative transform
                        : _vox_transform_multiply(node->u.transform.transform, transform);         // flatten the transform if we're not generating groups: child transform * parent transform
                const char* new_transform_name = node->u.transform.name[0] ? node->u.transform.name : NULL;
                transform_last_name = new_transform_name ? new_transform_name : transform_last_name;    // if this node has a name, use it instead of our parent name
                generate_instances_for_node(nodes, node->u.transform.child_node_id, child_id_array, node->u.transform.layer_id, new_transform, model_ptrs, transform_last_name, node->u.transform.hidden, instances, string_data, groups, group_index, generate_groups);
                break;
            }
            case k_nodetype_group:
            {
                // create a new group only if we're generating groups.
                uint32_t next_group_index = 0;
                if (generate_groups) {
                    next_group_index = (uint32_t)groups.size();
                    ogt_vox_group group;
                    group.parent_group_index = group_index;
                    group.transform          = transform;
                    group.hidden             = transform_last_hidden;
                    group.layer_index        = layer_index;
                    groups.push_back(group);
                }
                // child nodes will only be hidden if their immediate transform is hidden.
                transform_last_hidden = false;

                const uint32_t* child_node_ids = (const uint32_t*)& child_id_array[node->u.group.first_child_node_id_index];
                for (uint32_t i = 0; i < node->u.group.num_child_nodes; i++) {
                    generate_instances_for_node(nodes, child_node_ids[i], child_id_array, layer_index, transform, model_ptrs, transform_last_name, transform_last_hidden, instances, string_data, groups, next_group_index, generate_groups);
                }
                break;
            }
            case k_nodetype_shape:
            {
                assert(node->u.shape.model_id < model_ptrs.size());
                if (node->u.shape.model_id < model_ptrs.size() &&    // model ID is valid
                    model_ptrs[node->u.shape.model_id] != NULL )     // model is non-NULL.
                {
                    assert(generate_groups || group_index == 0);     // if we're not generating groups, group_index should be zero to map to the root group.
                    ogt_vox_instance new_instance;
                    new_instance.model_index = node->u.shape.model_id;
                    new_instance.transform   = transform;
                    new_instance.layer_index = layer_index;
                    new_instance.group_index = group_index;
                    new_instance.hidden      = transform_last_hidden;
                    // if we got a transform name, allocate space in string_data for it and keep track of the index
                    // within string data. This will be patched to a real pointer at the very end.
                    new_instance.name = 0;
                    if (transform_last_name && transform_last_name[0]) {
                        new_instance.name = (const char*)(string_data.size());
                        size_t name_size = _vox_strlen(transform_last_name) + 1;       // +1 for terminator
                        string_data.push_back_many(transform_last_name, name_size);
                    }
                    // create the instance
                    instances.push_back(new_instance);
                }
                break;
            }
            default:
            {
                assert(0); // unhandled node type!
            }
        }
    }

    // returns true if the 2 models are content-wise identical.
    static bool _vox_models_are_equal(const ogt_vox_model* lhs, const ogt_vox_model* rhs) {
        // early out: if hashes don't match, they can't be equal
        // if hashes match, they might be equal OR there might be a hash collision.
        if (lhs->voxel_hash != rhs->voxel_hash)
            return false;
        // early out: if number of voxels in the model's grid don't match, they can't be equal.
        uint32_t num_voxels_lhs = lhs->size_x * lhs->size_y * lhs->size_z;
        uint32_t num_voxels_rhs = rhs->size_x * rhs->size_y * rhs->size_z;
        if (num_voxels_lhs != num_voxels_rhs)
            return false;
        // Finally, we know their hashes are the same, and their dimensions are the same
        // but they are only equal if they have exactly the same voxel data.
        return memcmp(lhs->voxel_data, rhs->voxel_data, num_voxels_lhs) == 0 ? true : false;
    }

    const ogt_vox_scene* ogt_vox_read_scene_with_flags(const uint8_t * buffer, uint32_t buffer_size, uint32_t read_flags) {
        _vox_file file = { buffer, buffer_size, 0 };
        _vox_file* fp = &file;

        // parsing state/context
        _vox_array<ogt_vox_model*>   model_ptrs;
        _vox_array<_vox_scene_node_> nodes;
        _vox_array<ogt_vox_instance> instances;
        _vox_array<char>             string_data;
        _vox_array<ogt_vox_layer>    layers;
        _vox_array<ogt_vox_group>    groups;
        _vox_array<uint32_t>         child_ids;
        ogt_vox_palette              palette;
        ogt_vox_matl_array           materials;
        _vox_dictionary              dict;
        uint32_t                     size_x = 0;
        uint32_t                     size_y = 0;
        uint32_t                     size_z = 0;
        uint8_t                      index_map[256];
        bool                         found_index_map_chunk = false;

        // size some of our arrays to prevent resizing during the parsing for smallish cases.
        model_ptrs.reserve(64);
        instances.reserve(256);
        child_ids.reserve(256);
        nodes.reserve(16);
        layers.reserve(8);
        groups.reserve(0);
        string_data.reserve(256);

        // push a sentinel character into these datastructures. This allows us to keep indexes
        // rather than pointers into data-structures that grow, and still allow an index of 0
        // to means invalid
        string_data.push_back('X');
        child_ids.push_back(UINT32_MAX);

        // copy the default palette into the scene. It may get overwritten by a palette chunk later
        memcpy(&palette, k_default_vox_palette, sizeof(ogt_vox_palette));

        // zero initialize materials (this sets valid defaults)
        memset(&materials, 0, sizeof(materials));

        // load and validate fileheader and file version.
        uint32_t file_header = 0;
        uint32_t file_version = 0;
        _vox_file_read(fp, &file_header, sizeof(uint32_t));
        _vox_file_read(fp, &file_version, sizeof(uint32_t));
        if (file_header != CHUNK_ID_VOX_ || file_version != 150)
            return NULL;

        // parse chunks until we reach the end of the file/buffer
        while (_vox_file_bytes_remaining(fp) >= sizeof(uint32_t) * 3)
        {
            // read the fields common to all chunks
            uint32_t chunk_id         = 0;
            uint32_t chunk_size       = 0;
            uint32_t chunk_child_size = 0;
            _vox_file_read(fp, &chunk_id, sizeof(uint32_t));
            _vox_file_read(fp, &chunk_size, sizeof(uint32_t));
            _vox_file_read(fp, &chunk_child_size, sizeof(uint32_t));

            // process the chunk.
            switch (chunk_id)
            {
                case CHUNK_ID_MAIN:
                {
                    break;
                }
                case CHUNK_ID_SIZE:
                {
                    assert(chunk_size == 12 && chunk_child_size == 0);
                    _vox_file_read(fp, &size_x, sizeof(uint32_t));
                    _vox_file_read(fp, &size_y, sizeof(uint32_t));
                    _vox_file_read(fp, &size_z, sizeof(uint32_t));
                    break;
                }
                case CHUNK_ID_XYZI:
                {
                    assert(size_x && size_y && size_z);    // must have read a SIZE chunk prior to XYZI.
                    // read the number of voxels to process for this moodel
                    uint32_t num_voxels_in_chunk = 0;
                    _vox_file_read(fp, &num_voxels_in_chunk, sizeof(uint32_t));
                    if (num_voxels_in_chunk != 0) {
                        uint32_t voxel_count = size_x * size_y * size_z;
                        ogt_vox_model * model = (ogt_vox_model*)_vox_calloc(sizeof(ogt_vox_model) + voxel_count);        // 1 byte for each voxel
                        if (!model)
                            return NULL;
                        uint8_t * voxel_data = (uint8_t*)&model[1];

                        // insert the model into the model array
                        model_ptrs.push_back(model);

                        // now setup the model
                        model->size_x = size_x;
                        model->size_y = size_y;
                        model->size_z = size_z;
                        model->voxel_data = voxel_data;

                        // setup some strides for computing voxel index based on x/y/z
                        const uint32_t k_stride_x = 1;
                        const uint32_t k_stride_y = size_x;
                        const uint32_t k_stride_z = size_x * size_y;

                        // read this many voxels and store it in voxel data.
                        const uint8_t * packed_voxel_data = (const uint8_t*)_vox_file_data_pointer(fp);
                        const uint32_t voxels_to_read = _vox_min(_vox_file_bytes_remaining(fp) / 4, num_voxels_in_chunk);
                        for (uint32_t i = 0; i < voxels_to_read; i++) {
                            uint8_t x = packed_voxel_data[i * 4 + 0];
                            uint8_t y = packed_voxel_data[i * 4 + 1];
                            uint8_t z = packed_voxel_data[i * 4 + 2];
                            uint8_t color_index = packed_voxel_data[i * 4 + 3];
                            assert(x < size_x && y < size_y && z < size_z);
                            voxel_data[(x * k_stride_x) + (y * k_stride_y) + (z * k_stride_z)] = color_index;
                        }
                        _vox_file_seek_forwards(fp, num_voxels_in_chunk * 4);
                        // compute the hash of the voxels in this model-- used to accelerate duplicate models checking.
                        model->voxel_hash = _vox_hash(voxel_data, size_x * size_y * size_z);
                    }
                    else {
                        model_ptrs.push_back(NULL);
                    }
                    break;
                }
                case CHUNK_ID_RGBA:
                {
                    assert(chunk_size == sizeof(palette));
                    _vox_file_read(fp, &palette, sizeof(palette));
                    break;
                }
                case CHUNK_ID_nTRN:
                {
                    uint32_t node_id = 0;
                    _vox_file_read(fp, &node_id, sizeof(node_id));

                    // Parse the node dictionary, which can contain:
                    //   _name:   string
                    //   _hidden: 0/1
                    char node_name[65];
                    bool hidden = false;
                    node_name[0] = 0;
                    {
                        _vox_file_read_dict(&dict, fp);
                        const char* name_string = _vox_dict_get_value_as_string(&dict, "_name");
                        if (name_string)
                            _vox_strcpy_static(node_name, name_string);
                        // if we got a hidden attribute - assign it now.
                        const char* hidden_string = _vox_dict_get_value_as_string(&dict, "_hidden", "0");
                        if (hidden_string)
                            hidden = (hidden_string[0] == '1' ? true : false);
                    }


                    // get other properties.
                    uint32_t child_node_id = 0, reserved_id = 0, layer_id = 0, num_frames = 0;
                    _vox_file_read(fp, &child_node_id, sizeof(child_node_id));
                    _vox_file_read(fp, &reserved_id,   sizeof(reserved_id));
                    _vox_file_read(fp, &layer_id,      sizeof(layer_id));
                    _vox_file_read(fp, &num_frames,    sizeof(num_frames));
                    assert(reserved_id == UINT32_MAX && num_frames == 1); // must be these values according to the spec

                    // Parse the frame dictionary that contains:
                    //   _r : int8 ROTATION (c)
                    //   _t : int32x3 translation
                    // and extract a transform
                    ogt_vox_transform frame_transform;
                    {
                        _vox_file_read_dict(&dict, fp);
                        const char* rotation_value    = _vox_dict_get_value_as_string(&dict, "_r");
                        const char* translation_value = _vox_dict_get_value_as_string(&dict, "_t");
                        frame_transform = _vox_make_transform_from_dict_strings(rotation_value, translation_value);
                    }
                    // setup the transform node.
                    {
                        nodes.grow_to_fit_index(node_id);
                        _vox_scene_node_* transform_node = &nodes[node_id];
                        assert(transform_node);
                        transform_node->node_type = k_nodetype_transform;
                        transform_node->u.transform.child_node_id = child_node_id;
                        transform_node->u.transform.layer_id      = layer_id;
                        transform_node->u.transform.transform     = frame_transform;
                        transform_node->u.transform.hidden        = hidden;
                        // assign the name
                        _vox_strcpy_static(transform_node->u.transform.name, node_name);
                    }
                    break;
                }
                case CHUNK_ID_nGRP:
                {
                    uint32_t node_id = 0;
                    _vox_file_read(fp, &node_id, sizeof(node_id));

                    // parse the node dictionary - data is unused.
                    _vox_file_read_dict(&dict, fp);

                    // setup the group node
                    nodes.grow_to_fit_index(node_id);
                    _vox_scene_node_* group_node = &nodes[node_id];
                    group_node->node_type = k_nodetype_group;
                    group_node->u.group.first_child_node_id_index = 0;
                    group_node->u.group.num_child_nodes           = 0;

                    // setup all child scene nodes to point back to this node.
                    uint32_t num_child_nodes = 0;
                    _vox_file_read(fp, &num_child_nodes, sizeof(num_child_nodes));

                    // allocate space for all the child node IDs
                    if (num_child_nodes) {
                        size_t prior_size = child_ids.size();
                        assert(prior_size > 0); // should be guaranteed by the sentinel we reserved at the very beginning.
                        child_ids.resize(prior_size + num_child_nodes);
                        _vox_file_read(fp, &child_ids[prior_size], sizeof(uint32_t) * num_child_nodes);
                        group_node->u.group.first_child_node_id_index = (uint32_t)prior_size;
                        group_node->u.group.num_child_nodes = num_child_nodes;
                    }
                    break;
                }
                case CHUNK_ID_nSHP:
                {
                    uint32_t node_id = 0;
                    _vox_file_read(fp, &node_id, sizeof(node_id));

                    // setup the shape node
                    nodes.grow_to_fit_index(node_id);
                    _vox_scene_node_* shape_node = &nodes[node_id];
                    shape_node->node_type = k_nodetype_shape;
                    shape_node->u.shape.model_id = UINT32_MAX;

                    // parse the node dictionary - data is unused.
                    _vox_file_read_dict(&dict, fp);

                    uint32_t num_models = 0;
                    _vox_file_read(fp, &num_models, sizeof(num_models));
                    assert(num_models == 1); // must be 1 according to the spec.

                    // assign instances
                    _vox_file_read(fp, &shape_node->u.shape.model_id, sizeof(uint32_t));
                    assert(shape_node->u.shape.model_id < model_ptrs.size());

                    // parse the model dictionary - data is unsued.
                    _vox_file_read_dict(&dict, fp);
                    break;
                }
                case CHUNK_ID_IMAP:
                {
                    assert(chunk_size == 256);
                    _vox_file_read(fp, index_map, 256);
                    found_index_map_chunk = true;
                    break;
                }
                case CHUNK_ID_LAYR:
                {
                    int32_t layer_id = 0;
                    int32_t reserved_id = 0;
                    _vox_file_read(fp, &layer_id, sizeof(layer_id));
                    _vox_file_read_dict(&dict, fp);
                    _vox_file_read(fp, &reserved_id, sizeof(reserved_id));
                    assert(reserved_id == -1);

                    layers.grow_to_fit_index(layer_id);
                    layers[layer_id].name = NULL;
                    layers[layer_id].hidden = false;

                    // if we got a layer name from the LAYR dictionary, allocate space in string_data for it and keep track of the index
                    // within string data. This will be patched to a real pointer at the very end.
                    const char* name_string = _vox_dict_get_value_as_string(&dict, "_name", NULL);
                    if (name_string) {
                        layers[layer_id].name = (const char*)(string_data.size());
                        size_t name_size = _vox_strlen(name_string) + 1;       // +1 for terminator
                        string_data.push_back_many(name_string, name_size);
                    }
                    // if we got a hidden attribute - assign it now.
                    const char* hidden_string = _vox_dict_get_value_as_string(&dict, "_hidden", "0");
                    if (hidden_string)
                        layers[layer_id].hidden = (hidden_string[0] == '1' ? true : false);
                    break;
                }
                case CHUNK_ID_MATL:
                {
                    int32_t material_id = 0;
                    _vox_file_read(fp, &material_id, sizeof(material_id));
                    material_id = material_id & 0xFF; // incoming material 256 is material 0
                    _vox_file_read_dict(&dict, fp);
                    const char* type_string = _vox_dict_get_value_as_string(&dict, "_type", NULL);
                    if (type_string) {
                        if (0 == _vox_strcmp(type_string,"_diffuse")) {
                            materials.matl[material_id].type = ogt_matl_type_diffuse;
                        }
                        else if (0 == _vox_strcmp(type_string,"_metal")) {
                            materials.matl[material_id].type = ogt_matl_type_metal;
                        }
                        else if (0 == _vox_strcmp(type_string,"_glass")) {
                            materials.matl[material_id].type = ogt_matl_type_glass;
                        }
                        else if (0 == _vox_strcmp(type_string,"_emit")) {
                            materials.matl[material_id].type = ogt_matl_type_emit;
                        }
                        else if (0 == _vox_strcmp(type_string,"_blend")) {
                            materials.matl[material_id].type = ogt_matl_type_blend;
                        }
                        else if (0 == _vox_strcmp(type_string,"_media")) {
                            materials.matl[material_id].type = ogt_matl_type_media;
                        }
                    }
                    const char* metal_string = _vox_dict_get_value_as_string(&dict, "_metal", NULL);
                    if (metal_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_metal;
                        materials.matl[material_id].metal = (float)atof(metal_string);
                    }
                    const char* rough_string = _vox_dict_get_value_as_string(&dict, "_rough", NULL);
                    if (rough_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_rough;
                        materials.matl[material_id].rough = (float)atof(rough_string);
                    }
                    const char* spec_string = _vox_dict_get_value_as_string(&dict, "_spec", NULL);
                    if (spec_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_spec;
                        materials.matl[material_id].spec = (float)atof(spec_string);
                    }
                    const char* ior_string = _vox_dict_get_value_as_string(&dict, "_ior", NULL);
                    if (ior_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_ior;
                        materials.matl[material_id].ior = (float)atof(ior_string);
                    }
                    const char* att_string = _vox_dict_get_value_as_string(&dict, "_att", NULL);
                    if (att_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_att;
                        materials.matl[material_id].att = (float)atof(att_string);
                    }
                    const char* flux_string = _vox_dict_get_value_as_string(&dict, "_flux", NULL);
                    if (flux_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_flux;
                        materials.matl[material_id].flux = (float)atof(flux_string);
                    }
                    const char* emit_string = _vox_dict_get_value_as_string(&dict, "_emit", NULL);
                    if (emit_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_emit;
                        materials.matl[material_id].emit = (float)atof(emit_string);
                    }
                    const char* ldr_string = _vox_dict_get_value_as_string(&dict, "_ldr", NULL);
                    if (ldr_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_ldr;
                        materials.matl[material_id].ldr = (float)atof(ldr_string);
                    }
                    const char* trans_string = _vox_dict_get_value_as_string(&dict, "_trans", NULL);
                    if (trans_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_trans;
                        materials.matl[material_id].trans = (float)atof(trans_string);
                    }
                    const char* alpha_string = _vox_dict_get_value_as_string(&dict, "_alpha", NULL);
                    if (alpha_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_alpha;
                        materials.matl[material_id].alpha = (float)atof(alpha_string);
                    }
                    const char* d_string = _vox_dict_get_value_as_string(&dict, "_d", NULL);
                    if (d_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_d;
                        materials.matl[material_id].d = (float)atof(d_string);
                    }
                    const char* sp_string = _vox_dict_get_value_as_string(&dict, "_sp", NULL);
                    if (sp_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_sp;
                        materials.matl[material_id].sp = (float)atof(sp_string);
                    }
                    const char* g_string = _vox_dict_get_value_as_string(&dict, "_g", NULL);
                    if (g_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_g;
                        materials.matl[material_id].g = (float)atof(g_string);
                    }
                    const char* media_string = _vox_dict_get_value_as_string(&dict, "_media", NULL);
                    if (media_string) {
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_media;
                        materials.matl[material_id].media = (float)atof(media_string);
                    }
                    break;
                }
                case CHUNK_ID_MATT:
                {
                    int32_t material_id = 0;
                    _vox_file_read(fp, &material_id, sizeof(material_id));
                    material_id = material_id & 0xFF; // incoming material 256 is material 0

                    // 0 : diffuse
                    // 1 : metal
                    // 2 : glass
                    // 3 : emissive
                    int32_t material_type = 0;
                    _vox_file_read(fp, &material_type, sizeof(material_type));

                    // diffuse  : 1.0
                    // metal    : (0.0 - 1.0] : blend between metal and diffuse material
                    // glass    : (0.0 - 1.0] : blend between glass and diffuse material
                    // emissive : (0.0 - 1.0] : self-illuminated material
                    float material_weight = 0.0f;
                    _vox_file_read(fp, &material_weight, sizeof(material_weight));

                    // bit(0) : Plastic
                    // bit(1) : Roughness
                    // bit(2) : Specular
                    // bit(3) : IOR
                    // bit(4) : Attenuation
                    // bit(5) : Power
                    // bit(6) : Glow
                    // bit(7) : isTotalPower (*no value)
                    uint32_t property_bits = 0u;
                    _vox_file_read(fp, &property_bits, sizeof(property_bits));

                    materials.matl[material_id].type = (ogt_matl_type)material_type;
                    switch (material_type) {
                    case ogt_matl_type_diffuse:
                        break;
                    case ogt_matl_type_metal:
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_metal;
                        materials.matl[material_id].metal = material_weight;
                        break;
                    case ogt_matl_type_glass:
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_trans;
                        materials.matl[material_id].trans = material_weight;
                        break;
                    case ogt_matl_type_emit:
                        materials.matl[material_id].content_flags |= k_ogt_vox_matl_have_emit;
                        materials.matl[material_id].emit = material_weight;
                        break;
                    }

                    assert(chunk_size >= 16u);
                    const uint32_t remaining = chunk_size - 16u;
                    _vox_file_seek_forwards(fp, remaining);
                    break;
                }
                // we don't handle rOBJ (just a dict of render settings), so we just skip the chunk payload.
                case CHUNK_ID_rOBJ:
                default:
                {
                    _vox_file_seek_forwards(fp, chunk_size);
                    break;
                }
            } // end switch
        }

        // ok, now that we've parsed all scene nodes - walk the scene hierarchy, and generate instances
        // we can't do this while parsing chunks unfortunately because some chunks reference chunks
        // that are later in the file than them.
        if (nodes.size()) {
            bool generate_groups = read_flags & k_read_scene_flags_groups ? true : false;
            // if we're not reading scene-embedded groups, we generate only one and then flatten all instance transforms.
            if (!generate_groups) {
                ogt_vox_group root_group;
                root_group.transform          = _vox_transform_identity();
                root_group.parent_group_index = k_invalid_group_index;
                root_group.layer_index        = 0;
                root_group.hidden             = false;
                groups.push_back(root_group);
            }
            generate_instances_for_node(nodes, 0, child_ids, 0, _vox_transform_identity(), model_ptrs, NULL, false, instances, string_data, groups, k_invalid_group_index, generate_groups);
        }
        else if (model_ptrs.size() == 1) {
            // add a single instance
            ogt_vox_instance new_instance;
            new_instance.model_index = 0;
            new_instance.group_index = 0;
            new_instance.transform   = _vox_transform_identity();
            new_instance.layer_index = 0;
            new_instance.name        = 0;
            new_instance.hidden      = false;
            instances.push_back(new_instance);
        }

        // if we didn't get a layer chunk -- just create a default layer.
        if (layers.size() == 0) {
            // go through all instances and ensure they are only mapped to layer 0
            for (uint32_t i = 0; i < instances.size(); i++)
                instances[i].layer_index = 0;
            // add a single layer
            ogt_vox_layer new_layer;
            new_layer.hidden = false;
            new_layer.name   = NULL;
            layers.push_back(new_layer);
        }

        // To support index-level assumptions (eg. artists using top 16 colors for color/palette cycling,
        // other ranges for emissive etc), we must ensure the order of colors that the artist sees in the
        // magicavoxel tool matches the actual index we'll end up using here. Unfortunately, magicavoxel
        // does an unexpected thing when remapping colors in the editor using ctrl+drag within the palette.
        // Instead of remapping all indices in all models, it just keeps track of a display index to actual
        // palette map and uses that to show reordered colors in the palette window. This is how that
        // map works:
        //   displaycolor[k] = paletteColor[imap[k]]
        // To ensure our indices are in the same order as displayed by magicavoxel within the palette
        // window, we apply the mapping from the IMAP chunk both to the color palette and indices within each
        // voxel model.
        if (found_index_map_chunk)
        {
            // the imap chunk maps from display index to actual index.
            // generate an inverse index map (maps from actual index to display index)
            uint8_t index_map_inverse[256];
            for (uint32_t i = 0; i < 256; i++) {
                index_map_inverse[index_map[i]] = (uint8_t)i;
            }

            // reorder colors in the palette so the palette contains colors in display order
            ogt_vox_palette old_palette = palette;
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t remapped_index = (index_map[i] + 255) & 0xFF;
                palette.color[i] = old_palette.color[remapped_index];
            }

            // reorder materials
            ogt_vox_matl_array old_materials = materials;
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t remapped_i = (i + 255) & 0xFF;
                uint32_t remapped_index = index_map[remapped_i];
                materials.matl[i] = old_materials.matl[remapped_index];
            }


            // ensure that all models are remapped so they are using display order palette indices.
            for (uint32_t i = 0; i < model_ptrs.size(); i++) {
                ogt_vox_model* model = model_ptrs[i];
                if (model) {
                    uint32_t num_voxels = model->size_x * model->size_y * model->size_z;
                    uint8_t* voxels = (uint8_t*)&model[1];
                    for (uint32_t j = 0; j < num_voxels; j++)
                        voxels[j] = 1 + index_map_inverse[voxels[j]];
                }
            }
        }

        // rotate the scene palette now so voxel indices can just map straight into the palette
        {
            ogt_vox_rgba last_color = palette.color[255];
            for (uint32_t i = 255; i > 0; i--)
                palette.color[i] = palette.color[i - 1];
            palette.color[0] = last_color;
            palette.color[0].a = 0;  // alpha is zero for the 0th color as that color index represents a transparent voxel.
        }

        // check for models that are identical by doing a pair-wise compare. If we find identical
        // models, we'll end up with NULL gaps in the model_ptrs array, but instances will have
        // been remapped to keep the earlier model.
        for (uint32_t i = 0; i < model_ptrs.size(); i++) {
            if (!model_ptrs[i])
                continue;
            for (uint32_t j = i+1; j < model_ptrs.size(); j++) {
                if (!model_ptrs[j] || !_vox_models_are_equal(model_ptrs[i], model_ptrs[j]))
                    continue;
                // model i and model j are the same, so free model j and keep model i.
                _vox_free(model_ptrs[j]);
                model_ptrs[j] = NULL;
                // remap all instances that were referring to j to now refer to i.
                for (uint32_t k = 0; k < instances.size(); k++)
                    if (instances[k].model_index == j)
                        instances[k].model_index = i;
            }
        }

        // sometimes a model can be created which has no solid voxels within just due to the
        // authoring flow within magicavoxel. We have already have prevented creation of
        // instances that refer to empty models, but here we want to compact the model_ptrs
        // array such that it contains no more NULL models. This also requires we remap the
        // indices for instances so they continue to refer to their correct models.
        {
            // first, check to see if we find any empty model. No need to do work otherwise.
            bool found_empty_model = false;
            for (uint32_t i = 0; i < model_ptrs.size() && !found_empty_model; i++) {
                if (model_ptrs[i] == NULL)
                    found_empty_model = true;
            }
            if (found_empty_model) {
                // build a remap table for all instances and simultaneously compact the model_ptrs array.
                uint32_t* model_remap = (uint32_t*)_vox_malloc(model_ptrs.size() * sizeof(uint32_t));
                uint32_t num_output_models = 0;
                for (uint32_t i = 0; i < model_ptrs.size(); i++) {
                    if (model_ptrs[i] != NULL) {
                        model_ptrs[num_output_models] = model_ptrs[i];
                        model_remap[i] = num_output_models;
                        num_output_models++;
                    }
                    else {
                        model_remap[i] = UINT32_MAX;
                    }
                }
                model_ptrs.resize(num_output_models);

                // remap all instances to point to the compacted model index
                for (uint32_t i = 0; i < instances.size(); i++) {
                    uint32_t new_model_index = model_remap[instances[i].model_index];
                    assert(new_model_index != UINT32_MAX);   // we should have suppressed instances already that point to NULL models.
                    instances[i].model_index = new_model_index;
                }

                // free remap table
                _vox_free(model_remap);
                model_remap = NULL;
            }
        }

        // finally, construct the output scene..
        size_t scene_size = sizeof(ogt_vox_scene) + string_data.size();
        ogt_vox_scene* scene = (ogt_vox_scene*)_vox_calloc(scene_size);
        {
            // copy name data into the scene
            char* scene_string_data = (char*)&scene[1];
            memcpy(scene_string_data, &string_data[0], sizeof(char) * string_data.size());

            // copy instances over to scene
            size_t num_scene_instances = instances.size();
            ogt_vox_instance* scene_instances = (ogt_vox_instance*)_vox_malloc(sizeof(ogt_vox_instance) * num_scene_instances);
            if (num_scene_instances) {
                memcpy(scene_instances, &instances[0], sizeof(ogt_vox_instance) * num_scene_instances);
            }
            scene->instances = scene_instances;
            scene->num_instances = (uint32_t)instances.size();

            // copy model pointers over to the scene,
            size_t num_scene_models = model_ptrs.size();
            ogt_vox_model** scene_models = (ogt_vox_model * *)_vox_malloc(sizeof(ogt_vox_model*) * num_scene_models);
            if (num_scene_models)
                memcpy(scene_models, &model_ptrs[0], sizeof(ogt_vox_model*) * num_scene_models);
            scene->models     = (const ogt_vox_model **)scene_models;
            scene->num_models = (uint32_t)num_scene_models;

            // copy layer pointers over to the scene
            size_t num_scene_layers = layers.size();
            ogt_vox_layer* scene_layers = (ogt_vox_layer*)_vox_malloc(sizeof(ogt_vox_layer) * num_scene_layers);
            memcpy(scene_layers, &layers[0], sizeof(ogt_vox_layer) * num_scene_layers);
            scene->layers     = scene_layers;
            scene->num_layers = (uint32_t)num_scene_layers;

            // copy group pointers over to the scene
            size_t num_scene_groups = groups.size();
            ogt_vox_group* scene_groups = num_scene_groups ? (ogt_vox_group*)_vox_malloc(sizeof(ogt_vox_group) * num_scene_groups) : NULL;
            if (num_scene_groups)
                memcpy(scene_groups, &groups[0], sizeof(ogt_vox_group)* num_scene_groups);
            scene->groups     = scene_groups;
            scene->num_groups = (uint32_t)num_scene_groups;

            // now patch up instance name pointers to point into the scene string area
            for (uint32_t i = 0; i < num_scene_instances; i++)
                if (scene_instances[i].name)
                    scene_instances[i].name = scene_string_data + (size_t)scene_instances[i].name;

            // now patch up layer name pointers to point into the scene string area
            for (uint32_t i = 0; i < num_scene_layers; i++)
                if (scene_layers[i].name)
                    scene_layers[i].name = scene_string_data + (size_t)scene_layers[i].name;

            // copy the palette.
            scene->palette = palette;

            // copy the materials.
            scene->materials = materials;
        }
        return scene;
    }

    const ogt_vox_scene* ogt_vox_read_scene(const uint8_t* buffer, uint32_t buffer_size) {
        return ogt_vox_read_scene_with_flags(buffer, buffer_size, 0);
    }

    void ogt_vox_destroy_scene(const ogt_vox_scene * _scene) {
        ogt_vox_scene* scene = const_cast<ogt_vox_scene*>(_scene);
        // free models from model array
        for (uint32_t i = 0; i < scene->num_models; i++)
            _vox_free((void*)scene->models[i]);
        // free model array itself
        if (scene->models) {
            _vox_free(scene->models);
            scene->models = NULL;
        }
        // free instance array
        if (scene->instances) {
            _vox_free(const_cast<ogt_vox_instance*>(scene->instances));
            scene->instances = NULL;
        }
        // free layer array
        if (scene->layers) {
            _vox_free(const_cast<ogt_vox_layer*>(scene->layers));
            scene->layers = NULL;
        }
        // free groups array
        if (scene->groups) {
            _vox_free(const_cast<ogt_vox_group*>(scene->groups));
            scene->groups = NULL;
        }
        // finally, free the scene.
        _vox_free(scene);
    }

    // the vector should be a unit vector aligned along one of the cardinal directions exactly. eg. (1,0,0) or (0, 0, -1)
       // this function returns the non-zero column index in out_index and the returns whether that entry is negative.
    static bool _vox_get_vec3_rotation_bits(const vec3& vec, uint8_t& out_index) {
        const float* f = &vec.x;
        out_index = 3;
        bool is_negative = false;
        for (uint8_t i = 0; i < 3; i++) {
            if (f[i] == 1.0f || f[i] == -1.0f) {
                out_index = i;
                is_negative = f[i] < 0.0f ? true : false;
            }
            else {
                assert(f[i] == 0.0f);   // must be zero
            }
        }
        assert(out_index != 3); // if you hit this, you probably have all zeroes in the vector!
        return is_negative;
    }

    static uint8_t _vox_make_packed_rotation_from_transform(const ogt_vox_transform * transform) {
        // magicavoxel stores rows, and we have columns, so we do the swizzle here into rows
        vec3 row0 = vec3_make(transform->m00, transform->m10, transform->m20);
        vec3 row1 = vec3_make(transform->m01, transform->m11, transform->m21);
        vec3 row2 = vec3_make(transform->m02, transform->m12, transform->m22);
        uint8_t row0_index = 3, row1_index = 3, row2_index = 3;
        bool row0_negative = _vox_get_vec3_rotation_bits(row0, row0_index);
        bool row1_negative = _vox_get_vec3_rotation_bits(row1, row1_index);
        bool row2_negative = _vox_get_vec3_rotation_bits(row2, row2_index);
        assert(((1 << row0_index) | (1 << row1_index) | (1 << row2_index)) == 7); // check that rows are orthogonal. There must be a non-zero entry in column 0, 1 and 2 across these 3 rows.
        return (row0_index) | (row1_index << 2) | (row0_negative ? 1 << 4 : 0) | (row1_negative ? 1 << 5 : 0) | (row2_negative ? 1 << 6 : 0);
    }

    struct _vox_file_writeable {
        _vox_array<uint8_t> data;
    };

    static void _vox_file_writeable_init(_vox_file_writeable* fp) {
        fp->data.reserve(1024);
    }
    static void _vox_file_write(_vox_file_writeable* fp, const void* data, uint32_t data_size) {
        fp->data.push_back_many((const uint8_t*)data, data_size);
    }
    static void _vox_file_write_uint32(_vox_file_writeable* fp, uint32_t data) {
        fp->data.push_back_many((const uint8_t*)&data, sizeof(uint32_t));
    }
    static void _vox_file_write_uint8(_vox_file_writeable* fp, uint8_t data) {
        fp->data.push_back_many((const uint8_t*)&data, sizeof(uint8_t));
    }
    static uint32_t _vox_file_get_offset(const _vox_file_writeable* fp) {
        return (uint32_t)fp->data.count;
    }
    static uint8_t* _vox_file_get_data(_vox_file_writeable* fp) {
        return &fp->data[0];
    }
    static void _vox_file_write_dict_key_value(_vox_file_writeable* fp, const char* key, const char* value) {
        if (key == NULL || value == NULL)
            return;
        uint32_t key_len   = (uint32_t)_vox_strlen(key);
        uint32_t value_len = (uint32_t)_vox_strlen(value);
        _vox_file_write_uint32(fp, key_len);
        _vox_file_write(fp, key, key_len);
        _vox_file_write_uint32(fp, value_len);
        _vox_file_write(fp, value, value_len);
    }

    static uint32_t _vox_dict_key_value_size(const char* key, const char* value) {
        if (key == NULL || value == NULL)
            return 0;
        size_t size = sizeof(uint32_t) + _vox_strlen(key) + sizeof(uint32_t) + _vox_strlen(value);
        return (uint32_t)size;
    }

    static void _vox_file_write_chunk_nTRN(_vox_file_writeable* fp, uint32_t node_id, uint32_t child_node_id, const char* name, bool hidden, const ogt_vox_transform* transform, uint32_t layer_id)
    {
        // obtain dictionary string pointers
        const char* hidden_string = hidden ? "1" : NULL;
        const char* t_string = NULL;
        const char* r_string = NULL;
        char t_string_buf[65];
        char r_string_buf[65];
        t_string_buf[0] = 0;
        r_string_buf[0] = 0;
        if (transform != NULL) {
            uint8_t packed_rotation_bits = _vox_make_packed_rotation_from_transform(transform);
            _vox_sprintf(t_string_buf, sizeof(t_string_buf), "%i %i %i", (int32_t)transform->m30, (int32_t)transform->m31, (int32_t)transform->m32);
            _vox_sprintf(r_string_buf, sizeof(r_string_buf), "%u", packed_rotation_bits);
            t_string = t_string_buf;
            r_string = r_string_buf;
        }

        uint32_t node_dict_size =
            sizeof(uint32_t) + // num key value pairs
            _vox_dict_key_value_size("_name",   name) +
            _vox_dict_key_value_size("_hidden", hidden_string);

        uint32_t frame_dict_size =
            sizeof(uint32_t) + // num key value pairs
            _vox_dict_key_value_size("_t", t_string) +
            _vox_dict_key_value_size("_r", r_string);

        uint32_t chunk_size_ntrn =
            sizeof(uint32_t) +   // node_id
            node_dict_size +     // node dictionary
            4 * sizeof(uint32_t) + // middle section - 4 uint32s
            frame_dict_size;

        // write the nTRN header
        _vox_file_write_uint32(fp, CHUNK_ID_nTRN);
        _vox_file_write_uint32(fp, chunk_size_ntrn);
        _vox_file_write_uint32(fp, 0);

        // write the nTRN payload
        _vox_file_write_uint32(fp, node_id);

        // write the node dictionary
        uint32_t node_dict_keyvalue_count = (name ? 1 : 0) + (hidden_string ? 1 : 0);
        _vox_file_write_uint32(fp, node_dict_keyvalue_count);  // num key values
        _vox_file_write_dict_key_value(fp, "_name", name);
        _vox_file_write_dict_key_value(fp, "_hidden", hidden_string);

        // get other properties.
        _vox_file_write_uint32(fp, child_node_id);
        _vox_file_write_uint32(fp, UINT32_MAX); // reserved_id must have all bits set.
        _vox_file_write_uint32(fp, layer_id);
        _vox_file_write_uint32(fp, 1);          // num_frames must be 1

        // write the frame dictionary
        _vox_file_write_uint32(fp, (r_string ? 1 : 0) + (t_string ? 1 : 0));  // num key values
        _vox_file_write_dict_key_value(fp, "_r", r_string);
        _vox_file_write_dict_key_value(fp, "_t", t_string);
    }

    // saves the scene out to a buffer that when saved as a .vox file can be loaded with magicavoxel.
    uint8_t* ogt_vox_write_scene(const ogt_vox_scene* scene, uint32_t* buffer_size) {
        _vox_file_writeable file;
        _vox_file_writeable_init(&file);
        _vox_file_writeable* fp = &file;

        // write file header and file version
        _vox_file_write_uint32(fp, CHUNK_ID_VOX_);
        _vox_file_write_uint32(fp, 150);

        // write the main chunk
        _vox_file_write_uint32(fp, CHUNK_ID_MAIN);
        _vox_file_write_uint32(fp, 0);
        _vox_file_write_uint32(fp, 0);  // this main_chunk_child_size will get patched up once everything is written.

        // we need to know how to patch up the main chunk size after we've written everything
        const uint32_t offset_post_main_chunk = _vox_file_get_offset(fp);

        // write out all model chunks
        for (uint32_t i = 0; i < scene->num_models; i++) {
            const ogt_vox_model* model = scene->models[i];
            assert(model->size_x <= 256 && model->size_y <= 256 && model->size_z <= 256);
            // count the number of solid voxels in the grid
            uint32_t num_voxels_in_grid = model->size_x * model->size_y * model->size_z;
            uint32_t num_solid_voxels = 0;
            for (uint32_t voxel_index = 0; voxel_index < num_voxels_in_grid; voxel_index++)
                if (model->voxel_data[voxel_index] != 0)
                    num_solid_voxels++;
            uint32_t chunk_size_xyzi = sizeof(uint32_t) + 4 * num_solid_voxels;

            // write the SIZE chunk header
            _vox_file_write_uint32(fp, CHUNK_ID_SIZE);
            _vox_file_write_uint32(fp, 12);
            _vox_file_write_uint32(fp, 0);

            // write the SIZE chunk payload
            _vox_file_write_uint32(fp, model->size_x);
            _vox_file_write_uint32(fp, model->size_y);
            _vox_file_write_uint32(fp, model->size_z);

            // write the XYZI chunk header
            _vox_file_write_uint32(fp, CHUNK_ID_XYZI);
            _vox_file_write_uint32(fp, chunk_size_xyzi);
            _vox_file_write_uint32(fp, 0);

            // write out XYZI chunk payload
            _vox_file_write_uint32(fp, num_solid_voxels);
            uint32_t voxel_index = 0;
            for (uint32_t z = 0; z < model->size_z; z++) {
                for (uint32_t y = 0; y < model->size_y; y++) {
                    for (uint32_t x = 0; x < model->size_x; x++, voxel_index++) {
                        uint8_t color_index = model->voxel_data[voxel_index];
                        if (color_index != 0) {
                            _vox_file_write_uint8(fp, (uint8_t)x);
                            _vox_file_write_uint8(fp, (uint8_t)y);
                            _vox_file_write_uint8(fp, (uint8_t)z);
                            _vox_file_write_uint8(fp, color_index);
                        }
                    }
                }
            }
        }

        // define our node_id ranges.
        assert(scene->num_groups);
        uint32_t first_group_transform_node_id    = 0;
        uint32_t first_group_node_id              = first_group_transform_node_id + scene->num_groups;
        uint32_t first_shape_node_id              = first_group_node_id + scene->num_groups;
        uint32_t first_instance_transform_node_id = first_shape_node_id + scene->num_models;

        // write the nTRN nodes for each of the groups in the scene.
        for (uint32_t group_index = 0; group_index < scene->num_groups; group_index++) {
            const ogt_vox_group* group = &scene->groups[group_index];
            _vox_file_write_chunk_nTRN(fp, first_group_transform_node_id + group_index, first_group_node_id + group_index, NULL, group->hidden, &group->transform, group->layer_index);
        }
        // write the group nodes for each of the groups in the scene
        for (uint32_t group_index = 0; group_index < scene->num_groups; group_index++) {
            // count how many childnodes  there are. This is simply the sum of all
            // groups and instances that have this group as its parent
            uint32_t num_child_nodes = 0;
            for (uint32_t child_group_index = 0; child_group_index < scene->num_groups; child_group_index++)
                if (scene->groups[child_group_index].parent_group_index == group_index)
                    num_child_nodes++;
            for (uint32_t child_instance_index = 0; child_instance_index < scene->num_instances; child_instance_index++)
                if (scene->instances[child_instance_index].group_index == group_index)
                    num_child_nodes++;

            // count number of dictionary items
            const char* hidden_string = scene->groups[group_index].hidden ? "1" : NULL;
            uint32_t group_dict_keyvalue_count = (hidden_string ? 1 : 0);

            // compute the chunk size.
            uint32_t chunk_size_ngrp =
                sizeof(uint32_t) +                   // node_id
                sizeof(uint32_t) +                   // num keyvalue pairs in node dictionary
                _vox_dict_key_value_size("_hidden", hidden_string) +
                sizeof(uint32_t) +                   // num_child_nodes field
                sizeof(uint32_t) * num_child_nodes;  // uint32_t for each child node id.

            // write the nGRP header
            _vox_file_write_uint32(fp, CHUNK_ID_nGRP);
            _vox_file_write_uint32(fp, chunk_size_ngrp);
            _vox_file_write_uint32(fp, 0);
            // write the nGRP payload
            _vox_file_write_uint32(fp, first_group_node_id + group_index);       // node_id
            _vox_file_write_uint32(fp, group_dict_keyvalue_count); // num keyvalue pairs in node dictionary
            _vox_file_write_dict_key_value(fp, "_hidden", hidden_string);
            _vox_file_write_uint32(fp, num_child_nodes);
            // write the child group transform nodes
            for (uint32_t child_group_index = 0; child_group_index < scene->num_groups; child_group_index++)
                if (scene->groups[child_group_index].parent_group_index == group_index)
                    _vox_file_write_uint32(fp, first_group_transform_node_id + child_group_index);
            // write the child instance transform nodes
            for (uint32_t child_instance_index = 0; child_instance_index < scene->num_instances; child_instance_index++)
                if (scene->instances[child_instance_index].group_index == group_index)
                    _vox_file_write_uint32(fp, first_instance_transform_node_id + child_instance_index);
        }

        // write out an nSHP chunk for each of the models
        for (uint32_t i = 0; i < scene->num_models; i++) {
            // compute the size of the nSHP chunk
            uint32_t chunk_size_nshp =
                sizeof(uint32_t) +      // node_id
                sizeof(uint32_t) +      // num keyvalue pairs in node dictionary
                sizeof(uint32_t) +      // num_models
                sizeof(uint32_t) +      // model_id
                sizeof(uint32_t);       // num keyvalue pairs in model dictionary
            // write the nSHP chunk header
            _vox_file_write_uint32(fp, CHUNK_ID_nSHP);
            _vox_file_write_uint32(fp, chunk_size_nshp);
            _vox_file_write_uint32(fp, 0);
            // write the nSHP chunk payload
            _vox_file_write_uint32(fp, first_shape_node_id + i);    // node_id
            _vox_file_write_uint32(fp, 0);                          // num keyvalue pairs in node dictionary
            _vox_file_write_uint32(fp, 1);                          // num_models must be 1
            _vox_file_write_uint32(fp, i);                          // model_id
            _vox_file_write_uint32(fp, 0);                          // num keyvalue pairs in model dictionary
        }
        // write out an nTRN chunk for all instances - and make them point to the relevant nSHP chunk
        for (uint32_t i = 0; i < scene->num_instances; i++) {
            const ogt_vox_instance* instance = &scene->instances[i];
            uint32_t node_id       = first_instance_transform_node_id + i;
            uint32_t child_node_id = first_shape_node_id + instance->model_index;
            _vox_file_write_chunk_nTRN(fp, node_id, child_node_id, instance->name, instance->hidden, &instance->transform, instance->layer_index);
        }

        // write out RGBA chunk for the palette
        {
            // .vox stores palette rotated by 1 color index, so do that now.
            ogt_vox_palette rotated_palette;
            for (uint32_t i = 0; i < 256; i++)
                rotated_palette.color[i] = scene->palette.color[(i + 1) & 255];

            // write the palette chunk header
            _vox_file_write_uint32(fp, CHUNK_ID_RGBA);
            _vox_file_write_uint32(fp, sizeof(ogt_vox_palette));
            _vox_file_write_uint32(fp, 0);
            // write the palette chunk payload
            _vox_file_write(fp, &rotated_palette, sizeof(ogt_vox_palette));
        }

        // write out MATL chunk
        {
            // keep in sync with ogt_matl_type
            static const char *type_str[] = {"_diffuse", "_metal", "_glass", "_emit", "_blend", "_media"};

            for (int32_t i = 0; i < 256; ++i) {
                const ogt_vox_matl &matl = scene->materials.matl[i];
                if (matl.content_flags == 0u) {
                    continue;
                }
                char matl_metal[16] = "";
                char matl_rough[16] = "";
                char matl_spec[16] = "";
                char matl_ior[16] = "";
                char matl_att[16] = "";
                char matl_flux[16] = "";
                char matl_emit[16] = "";
                char matl_ldr[16] = "";
                char matl_trans[16] = "";
                char matl_alpha[16] = "";
                char matl_d[16] = "";
                char matl_sp[16] = "";
                char matl_g[16] = "";
                char matl_media[16] = "";
                uint32_t matl_dict_size = 0;
                uint32_t matl_dict_keyvalue_count = 0;

                if (matl.content_flags & k_ogt_vox_matl_have_metal) {
                    _vox_sprintf(matl_metal, sizeof(matl_metal), "%f", matl.metal);
                    matl_dict_size += _vox_dict_key_value_size("_metal", matl_metal);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_rough) {
                    _vox_sprintf(matl_rough, sizeof(matl_rough), "%f", matl.rough);
                    matl_dict_size += _vox_dict_key_value_size("_rough", matl_rough);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_spec) {
                    _vox_sprintf(matl_spec, sizeof(matl_spec), "%f", matl.spec);
                    matl_dict_size += _vox_dict_key_value_size("_spec", matl_spec);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_ior) {
                    _vox_sprintf(matl_ior, sizeof(matl_ior), "%f", matl.ior);
                    matl_dict_size += _vox_dict_key_value_size("_ior", matl_ior);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_att) {
                    _vox_sprintf(matl_att, sizeof(matl_att), "%f", matl.att);
                    matl_dict_size += _vox_dict_key_value_size("_att", matl_att);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_flux) {
                    _vox_sprintf(matl_flux, sizeof(matl_flux), "%f", matl.flux);
                    matl_dict_size += _vox_dict_key_value_size("_flux", matl_flux);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_emit) {
                    _vox_sprintf(matl_emit, sizeof(matl_emit), "%f", matl.emit);
                    matl_dict_size += _vox_dict_key_value_size("_emit", matl_emit);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_ldr) {
                    _vox_sprintf(matl_ldr, sizeof(matl_ldr), "%f", matl.ldr);
                    matl_dict_size += _vox_dict_key_value_size("_ldr", matl_ldr);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_trans) {
                    _vox_sprintf(matl_trans, sizeof(matl_trans), "%f", matl.trans);
                    matl_dict_size += _vox_dict_key_value_size("_trans", matl_trans);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_alpha) {
                    _vox_sprintf(matl_alpha, sizeof(matl_alpha), "%f", matl.alpha);
                    matl_dict_size += _vox_dict_key_value_size("_alpha", matl_alpha);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_d) {
                    _vox_sprintf(matl_d, sizeof(matl_d), "%f", matl.d);
                    matl_dict_size += _vox_dict_key_value_size("_d", matl_d);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_sp) {
                    _vox_sprintf(matl_sp, sizeof(matl_sp), "%f", matl.sp);
                    matl_dict_size += _vox_dict_key_value_size("_sp", matl_sp);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_g) {
                    _vox_sprintf(matl_g, sizeof(matl_g), "%f", matl.g);
                    matl_dict_size += _vox_dict_key_value_size("_g", matl_g);
                    ++matl_dict_keyvalue_count;
                }
                if (matl.content_flags & k_ogt_vox_matl_have_media) {
                    _vox_sprintf(matl_media, sizeof(matl_media), "%f", matl.media);
                    matl_dict_size += _vox_dict_key_value_size("_media", matl_media);
                    ++matl_dict_keyvalue_count;
                }
                matl_dict_size += _vox_dict_key_value_size("_type", type_str[matl.type]);

                // write the material chunk header
                _vox_file_write_uint32(fp, CHUNK_ID_MATL);
                _vox_file_write_uint32(fp, sizeof(uint32_t) + sizeof(uint32_t) + matl_dict_size);
                _vox_file_write_uint32(fp, 0);
                _vox_file_write_uint32(fp, i); // material id
                _vox_file_write_uint32(fp, matl_dict_keyvalue_count);
                _vox_file_write_dict_key_value(fp, "_type", type_str[matl.type]);
                if (matl.content_flags & k_ogt_vox_matl_have_metal) {
                    _vox_file_write_dict_key_value(fp, "_metal", matl_metal);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_rough) {
                    _vox_file_write_dict_key_value(fp, "_rough", matl_rough);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_spec) {
                    _vox_file_write_dict_key_value(fp, "_spec", matl_spec);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_ior) {
                    _vox_file_write_dict_key_value(fp, "_ior", matl_ior);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_att) {
                    _vox_file_write_dict_key_value(fp, "_att", matl_att);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_flux) {
                    _vox_file_write_dict_key_value(fp, "_flux", matl_flux);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_emit) {
                    _vox_file_write_dict_key_value(fp, "_emit", matl_emit);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_ldr) {
                    _vox_file_write_dict_key_value(fp, "_ldr", matl_ldr);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_trans) {
                    _vox_file_write_dict_key_value(fp, "_trans", matl_trans);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_alpha) {
                    _vox_file_write_dict_key_value(fp, "_alpha", matl_alpha);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_d) {
                    _vox_file_write_dict_key_value(fp, "_d", matl_d);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_sp) {
                    _vox_file_write_dict_key_value(fp, "_sp", matl_sp);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_g) {
                    _vox_file_write_dict_key_value(fp, "_g", matl_g);
                }
                if (matl.content_flags & k_ogt_vox_matl_have_media) {
                    _vox_file_write_dict_key_value(fp, "_media", matl_media);
                }
            }
        }

        // write all layer chunks out.
        for (uint32_t i = 0; i < scene->num_layers; i++) {
            const char* layer_name_string = scene->layers[i].name;
            const char* hidden_string = scene->layers[i].hidden ? "1" : NULL;
            uint32_t layer_chunk_size =
                sizeof(int32_t) +   // layer_id
                sizeof(uint32_t) +  // num key value pairs
                _vox_dict_key_value_size("_name", layer_name_string) +
                _vox_dict_key_value_size("_hidden", hidden_string) +
                sizeof(int32_t);    // reserved id, must be -1
            uint32_t layer_dict_keyvalue_count = (layer_name_string ? 1 : 0) + (hidden_string ? 1 : 0);
            // write the layer chunk header
            _vox_file_write_uint32(fp, CHUNK_ID_LAYR);
            _vox_file_write_uint32(fp, layer_chunk_size);
            _vox_file_write_uint32(fp, 0);
            // write the layer chunk payload
            _vox_file_write_uint32(fp, i);                          // layer_id
            _vox_file_write_uint32(fp, layer_dict_keyvalue_count);  // num keyvalue pairs in layer dictionary
            _vox_file_write_dict_key_value(fp, "_name",   layer_name_string);
            _vox_file_write_dict_key_value(fp, "_hidden", hidden_string);
            _vox_file_write_uint32(fp, UINT32_MAX);                 // reserved id
        }

        // we deliberately don't free the fp->data field, just pass the buffer pointer and size out to the caller
        *buffer_size = (uint32_t)fp->data.count;
        uint8_t* buffer_data = _vox_file_get_data(fp);
        // we deliberately clear this pointer so it doesn't get auto-freed on exiting. The caller will own the memory hereafter.
        fp->data.data = NULL;

        // patch up the main chunk's child chunk size now that we've written everything we're going to write.
        {
            uint32_t* main_chunk_child_size = (uint32_t*)& buffer_data[offset_post_main_chunk - sizeof(uint32_t)];
            *main_chunk_child_size = *buffer_size - offset_post_main_chunk;
        }

        return buffer_data;
    }

    void* ogt_vox_malloc(size_t size) {
        return _vox_malloc(size);
    }

    void ogt_vox_free(void* mem) {
        _vox_free(mem);
    }

    // compute the minimum and maximum x coordinate within the scene.
    static void compute_scene_bounding_box_x(const ogt_vox_scene * scene, int32_t & out_min_x, int32_t & out_max_x) {
        if (scene->num_instances && scene->num_models)
        {
            // We don't apply orientation to the model dimensions and compute the exact min/max.
            // Instead we just conservatively use the maximum dimension of the model.
            int32_t scene_min_x =  0x7ffffff;
            int32_t scene_max_x = -0x7ffffff;
            for (uint32_t instance_index = 0; instance_index < scene->num_instances; instance_index++) {
                const ogt_vox_instance* instance = &scene->instances[instance_index];
                // compute the instance transform, taking into account the group hierarchy.
                ogt_vox_transform instance_transform = instance->transform;
                uint32_t parent_group_index = instance->group_index;
                while (parent_group_index != k_invalid_group_index) {
                    const ogt_vox_group* group = &scene->groups[parent_group_index];
                    instance_transform = _vox_transform_multiply(instance_transform, group->transform);
                    parent_group_index = group->parent_group_index;
                }

                const ogt_vox_model* model = scene->models[instance->model_index];
                // the instance_transform can be rotated, so we try to figure out whether the
                // model's local x, y or z size is aligned along the world x axis.
                // One of the column vectors of the transform must have a non-zero in its
                // x field and the dimension associated with that column is the correct choice of rus.
                int32_t max_dim = instance_transform.m00 != 0.0f ? model->size_x :
                                  instance_transform.m10 != 0.0f ? model->size_y :
                                  instance_transform.m20 != 0.0f ? model->size_z : model->size_x;
                int32_t half_dim = max_dim / 2;
                int32_t min_x = (int32_t)instance_transform.m30 - half_dim;
                int32_t max_x = (int32_t)instance_transform.m30 + half_dim;
                scene_min_x = min_x < scene_min_x ? min_x : scene_min_x;
                scene_max_x = max_x > scene_max_x ? max_x : scene_max_x;
            }
            // pass out the dimensions.
            out_min_x = scene_min_x;
            out_max_x = scene_max_x;
        }
        else {
            out_min_x = 0;
            out_max_x = 0;
        }
    }

    // returns a mask of which color indices are used by the specified scene.
    // used_mask[0] can be false at the end of this if all models 100% fill their voxel grid with solid voxels, so callers
    // should handle that case properly.
    static void compute_scene_used_color_index_mask(bool* used_mask, const ogt_vox_scene * scene) {
        memset(used_mask, 0, 256);
        for (uint32_t model_index = 0; model_index < scene->num_models; model_index++) {
            const ogt_vox_model* model = scene->models[model_index];
            uint32_t voxel_count = model->size_x * model->size_y * model->size_z;
            for (uint32_t voxel_index = 0; voxel_index < voxel_count; voxel_index++) {
                uint8_t color_index = model->voxel_data[voxel_index];
                used_mask[color_index] = true;
            }
        }
    }

    // finds an exact color in the specified palette if it exists, and UINT32_MAX otherwise
    static uint32_t find_exact_color_in_palette(const ogt_vox_rgba * palette, uint32_t palette_count, const ogt_vox_rgba color_to_find) {
        for (uint32_t color_index = 1; color_index < palette_count; color_index++) {
            const ogt_vox_rgba color_to_match = palette[color_index];
            // we only try to match r,g,b components exactly.
            if (color_to_match.r == color_to_find.r && color_to_match.g == color_to_find.g && color_to_match.b == color_to_find.b)
                return color_index;
        }
        // no exact color found
        return UINT32_MAX;
    }

    // finds the index within the specified palette that is closest to the color we want to find
    static uint32_t find_closest_color_in_palette(const ogt_vox_rgba * palette, uint32_t palette_count, const ogt_vox_rgba color_to_find) {
        // the lower the score the better, so initialize this to the maximum possible score
        int32_t  best_score = INT32_MAX;
        uint32_t best_index = 1;
        // Here we compute a score based on the pythagorean distance between each color in the palette and the color to find.
        // The distance is in R,G,B space, and we choose the color with the lowest score.
        for (uint32_t color_index = 1; color_index < palette_count; color_index++) {
            int32_t r_diff = (int32_t)color_to_find.r - (int32_t)palette[color_index].r;
            int32_t g_diff = (int32_t)color_to_find.g - (int32_t)palette[color_index].g;
            int32_t b_diff = (int32_t)color_to_find.b - (int32_t)palette[color_index].b;
            // There are 2 aspects of our treatment of color here you may want to experiment with:
            // 1. differences in R, differences in G, differences in B are weighted the same rather than perceptually. Different weightings may be better for you.
            // 2. We treat R,G,B as if they are in a perceptually linear within each channel. eg. the differences between
            //    a value of 5 and 8 in any channel is perceptually the same as the difference between 233 and 236 in the same channel.
            int32_t score = (r_diff * r_diff) + (g_diff * g_diff) + (b_diff * b_diff);
            if (score < best_score) {
                best_score = score;
                best_index = color_index;
            }
        }
        assert(best_score < INT32_MAX); // this might indicate a completely degenerate palette.
        return best_index;
    }

    static void update_master_palette_from_scene(ogt_vox_rgba * master_palette, uint32_t & master_palette_count, const ogt_vox_scene * scene, uint32_t * scene_to_master_map) {
        // compute the mask of used colors in the scene.
        bool scene_used_mask[256];
        compute_scene_used_color_index_mask(scene_used_mask, scene);

        // initialize the map that converts from scene color_index to master color_index
        scene_to_master_map[0] = 0;              // zero/empty always maps to zero/empty in the master palette
        for (uint32_t i = 1; i < 256; i++)
            scene_to_master_map[i] = UINT32_MAX; // UINT32_MAX means unassigned

        // for each used color in the scene, now allocate it into the master palette.
        for (uint32_t color_index = 1; color_index < 256; color_index++) {
            if (scene_used_mask[color_index]) {
                const ogt_vox_rgba color = scene->palette.color[color_index];
                // find the exact color in the master palette. Will be UINT32_MAX if the color doesn't already exist
                uint32_t master_index = find_exact_color_in_palette(master_palette, master_palette_count, color);
                if (master_index == UINT32_MAX) {
                    if (master_palette_count < 256) {
                        // master palette capacity hasn't been exceeded so far, allocate the color to it.
                        master_palette[master_palette_count] = color;
                        master_index = master_palette_count++;
                    }
                    else {
                        // otherwise, find the color that is perceptually closest to the original color.

                        // TODO(jpaver): It is potentially problematic if we hit this path for a many-scene merge.
                        // Earlier scenes will reserve their colors exactly into the master palette, whereas later
                        // scenes will end up having some of their colors remapped to different colors.

                        // A more holistic approach to color allocation may be necessary here eg.
                        // we might allow the master palette to grow to more than 256 entries, and then use
                        // similarity/frequency metrics to reduce the palette from that down to 256 entries. This
                        // will mean all scenes will have be equally important if they have a high-frequency
                        // usage of a color.
                        master_index = find_closest_color_in_palette(master_palette, master_palette_count, color);
                    }
                }
                // caller needs to know how to map its original color index into the master palette
                scene_to_master_map[color_index] = master_index;
            }
        }
    }

    ogt_vox_scene* ogt_vox_merge_scenes(const ogt_vox_scene** scenes, uint32_t scene_count, const ogt_vox_rgba* required_colors, const uint32_t required_color_count) {
        assert(required_color_count <= 255);    // can't exceed the maximum colors in the master palette plus the empty slot.

        // initialize the master palette. If required colors are specified, map them into the master palette now.
        ogt_vox_rgba  master_palette[256];
        uint32_t master_palette_count = 1;          // color_index 0 is reserved for empty color!
        memset(&master_palette, 0, sizeof(master_palette));
        for (uint32_t required_index = 0; required_index < required_color_count; required_index++)
            master_palette[master_palette_count++] = required_colors[required_index];

        // count the number of required models, instances in the master scene
        uint32_t max_layers = 1;  // we don't actually merge layers. Every instance will be in layer 0.
        uint32_t max_models = 0;
        uint32_t max_instances = 0;
        uint32_t max_groups = 1;  // we add 1 root global group that everything will ultimately be parented to.
        for (uint32_t scene_index = 0; scene_index < scene_count; scene_index++) {
            if (!scenes[scene_index])
                continue;
            max_instances += scenes[scene_index]->num_instances;
            max_models += scenes[scene_index]->num_models;
            max_groups += scenes[scene_index]->num_groups;
        }

        // allocate the master instances array
        ogt_vox_instance* instances = (ogt_vox_instance*)_vox_malloc(sizeof(ogt_vox_instance) * max_instances);
        ogt_vox_model** models      = (ogt_vox_model**)_vox_malloc(sizeof(ogt_vox_model*) * max_models);
        ogt_vox_layer* layers       = (ogt_vox_layer*)_vox_malloc(sizeof(ogt_vox_layer) * max_layers);
        ogt_vox_group* groups       = (ogt_vox_group*)_vox_malloc(sizeof(ogt_vox_group) * max_groups);
        uint32_t num_instances = 0;
        uint32_t num_models    = 0;
        uint32_t num_layers    = 0;
        uint32_t num_groups    = 0;

        // add a single layer.
        layers[num_layers].hidden = false;
        layers[num_layers].name = "merged";
        num_layers++;

        // magicavoxel expects exactly 1 root group, so if we have multiple scenes with multiple roots,
        // we must ensure all merged scenes are parented to the same root group. Allocate it now for the
        // merged scene.
        uint32_t global_root_group_index = num_groups;
        {
            assert(global_root_group_index == 0);
            ogt_vox_group root_group;
            root_group.hidden             = false;
            root_group.layer_index        = 0;
            root_group.parent_group_index = k_invalid_group_index;
            root_group.transform          = _vox_transform_identity();
            groups[num_groups++] = root_group;
        }

        // go ahead and do the merge now!
        size_t string_data_size = 0;
        int32_t offset_x = 0;
        for (uint32_t scene_index = 0; scene_index < scene_count; scene_index++) {
            const ogt_vox_scene* scene = scenes[scene_index];
            if (!scene)
                continue;

            // update the master palette, and get the map of this scene's color indices into the master palette.
            uint32_t scene_color_index_to_master_map[256];
            update_master_palette_from_scene(master_palette, master_palette_count, scene, scene_color_index_to_master_map);

            // cache away the base model index for this scene.
            uint32_t base_model_index = num_models;
            uint32_t base_group_index = num_groups;

            // create copies of all models that have color indices remapped.
            for (uint32_t model_index = 0; model_index < scene->num_models; model_index++) {
                const ogt_vox_model* model = scene->models[model_index];
                uint32_t voxel_count = model->size_x * model->size_y * model->size_z;
                // clone the model
                ogt_vox_model* override_model = (ogt_vox_model*)_vox_malloc(sizeof(ogt_vox_model) + voxel_count);
                uint8_t * override_voxel_data = (uint8_t*)& override_model[1];

                // remap all color indices in the cloned model so they reference the master palette now!
                for (uint32_t voxel_index = 0; voxel_index < voxel_count; voxel_index++) {
                    uint8_t  old_color_index = model->voxel_data[voxel_index];
                    uint32_t new_color_index = scene_color_index_to_master_map[old_color_index];
                    assert(new_color_index < 256);
                    override_voxel_data[voxel_index] = (uint8_t)new_color_index;
                }
                // assign the new model.
                *override_model = *model;
                override_model->voxel_data = override_voxel_data;
                override_model->voxel_hash = _vox_hash(override_voxel_data, voxel_count);

                models[num_models++] = override_model;
            }

            // compute the scene bounding box on x dimension. this is used to offset instances
            // and groups in the merged model along X dimension such that they do not overlap
            // with instances from another scene in the merged model.
            int32_t scene_min_x, scene_max_x;
            compute_scene_bounding_box_x(scene, scene_min_x, scene_max_x);
            float scene_offset_x = (float)(offset_x - scene_min_x);

            // each scene has a root group, and it must the 0th group in its local groups[] array,
            assert(scene->groups[0].parent_group_index == k_invalid_group_index);
            // create copies of all groups into the merged scene (except the root group from each scene -- which is why we start group_index at 1 here)
            for (uint32_t group_index = 1; group_index < scene->num_groups; group_index++) {
                const ogt_vox_group* src_group = &scene->groups[group_index];
                assert(src_group->parent_group_index != k_invalid_group_index); // there can be only 1 root group per scene and it must be the 0th group.
                ogt_vox_group dst_group = *src_group;
                assert(dst_group.parent_group_index < scene->num_groups);
                dst_group.layer_index        = 0;
                dst_group.parent_group_index = (dst_group.parent_group_index == 0) ? global_root_group_index : base_group_index + (dst_group.parent_group_index - 1);
                // if this group belongs to the global root group, it must be translated so it doesn't overlap with other scenes.
                if (dst_group.parent_group_index == global_root_group_index)
                    dst_group.transform.m30 += scene_offset_x;
                groups[num_groups++] = dst_group;
            }

            // create copies of all instances (and bias them such that minimum on x starts at zero)
            for (uint32_t instance_index = 0; instance_index < scene->num_instances; instance_index++) {
                const ogt_vox_instance* src_instance = &scene->instances[instance_index];
                assert(src_instance->group_index < scene->num_groups);  // every instance must be mapped to a group.
                ogt_vox_instance* dst_instance = &instances[num_instances++];
                *dst_instance = *src_instance;
                dst_instance->layer_index = 0;
                dst_instance->group_index = (dst_instance->group_index == 0) ? global_root_group_index : base_group_index + (dst_instance->group_index - 1);
                dst_instance->model_index += base_model_index;
                if (dst_instance->name)
                    string_data_size += _vox_strlen(dst_instance->name) + 1; // + 1 for zero terminator
                // if this instance belongs to the global rot group, it must be translated so it doesn't overlap with other scenes.
                if (dst_instance->group_index == global_root_group_index)
                    dst_instance->transform.m30 += scene_offset_x;
            }

            offset_x += (scene_max_x - scene_min_x); // step the width of the scene in x dimension
            offset_x += 4;                           // a margin of this many voxels between scenes
        }

        // fill any unused master palette entries with purple/invalid color.
        const ogt_vox_rgba k_invalid_color = { 255, 0, 255, 255 };  // purple = invalid
        for (uint32_t color_index = master_palette_count; color_index < 256; color_index++)
            master_palette[color_index] = k_invalid_color;

        // assign the master scene on output. string_data is part of the scene allocation.
        size_t scene_size = sizeof(ogt_vox_scene) + string_data_size;
        ogt_vox_scene * merged_scene = (ogt_vox_scene*)_vox_calloc(scene_size);

        // copy name data into the string section and make instances point to it. This makes the merged model self-contained.
        char* scene_string_data = (char*)&merged_scene[1];
        for (uint32_t instance_index = 0; instance_index < num_instances; instance_index++) {
            if (instances[instance_index].name) {
                size_t string_len = _vox_strlen(instances[instance_index].name) + 1; // +1 for zero terminator
                memcpy(scene_string_data, instances[instance_index].name, string_len);
                instances[instance_index].name = scene_string_data;
                scene_string_data += string_len;
            }
        }

        assert(num_groups <= max_groups);

        memset(merged_scene, 0, sizeof(ogt_vox_scene));
        merged_scene->instances     = instances;
        merged_scene->num_instances = max_instances;
        merged_scene->models        = (const ogt_vox_model * *)models;
        merged_scene->num_models    = max_models;
        merged_scene->layers        = layers;
        merged_scene->num_layers    = max_layers;
        merged_scene->groups        = groups;
        merged_scene->num_groups    = num_groups;
        // copy color palette into the merged scene
        for (uint32_t color_index = 0; color_index < 256; color_index++)
            merged_scene->palette.color[color_index] = master_palette[color_index];

        return merged_scene;
    }

 #endif // #ifdef OGT_VOX_IMPLEMENTATION

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/* -------------------------------------------------------------------------------------------------------------------------------------------------

    MIT License

    Copyright (c) 2019 Justin Paver

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.

------------------------------------------------------------------------------------------------------------------------------------------------- */
