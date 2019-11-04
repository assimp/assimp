/*
 * m3d.h
 *
 * Copyright (C) 2019 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @brief ANSI C89 / C++11 single header importer / exporter SDK for the Model 3D (.M3D) format
 * https://gitlab.com/bztsrc/model3d
 *
 * PNG decompressor included from (with minor modifications to make it C89 valid):
 *  stb_image - v2.13 - public domain image loader - http://nothings.org/stb_image.h
 *
 * @version: 1.0.0
 */

#ifndef _M3D_H_
#define _M3D_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*** configuration ***/
#ifndef M3D_MALLOC
# define M3D_MALLOC(sz)     malloc(sz)
#endif
#ifndef M3D_REALLOC
# define M3D_REALLOC(p,nsz) realloc(p,nsz)
#endif
#ifndef M3D_FREE
# define M3D_FREE(p)        free(p)
#endif
#ifndef M3D_LOG
# define M3D_LOG(x)
#endif
#ifndef M3D_APIVERSION
#define M3D_APIVERSION      0x0100
#ifndef M3D_DOUBLE
typedef float M3D_FLOAT;
#else
typedef double M3D_FLOAT;
#endif
#if !defined(M3D_SMALLINDEX)
typedef uint32_t M3D_INDEX;
#define M3D_INDEXMAX 0xfffffffe
#else
typedef uint16_t M3D_INDEX;
#define M3D_INDEXMAX 0xfffe
#endif
#ifndef M3D_NUMBONE
#define M3D_NUMBONE 4
#endif
#ifndef M3D_BONEMAXLEVEL
#define M3D_BONEMAXLEVEL 8
#endif
#ifndef _MSC_VER
#define _inline __inline__
#define _pack __attribute__((packed))
#define _unused __attribute__((unused))
#else
#define _inline
#define _pack
#define _unused
#endif
#ifndef  __cplusplus
#define _register register
#else
#define _register
#endif

/*** File format structures ***/

/**
 * M3D file format structure
 *  3DMO m3dchunk_t file header chunk, may followed by compressed data
 *  HEAD m3dhdr_t model header chunk
 *  n x m3dchunk_t more chunks follow
 *      CMAP color map chunk (optional)
 *      TMAP texture map chunk (optional)
 *      VRTS vertex data chunk (optional if it's a material library)
 *      BONE bind-pose skeleton, bone hierarchy chunk (optional)
 *          n x m3db_t contains propably more, but at least one bone
 *      MTRL* material chunk(s), can be more (optional)
 *          n x m3dp_t each material contains propapbly more, but at least one property
 *                     the properties are configurable with a static array, see m3d_propertytypes
 *      n x m3dchunk_t at least one, but maybe more face chunks
 *          PROC* procedural face, or
 *          MESH* triangle mesh (vertex index list)
 *      ACTN* action chunk(s), animation-pose skeletons, can be more (optional)
 *          n x m3dfr_t each action contains probably more, but at least one frame
 *              n x m3dtr_t each frame contains probably more, but at least one transformation
 *      ASET* inlined asset chunk(s), can be more (optional)
 *  OMD3 end chunk
 */
typedef struct {
    char magic[4];
    uint32_t length;
    float scale; /* deliberately not M3D_FLOAT */
    uint32_t types;
} _pack m3dhdr_t;

typedef struct {
    char magic[4];
    uint32_t length;
} _pack m3dchunk_t;

/*** in-memory model structure ***/

/* textmap entry */
typedef struct {
    M3D_FLOAT u;
    M3D_FLOAT v;
} m3dti_t;

/* texture */
typedef struct {
    char *name;                 /* texture name */
    uint32_t *d;                /* pixels data */
    uint16_t w;                 /* width */
    uint16_t h;                 /* height */
} _pack m3dtx_t;

typedef struct {
    M3D_INDEX vertexid;
    M3D_FLOAT weight;
} m3dw_t;

/* bone entry */
typedef struct {
    M3D_INDEX parent;           /* parent bone index */
    char *name;                 /* name for this bone */
    M3D_INDEX pos;              /* vertex index position */
    M3D_INDEX ori;              /* vertex index orientation (quaternion) */
    M3D_INDEX numweight;        /* number of controlled vertices */
    m3dw_t *weight;             /* weights for those vertices */
    M3D_FLOAT mat4[16];         /* transformation matrix */
} m3db_t;

/* skin: bone per vertex entry */
typedef struct {
    M3D_INDEX boneid[M3D_NUMBONE];
    M3D_FLOAT weight[M3D_NUMBONE];
} m3ds_t;

/* vertex entry */
typedef struct {
    M3D_FLOAT x;                /* 3D coordinates and weight */
    M3D_FLOAT y;
    M3D_FLOAT z;
    M3D_FLOAT w;
    uint32_t color;             /* default vertex color */
    M3D_INDEX skinid;           /* skin index */
} m3dv_t;

/* material property formats */
enum {
    m3dpf_color,
    m3dpf_uint8,
    m3dpf_uint16,
    m3dpf_uint32,
    m3dpf_float,
    m3dpf_map
};
typedef struct {
    uint8_t format;
    uint8_t id;
#ifdef M3D_ASCII
#define M3D_PROPERTYDEF(f,i,n) { (f), (i), (char*)(n) }
    char *key;
#else
#define M3D_PROPERTYDEF(f,i,n) { (f), (i) }
#endif
} m3dpd_t;

/* material property types */
/* You shouldn't change the first 8 display and first 4 physical property. Assign the rest as you like. */
enum {
    m3dp_Kd = 0,                /* scalar display properties */
    m3dp_Ka,
    m3dp_Ks,
    m3dp_Ns,
    m3dp_Ke,
    m3dp_Tf,
    m3dp_Km,
    m3dp_d,
    m3dp_il,

    m3dp_Pr = 64,               /* scalar physical properties */
    m3dp_Pm,
    m3dp_Ps,
    m3dp_Ni,

    m3dp_map_Kd = 128,          /* textured display map properties */
    m3dp_map_Ka,
    m3dp_map_Ks,
    m3dp_map_Ns,
    m3dp_map_Ke,
    m3dp_map_Tf,
    m3dp_map_Km, /* bump map */
    m3dp_map_D,
    m3dp_map_il, /* reflection map */

    m3dp_map_Pr = 192,          /* textured physical map properties */
    m3dp_map_Pm,
    m3dp_map_Ps,
    m3dp_map_Ni
};
enum {                          /* aliases */
    m3dp_bump = m3dp_map_Km,
    m3dp_refl = m3dp_map_Pm
};

/* material property */
typedef struct {
    uint8_t type;               /* property type, see "m3dp_*" enumeration */
    union {
        uint32_t color;         /* if value is a color, m3dpf_color */
        uint32_t num;           /* if value is a number, m3dpf_uint8, m3pf_uint16, m3dpf_uint32 */
        float    fnum;          /* if value is a floating point number, m3dpf_float */
        M3D_INDEX textureid;    /* if value is a texture, m3dpf_map */
    } value;
} m3dp_t;

/* material entry */
typedef struct {
    char *name;                 /* name of the material */
    uint8_t numprop;            /* number of properties */
    m3dp_t *prop;               /* properties array */
} m3dm_t;

/* face entry */
typedef struct {
    M3D_INDEX materialid;       /* material index */
    M3D_INDEX vertex[3];        /* 3D points of the triangle in CCW order */
    M3D_INDEX normal[3];        /* normal vectors */
    M3D_INDEX texcoord[3];      /* UV coordinates */
} m3df_t;

/* frame transformations / working copy skeleton entry */
typedef struct {
    M3D_INDEX boneid;           /* selects a node in bone hierarchy */
    M3D_INDEX pos;              /* vertex index new position */
    M3D_INDEX ori;              /* vertex index new orientation (quaternion) */
} m3dtr_t;

/* animation frame entry */
typedef struct {
    uint32_t msec;              /* frame's position on the timeline, timestamp */
    M3D_INDEX numtransform;     /* number of transformations in this frame */
    m3dtr_t *transform;         /* transformations */
} m3dfr_t;

/* model action entry */
typedef struct {
    char *name;                 /* name of the action */
    uint32_t durationmsec;      /* duration in millisec (1/1000 sec) */
    M3D_INDEX numframe;         /* number of frames in this animation */
    m3dfr_t *frame;             /* frames array */
} m3da_t;

/* inlined asset */
typedef struct {
    char *name;                 /* asset name (same pointer as in texture[].name) */
    uint8_t *data;              /* compressed asset data */
    uint32_t length;            /* compressed data length */
} m3di_t;

/*** in-memory model structure ***/
#define M3D_FLG_FREERAW     (1<<0)
#define M3D_FLG_FREESTR     (1<<1)
#define M3D_FLG_MTLLIB      (1<<2)

typedef struct {
    m3dhdr_t *raw;              /* pointer to raw data */
    char flags;                 /* internal flags */
    char errcode;               /* returned error code */
    char vc_s, vi_s, si_s, ci_s, ti_s, bi_s, nb_s, sk_s, fi_s;  /* decoded sizes for types */
    char *name;                 /* name of the model, like "Utah teapot" */
    char *license;              /* usage condition or license, like "MIT", "LGPL" or "BSD-3clause" */
    char *author;               /* nickname, email, homepage or github URL etc. */
    char *desc;                 /* comments, descriptions. May contain '\n' newline character */
    M3D_FLOAT scale;            /* the model's bounding cube's size in SI meters */
    M3D_INDEX numcmap;
    uint32_t *cmap;             /* color map */
    M3D_INDEX numtmap;
    m3dti_t *tmap;              /* texture map indices */
    M3D_INDEX numtexture;
    m3dtx_t *texture;           /* uncompressed textures */
    M3D_INDEX numbone;
    m3db_t *bone;               /* bone hierarchy */
    M3D_INDEX numvertex;
    m3dv_t *vertex;             /* vertex data */
    M3D_INDEX numskin;
    m3ds_t *skin;               /* skin data */
    M3D_INDEX nummaterial;
    m3dm_t *material;           /* material list */
    M3D_INDEX numface;
    m3df_t *face;               /* model face, triangle mesh */
    M3D_INDEX numaction;
    m3da_t *action;             /* action animations */
    M3D_INDEX numinlined;
    m3di_t *inlined;            /* inlined assets */
    M3D_INDEX numunknown;
    m3dchunk_t **unknown;       /* unknown chunks, application / engine specific data probably */
} m3d_t;

/*** export parameters ***/
#define M3D_EXP_INT8        0
#define M3D_EXP_INT16       1
#define M3D_EXP_FLOAT       2
#define M3D_EXP_DOUBLE      3

#define M3D_EXP_NOCMAP      (1<<0)
#define M3D_EXP_NOMATERIAL  (1<<1)
#define M3D_EXP_NOFACE      (1<<2)
#define M3D_EXP_NONORMAL    (1<<3)
#define M3D_EXP_NOTXTCRD    (1<<4)
#define M3D_EXP_FLIPTXTCRD  (1<<5)
#define M3D_EXP_NORECALC    (1<<6)
#define M3D_EXP_IDOSUCK     (1<<7)
#define M3D_EXP_NOBONE      (1<<8)
#define M3D_EXP_NOACTION    (1<<9)
#define M3D_EXP_INLINE      (1<<10)
#define M3D_EXP_EXTRA       (1<<11)
#define M3D_EXP_NOZLIB      (1<<14)
#define M3D_EXP_ASCII       (1<<15)

/*** error codes ***/
#define M3D_SUCCESS         0
#define M3D_ERR_ALLOC       -1
#define M3D_ERR_BADFILE     -2
#define M3D_ERR_UNIMPL      -65
#define M3D_ERR_UNKPROP     -66
#define M3D_ERR_UNKMESH     -67
#define M3D_ERR_UNKIMG      -68
#define M3D_ERR_UNKFRAME    -69
#define M3D_ERR_TRUNC       -70
#define M3D_ERR_CMAP        -71
#define M3D_ERR_TMAP        -72
#define M3D_ERR_VRTS        -73
#define M3D_ERR_BONE        -74
#define M3D_ERR_MTRL        -75

#define M3D_ERR_ISFATAL(x)  ((x) < 0 && (x) > -65)

/* callbacks */
typedef unsigned char *(*m3dread_t)(char *filename, unsigned int *size);                        /* read file contents into buffer */
typedef void (*m3dfree_t)(void *buffer);                                                        /* free file contents buffer */
typedef int (*m3dtxsc_t)(const char *name, const void *script, uint32_t len, m3dtx_t *output);  /* interpret texture script */
typedef int (*m3dprsc_t)(const char *name, const void *script, uint32_t len, m3d_t *model);     /* interpret surface script */
#endif /* ifndef M3D_APIVERSION */

/*** C prototypes ***/
/* import / export */
m3d_t *m3d_load(unsigned char *data, m3dread_t readfilecb, m3dfree_t freecb, m3d_t *mtllib);
unsigned char *m3d_save(m3d_t *model, int quality, int flags, unsigned int *size);
void m3d_free(m3d_t *model);
/* generate animation pose skeleton */
m3dtr_t *m3d_frame(m3d_t *model, M3D_INDEX actionid, M3D_INDEX frameid, m3dtr_t *skeleton);
m3db_t *m3d_pose(m3d_t *model, M3D_INDEX actionid, uint32_t msec);

/* private prototypes used by both importer and exporter */
m3ds_t *_m3d_addskin(m3ds_t *skin, uint32_t *numskin, m3ds_t *s, uint32_t *idx);
m3dv_t *_m3d_addnorm(m3dv_t *vrtx, uint32_t *numvrtx, m3dv_t *v, uint32_t *idx);
char *_m3d_safestr(char *in, int morelines);

/*** C implementation ***/
#ifdef M3D_IMPLEMENTATION
#if !defined(M3D_NOIMPORTER) || defined(M3D_EXPORTER)
/* material property definitions */
static m3dpd_t m3d_propertytypes[] = {
    M3D_PROPERTYDEF(m3dpf_color, m3dp_Kd, "Kd"),    /* diffuse color */
    M3D_PROPERTYDEF(m3dpf_color, m3dp_Ka, "Ka"),    /* ambient color */
    M3D_PROPERTYDEF(m3dpf_color, m3dp_Ks, "Ks"),    /* specular color */
    M3D_PROPERTYDEF(m3dpf_float, m3dp_Ns, "Ns"),    /* specular exponent */
    M3D_PROPERTYDEF(m3dpf_color, m3dp_Ke, "Ke"),    /* emissive (emitting light of this color) */
    M3D_PROPERTYDEF(m3dpf_color, m3dp_Tf, "Tf"),    /* transmission color */
    M3D_PROPERTYDEF(m3dpf_float, m3dp_Km, "Km"),    /* bump strength */
    M3D_PROPERTYDEF(m3dpf_float, m3dp_d,  "d"),     /* dissolve (transparency) */
    M3D_PROPERTYDEF(m3dpf_uint8, m3dp_il, "il"),    /* illumination model (informational, ignored by PBR-shaders) */

    M3D_PROPERTYDEF(m3dpf_float, m3dp_Pr, "Pr"),    /* roughness */
    M3D_PROPERTYDEF(m3dpf_float, m3dp_Pm, "Pm"),    /* metallic, also reflection */
    M3D_PROPERTYDEF(m3dpf_float, m3dp_Ps, "Ps"),    /* sheen */
    M3D_PROPERTYDEF(m3dpf_float, m3dp_Ni, "Ni"),    /* index of refraction (optical density) */

    /* aliases, note that "map_*" aliases are handled automatically */
    M3D_PROPERTYDEF(m3dpf_map, m3dp_map_Km, "bump"),
    M3D_PROPERTYDEF(m3dpf_map, m3dp_map_Pm, "refl")
};
#endif

#include <stdlib.h>
#include <string.h>

#if !defined(M3D_NOIMPORTER) && !defined(STBI_INCLUDE_STB_IMAGE_H)
/* PNG decompressor from

   stb_image - v2.23 - public domain image loader - http://nothings.org/stb_image.h
*/
static const char *stbi__g_failure_reason;

enum
{
   STBI_default = 0,

   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4
};

enum
{
   STBI__SCAN_load=0,
   STBI__SCAN_type,
   STBI__SCAN_header
};

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

typedef uint16_t stbi__uint16;
typedef int16_t  stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t  stbi__int32;

typedef struct
{
   stbi__uint32 img_x, img_y;
   int img_n, img_out_n;

   void *io_user_data;

   int read_from_callbacks;
   int buflen;
   stbi_uc buffer_start[128];

   stbi_uc *img_buffer, *img_buffer_end;
   stbi_uc *img_buffer_original, *img_buffer_original_end;
} stbi__context;

typedef struct
{
   int bits_per_channel;
   int num_channels;
   int channel_order;
} stbi__result_info;

#define STBI_ASSERT(v)
#define STBI_NOTUSED(v)  (void)sizeof(v)
#define STBI__BYTECAST(x)  ((stbi_uc) ((x) & 255))
#define STBI_MALLOC(sz)           M3D_MALLOC(sz)
#define STBI_REALLOC(p,newsz)     M3D_REALLOC(p,newsz)
#define STBI_FREE(p)              M3D_FREE(p)
#define STBI_REALLOC_SIZED(p,oldsz,newsz) STBI_REALLOC(p,newsz)

_inline static stbi_uc stbi__get8(stbi__context *s)
{
   if (s->img_buffer < s->img_buffer_end)
      return *s->img_buffer++;
   return 0;
}

_inline static int stbi__at_eof(stbi__context *s)
{
   return s->img_buffer >= s->img_buffer_end;
}

static void stbi__skip(stbi__context *s, int n)
{
   if (n < 0) {
      s->img_buffer = s->img_buffer_end;
      return;
   }
   s->img_buffer += n;
}

static int stbi__getn(stbi__context *s, stbi_uc *buffer, int n)
{
   if (s->img_buffer+n <= s->img_buffer_end) {
      memcpy(buffer, s->img_buffer, n);
      s->img_buffer += n;
      return 1;
   } else
      return 0;
}

static int stbi__get16be(stbi__context *s)
{
   int z = stbi__get8(s);
   return (z << 8) + stbi__get8(s);
}

static stbi__uint32 stbi__get32be(stbi__context *s)
{
   stbi__uint32 z = stbi__get16be(s);
   return (z << 16) + stbi__get16be(s);
}

#define stbi__err(x,y)  stbi__errstr(y)
static int stbi__errstr(const char *str)
{
   stbi__g_failure_reason = str;
   return 0;
}

_inline static void *stbi__malloc(size_t size)
{
    return STBI_MALLOC(size);
}

static int stbi__addsizes_valid(int a, int b)
{
   if (b < 0) return 0;
   return a <= 2147483647 - b;
}

static int stbi__mul2sizes_valid(int a, int b)
{
   if (a < 0 || b < 0) return 0;
   if (b == 0) return 1;
   return a <= 2147483647/b;
}

static int stbi__mad2sizes_valid(int a, int b, int add)
{
   return stbi__mul2sizes_valid(a, b) && stbi__addsizes_valid(a*b, add);
}

static int stbi__mad3sizes_valid(int a, int b, int c, int add)
{
   return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a*b, c) &&
      stbi__addsizes_valid(a*b*c, add);
}

static void *stbi__malloc_mad2(int a, int b, int add)
{
   if (!stbi__mad2sizes_valid(a, b, add)) return NULL;
   return stbi__malloc(a*b + add);
}

static void *stbi__malloc_mad3(int a, int b, int c, int add)
{
   if (!stbi__mad3sizes_valid(a, b, c, add)) return NULL;
   return stbi__malloc(a*b*c + add);
}

static stbi_uc stbi__compute_y(int r, int g, int b)
{
   return (stbi_uc) (((r*77) + (g*150) +  (29*b)) >> 8);
}

static unsigned char *stbi__convert_format(unsigned char *data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
   int i,j;
   unsigned char *good;

   if (req_comp == img_n) return data;
   STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

   good = (unsigned char *) stbi__malloc_mad3(req_comp, x, y, 0);
   if (good == NULL) {
      STBI_FREE(data);
      stbi__err("outofmem", "Out of memory");
      return NULL;
   }

   for (j=0; j < (int) y; ++j) {
      unsigned char *src  = data + j * x * img_n   ;
      unsigned char *dest = good + j * x * req_comp;

      #define STBI__COMBO(a,b)  ((a)*8+(b))
      #define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      switch (STBI__COMBO(img_n, req_comp)) {
         STBI__CASE(1,2) { dest[0]=src[0], dest[1]=255;                                     } break;
         STBI__CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                                  } break;
         STBI__CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0], dest[3]=255;                     } break;
         STBI__CASE(2,1) { dest[0]=src[0];                                                  } break;
         STBI__CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                                  } break;
         STBI__CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0], dest[3]=src[1];                  } break;
         STBI__CASE(3,4) { dest[0]=src[0],dest[1]=src[1],dest[2]=src[2],dest[3]=255;        } break;
         STBI__CASE(3,1) { dest[0]=stbi__compute_y(src[0],src[1],src[2]);                   } break;
         STBI__CASE(3,2) { dest[0]=stbi__compute_y(src[0],src[1],src[2]), dest[1] = 255;    } break;
         STBI__CASE(4,1) { dest[0]=stbi__compute_y(src[0],src[1],src[2]);                   } break;
         STBI__CASE(4,2) { dest[0]=stbi__compute_y(src[0],src[1],src[2]), dest[1] = src[3]; } break;
         STBI__CASE(4,3) { dest[0]=src[0],dest[1]=src[1],dest[2]=src[2];                    } break;
         default: STBI_ASSERT(0);
      }
      #undef STBI__CASE
   }

   STBI_FREE(data);
   return good;
}

static stbi__uint16 stbi__compute_y_16(int r, int g, int b)
{
   return (stbi__uint16) (((r*77) + (g*150) +  (29*b)) >> 8);
}

static stbi__uint16 *stbi__convert_format16(stbi__uint16 *data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
   int i,j;
   stbi__uint16 *good;

   if (req_comp == img_n) return data;
   STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

   good = (stbi__uint16 *) stbi__malloc(req_comp * x * y * 2);
   if (good == NULL) {
      STBI_FREE(data);
      stbi__err("outofmem", "Out of memory");
      return NULL;
   }

   for (j=0; j < (int) y; ++j) {
      stbi__uint16 *src  = data + j * x * img_n   ;
      stbi__uint16 *dest = good + j * x * req_comp;

      #define STBI__COMBO(a,b)  ((a)*8+(b))
      #define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      switch (STBI__COMBO(img_n, req_comp)) {
         STBI__CASE(1,2) { dest[0]=src[0], dest[1]=0xffff;                                     } break;
         STBI__CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                                     } break;
         STBI__CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0], dest[3]=0xffff;                     } break;
         STBI__CASE(2,1) { dest[0]=src[0];                                                     } break;
         STBI__CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                                     } break;
         STBI__CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0], dest[3]=src[1];                     } break;
         STBI__CASE(3,4) { dest[0]=src[0],dest[1]=src[1],dest[2]=src[2],dest[3]=0xffff;        } break;
         STBI__CASE(3,1) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]);                   } break;
         STBI__CASE(3,2) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]), dest[1] = 0xffff; } break;
         STBI__CASE(4,1) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]);                   } break;
         STBI__CASE(4,2) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]), dest[1] = src[3]; } break;
         STBI__CASE(4,3) { dest[0]=src[0],dest[1]=src[1],dest[2]=src[2];                       } break;
         default: STBI_ASSERT(0);
      }
      #undef STBI__CASE
   }

   STBI_FREE(data);
   return good;
}

#define STBI__ZFAST_BITS  9
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

typedef struct
{
   stbi__uint16 fast[1 << STBI__ZFAST_BITS];
   stbi__uint16 firstcode[16];
   int maxcode[17];
   stbi__uint16 firstsymbol[16];
   stbi_uc  size[288];
   stbi__uint16 value[288];
} stbi__zhuffman;

_inline static int stbi__bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

_inline static int stbi__bit_reverse(int v, int bits)
{
   STBI_ASSERT(bits <= 16);
   return stbi__bitreverse16(v) >> (16-bits);
}

static int stbi__zbuild_huffman(stbi__zhuffman *z, stbi_uc *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 0, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      if (sizes[i] > (1 << i))
         return stbi__err("bad sizes", "Corrupt PNG");
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (stbi__uint16) code;
      z->firstsymbol[i] = (stbi__uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return stbi__err("bad codelengths","Corrupt PNG");
      z->maxcode[i] = code << (16-i);
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000;
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         stbi__uint16 fastv = (stbi__uint16) ((s << 9) | i);
         z->size [c] = (stbi_uc     ) s;
         z->value[c] = (stbi__uint16) i;
         if (s <= STBI__ZFAST_BITS) {
            int j = stbi__bit_reverse(next_code[s],s);
            while (j < (1 << STBI__ZFAST_BITS)) {
               z->fast[j] = fastv;
               j += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

typedef struct
{
   stbi_uc *zbuffer, *zbuffer_end;
   int num_bits;
   stbi__uint32 code_buffer;

   char *zout;
   char *zout_start;
   char *zout_end;
   int   z_expandable;

   stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

_inline static stbi_uc stbi__zget8(stbi__zbuf *z)
{
   if (z->zbuffer >= z->zbuffer_end) return 0;
   return *z->zbuffer++;
}

static void stbi__fill_bits(stbi__zbuf *z)
{
   do {
      STBI_ASSERT(z->code_buffer < (1U << z->num_bits));
      z->code_buffer |= (unsigned int) stbi__zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

_inline static unsigned int stbi__zreceive(stbi__zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) stbi__fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, stbi__zhuffman *z)
{
   int b,s,k;
   k = stbi__bit_reverse(a->code_buffer, 16);
   for (s=STBI__ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1;
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   STBI_ASSERT(z->size[b] == s);
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

_inline static int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z)
{
   int b,s;
   if (a->num_bits < 16) stbi__fill_bits(a);
   b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
   if (b) {
      s = b >> 9;
      a->code_buffer >>= s;
      a->num_bits -= s;
      return b & 511;
   }
   return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zexpand(stbi__zbuf *z, char *zout, int n)
{
   char *q;
   int cur, limit, old_limit;
   z->zout = zout;
   if (!z->z_expandable) return stbi__err("output buffer limit","Corrupt PNG");
   cur   = (int) (z->zout     - z->zout_start);
   limit = old_limit = (int) (z->zout_end - z->zout_start);
   while (cur + n > limit)
      limit *= 2;
   q = (char *) STBI_REALLOC_SIZED(z->zout_start, old_limit, limit);
   STBI_NOTUSED(old_limit);
   if (q == NULL) return stbi__err("outofmem", "Out of memory");
   z->zout_start = q;
   z->zout       = q + cur;
   z->zout_end   = q + limit;
   return 1;
}

static int stbi__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static int stbi__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static int stbi__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static int stbi__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int stbi__parse_huffman_block(stbi__zbuf *a)
{
   char *zout = a->zout;
   for(;;) {
      int z = stbi__zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return stbi__err("bad huffman code","Corrupt PNG");
         if (zout >= a->zout_end) {
            if (!stbi__zexpand(a, zout, 1)) return 0;
            zout = a->zout;
         }
         *zout++ = (char) z;
      } else {
         stbi_uc *p;
         int len,dist;
         if (z == 256) {
            a->zout = zout;
            return 1;
         }
         z -= 257;
         len = stbi__zlength_base[z];
         if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
         z = stbi__zhuffman_decode(a, &a->z_distance);
         if (z < 0) return stbi__err("bad huffman code","Corrupt PNG");
         dist = stbi__zdist_base[z];
         if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);
         if (zout - a->zout_start < dist) return stbi__err("bad dist","Corrupt PNG");
         if (zout + len > a->zout_end) {
            if (!stbi__zexpand(a, zout, len)) return 0;
            zout = a->zout;
         }
         p = (stbi_uc *) (zout - dist);
         if (dist == 1) {
            stbi_uc v = *p;
            if (len) { do *zout++ = v; while (--len); }
         } else {
            if (len) { do *zout++ = *p++; while (--len); }
         }
      }
   }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a)
{
   static stbi_uc length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   stbi__zhuffman z_codelength;
   stbi_uc lencodes[286+32+137];
   stbi_uc codelength_sizes[19];
   int i,n;

   int hlit  = stbi__zreceive(a,5) + 257;
   int hdist = stbi__zreceive(a,5) + 1;
   int hclen = stbi__zreceive(a,4) + 4;
   int ntot  = hlit + hdist;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = stbi__zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (stbi_uc) s;
   }
   if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < ntot) {
      int c = stbi__zhuffman_decode(a, &z_codelength);
      if (c < 0 || c >= 19) return stbi__err("bad codelengths", "Corrupt PNG");
      if (c < 16)
         lencodes[n++] = (stbi_uc) c;
      else {
         stbi_uc fill = 0;
         if (c == 16) {
            c = stbi__zreceive(a,2)+3;
            if (n == 0) return stbi__err("bad codelengths", "Corrupt PNG");
            fill = lencodes[n-1];
         } else if (c == 17)
            c = stbi__zreceive(a,3)+3;
         else {
            STBI_ASSERT(c == 18);
            c = stbi__zreceive(a,7)+11;
         }
         if (ntot - n < c) return stbi__err("bad codelengths", "Corrupt PNG");
         memset(lencodes+n, fill, c);
         n += c;
      }
   }
   if (n != ntot) return stbi__err("bad codelengths","Corrupt PNG");
   if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!stbi__zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

_inline static int stbi__parse_uncompressed_block(stbi__zbuf *a)
{
   stbi_uc header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      stbi__zreceive(a, a->num_bits & 7);
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (stbi_uc) (a->code_buffer & 255);
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   STBI_ASSERT(a->num_bits == 0);
   while (k < 4)
      header[k++] = stbi__zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return stbi__err("zlib corrupt","Corrupt PNG");
   if (a->zbuffer + len > a->zbuffer_end) return stbi__err("read past buffer","Corrupt PNG");
   if (a->zout + len > a->zout_end)
      if (!stbi__zexpand(a, a->zout, len)) return 0;
   memcpy(a->zout, a->zbuffer, len);
   a->zbuffer += len;
   a->zout += len;
   return 1;
}

static int stbi__parse_zlib_header(stbi__zbuf *a)
{
   int cmf   = stbi__zget8(a);
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = stbi__zget8(a);
   if ((cmf*256+flg) % 31 != 0) return stbi__err("bad zlib header","Corrupt PNG");
   if (flg & 32) return stbi__err("no preset dict","Corrupt PNG");
   if (cm != 8) return stbi__err("bad compression","Corrupt PNG");
   return 1;
}

static stbi_uc stbi__zdefault_length[288], stbi__zdefault_distance[32];
static void stbi__init_zdefaults(void)
{
   int i;
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}

static int stbi__parse_zlib(stbi__zbuf *a, int parse_header)
{
   int final, type;
   if (parse_header)
      if (!stbi__parse_zlib_header(a)) return 0;
   a->num_bits = 0;
   a->code_buffer = 0;
   do {
      final = stbi__zreceive(a,1);
      type = stbi__zreceive(a,2);
      if (type == 0) {
         if (!stbi__parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            if (!stbi__zdefault_distance[31]) stbi__init_zdefaults();
            if (!stbi__zbuild_huffman(&a->z_length  , stbi__zdefault_length  , 288)) return 0;
            if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance,  32)) return 0;
         } else {
            if (!stbi__compute_huffman_codes(a)) return 0;
         }
         if (!stbi__parse_huffman_block(a)) return 0;
      }
   } while (!final);
   return 1;
}

static int stbi__do_zlib(stbi__zbuf *a, char *obuf, int olen, int exp, int parse_header)
{
   a->zout_start = obuf;
   a->zout       = obuf;
   a->zout_end   = obuf + olen;
   a->z_expandable = exp;

   return stbi__parse_zlib(a, parse_header);
}

char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
{
   stbi__zbuf a;
   char *p = (char *) stbi__malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (stbi_uc *) buffer;
   a.zbuffer_end = (stbi_uc *) buffer + len;
   if (stbi__do_zlib(&a, p, initial_size, 1, parse_header)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      STBI_FREE(a.zout_start);
      return NULL;
   }
}

typedef struct
{
   stbi__uint32 length;
   stbi__uint32 type;
} stbi__pngchunk;

static stbi__pngchunk stbi__get_chunk_header(stbi__context *s)
{
   stbi__pngchunk c;
   c.length = stbi__get32be(s);
   c.type   = stbi__get32be(s);
   return c;
}

_inline static int stbi__check_png_header(stbi__context *s)
{
   static stbi_uc png_sig[8] = { 137,80,78,71,13,10,26,10 };
   int i;
   for (i=0; i < 8; ++i)
      if (stbi__get8(s) != png_sig[i]) return stbi__err("bad png sig","Not a PNG");
   return 1;
}

typedef struct
{
   stbi__context *s;
   stbi_uc *idata, *expanded, *out;
   int depth;
} stbi__png;


enum {
   STBI__F_none=0,
   STBI__F_sub=1,
   STBI__F_up=2,
   STBI__F_avg=3,
   STBI__F_paeth=4,
   STBI__F_avg_first,
   STBI__F_paeth_first
};

static stbi_uc first_row_filter[5] =
{
   STBI__F_none,
   STBI__F_sub,
   STBI__F_none,
   STBI__F_avg_first,
   STBI__F_paeth_first
};

static int stbi__paeth(int a, int b, int c)
{
   int p = a + b - c;
   int pa = abs(p-a);
   int pb = abs(p-b);
   int pc = abs(p-c);
   if (pa <= pb && pa <= pc) return a;
   if (pb <= pc) return b;
   return c;
}

static stbi_uc stbi__depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0,0,0, 0x01 };

static int stbi__create_png_image_raw(stbi__png *a, stbi_uc *raw, stbi__uint32 raw_len, int out_n, stbi__uint32 x, stbi__uint32 y, int depth, int color)
{
   int bytes = (depth == 16? 2 : 1);
   stbi__context *s = a->s;
   stbi__uint32 i,j,stride = x*out_n*bytes;
   stbi__uint32 img_len, img_width_bytes;
   int k;
   int img_n = s->img_n;

   int output_bytes = out_n*bytes;
   int filter_bytes = img_n*bytes;
   int width = x;

   STBI_ASSERT(out_n == s->img_n || out_n == s->img_n+1);
   a->out = (stbi_uc *) stbi__malloc_mad3(x, y, output_bytes, 0);
   if (!a->out) return stbi__err("outofmem", "Out of memory");

   img_width_bytes = (((img_n * x * depth) + 7) >> 3);
   img_len = (img_width_bytes + 1) * y;
   if (s->img_x == x && s->img_y == y) {
      if (raw_len != img_len) return stbi__err("not enough pixels","Corrupt PNG");
   } else {
      if (raw_len < img_len) return stbi__err("not enough pixels","Corrupt PNG");
   }

   for (j=0; j < y; ++j) {
      stbi_uc *cur = a->out + stride*j;
      stbi_uc *prior = cur - stride;
      int filter = *raw++;

      if (filter > 4)
         return stbi__err("invalid filter","Corrupt PNG");

      if (depth < 8) {
         STBI_ASSERT(img_width_bytes <= x);
         cur += x*out_n - img_width_bytes;
         filter_bytes = 1;
         width = img_width_bytes;
      }

      if (j == 0) filter = first_row_filter[filter];

      for (k=0; k < filter_bytes; ++k) {
         switch (filter) {
            case STBI__F_none       : cur[k] = raw[k]; break;
            case STBI__F_sub        : cur[k] = raw[k]; break;
            case STBI__F_up         : cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
            case STBI__F_avg        : cur[k] = STBI__BYTECAST(raw[k] + (prior[k]>>1)); break;
            case STBI__F_paeth      : cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(0,prior[k],0)); break;
            case STBI__F_avg_first  : cur[k] = raw[k]; break;
            case STBI__F_paeth_first: cur[k] = raw[k]; break;
         }
      }

      if (depth == 8) {
         if (img_n != out_n)
            cur[img_n] = 255;
         raw += img_n;
         cur += out_n;
         prior += out_n;
      } else if (depth == 16) {
         if (img_n != out_n) {
            cur[filter_bytes]   = 255;
            cur[filter_bytes+1] = 255;
         }
         raw += filter_bytes;
         cur += output_bytes;
         prior += output_bytes;
      } else {
         raw += 1;
         cur += 1;
         prior += 1;
      }

      if (depth < 8 || img_n == out_n) {
         int nk = (width - 1)*filter_bytes;
         #define STBI__CASE(f) \
             case f:     \
                for (k=0; k < nk; ++k)
         switch (filter) {
            case STBI__F_none:         memcpy(cur, raw, nk); break;
            STBI__CASE(STBI__F_sub)          { cur[k] = STBI__BYTECAST(raw[k] + cur[k-filter_bytes]); } break;
            STBI__CASE(STBI__F_up)           { cur[k] = STBI__BYTECAST(raw[k] + prior[k]); } break;
            STBI__CASE(STBI__F_avg)          { cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k-filter_bytes])>>1)); } break;
            STBI__CASE(STBI__F_paeth)        { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k-filter_bytes],prior[k],prior[k-filter_bytes])); } break;
            STBI__CASE(STBI__F_avg_first)    { cur[k] = STBI__BYTECAST(raw[k] + (cur[k-filter_bytes] >> 1)); } break;
            STBI__CASE(STBI__F_paeth_first)  { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k-filter_bytes],0,0)); } break;
         }
         #undef STBI__CASE
         raw += nk;
      } else {
         STBI_ASSERT(img_n+1 == out_n);
         #define STBI__CASE(f) \
             case f:     \
                for (i=x-1; i >= 1; --i, cur[filter_bytes]=255,raw+=filter_bytes,cur+=output_bytes,prior+=output_bytes) \
                   for (k=0; k < filter_bytes; ++k)
         switch (filter) {
            STBI__CASE(STBI__F_none)         { cur[k] = raw[k]; } break;
            STBI__CASE(STBI__F_sub)          { cur[k] = STBI__BYTECAST(raw[k] + cur[k- output_bytes]); } break;
            STBI__CASE(STBI__F_up)           { cur[k] = STBI__BYTECAST(raw[k] + prior[k]); } break;
            STBI__CASE(STBI__F_avg)          { cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k- output_bytes])>>1)); } break;
            STBI__CASE(STBI__F_paeth)        { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k- output_bytes],prior[k],prior[k- output_bytes])); } break;
            STBI__CASE(STBI__F_avg_first)    { cur[k] = STBI__BYTECAST(raw[k] + (cur[k- output_bytes] >> 1)); } break;
            STBI__CASE(STBI__F_paeth_first)  { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k- output_bytes],0,0)); } break;
         }
         #undef STBI__CASE

         if (depth == 16) {
            cur = a->out + stride*j;
            for (i=0; i < x; ++i,cur+=output_bytes) {
               cur[filter_bytes+1] = 255;
            }
         }
      }
   }

   if (depth < 8) {
      for (j=0; j < y; ++j) {
         stbi_uc *cur = a->out + stride*j;
         stbi_uc *in  = a->out + stride*j + x*out_n - img_width_bytes;
         stbi_uc scale = (color == 0) ? stbi__depth_scale_table[depth] : 1;

         if (depth == 4) {
            for (k=x*img_n; k >= 2; k-=2, ++in) {
               *cur++ = scale * ((*in >> 4)       );
               *cur++ = scale * ((*in     ) & 0x0f);
            }
            if (k > 0) *cur++ = scale * ((*in >> 4)       );
         } else if (depth == 2) {
            for (k=x*img_n; k >= 4; k-=4, ++in) {
               *cur++ = scale * ((*in >> 6)       );
               *cur++ = scale * ((*in >> 4) & 0x03);
               *cur++ = scale * ((*in >> 2) & 0x03);
               *cur++ = scale * ((*in     ) & 0x03);
            }
            if (k > 0) *cur++ = scale * ((*in >> 6)       );
            if (k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
            if (k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
         } else if (depth == 1) {
            for (k=x*img_n; k >= 8; k-=8, ++in) {
               *cur++ = scale * ((*in >> 7)       );
               *cur++ = scale * ((*in >> 6) & 0x01);
               *cur++ = scale * ((*in >> 5) & 0x01);
               *cur++ = scale * ((*in >> 4) & 0x01);
               *cur++ = scale * ((*in >> 3) & 0x01);
               *cur++ = scale * ((*in >> 2) & 0x01);
               *cur++ = scale * ((*in >> 1) & 0x01);
               *cur++ = scale * ((*in     ) & 0x01);
            }
            if (k > 0) *cur++ = scale * ((*in >> 7)       );
            if (k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
            if (k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
            if (k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
            if (k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
            if (k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
            if (k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
         }
         if (img_n != out_n) {
            int q;
            cur = a->out + stride*j;
            if (img_n == 1) {
               for (q=x-1; q >= 0; --q) {
                  cur[q*2+1] = 255;
                  cur[q*2+0] = cur[q];
               }
            } else {
               STBI_ASSERT(img_n == 3);
               for (q=x-1; q >= 0; --q) {
                  cur[q*4+3] = 255;
                  cur[q*4+2] = cur[q*3+2];
                  cur[q*4+1] = cur[q*3+1];
                  cur[q*4+0] = cur[q*3+0];
               }
            }
         }
      }
   } else if (depth == 16) {
      stbi_uc *cur = a->out;
      stbi__uint16 *cur16 = (stbi__uint16*)cur;

      for(i=0; i < x*y*out_n; ++i,cur16++,cur+=2) {
         *cur16 = (cur[0] << 8) | cur[1];
      }
   }

   return 1;
}

static int stbi__create_png_image(stbi__png *a, stbi_uc *image_data, stbi__uint32 image_data_len, int out_n, int depth, int color, int interlaced)
{
   int bytes = (depth == 16 ? 2 : 1);
   int out_bytes = out_n * bytes;
   stbi_uc *final;
   int p;
   if (!interlaced)
      return stbi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color);

   final = (stbi_uc *) stbi__malloc_mad3(a->s->img_x, a->s->img_y, out_bytes, 0);
   for (p=0; p < 7; ++p) {
      int xorig[] = { 0,4,0,2,0,1,0 };
      int yorig[] = { 0,0,4,0,2,0,1 };
      int xspc[]  = { 8,8,4,4,2,2,1 };
      int yspc[]  = { 8,8,8,4,4,2,2 };
      int i,j,x,y;
      x = (a->s->img_x - xorig[p] + xspc[p]-1) / xspc[p];
      y = (a->s->img_y - yorig[p] + yspc[p]-1) / yspc[p];
      if (x && y) {
         stbi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;
         if (!stbi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color)) {
            STBI_FREE(final);
            return 0;
         }
         for (j=0; j < y; ++j) {
            for (i=0; i < x; ++i) {
               int out_y = j*yspc[p]+yorig[p];
               int out_x = i*xspc[p]+xorig[p];
               memcpy(final + out_y*a->s->img_x*out_bytes + out_x*out_bytes,
                      a->out + (j*x+i)*out_bytes, out_bytes);
            }
         }
         STBI_FREE(a->out);
         image_data += img_len;
         image_data_len -= img_len;
      }
   }
   a->out = final;

   return 1;
}

static int stbi__compute_transparency(stbi__png *z, stbi_uc tc[3], int out_n)
{
   stbi__context *s = z->s;
   stbi__uint32 i, pixel_count = s->img_x * s->img_y;
   stbi_uc *p = z->out;

   STBI_ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i=0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 255);
         p += 2;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int stbi__compute_transparency16(stbi__png *z, stbi__uint16 tc[3], int out_n)
{
   stbi__context *s = z->s;
   stbi__uint32 i, pixel_count = s->img_x * s->img_y;
   stbi__uint16 *p = (stbi__uint16*) z->out;

   STBI_ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i = 0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 65535);
         p += 2;
      }
   } else {
      for (i = 0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int stbi__expand_png_palette(stbi__png *a, stbi_uc *palette, int len, int pal_img_n)
{
   stbi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
   stbi_uc *p, *temp_out, *orig = a->out;

   p = (stbi_uc *) stbi__malloc_mad2(pixel_count, pal_img_n, 0);
   if (p == NULL) return stbi__err("outofmem", "Out of memory");

   temp_out = p;

   if (pal_img_n == 3) {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p += 3;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p[3] = palette[n+3];
         p += 4;
      }
   }
   STBI_FREE(a->out);
   a->out = temp_out;

   STBI_NOTUSED(len);

   return 1;
}

static int stbi__unpremultiply_on_load = 0;
static int stbi__de_iphone_flag = 0;

void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
{
   stbi__unpremultiply_on_load = flag_true_if_should_unpremultiply;
}

void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
{
   stbi__de_iphone_flag = flag_true_if_should_convert;
}

static void stbi__de_iphone(stbi__png *z)
{
   stbi__context *s = z->s;
   stbi__uint32 i, pixel_count = s->img_x * s->img_y;
   stbi_uc *p = z->out;

   if (s->img_out_n == 3) {
      for (i=0; i < pixel_count; ++i) {
         stbi_uc t = p[0];
         p[0] = p[2];
         p[2] = t;
         p += 3;
      }
   } else {
      STBI_ASSERT(s->img_out_n == 4);
      if (stbi__unpremultiply_on_load) {
         for (i=0; i < pixel_count; ++i) {
            stbi_uc a = p[3];
            stbi_uc t = p[0];
            if (a) {
               p[0] = p[2] * 255 / a;
               p[1] = p[1] * 255 / a;
               p[2] =  t   * 255 / a;
            } else {
               p[0] = p[2];
               p[2] = t;
            }
            p += 4;
         }
      } else {
         for (i=0; i < pixel_count; ++i) {
            stbi_uc t = p[0];
            p[0] = p[2];
            p[2] = t;
            p += 4;
         }
      }
   }
}

#define STBI__PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

static int stbi__parse_png_file(stbi__png *z, int scan, int req_comp)
{
   stbi_uc palette[1024], pal_img_n=0;
   stbi_uc has_trans=0, tc[3];
   stbi__uint16 tc16[3];
   stbi__uint32 ioff=0, idata_limit=0, i, pal_len=0;
   int first=1,k,interlace=0, color=0, is_iphone=0;
   stbi__context *s = z->s;

   z->expanded = NULL;
   z->idata = NULL;
   z->out = NULL;

   if (!stbi__check_png_header(s)) return 0;

   if (scan == STBI__SCAN_type) return 1;

   for (;;) {
      stbi__pngchunk c = stbi__get_chunk_header(s);
      switch (c.type) {
         case STBI__PNG_TYPE('C','g','B','I'):
            is_iphone = 1;
            stbi__skip(s, c.length);
            break;
         case STBI__PNG_TYPE('I','H','D','R'): {
            int comp,filter;
            if (!first) return stbi__err("multiple IHDR","Corrupt PNG");
            first = 0;
            if (c.length != 13) return stbi__err("bad IHDR len","Corrupt PNG");
            s->img_x = stbi__get32be(s); if (s->img_x > (1 << 24)) return stbi__err("too large","Very large image (corrupt?)");
            s->img_y = stbi__get32be(s); if (s->img_y > (1 << 24)) return stbi__err("too large","Very large image (corrupt?)");
            z->depth = stbi__get8(s);  if (z->depth != 1 && z->depth != 2 && z->depth != 4 && z->depth != 8 && z->depth != 16)  return stbi__err("1/2/4/8/16-bit only","PNG not supported: 1/2/4/8/16-bit only");
            color = stbi__get8(s);  if (color > 6)         return stbi__err("bad ctype","Corrupt PNG");
            if (color == 3 && z->depth == 16)                  return stbi__err("bad ctype","Corrupt PNG");
            if (color == 3) pal_img_n = 3; else if (color & 1) return stbi__err("bad ctype","Corrupt PNG");
            comp  = stbi__get8(s);  if (comp) return stbi__err("bad comp method","Corrupt PNG");
            filter= stbi__get8(s);  if (filter) return stbi__err("bad filter method","Corrupt PNG");
            interlace = stbi__get8(s); if (interlace>1) return stbi__err("bad interlace method","Corrupt PNG");
            if (!s->img_x || !s->img_y) return stbi__err("0-pixel image","Corrupt PNG");
            if (!pal_img_n) {
               s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
               if ((1 << 30) / s->img_x / s->img_n < s->img_y) return stbi__err("too large", "Image too large to decode");
               if (scan == STBI__SCAN_header) return 1;
            } else {
               s->img_n = 1;
               if ((1 << 30) / s->img_x / 4 < s->img_y) return stbi__err("too large","Corrupt PNG");
            }
            break;
         }

         case STBI__PNG_TYPE('P','L','T','E'):  {
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (c.length > 256*3) return stbi__err("invalid PLTE","Corrupt PNG");
            pal_len = c.length / 3;
            if (pal_len * 3 != c.length) return stbi__err("invalid PLTE","Corrupt PNG");
            for (i=0; i < pal_len; ++i) {
               palette[i*4+0] = stbi__get8(s);
               palette[i*4+1] = stbi__get8(s);
               palette[i*4+2] = stbi__get8(s);
               palette[i*4+3] = 255;
            }
            break;
         }

         case STBI__PNG_TYPE('t','R','N','S'): {
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (z->idata) return stbi__err("tRNS after IDAT","Corrupt PNG");
            if (pal_img_n) {
               if (scan == STBI__SCAN_header) { s->img_n = 4; return 1; }
               if (pal_len == 0) return stbi__err("tRNS before PLTE","Corrupt PNG");
               if (c.length > pal_len) return stbi__err("bad tRNS len","Corrupt PNG");
               pal_img_n = 4;
               for (i=0; i < c.length; ++i)
                  palette[i*4+3] = stbi__get8(s);
            } else {
               if (!(s->img_n & 1)) return stbi__err("tRNS with alpha","Corrupt PNG");
               if (c.length != (stbi__uint32) s->img_n*2) return stbi__err("bad tRNS len","Corrupt PNG");
               has_trans = 1;
               if (z->depth == 16) {
                  for (k = 0; k < s->img_n; ++k) tc16[k] = (stbi__uint16)stbi__get16be(s);
               } else {
                  for (k = 0; k < s->img_n; ++k) tc[k] = (stbi_uc)(stbi__get16be(s) & 255) * stbi__depth_scale_table[z->depth];
               }
            }
            break;
         }

         case STBI__PNG_TYPE('I','D','A','T'): {
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (pal_img_n && !pal_len) return stbi__err("no PLTE","Corrupt PNG");
            if (scan == STBI__SCAN_header) { s->img_n = pal_img_n; return 1; }
            if ((int)(ioff + c.length) < (int)ioff) return 0;
            if (ioff + c.length > idata_limit) {
               stbi__uint32 idata_limit_old = idata_limit;
               stbi_uc *p;
               if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
               while (ioff + c.length > idata_limit)
                  idata_limit *= 2;
               STBI_NOTUSED(idata_limit_old);
               p = (stbi_uc *) STBI_REALLOC_SIZED(z->idata, idata_limit_old, idata_limit); if (p == NULL) return stbi__err("outofmem", "Out of memory");
               z->idata = p;
            }
            if (!stbi__getn(s, z->idata+ioff,c.length)) return stbi__err("outofdata","Corrupt PNG");
            ioff += c.length;
            break;
         }

         case STBI__PNG_TYPE('I','E','N','D'): {
            stbi__uint32 raw_len, bpl;
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (scan != STBI__SCAN_load) return 1;
            if (z->idata == NULL) return stbi__err("no IDAT","Corrupt PNG");
            bpl = (s->img_x * z->depth + 7) / 8;
            raw_len = bpl * s->img_y * s->img_n /* pixels */ + s->img_y /* filter mode per row */;
            z->expanded = (stbi_uc *) stbi_zlib_decode_malloc_guesssize_headerflag((char *) z->idata, ioff, raw_len, (int *) &raw_len, !is_iphone);
            if (z->expanded == NULL) return 0;
            STBI_FREE(z->idata); z->idata = NULL;
            if ((req_comp == s->img_n+1 && req_comp != 3 && !pal_img_n) || has_trans)
               s->img_out_n = s->img_n+1;
            else
               s->img_out_n = s->img_n;
            if (!stbi__create_png_image(z, z->expanded, raw_len, s->img_out_n, z->depth, color, interlace)) return 0;
            if (has_trans) {
               if (z->depth == 16) {
                  if (!stbi__compute_transparency16(z, tc16, s->img_out_n)) return 0;
               } else {
                  if (!stbi__compute_transparency(z, tc, s->img_out_n)) return 0;
               }
            }
            if (is_iphone && stbi__de_iphone_flag && s->img_out_n > 2)
               stbi__de_iphone(z);
            if (pal_img_n) {
               s->img_n = pal_img_n;
               s->img_out_n = pal_img_n;
               if (req_comp >= 3) s->img_out_n = req_comp;
               if (!stbi__expand_png_palette(z, palette, pal_len, s->img_out_n))
                  return 0;
            }
            STBI_FREE(z->expanded); z->expanded = NULL;
            return 1;
         }

         default:
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if ((c.type & (1 << 29)) == 0) {
               return stbi__err("invalid_chunk", "PNG not supported: unknown PNG chunk type");
            }
            stbi__skip(s, c.length);
            break;
      }
      stbi__get32be(s);
   }
}

static void *stbi__do_png(stbi__png *p, int *x, int *y, int *n, int req_comp, stbi__result_info *ri)
{
   void *result=NULL;
   if (req_comp < 0 || req_comp > 4) { stbi__err("bad req_comp", "Internal error"); return NULL; }
   if (stbi__parse_png_file(p, STBI__SCAN_load, req_comp)) {
      ri->bits_per_channel = p->depth;
      result = p->out;
      p->out = NULL;
      if (req_comp && req_comp != p->s->img_out_n) {
         if (ri->bits_per_channel == 8)
            result = stbi__convert_format((unsigned char *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         else
            result = stbi__convert_format16((stbi__uint16 *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         p->s->img_out_n = req_comp;
         if (result == NULL) return result;
      }
      *x = p->s->img_x;
      *y = p->s->img_y;
      if (n) *n = p->s->img_n;
   }
   STBI_FREE(p->out);      p->out      = NULL;
   STBI_FREE(p->expanded); p->expanded = NULL;
   STBI_FREE(p->idata);    p->idata    = NULL;

   return result;
}

static void *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   stbi__png p;
   p.s = s;
   return stbi__do_png(&p, x,y,comp,req_comp, ri);
}
#endif

#if defined(M3D_EXPORTER) && !defined(INCLUDE_STB_IMAGE_WRITE_H)
/* zlib_compressor from

   stb_image_write - v1.13 - public domain - http://nothings.org/stb/stb_image_write.h
*/
typedef unsigned char stbiw_uc;
typedef unsigned short stbiw_us;

typedef uint16_t stbiw_uint16;
typedef int16_t  stbiw_int16;
typedef uint32_t stbiw_uint32;
typedef int32_t  stbiw_int32;

#define STBIW_MALLOC(s)     M3D_MALLOC(s)
#define STBIW_REALLOC(p,ns) M3D_REALLOC(p,ns)
#define STBIW_REALLOC_SIZED(p,oldsz,newsz) STBIW_REALLOC(p,newsz)
#define STBIW_FREE          M3D_FREE
#define STBIW_MEMMOVE       memmove
#define STBIW_UCHAR         (uint8_t)
#define STBIW_ASSERT(x)
#define stbiw__sbraw(a)     ((int *) (a) - 2)
#define stbiw__sbm(a)       stbiw__sbraw(a)[0]
#define stbiw__sbn(a)       stbiw__sbraw(a)[1]

#define stbiw__sbneedgrow(a,n)  ((a)==0 || stbiw__sbn(a)+n >= stbiw__sbm(a))
#define stbiw__sbmaybegrow(a,n) (stbiw__sbneedgrow(a,(n)) ? stbiw__sbgrow(a,n) : 0)
#define stbiw__sbgrow(a,n)  stbiw__sbgrowf((void **) &(a), (n), sizeof(*(a)))

#define stbiw__sbpush(a, v)      (stbiw__sbmaybegrow(a,1), (a)[stbiw__sbn(a)++] = (v))
#define stbiw__sbcount(a)        ((a) ? stbiw__sbn(a) : 0)
#define stbiw__sbfree(a)         ((a) ? STBIW_FREE(stbiw__sbraw(a)),0 : 0)

static void *stbiw__sbgrowf(void **arr, int increment, int itemsize)
{
   int m = *arr ? 2*stbiw__sbm(*arr)+increment : increment+1;
   void *p = STBIW_REALLOC_SIZED(*arr ? stbiw__sbraw(*arr) : 0, *arr ? (stbiw__sbm(*arr)*itemsize + sizeof(int)*2) : 0, itemsize * m + sizeof(int)*2);
   STBIW_ASSERT(p);
   if (p) {
      if (!*arr) ((int *) p)[1] = 0;
      *arr = (void *) ((int *) p + 2);
      stbiw__sbm(*arr) = m;
   }
   return *arr;
}

static unsigned char *stbiw__zlib_flushf(unsigned char *data, unsigned int *bitbuffer, int *bitcount)
{
   while (*bitcount >= 8) {
      stbiw__sbpush(data, STBIW_UCHAR(*bitbuffer));
      *bitbuffer >>= 8;
      *bitcount -= 8;
   }
   return data;
}

static int stbiw__zlib_bitrev(int code, int codebits)
{
   int res=0;
   while (codebits--) {
      res = (res << 1) | (code & 1);
      code >>= 1;
   }
   return res;
}

static unsigned int stbiw__zlib_countm(unsigned char *a, unsigned char *b, int limit)
{
   int i;
   for (i=0; i < limit && i < 258; ++i)
      if (a[i] != b[i]) break;
   return i;
}

static unsigned int stbiw__zhash(unsigned char *data)
{
   stbiw_uint32 hash = data[0] + (data[1] << 8) + (data[2] << 16);
   hash ^= hash << 3;
   hash += hash >> 5;
   hash ^= hash << 4;
   hash += hash >> 17;
   hash ^= hash << 25;
   hash += hash >> 6;
   return hash;
}

#define stbiw__zlib_flush() (out = stbiw__zlib_flushf(out, &bitbuf, &bitcount))
#define stbiw__zlib_add(code,codebits) \
      (bitbuf |= (code) << bitcount, bitcount += (codebits), stbiw__zlib_flush())
#define stbiw__zlib_huffa(b,c)  stbiw__zlib_add(stbiw__zlib_bitrev(b,c),c)
#define stbiw__zlib_huff1(n)  stbiw__zlib_huffa(0x30 + (n), 8)
#define stbiw__zlib_huff2(n)  stbiw__zlib_huffa(0x190 + (n)-144, 9)
#define stbiw__zlib_huff3(n)  stbiw__zlib_huffa(0 + (n)-256,7)
#define stbiw__zlib_huff4(n)  stbiw__zlib_huffa(0xc0 + (n)-280,8)
#define stbiw__zlib_huff(n)  ((n) <= 143 ? stbiw__zlib_huff1(n) : (n) <= 255 ? stbiw__zlib_huff2(n) : (n) <= 279 ? stbiw__zlib_huff3(n) : stbiw__zlib_huff4(n))
#define stbiw__zlib_huffb(n) ((n) <= 143 ? stbiw__zlib_huff1(n) : stbiw__zlib_huff2(n))

#define stbiw__ZHASH   16384

unsigned char * stbi_zlib_compress(unsigned char *data, int data_len, int *out_len, int quality)
{
   static unsigned short lengthc[] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258, 259 };
   static unsigned char  lengtheb[]= { 0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0 };
   static unsigned short distc[]   = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577, 32768 };
   static unsigned char  disteb[]  = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };
   unsigned int bitbuf=0;
   int i,j, bitcount=0;
   unsigned char *out = NULL;
   unsigned char ***hash_table = (unsigned char***) STBIW_MALLOC(stbiw__ZHASH * sizeof(char**));
   if (quality < 5) quality = 5;

   stbiw__sbpush(out, 0x78);
   stbiw__sbpush(out, 0x5e);
   stbiw__zlib_add(1,1);
   stbiw__zlib_add(1,2);

   for (i=0; i < stbiw__ZHASH; ++i)
      hash_table[i] = NULL;

   i=0;
   while (i < data_len-3) {
      int h = stbiw__zhash(data+i)&(stbiw__ZHASH-1), best=3;
      unsigned char *bestloc = 0;
      unsigned char **hlist = hash_table[h];
      int n = stbiw__sbcount(hlist);
      for (j=0; j < n; ++j) {
         if (hlist[j]-data > i-32768) {
            int d = stbiw__zlib_countm(hlist[j], data+i, data_len-i);
            if (d >= best) best=d,bestloc=hlist[j];
         }
      }
      if (hash_table[h] && stbiw__sbn(hash_table[h]) == 2*quality) {
         STBIW_MEMMOVE(hash_table[h], hash_table[h]+quality, sizeof(hash_table[h][0])*quality);
         stbiw__sbn(hash_table[h]) = quality;
      }
      stbiw__sbpush(hash_table[h],data+i);

      if (bestloc) {
         h = stbiw__zhash(data+i+1)&(stbiw__ZHASH-1);
         hlist = hash_table[h];
         n = stbiw__sbcount(hlist);
         for (j=0; j < n; ++j) {
            if (hlist[j]-data > i-32767) {
               int e = stbiw__zlib_countm(hlist[j], data+i+1, data_len-i-1);
               if (e > best) {
                  bestloc = NULL;
                  break;
               }
            }
         }
      }

      if (bestloc) {
         int d = (int) (data+i - bestloc);
         STBIW_ASSERT(d <= 32767 && best <= 258);
         for (j=0; best > lengthc[j+1]-1; ++j);
         stbiw__zlib_huff(j+257);
         if (lengtheb[j]) stbiw__zlib_add(best - lengthc[j], lengtheb[j]);
         for (j=0; d > distc[j+1]-1; ++j);
         stbiw__zlib_add(stbiw__zlib_bitrev(j,5),5);
         if (disteb[j]) stbiw__zlib_add(d - distc[j], disteb[j]);
         i += best;
      } else {
         stbiw__zlib_huffb(data[i]);
         ++i;
      }
   }
   for (;i < data_len; ++i)
      stbiw__zlib_huffb(data[i]);
   stbiw__zlib_huff(256);
   while (bitcount)
      stbiw__zlib_add(0,1);

   for (i=0; i < stbiw__ZHASH; ++i)
      (void) stbiw__sbfree(hash_table[i]);
   STBIW_FREE(hash_table);

   {
      unsigned int s1=1, s2=0;
      int blocklen = (int) (data_len % 5552);
      j=0;
      while (j < data_len) {
         for (i=0; i < blocklen; ++i) s1 += data[j+i], s2 += s1;
         s1 %= 65521, s2 %= 65521;
         j += blocklen;
         blocklen = 5552;
      }
      stbiw__sbpush(out, STBIW_UCHAR(s2 >> 8));
      stbiw__sbpush(out, STBIW_UCHAR(s2));
      stbiw__sbpush(out, STBIW_UCHAR(s1 >> 8));
      stbiw__sbpush(out, STBIW_UCHAR(s1));
   }
   *out_len = stbiw__sbn(out);
   STBIW_MEMMOVE(stbiw__sbraw(out), out, *out_len);
   return (unsigned char *) stbiw__sbraw(out);
}
#endif

#define M3D_CHUNKMAGIC(m, a,b,c,d) ((m)[0]==(a) && (m)[1]==(b) && (m)[2]==(c) && (m)[3]==(d))

#ifdef M3D_ASCII
#include <stdio.h>          /* get sprintf */
#include <locale.h>         /* sprintf and strtod cares about number locale */
#endif

#if !defined(M3D_NOIMPORTER) && defined(M3D_ASCII)
/* helper functions for the ASCII parser */
static char *_m3d_findarg(char *s) {
    while(s && *s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') s++;
    while(s && *s && (*s == ' ' || *s == '\t')) s++;
    return s;
}
static char *_m3d_findnl(char *s) {
    while(s && *s && *s != '\r' && *s != '\n') s++;
    if(*s == '\r') s++;
    if(*s == '\n') s++;
    return s;
}
static char *_m3d_gethex(char *s, uint32_t *ret)
{
    if(*s == '#') s++;
    *ret = 0;
    for(; *s; s++) {
        if(*s >= '0' && *s <= '9') {      *ret <<= 4; *ret += (uint32_t)(*s-'0'); }
        else if(*s >= 'a' && *s <= 'f') { *ret <<= 4; *ret += (uint32_t)(*s-'a'+10); }
        else if(*s >= 'A' && *s <= 'F') { *ret <<= 4; *ret += (uint32_t)(*s-'A'+10); }
        else break;
    }
    return _m3d_findarg(s);
}
static char *_m3d_getint(char *s, uint32_t *ret)
{
    char *e = s;
    if(!s || !*s) return s;
    for(; *e >= '0' && *e <= '9'; e++);
    *ret = atoi(s);
    return e;
}
static char *_m3d_getfloat(char *s, M3D_FLOAT *ret)
{
    char *e = s;
    if(!s || !*s) return s;
    for(; *e == '-' || *e == '+' || *e == '.' || (*e >= '0' && *e <= '9') || *e == 'e' || *e == 'E'; e++);
    *ret = (M3D_FLOAT)strtod(s, NULL);
    return _m3d_findarg(e);
}
#endif
#if !defined(M3D_NODUP) && (!defined(M3D_NONORMALS) || defined(M3D_EXPORTER))
/* add vertex to list, only compare x,y,z */
m3dv_t *_m3d_addnorm(m3dv_t *vrtx, uint32_t *numvrtx, m3dv_t *v, uint32_t *idx)
{
    uint32_t i;
    if(v->x == (M3D_FLOAT)-0.0) v->x = (M3D_FLOAT)0.0;
    if(v->y == (M3D_FLOAT)-0.0) v->y = (M3D_FLOAT)0.0;
    if(v->z == (M3D_FLOAT)-0.0) v->z = (M3D_FLOAT)0.0;
    if(v->w == (M3D_FLOAT)-0.0) v->w = (M3D_FLOAT)0.0;
    if(vrtx) {
        for(i = 0; i < *numvrtx; i++)
            if(vrtx[i].x == v->x && vrtx[i].y == v->y && vrtx[i].z == v->z) { *idx = i; return vrtx; }
    }
    vrtx = (m3dv_t*)M3D_REALLOC(vrtx, ((*numvrtx) + 1) * sizeof(m3dv_t));
    memcpy(&vrtx[*numvrtx], v, sizeof(m3dv_t));
    vrtx[*numvrtx].color = 0;
    vrtx[*numvrtx].w = (M3D_FLOAT)1.0;
    *idx = *numvrtx;
    (*numvrtx)++;
    return vrtx;
}
#endif
#if !defined(M3D_NODUP) && (defined(M3D_ASCII) || defined(M3D_EXPORTER))
m3ds_t *_m3d_addskin(m3ds_t *skin, uint32_t *numskin, m3ds_t *s, uint32_t *idx)
{
    uint32_t i;
    M3D_FLOAT w = (M3D_FLOAT)0.0;
    for(i = 0; i < M3D_NUMBONE && s->weight[i] > (M3D_FLOAT)0.0; i++)
        w += s->weight[i];
    if(w != (M3D_FLOAT)1.0 && w != (M3D_FLOAT)0.0)
        for(i = 0; i < M3D_NUMBONE && s->weight[i] > (M3D_FLOAT)0.0; i++)
            s->weight[i] /= w;
    if(skin) {
        for(i = 0; i < *numskin; i++)
            if(!memcmp(&skin[i], s, sizeof(m3ds_t))) { *idx = i; return skin; }
    }
    skin = (m3ds_t*)M3D_REALLOC(skin, ((*numskin) + 1) * sizeof(m3ds_t));
    memcpy(&skin[*numskin], s, sizeof(m3ds_t));
    *idx = *numskin;
    (*numskin)++;
    return skin;
}
/* helper function to create safe strings */
char *_m3d_safestr(char *in, int morelines)
{
    char *out, *o, *i = in;
    int l;
    if(!in || !*in) {
        out = (char*)M3D_MALLOC(1);
        if(!out) return NULL;
        out[0] =0;
    } else {
        for(o = in, l = 0; *o && ((morelines & 1) || (*o != '\r' && *o != '\n')) && l < 256; o++, l++);
        out = o = (char*)M3D_MALLOC(l+1);
        if(!out) return NULL;
        while(*i == ' ' || *i == '\t' || *i == '\r' || (morelines && *i == '\n')) i++;
        for(; *i && (morelines || (*i != '\r' && *i != '\n')); i++) {
            if(*i == '\r') continue;
            if(*i == '\n') {
                if(morelines >= 3 && o > out && *(o-1) == '\n') break;
                if(i > in && *(i-1) == '\n') continue;
                if(morelines & 1) {
                    if(morelines == 1) *o++ = '\r';
                    *o++ = '\n';
                } else
                    break;
            } else
            if(*i == ' ' || *i == '\t') {
                *o++ = morelines? ' ' : '_';
            } else
                *o++ = !morelines && (*i == '/' || *i == '\\') ? '_' : *i;
        }
        for(; o > out && (*(o-1) == ' ' || *(o-1) == '\t' || *(o-1) == '\r' || *(o-1) == '\n'); o--);
        *o = 0;
        out = (char*)M3D_REALLOC(out, (uint64_t)o - (uint64_t)out + 1);
    }
    return out;
}
#endif
#ifndef M3D_NOIMPORTER
/* helper function to load and decode/generate a texture */
M3D_INDEX _m3d_gettx(m3d_t *model, m3dread_t readfilecb, m3dfree_t freecb, char *fn)
{
    unsigned int i, len = 0, w, h;
    unsigned char *buff = NULL;
    char *fn2;
    stbi__context s;
    stbi__result_info ri;

    for(i = 0; i < model->numtexture; i++)
        if(!strcmp(fn, model->texture[i].name)) return i;
    if(readfilecb) {
        i = strlen(fn);
        if(i < 5 || fn[i - 4] != '.') {
            fn2 = (char*)M3D_MALLOC(i + 5);
            if(!fn2) { model->errcode = M3D_ERR_ALLOC; return (M3D_INDEX)-1U; }
            memcpy(fn2, fn, i);
            memcpy(fn2+i, ".png", 5);
            buff = (*readfilecb)(fn2, &len);
            M3D_FREE(fn2);
        }
        if(!buff)
            buff = (*readfilecb)(fn, &len);
    }
    if(!buff && model->inlined) {
        for(i = 0; i < model->numinlined; i++)
            if(!strcmp(fn, model->inlined[i].name)) {
                buff = model->inlined[i].data;
                len = model->inlined[i].length;
                freecb = NULL;
                break;
            }
    }
    if(!buff) return (M3D_INDEX)-1U;
    i = model->numtexture++;
    model->texture = (m3dtx_t*)M3D_REALLOC(model->texture, model->numtexture * sizeof(m3dtx_t));
    if(!model->texture) {
        if(freecb) (*freecb)(buff);
        model->errcode = M3D_ERR_ALLOC; return (M3D_INDEX)-1U;
    }
    model->texture[i].w = model->texture[i].h = 0; model->texture[i].d = NULL;
    if(buff[0] == 0x89 && buff[1] == 'P' && buff[2] == 'N' && buff[3] == 'G') {
        s.read_from_callbacks = 0;
        s.img_buffer = s.img_buffer_original = (stbi_uc *) buff;
        s.img_buffer_end = s.img_buffer_original_end = (stbi_uc *) buff+len;
        /* don't use model->texture[i].w directly, it's a uint16_t */
        w = h = 0;
        model->texture[i].d = (uint32_t*)stbi__png_load(&s, (int*)&w, (int*)&h, (int*)&len, STBI_rgb_alpha, &ri);
        model->texture[i].w = w;
        model->texture[i].h = h;
    } else {
#ifdef M3D_TX_INTERP
        if((model->errcode = M3D_TX_INTERP(fn, buff, len, &model->texture[i])) != M3D_SUCCESS) {
            M3D_LOG("Unable to generate texture");
            M3D_LOG(fn);
        }
#else
        M3D_LOG("Unimplemented interpreter");
        M3D_LOG(fn);
#endif
    }
    if(freecb) (*freecb)(buff);
    if(!model->texture[i].d) {
        M3D_FREE(model->texture[i].d);
        model->errcode = M3D_ERR_UNKIMG;
        model->numtexture--;
        return (M3D_INDEX)-1U;
    }
    model->texture[i].name = fn;
    return i;
}

/* helper function to load and generate a procedural surface */
void _m3d_getpr(m3d_t *model, _unused m3dread_t readfilecb, _unused  m3dfree_t freecb, _unused char *fn)
{
#ifdef M3D_PR_INTERP
    unsigned int i, len = 0;
    unsigned char *buff = readfilecb ? (*readfilecb)(fn, &len) : NULL;

    if(!buff && model->inlined) {
        for(i = 0; i < model->numinlined; i++)
            if(!strcmp(fn, model->inlined[i].name)) {
                buff = model->inlined[i].data;
                len = model->inlined[i].length;
                freecb = NULL;
                break;
            }
    }
    if(!buff || !len || (model->errcode = M3D_PR_INTERP(fn, buff, len, model)) != M3D_SUCCESS) {
        M3D_LOG("Unable to generate procedural surface");
        M3D_LOG(fn);
        model->errcode = M3D_ERR_UNKIMG;
    }
    if(freecb && buff) (*freecb)(buff);
#else
    M3D_LOG("Unimplemented interpreter");
    M3D_LOG(fn);
    model->errcode = M3D_ERR_UNIMPL;
#endif
}
/* helpers to read indices from data stream */
#define M3D_GETSTR(x) do{offs=0;data=_m3d_getidx(data,model->si_s,&offs);x=offs?((char*)model->raw+16+offs):NULL;}while(0)
_inline static unsigned char *_m3d_getidx(unsigned char *data, char type, M3D_INDEX *idx)
{
    switch(type) {
        case 1: *idx = data[0] > 253 ? (int8_t)data[0] : data[0]; data++; break;
        case 2: *idx = (uint16_t)((data[1]<<8)|data[0]) > 65533 ? (int16_t)((data[1]<<8)|data[0]) : (uint16_t)((data[1]<<8)|data[0]); data += 2; break;
        case 4: *idx = (int32_t)((data[3]<<24)|(data[2]<<16)|(data[1]<<8)|data[0]); data += 4; break;
    }
    return data;
}

#ifndef M3D_NOANIMATION
/* multiply 4 x 4 matrices. Do not use float *r[16] as argument, because some compilers misinterpret that as
 * 16 pointers each pointing to a float, but we need a single pointer to 16 floats. */
void _m3d_mul(M3D_FLOAT *r, M3D_FLOAT *a, M3D_FLOAT *b)
{
    r[ 0] = b[ 0] * a[ 0] + b[ 4] * a[ 1] + b[ 8] * a[ 2] + b[12] * a[ 3];
    r[ 1] = b[ 1] * a[ 0] + b[ 5] * a[ 1] + b[ 9] * a[ 2] + b[13] * a[ 3];
    r[ 2] = b[ 2] * a[ 0] + b[ 6] * a[ 1] + b[10] * a[ 2] + b[14] * a[ 3];
    r[ 3] = b[ 3] * a[ 0] + b[ 7] * a[ 1] + b[11] * a[ 2] + b[15] * a[ 3];
    r[ 4] = b[ 0] * a[ 4] + b[ 4] * a[ 5] + b[ 8] * a[ 6] + b[12] * a[ 7];
    r[ 5] = b[ 1] * a[ 4] + b[ 5] * a[ 5] + b[ 9] * a[ 6] + b[13] * a[ 7];
    r[ 6] = b[ 2] * a[ 4] + b[ 6] * a[ 5] + b[10] * a[ 6] + b[14] * a[ 7];
    r[ 7] = b[ 3] * a[ 4] + b[ 7] * a[ 5] + b[11] * a[ 6] + b[15] * a[ 7];
    r[ 8] = b[ 0] * a[ 8] + b[ 4] * a[ 9] + b[ 8] * a[10] + b[12] * a[11];
    r[ 9] = b[ 1] * a[ 8] + b[ 5] * a[ 9] + b[ 9] * a[10] + b[13] * a[11];
    r[10] = b[ 2] * a[ 8] + b[ 6] * a[ 9] + b[10] * a[10] + b[14] * a[11];
    r[11] = b[ 3] * a[ 8] + b[ 7] * a[ 9] + b[11] * a[10] + b[15] * a[11];
    r[12] = b[ 0] * a[12] + b[ 4] * a[13] + b[ 8] * a[14] + b[12] * a[15];
    r[13] = b[ 1] * a[12] + b[ 5] * a[13] + b[ 9] * a[14] + b[13] * a[15];
    r[14] = b[ 2] * a[12] + b[ 6] * a[13] + b[10] * a[14] + b[14] * a[15];
    r[15] = b[ 3] * a[12] + b[ 7] * a[13] + b[11] * a[14] + b[15] * a[15];
}
/* calculate 4 x 4 matrix inverse */
void _m3d_inv(M3D_FLOAT *m)
{
    M3D_FLOAT r[16];
    M3D_FLOAT det =
          m[ 0]*m[ 5]*m[10]*m[15] - m[ 0]*m[ 5]*m[11]*m[14] + m[ 0]*m[ 6]*m[11]*m[13] - m[ 0]*m[ 6]*m[ 9]*m[15]
        + m[ 0]*m[ 7]*m[ 9]*m[14] - m[ 0]*m[ 7]*m[10]*m[13] - m[ 1]*m[ 6]*m[11]*m[12] + m[ 1]*m[ 6]*m[ 8]*m[15]
        - m[ 1]*m[ 7]*m[ 8]*m[14] + m[ 1]*m[ 7]*m[10]*m[12] - m[ 1]*m[ 4]*m[10]*m[15] + m[ 1]*m[ 4]*m[11]*m[14]
        + m[ 2]*m[ 7]*m[ 8]*m[13] - m[ 2]*m[ 7]*m[ 9]*m[12] + m[ 2]*m[ 4]*m[ 9]*m[15] - m[ 2]*m[ 4]*m[11]*m[13]
        + m[ 2]*m[ 5]*m[11]*m[12] - m[ 2]*m[ 5]*m[ 8]*m[15] - m[ 3]*m[ 4]*m[ 9]*m[14] + m[ 3]*m[ 4]*m[10]*m[13]
        - m[ 3]*m[ 5]*m[10]*m[12] + m[ 3]*m[ 5]*m[ 8]*m[14] - m[ 3]*m[ 6]*m[ 8]*m[13] + m[ 3]*m[ 6]*m[ 9]*m[12];
    if(det == (M3D_FLOAT)0.0 || det == (M3D_FLOAT)-0.0) det = (M3D_FLOAT)1.0; else det = (M3D_FLOAT)1.0 / det;
    r[ 0] = det *(m[ 5]*(m[10]*m[15] - m[11]*m[14]) + m[ 6]*(m[11]*m[13] - m[ 9]*m[15]) + m[ 7]*(m[ 9]*m[14] - m[10]*m[13]));
    r[ 1] = -det*(m[ 1]*(m[10]*m[15] - m[11]*m[14]) + m[ 2]*(m[11]*m[13] - m[ 9]*m[15]) + m[ 3]*(m[ 9]*m[14] - m[10]*m[13]));
    r[ 2] = det *(m[ 1]*(m[ 6]*m[15] - m[ 7]*m[14]) + m[ 2]*(m[ 7]*m[13] - m[ 5]*m[15]) + m[ 3]*(m[ 5]*m[14] - m[ 6]*m[13]));
    r[ 3] = -det*(m[ 1]*(m[ 6]*m[11] - m[ 7]*m[10]) + m[ 2]*(m[ 7]*m[ 9] - m[ 5]*m[11]) + m[ 3]*(m[ 5]*m[10] - m[ 6]*m[ 9]));
    r[ 4] = -det*(m[ 4]*(m[10]*m[15] - m[11]*m[14]) + m[ 6]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 7]*(m[ 8]*m[14] - m[10]*m[12]));
    r[ 5] = det *(m[ 0]*(m[10]*m[15] - m[11]*m[14]) + m[ 2]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 3]*(m[ 8]*m[14] - m[10]*m[12]));
    r[ 6] = -det*(m[ 0]*(m[ 6]*m[15] - m[ 7]*m[14]) + m[ 2]*(m[ 7]*m[12] - m[ 4]*m[15]) + m[ 3]*(m[ 4]*m[14] - m[ 6]*m[12]));
    r[ 7] = det *(m[ 0]*(m[ 6]*m[11] - m[ 7]*m[10]) + m[ 2]*(m[ 7]*m[ 8] - m[ 4]*m[11]) + m[ 3]*(m[ 4]*m[10] - m[ 6]*m[ 8]));
    r[ 8] = det *(m[ 4]*(m[ 9]*m[15] - m[11]*m[13]) + m[ 5]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 7]*(m[ 8]*m[13] - m[ 9]*m[12]));
    r[ 9] = -det*(m[ 0]*(m[ 9]*m[15] - m[11]*m[13]) + m[ 1]*(m[11]*m[12] - m[ 8]*m[15]) + m[ 3]*(m[ 8]*m[13] - m[ 9]*m[12]));
    r[10] = det *(m[ 0]*(m[ 5]*m[15] - m[ 7]*m[13]) + m[ 1]*(m[ 7]*m[12] - m[ 4]*m[15]) + m[ 3]*(m[ 4]*m[13] - m[ 5]*m[12]));
    r[11] = -det*(m[ 0]*(m[ 5]*m[11] - m[ 7]*m[ 9]) + m[ 1]*(m[ 7]*m[ 8] - m[ 4]*m[11]) + m[ 3]*(m[ 4]*m[ 9] - m[ 5]*m[ 8]));
    r[12] = -det*(m[ 4]*(m[ 9]*m[14] - m[10]*m[13]) + m[ 5]*(m[10]*m[12] - m[ 8]*m[14]) + m[ 6]*(m[ 8]*m[13] - m[ 9]*m[12]));
    r[13] = det *(m[ 0]*(m[ 9]*m[14] - m[10]*m[13]) + m[ 1]*(m[10]*m[12] - m[ 8]*m[14]) + m[ 2]*(m[ 8]*m[13] - m[ 9]*m[12]));
    r[14] = -det*(m[ 0]*(m[ 5]*m[14] - m[ 6]*m[13]) + m[ 1]*(m[ 6]*m[12] - m[ 4]*m[14]) + m[ 2]*(m[ 4]*m[13] - m[ 5]*m[12]));
    r[15] = det *(m[ 0]*(m[ 5]*m[10] - m[ 6]*m[ 9]) + m[ 1]*(m[ 6]*m[ 8] - m[ 4]*m[10]) + m[ 2]*(m[ 4]*m[ 9] - m[ 5]*m[ 8]));
    memcpy(m, &r, sizeof(r));
}
/* compose a coloumn major 4 x 4 matrix from vec3 position and vec4 orientation/rotation quaternion */
#ifndef M3D_EPSILON
/* carefully choosen for IEEE 754 don't change */
#define M3D_EPSILON ((M3D_FLOAT)1e-7)
#endif
void _m3d_mat(M3D_FLOAT *r, m3dv_t *p, m3dv_t *q)
{
    if(q->x == (M3D_FLOAT)0.0 && q->y == (M3D_FLOAT)0.0 && q->z >=(M3D_FLOAT) 0.7071065 && q->z <= (M3D_FLOAT)0.7071075 &&
        q->w == (M3D_FLOAT)0.0) {
        r[ 1] = r[ 2] = r[ 4] = r[ 6] = r[ 8] = r[ 9] = (M3D_FLOAT)0.0;
        r[ 0] = r[ 5] = r[10] = (M3D_FLOAT)-1.0;
    } else {
        r[ 0] = 1 - 2 * (q->y * q->y + q->z * q->z); if(r[ 0]>-M3D_EPSILON && r[ 0]<M3D_EPSILON) r[ 0]=(M3D_FLOAT)0.0;
        r[ 1] = 2 * (q->x * q->y - q->z * q->w);     if(r[ 1]>-M3D_EPSILON && r[ 1]<M3D_EPSILON) r[ 1]=(M3D_FLOAT)0.0;
        r[ 2] = 2 * (q->x * q->z + q->y * q->w);     if(r[ 2]>-M3D_EPSILON && r[ 2]<M3D_EPSILON) r[ 2]=(M3D_FLOAT)0.0;
        r[ 4] = 2 * (q->x * q->y + q->z * q->w);     if(r[ 4]>-M3D_EPSILON && r[ 4]<M3D_EPSILON) r[ 4]=(M3D_FLOAT)0.0;
        r[ 5] = 1 - 2 * (q->x * q->x + q->z * q->z); if(r[ 5]>-M3D_EPSILON && r[ 5]<M3D_EPSILON) r[ 5]=(M3D_FLOAT)0.0;
        r[ 6] = 2 * (q->y * q->z - q->x * q->w);     if(r[ 6]>-M3D_EPSILON && r[ 6]<M3D_EPSILON) r[ 6]=(M3D_FLOAT)0.0;
        r[ 8] = 2 * (q->x * q->z - q->y * q->w);     if(r[ 8]>-M3D_EPSILON && r[ 8]<M3D_EPSILON) r[ 8]=(M3D_FLOAT)0.0;
        r[ 9] = 2 * (q->y * q->z + q->x * q->w);     if(r[ 9]>-M3D_EPSILON && r[ 9]<M3D_EPSILON) r[ 9]=(M3D_FLOAT)0.0;
        r[10] = 1 - 2 * (q->x * q->x + q->y * q->y); if(r[10]>-M3D_EPSILON && r[10]<M3D_EPSILON) r[10]=(M3D_FLOAT)0.0;
    }
    r[ 3] = p->x; r[ 7] = p->y; r[11] = p->z;
    r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
}
#endif
#if !defined(M3D_NOANIMATION) || !defined(M3D_NONORMALS)
/* fast inverse square root calculation. returns 1/sqrt(x) */
static M3D_FLOAT _m3d_rsq(M3D_FLOAT x)
{
#ifdef M3D_DOUBLE
    return ((M3D_FLOAT)15.0/(M3D_FLOAT)8.0) + ((M3D_FLOAT)-5.0/(M3D_FLOAT)4.0)*x + ((M3D_FLOAT)3.0/(M3D_FLOAT)8.0)*x*x;
#else
    /* John Carmack's */
    float x2 = x * 0.5f;
    *((uint32_t*)&x) = (0x5f3759df - (*((uint32_t*)&x) >> 1));
    return x * (1.5f - (x2 * x * x));
#endif
}
#endif

/**
 * Function to decode a Model 3D into in-memory format
 */
m3d_t *m3d_load(unsigned char *data, m3dread_t readfilecb, m3dfree_t freecb, m3d_t *mtllib)
{
    unsigned char *end, *chunk, *buff, weights[8];
    unsigned int i, j, k, n, am, len = 0, reclen, offs;
    char *material;
#ifndef M3D_NONORMALS
    unsigned int numnorm = 0;
    m3dv_t *norm = NULL, *v0, *v1, *v2, va, vb, vn;
    M3D_INDEX *ni = NULL, *vi = NULL;
#endif
    m3d_t *model;
    M3D_INDEX mi;
    M3D_FLOAT w;
#ifndef M3D_NOANIMATION
    M3D_FLOAT r[16];
#endif
    m3dtx_t *tx;
    m3dm_t *m;
    m3da_t *a;
    m3db_t *b;
    m3di_t *t;
    m3ds_t *sk;
#ifdef M3D_ASCII
    m3ds_t s;
    M3D_INDEX bi[M3D_BONEMAXLEVEL+1], level;
    const char *ol;
    char *ptr, *pe;
#endif

    if(!data || (!M3D_CHUNKMAGIC(data, '3','D','M','O')
#ifdef M3D_ASCII
        && !M3D_CHUNKMAGIC(data, '3','d','m','o')
#endif
        )) return NULL;
    model = (m3d_t*)M3D_MALLOC(sizeof(m3d_t));
    if(!model) {
        M3D_LOG("Out of memory");
        return NULL;
    }
    memset(model, 0, sizeof(m3d_t));

    if(mtllib) {
        model->nummaterial = mtllib->nummaterial;
        model->material = mtllib->material;
        model->numtexture = mtllib->numtexture;
        model->texture = mtllib->texture;
        model->flags |= M3D_FLG_MTLLIB;
    }
#ifdef M3D_ASCII
    /* ASCII variant? */
    if(M3D_CHUNKMAGIC(data, '3','d','m','o')) {
        model->errcode = M3D_ERR_BADFILE;
        model->flags |= M3D_FLG_FREESTR;
        model->raw = (m3dhdr_t*)data;
        ptr = (char*)data;
        ol = setlocale(LC_NUMERIC, NULL);
        setlocale(LC_NUMERIC, "C");
        /* parse header. Don't use sscanf, that's incredibly slow */
        ptr = _m3d_findarg(ptr);
        if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
        pe = _m3d_findnl(ptr);
        model->scale = (float)strtod(ptr, NULL); ptr = pe;
        if(model->scale <= (M3D_FLOAT)0.0) model->scale = (M3D_FLOAT)1.0;
        model->name = _m3d_safestr(ptr, 0); ptr = _m3d_findnl(ptr);
        if(!*ptr) goto asciiend;
        model->license = _m3d_safestr(ptr, 2); ptr = _m3d_findnl(ptr);
        if(!*ptr) goto asciiend;
        model->author = _m3d_safestr(ptr, 2); ptr = _m3d_findnl(ptr);
        if(!*ptr) goto asciiend;
        model->desc = _m3d_safestr(ptr, 3);
        while(*ptr) {
            while(*ptr && *ptr!='\n') ptr++;
            ptr++; if(*ptr=='\r') ptr++;
            if(*ptr == '\n') break;
        }

        /* the main chunk reader loop */
        while(*ptr) {
            while(*ptr && (*ptr == '\r' || *ptr == '\n')) ptr++;
            if(!*ptr || (ptr[0]=='E' && ptr[1]=='n' && ptr[2]=='d')) break;
            /* make sure there's at least one data row */
            pe = ptr; ptr = _m3d_findnl(ptr);
            if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
            /* texture map chunk */
            if(!memcmp(pe, "Textmap", 7)) {
                if(model->tmap) { M3D_LOG("More texture map chunks, should be unique"); goto asciiend; }
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    i = model->numtmap++;
                    model->tmap = (m3dti_t*)M3D_REALLOC(model->tmap, model->numtmap * sizeof(m3dti_t));
                    if(!model->tmap) goto memerr;
                    ptr = _m3d_getfloat(ptr, &model->tmap[i].u);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    ptr = _m3d_getfloat(ptr, &model->tmap[i].v);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    ptr = _m3d_findnl(ptr);
                }
            } else
            /* vertex chunk */
            if(!memcmp(pe, "Vertex", 6)) {
                if(model->vertex) { M3D_LOG("More vertex chunks, should be unique"); goto asciiend; }
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    i = model->numvertex++;
                    model->vertex = (m3dv_t*)M3D_REALLOC(model->vertex, model->numvertex * sizeof(m3dv_t));
                    if(!model->vertex) goto memerr;
                    model->vertex[i].skinid = (M3D_INDEX)-1U;
                    model->vertex[i].color = 0;
                    ptr = _m3d_getfloat(ptr, &model->vertex[i].x);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    ptr = _m3d_getfloat(ptr, &model->vertex[i].y);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    ptr = _m3d_getfloat(ptr, &model->vertex[i].z);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    ptr = _m3d_getfloat(ptr, &model->vertex[i].w);
                    if(model->vertex[i].w != 1.0) model->vertex[i].skinid = (M3D_INDEX)-2U;
                    if(!*ptr) goto asciiend;
                    if(*ptr == '#') {
                        ptr = _m3d_gethex(ptr, &model->vertex[i].color);
                        if(!*ptr) goto asciiend;
                    }
                    /* parse skin */
                    memset(&s, 0, sizeof(m3ds_t));
                    for(j = 0; j < M3D_NUMBONE && *ptr && *ptr != '\r' && *ptr != '\n'; j++) {
                        ptr = _m3d_findarg(ptr);
                        if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                        ptr = _m3d_getint(ptr, &k);
                        s.boneid[j] = (M3D_INDEX)k;
                        if(*ptr == ':') {
                            ptr++;
                            ptr = _m3d_getfloat(ptr, &s.weight[j]);
                        } else if(!j)
                            s.weight[j] = (M3D_FLOAT)1.0;
                        if(!*ptr) goto asciiend;
                    }
                    if(s.boneid[0] != (M3D_INDEX)-1U && s.weight[0] > (M3D_FLOAT)0.0) {
                        model->skin = _m3d_addskin(model->skin, &model->numskin, &s, &k);
                        model->vertex[i].skinid = (M3D_INDEX)k;
                    }
                    ptr = _m3d_findnl(ptr);
                }
            } else
            /* Skeleton, bone hierarchy */
            if(!memcmp(pe, "Bones", 5)) {
                if(model->bone) { M3D_LOG("More bones chunks, should be unique"); goto asciiend; }
                bi[0] = (M3D_INDEX)-1U;
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    i = model->numbone++;
                    model->bone = (m3db_t*)M3D_REALLOC(model->bone, model->numbone * sizeof(m3db_t));
                    if(!model->bone) goto memerr;
                    for(level = 0; *ptr == '/'; ptr++, level++);
                    if(level > M3D_BONEMAXLEVEL || !*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    bi[level+1] = i;
                    model->bone[i].numweight = 0;
                    model->bone[i].weight = NULL;
                    model->bone[i].parent = bi[level];
                    ptr = _m3d_getint(ptr, &k);
                    ptr = _m3d_findarg(ptr);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    model->bone[i].pos = (M3D_INDEX)k;
                    ptr = _m3d_getint(ptr, &k);
                    ptr = _m3d_findarg(ptr);
                    if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                    model->bone[i].ori = (M3D_INDEX)k;
                    pe = _m3d_safestr(ptr, 0);
                    if(!pe || !*pe) goto asciiend;
                    model->bone[i].name = pe;
                    ptr = _m3d_findnl(ptr);
                }
            } else
            /* material chunk */
            if(!memcmp(pe, "Material", 8)) {
                pe = _m3d_findarg(pe);
                if(!*pe || *pe == '\r' || *pe == '\n') goto asciiend;
                pe = _m3d_safestr(pe, 0);
                if(!pe || !*pe) goto asciiend;
                for(i = 0; i < model->nummaterial; i++)
                    if(!strcmp(pe, model->material[i].name)) {
                        M3D_LOG("Multiple definitions for material");
                        M3D_LOG(pe);
                        M3D_FREE(pe);
                        pe = NULL;
                        while(*ptr && *ptr != '\r' && *ptr != '\n') ptr = _m3d_findnl(ptr);
                        break;
                    }
                if(!pe) continue;
                i = model->nummaterial++;
                if(model->flags & M3D_FLG_MTLLIB) {
                    m = model->material;
                    model->material = (m3dm_t*)M3D_MALLOC(model->nummaterial * sizeof(m3dm_t));
                    if(!model->material) goto memerr;
                    memcpy(model->material, m, (model->nummaterial - 1) * sizeof(m3dm_t));
                    if(model->texture) {
                        tx = model->texture;
                        model->texture = (m3dtx_t*)M3D_MALLOC(model->numtexture * sizeof(m3dtx_t));
                        if(!model->texture) goto memerr;
                        memcpy(model->texture, tx, model->numtexture * sizeof(m3dm_t));
                    }
                    model->flags &= ~M3D_FLG_MTLLIB;
                } else {
                    model->material = (m3dm_t*)M3D_REALLOC(model->material, model->nummaterial * sizeof(m3dm_t));
                    if(!model->material) goto memerr;
                }
                m = &model->material[i];
                m->name = pe;
                m->numprop = 0;
                m->prop = NULL;
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    k = n = 256;
                    if(*ptr == 'm' && *(ptr+1) == 'a' && *(ptr+2) == 'p' && *(ptr+3) == '_') {
                        k = m3dpf_map;
                        ptr += 4;
                    }
                    for(j = 0; j < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); j++)
                        if(!memcmp(ptr, m3d_propertytypes[j].key, strlen(m3d_propertytypes[j].key))) {
                            n = m3d_propertytypes[j].id;
                            if(k != m3dpf_map) k = m3d_propertytypes[j].format;
                            break;
                        }
                    if(n != 256 && k != 256) {
                        ptr = _m3d_findarg(ptr);
                        if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                        j = m->numprop++;
                        m->prop = (m3dp_t*)M3D_REALLOC(m->prop, m->numprop * sizeof(m3dp_t));
                        if(!m->prop) goto memerr;
                        m->prop[j].type = n;
                        switch(k) {
                            case m3dpf_color: ptr = _m3d_gethex(ptr, &m->prop[j].value.color); break;
                            case m3dpf_uint8:
                            case m3dpf_uint16:
                            case m3dpf_uint32: ptr = _m3d_getint(ptr, &m->prop[j].value.num); break;
                            case m3dpf_float:  ptr = _m3d_getfloat(ptr, &m->prop[j].value.fnum); break;
                            case m3dpf_map:
                                pe = _m3d_safestr(ptr, 0);
                                if(!pe || !*pe) goto asciiend;
                                m->prop[j].value.textureid = _m3d_gettx(model, readfilecb, freecb, pe);
                                if(model->errcode == M3D_ERR_ALLOC) { M3D_FREE(pe); goto memerr; }
                                if(m->prop[j].value.textureid == (M3D_INDEX)-1U) {
                                    M3D_LOG("Texture not found");
                                    M3D_LOG(pe);
                                    m->numprop--;
                                }
                                M3D_FREE(pe);
                            break;
                        }
                    } else {
                        M3D_LOG("Unknown material property in");
                        M3D_LOG(m->name);
                        model->errcode = M3D_ERR_UNKPROP;
                    }
                    ptr = _m3d_findnl(ptr);
                }
                if(!m->numprop) model->nummaterial--;
            } else
            /* procedural, not implemented yet, skip chunk */
            if(!memcmp(pe, "Procedural", 10)) {
                pe = _m3d_safestr(ptr, 0);
                _m3d_getpr(model, readfilecb, freecb, pe);
                M3D_FREE(pe);
                while(*ptr && *ptr != '\r' && *ptr != '\n') ptr = _m3d_findnl(ptr);
            } else
            /* mesh */
            if(!memcmp(pe, "Mesh", 4)) {
                mi = (M3D_INDEX)-1U;
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    if(*ptr == 'u') {
                        ptr = _m3d_findarg(ptr);
                        if(!*ptr) goto asciiend;
                        mi = (M3D_INDEX)-1U;
                        if(*ptr != '\r' && *ptr != '\n') {
                            pe = _m3d_safestr(ptr, 0);
                            if(!pe || !*pe) goto asciiend;
                            for(j = 0; j < model->nummaterial; j++)
                                if(!strcmp(pe, model->material[j].name)) {
                                    mi = (M3D_INDEX)j;
                                    break;
                                }
                            M3D_FREE(pe);
                        }
                    } else {
                        i = model->numface++;
                        model->face = (m3df_t*)M3D_REALLOC(model->face, model->numface * sizeof(m3df_t));
                        if(!model->face) goto memerr;
                        memset(&model->face[i], 255, sizeof(m3df_t)); /* set all index to -1 by default */
                        model->face[i].materialid = mi;
                        /* hardcoded triangles. */
                        for(j = 0; j < 3; j++) {
                            /* vertex */
                            ptr = _m3d_getint(ptr, &k);
                            model->face[i].vertex[j] = (M3D_INDEX)k;
                            if(!*ptr) goto asciiend;
                            if(*ptr == '/') {
                                ptr++;
                                if(*ptr != '/') {
                                    /* texcoord */
                                    ptr = _m3d_getint(ptr, &k);
                                    model->face[i].texcoord[j] = (M3D_INDEX)k;
                                    if(!*ptr) goto asciiend;
                                }
                                if(*ptr == '/') {
                                    ptr++;
                                    /* normal */
                                    ptr = _m3d_getint(ptr, &k);
                                    model->face[i].normal[j] = (M3D_INDEX)k;
                                    if(!*ptr) goto asciiend;
                                }
                            }
                            ptr = _m3d_findarg(ptr);
                        }
                    }
                    ptr = _m3d_findnl(ptr);
                }
            } else
            /* action */
            if(!memcmp(pe, "Action", 6)) {
                pe = _m3d_findarg(pe);
                if(!*pe || *pe == '\r' || *pe == '\n') goto asciiend;
                pe = _m3d_getint(pe, &k);
                pe = _m3d_findarg(pe);
                if(!*pe || *pe == '\r' || *pe == '\n') goto asciiend;
                pe = _m3d_safestr(pe, 0);
                if(!pe || !*pe) goto asciiend;
                i = model->numaction++;
                model->action = (m3da_t*)M3D_REALLOC(model->action, model->numaction * sizeof(m3da_t));
                if(!model->action) goto memerr;
                a = &model->action[i];
                a->name = pe;
                a->durationmsec = k;
                /* skip the first frame marker as there's always at least one frame */
                a->numframe = 1;
                a->frame = (m3dfr_t*)M3D_MALLOC(sizeof(m3dfr_t));
                if(!a->frame) goto memerr;
                a->frame[0].msec = 0;
                a->frame[0].numtransform = 0;
                a->frame[0].transform = NULL;
                i = 0;
                if(*ptr == 'f')
                    ptr = _m3d_findnl(ptr);
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    if(*ptr == 'f') {
                        i = a->numframe++;
                        a->frame = (m3dfr_t*)M3D_REALLOC(a->frame, a->numframe * sizeof(m3dfr_t));
                        if(!a->frame) goto memerr;
                        ptr = _m3d_findarg(ptr);
                        ptr = _m3d_getint(ptr, &a->frame[i].msec);
                        a->frame[i].numtransform = 0;
                        a->frame[i].transform = NULL;
                    } else {
                        j = a->frame[i].numtransform++;
                        a->frame[i].transform = (m3dtr_t*)M3D_REALLOC(a->frame[i].transform,
                            a->frame[i].numtransform * sizeof(m3dtr_t));
                        if(!a->frame[i].transform) goto memerr;
                        ptr = _m3d_getint(ptr, &k);
                        ptr = _m3d_findarg(ptr);
                        if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                        a->frame[i].transform[j].boneid = (M3D_INDEX)k;
                        ptr = _m3d_getint(ptr, &k);
                        ptr = _m3d_findarg(ptr);
                        if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                        a->frame[i].transform[j].pos = (M3D_INDEX)k;
                        ptr = _m3d_getint(ptr, &k);
                        if(!*ptr || *ptr == '\r' || *ptr == '\n') goto asciiend;
                        a->frame[i].transform[j].ori = (M3D_INDEX)k;
                    }
                    ptr = _m3d_findnl(ptr);
                }
            } else
            /* extra chunks */
            if(!memcmp(pe, "Extra", 5)) {
                pe = _m3d_findarg(pe);
                if(!*pe || *pe == '\r' || *pe == '\n') goto asciiend;
                buff = (unsigned char*)_m3d_findnl(ptr);
                k = ((uint32_t)((uint64_t)buff - (uint64_t)ptr) / 3) + 1;
                i = model->numunknown++;
                model->unknown = (m3dchunk_t**)M3D_REALLOC(model->unknown, model->numunknown * sizeof(m3dchunk_t*));
                if(!model->unknown) goto memerr;
                model->unknown[i] = (m3dchunk_t*)M3D_MALLOC(k + sizeof(m3dchunk_t));
                if(!model->unknown[i]) goto memerr;
                memcpy(&model->unknown[i]->magic, pe, 4);
                model->unknown[i]->length = sizeof(m3dchunk_t);
                pe = (char*)model->unknown[i] + sizeof(m3dchunk_t);
                while(*ptr && *ptr != '\r' && *ptr != '\n') {
                    ptr = _m3d_gethex(ptr, &k);
                    *pe++ = (uint8_t)k;
                    model->unknown[i]->length++;
                }
            } else
                goto asciiend;
        }
        model->errcode = M3D_SUCCESS;
asciiend:
        setlocale(LC_NUMERIC, ol);
        goto postprocess;
    }
    /* Binary variant */
#endif
    if(!M3D_CHUNKMAGIC(data + 8, 'H','E','A','D')) {
        stbi__g_failure_reason = "Corrupt file";
        buff = (unsigned char *)stbi_zlib_decode_malloc_guesssize_headerflag((const char*)data+8, ((m3dchunk_t*)data)->length-8,
            4096, (int*)&len, 1);
        if(!buff || !len || !M3D_CHUNKMAGIC(buff, 'H','E','A','D')) {
            M3D_LOG(stbi__g_failure_reason);
            if(buff) M3D_FREE(buff);
            M3D_FREE(model);
            return NULL;
        }
        buff = (unsigned char*)M3D_REALLOC(buff, len);
        model->flags |= M3D_FLG_FREERAW; /* mark that we have to free the raw buffer */
        data = buff;
    } else {
        len = ((m3dhdr_t*)data)->length;
        data += 8;
    }
    model->raw = (m3dhdr_t*)data;
    end = data + len;

    /* parse header */
    data += sizeof(m3dhdr_t);
    M3D_LOG(data);
    model->name = (char*)data;
    for(; data < end && *data; data++) {}; data++;
    model->license = (char*)data;
    for(; data < end && *data; data++) {}; data++;
    model->author = (char*)data;
    for(; data < end && *data; data++) {}; data++;
    model->desc = (char*)data;
    chunk = (unsigned char*)model->raw + model->raw->length;
    model->scale = (M3D_FLOAT)model->raw->scale;
    if(model->scale <= (M3D_FLOAT)0.0) model->scale = (M3D_FLOAT)1.0;
    model->vc_s = 1 << ((model->raw->types >> 0) & 3);  /* vertex coordinate size */
    model->vi_s = 1 << ((model->raw->types >> 2) & 3);  /* vertex index size */
    model->si_s = 1 << ((model->raw->types >> 4) & 3);  /* string offset size */
    model->ci_s = 1 << ((model->raw->types >> 6) & 3);  /* color index size */
    model->ti_s = 1 << ((model->raw->types >> 8) & 3);  /* tmap index size */
    model->bi_s = 1 << ((model->raw->types >>10) & 3);  /* bone index size */
    model->nb_s = 1 << ((model->raw->types >>12) & 3);  /* number of bones per vertex */
    model->sk_s = 1 << ((model->raw->types >>14) & 3);  /* skin index size */
    model->fi_s = 1 << ((model->raw->types >>16) & 3);  /* frame counter size */
    if(model->ci_s == 8) model->ci_s = 0;               /* optional indices */
    if(model->ti_s == 8) model->ti_s = 0;
    if(model->bi_s == 8) model->bi_s = 0;
    if(model->sk_s == 8) model->sk_s = 0;
    if(model->fi_s == 8) model->fi_s = 0;

    /* variable limit checks */
    if(sizeof(M3D_FLOAT) == 4 && model->vc_s > 4) {
        M3D_LOG("Double precision coordinates not supported, truncating to float...");
        model->errcode = M3D_ERR_TRUNC;
    }
    if(sizeof(M3D_INDEX) == 2 && (model->vi_s > 2 || model->si_s > 2 || model->ci_s > 2 || model->ti_s > 2 ||
        model->bi_s > 2 || model->sk_s > 2 || model->fi_s > 2)) {
        M3D_LOG("32 bit indices not supported, unable to load model");
        M3D_FREE(model);
        return NULL;
    }
    if(model->vi_s > 4 || model->si_s > 4) {
        M3D_LOG("Invalid index size, unable to load model");
        M3D_FREE(model);
        return NULL;
    }
    if(model->nb_s > M3D_NUMBONE) {
        M3D_LOG("Model has more bones per vertex than importer supports");
        model->errcode = M3D_ERR_TRUNC;
    }

    /* look for inlined assets in advance, material and procedural chunks may need them */
    buff = chunk;
    while(buff < end && !M3D_CHUNKMAGIC(buff, 'O','M','D','3')) {
        data = buff;
        len = ((m3dchunk_t*)data)->length;
        if(len < sizeof(m3dchunk_t)) {
            M3D_LOG("Invalid chunk size");
            break;
        }
        buff += len;
        len -= sizeof(m3dchunk_t) + model->si_s;

        /* inlined assets */
        if(M3D_CHUNKMAGIC(data, 'A','S','E','T') && len > 0) {
            M3D_LOG("Inlined asset");
            i = model->numinlined++;
            model->inlined = (m3di_t*)M3D_REALLOC(model->inlined, model->numinlined * sizeof(m3di_t));
            if(!model->inlined) {
memerr:         M3D_LOG("Out of memory");
                model->errcode = M3D_ERR_ALLOC;
                return model;
            }
            data += sizeof(m3dchunk_t);
            t = &model->inlined[i];
            M3D_GETSTR(t->name);
            M3D_LOG(t->name);
            t->data = (uint8_t*)data;
            t->length = len;
        }
    }

    /* parse chunks */
    while(chunk < end && !M3D_CHUNKMAGIC(chunk, 'O','M','D','3')) {
        data = chunk;
        len = ((m3dchunk_t*)chunk)->length;
        if(len < sizeof(m3dchunk_t)) {
            M3D_LOG("Invalid chunk size");
            break;
        }
        chunk += len;
        len -= sizeof(m3dchunk_t);

        /* color map */
        if(M3D_CHUNKMAGIC(data, 'C','M','A','P')) {
            M3D_LOG("Color map");
            if(model->cmap) { M3D_LOG("More color map chunks, should be unique"); model->errcode = M3D_ERR_CMAP; continue; }
            if(!model->ci_s) { M3D_LOG("Color map chunk, shouldn't be any"); model->errcode = M3D_ERR_CMAP; continue; }
            model->numcmap = len / sizeof(uint32_t);
            model->cmap = (uint32_t*)(data + sizeof(m3dchunk_t));
        } else
        /* texture map */
        if(M3D_CHUNKMAGIC(data, 'T','M','A','P')) {
            M3D_LOG("Texture map");
            if(model->tmap) { M3D_LOG("More texture map chunks, should be unique"); model->errcode = M3D_ERR_TMAP; continue; }
            if(!model->ti_s) { M3D_LOG("Texture map chunk, shouldn't be any"); model->errcode = M3D_ERR_TMAP; continue; }
            reclen = model->vc_s + model->vc_s;
            model->numtmap = len / reclen;
            model->tmap = (m3dti_t*)M3D_MALLOC(model->numtmap * sizeof(m3dti_t));
            if(!model->tmap) goto memerr;
            for(i = 0, data += sizeof(m3dchunk_t); data < chunk; i++) {
                switch(model->vc_s) {
                    case 1:
                        model->tmap[i].u = (M3D_FLOAT)(data[0]) / 255;
                        model->tmap[i].v = (M3D_FLOAT)(data[1]) / 255;
                    break;
                    case 2:
                        model->tmap[i].u = (M3D_FLOAT)(*((int16_t*)(data+0))) / 65535;
                        model->tmap[i].v = (M3D_FLOAT)(*((int16_t*)(data+2))) / 65535;
                    break;
                    case 4:
                        model->tmap[i].u = (M3D_FLOAT)(*((float*)(data+0)));
                        model->tmap[i].v = (M3D_FLOAT)(*((float*)(data+4)));
                    break;
                    case 8:
                        model->tmap[i].u = (M3D_FLOAT)(*((double*)(data+0)));
                        model->tmap[i].v = (M3D_FLOAT)(*((double*)(data+8)));
                    break;
                }
                data += reclen;
            }
        } else
        /* vertex list */
        if(M3D_CHUNKMAGIC(data, 'V','R','T','S')) {
            M3D_LOG("Vertex list");
            if(model->vertex) { M3D_LOG("More vertex chunks, should be unique"); model->errcode = M3D_ERR_VRTS; continue; }
            if(model->ci_s && model->ci_s < 4 && !model->cmap) model->errcode = M3D_ERR_CMAP;
            reclen = model->ci_s + model->sk_s + 4 * model->vc_s;
            model->numvertex = len / reclen;
            model->vertex = (m3dv_t*)M3D_MALLOC(model->numvertex * sizeof(m3dv_t));
            if(!model->vertex) goto memerr;
            memset(model->vertex, 0, model->numvertex * sizeof(m3dv_t));
            for(i = 0, data += sizeof(m3dchunk_t); data < chunk && i < model->numvertex; i++) {
                switch(model->vc_s) {
                    case 1:
                        model->vertex[i].x = (M3D_FLOAT)((int8_t)data[0]) / 127;
                        model->vertex[i].y = (M3D_FLOAT)((int8_t)data[1]) / 127;
                        model->vertex[i].z = (M3D_FLOAT)((int8_t)data[2]) / 127;
                        model->vertex[i].w = (M3D_FLOAT)((int8_t)data[3]) / 127;
                        data += 4;
                    break;
                    case 2:
                        model->vertex[i].x = (M3D_FLOAT)((int16_t)((data[1]<<8)|data[0])) / 32767;
                        model->vertex[i].y = (M3D_FLOAT)((int16_t)((data[3]<<8)|data[2])) / 32767;
                        model->vertex[i].z = (M3D_FLOAT)((int16_t)((data[5]<<8)|data[4])) / 32767;
                        model->vertex[i].w = (M3D_FLOAT)((int16_t)((data[7]<<8)|data[6])) / 32767;
                        data += 8;
                    break;
                    case 4:
                        model->vertex[i].x = (M3D_FLOAT)(*((float*)(data+0)));
                        model->vertex[i].y = (M3D_FLOAT)(*((float*)(data+4)));
                        model->vertex[i].z = (M3D_FLOAT)(*((float*)(data+8)));
                        model->vertex[i].w = (M3D_FLOAT)(*((float*)(data+12)));
                        data += 16;
                    break;
                    case 8:
                        model->vertex[i].x = (M3D_FLOAT)(*((double*)(data+0)));
                        model->vertex[i].y = (M3D_FLOAT)(*((double*)(data+8)));
                        model->vertex[i].z = (M3D_FLOAT)(*((double*)(data+16)));
                        model->vertex[i].w = (M3D_FLOAT)(*((double*)(data+24)));
                        data += 32;
                    break;
                }
                switch(model->ci_s) {
                    case 1: model->vertex[i].color = model->cmap ? model->cmap[data[0]] : 0; data++; break;
                    case 2: model->vertex[i].color = model->cmap ? model->cmap[*((uint16_t*)data)] : 0; data += 2; break;
                    case 4: model->vertex[i].color = *((uint32_t*)data); data += 4; break;
                    /* case 8: break; */
                }
                model->vertex[i].skinid = (M3D_INDEX)-1U;
                data = _m3d_getidx(data, model->sk_s, &model->vertex[i].skinid);
            }
        } else
        /* skeleton: bone hierarchy and skin */
        if(M3D_CHUNKMAGIC(data, 'B','O','N','E')) {
            M3D_LOG("Skeleton");
            if(model->bone) { M3D_LOG("More bone chunks, should be unique"); model->errcode = M3D_ERR_BONE; continue; }
            if(!model->bi_s) { M3D_LOG("Bone chunk, shouldn't be any"); model->errcode=M3D_ERR_BONE; continue; }
            if(!model->vertex) { M3D_LOG("No vertex chunk before bones"); model->errcode = M3D_ERR_VRTS; break; }
            data += sizeof(m3dchunk_t);
            model->numbone = 0;
            data = _m3d_getidx(data, model->bi_s, &model->numbone);
            if(model->numbone) {
                model->bone = (m3db_t*)M3D_MALLOC(model->numbone * sizeof(m3db_t));
                if(!model->bone) goto memerr;
            }
            model->numskin = 0;
            data = _m3d_getidx(data, model->sk_s, &model->numskin);
            if(model->numskin) {
                model->skin = (m3ds_t*)M3D_MALLOC(model->numskin * sizeof(m3ds_t));
                if(!model->skin) goto memerr;
                for(i = 0; i < model->numskin; i++)
                    for(j = 0; j < M3D_NUMBONE; j++) {
                        model->skin[i].boneid[j] = (M3D_INDEX)-1U;
                        model->skin[i].weight[j] = (M3D_FLOAT)0.0;
                    }
            }
            /* read bone hierarchy */
            for(i = 0; i < model->numbone; i++) {
                data = _m3d_getidx(data, model->bi_s, &model->bone[i].parent);
                M3D_GETSTR(model->bone[i].name);
                data = _m3d_getidx(data, model->vi_s, &model->bone[i].pos);
                data = _m3d_getidx(data, model->vi_s, &model->bone[i].ori);
                model->bone[i].numweight = 0;
                model->bone[i].weight = NULL;
            }
            /* read skin definitions */
            for(i = 0; data < chunk && i < model->numskin; i++) {
                memset(&weights, 0, sizeof(weights));
                if(model->nb_s == 1) weights[0] = 255;
                else {
                    memcpy(&weights, data, model->nb_s);
                    data += model->nb_s;
                }
                for(j = 0; j < (unsigned int)model->nb_s; j++) {
                    if(weights[j]) {
                        if(j >= M3D_NUMBONE)
                            data += model->bi_s;
                        else {
                            model->skin[i].weight[j] = (M3D_FLOAT)(weights[j]) / 255;
                            data = _m3d_getidx(data, model->bi_s, &model->skin[i].boneid[j]);
                        }
                    }
                }
            }
        } else
        /* material */
        if(M3D_CHUNKMAGIC(data, 'M','T','R','L')) {
            data += sizeof(m3dchunk_t);
            M3D_GETSTR(material);
            M3D_LOG("Material");
            M3D_LOG(material);
            if(model->ci_s < 4 && !model->numcmap) model->errcode = M3D_ERR_CMAP;
            for(i = 0; i < model->nummaterial; i++)
                if(!strcmp(material, model->material[i].name)) {
                    model->errcode = M3D_ERR_MTRL;
                    M3D_LOG("Multiple definitions for material");
                    M3D_LOG(material);
                    material = NULL;
                    break;
                }
            if(material) {
                i = model->nummaterial++;
                if(model->flags & M3D_FLG_MTLLIB) {
                    m = model->material;
                    model->material = (m3dm_t*)M3D_MALLOC(model->nummaterial * sizeof(m3dm_t));
                    if(!model->material) goto memerr;
                    memcpy(model->material, m, (model->nummaterial - 1) * sizeof(m3dm_t));
                    if(model->texture) {
                        tx = model->texture;
                        model->texture = (m3dtx_t*)M3D_MALLOC(model->numtexture * sizeof(m3dtx_t));
                        if(!model->texture) goto memerr;
                        memcpy(model->texture, tx, model->numtexture * sizeof(m3dm_t));
                    }
                    model->flags &= ~M3D_FLG_MTLLIB;
                } else {
                    model->material = (m3dm_t*)M3D_REALLOC(model->material, model->nummaterial * sizeof(m3dm_t));
                    if(!model->material) goto memerr;
                }
                m = &model->material[i];
                m->numprop = 0;
                m->prop = NULL;
                m->name = material;
                m->prop = (m3dp_t*)M3D_REALLOC(m->prop, (len / 2) * sizeof(m3dp_t));
                if(!m->prop) goto memerr;
                while(data < chunk) {
                    i = m->numprop++;
                    m->prop[i].type = *data++;
                    m->prop[i].value.num = 0;
                    if(m->prop[i].type >= 128)
                        k = m3dpf_map;
                    else {
                        for(k = 256, j = 0; j < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); j++)
                            if(m->prop[i].type == m3d_propertytypes[j].id) { k = m3d_propertytypes[j].format; break; }
                    }
                    switch(k) {
                        case m3dpf_color:
                            switch(model->ci_s) {
                                case 1: m->prop[i].value.color = model->cmap ? model->cmap[data[0]] : 0; data++; break;
                                case 2: m->prop[i].value.color = model->cmap ? model->cmap[*((uint16_t*)data)] : 0; data += 2; break;
                                case 4: m->prop[i].value.color = *((uint32_t*)data); data += 4; break;
                            }
                        break;

                        case m3dpf_uint8: m->prop[i].value.num = *data++; break;
                        case m3dpf_uint16:m->prop[i].value.num = *((uint16_t*)data); data += 2; break;
                        case m3dpf_uint32:m->prop[i].value.num = *((uint32_t*)data); data += 4; break;
                        case m3dpf_float: m->prop[i].value.fnum = *((float*)data); data += 4; break;

                        case m3dpf_map:
                            M3D_GETSTR(material);
                            m->prop[i].value.textureid = _m3d_gettx(model, readfilecb, freecb, material);
                            if(model->errcode == M3D_ERR_ALLOC) goto memerr;
                            if(m->prop[i].value.textureid == (M3D_INDEX)-1U) {
                                M3D_LOG("Texture not found");
                                M3D_LOG(material);
                                m->numprop--;
                            }
                        break;

                        default:
                            M3D_LOG("Unknown material property in");
                            M3D_LOG(m->name);
                            model->errcode = M3D_ERR_UNKPROP;
                            data = chunk;
                        break;
                    }
                }
                m->prop = (m3dp_t*)M3D_REALLOC(m->prop, m->numprop * sizeof(m3dp_t));
                if(!m->prop) goto memerr;
            }
        } else
        /* face */
        if(M3D_CHUNKMAGIC(data, 'P','R','O','C')) {
            /* procedural surface */
            M3D_GETSTR(material);
            M3D_LOG("Procedural surface");
            M3D_LOG(material);
            _m3d_getpr(model, readfilecb, freecb, material);
        } else
        if(M3D_CHUNKMAGIC(data, 'M','E','S','H')) {
            M3D_LOG("Mesh data");
            /* mesh */
            data += sizeof(m3dchunk_t);
            mi = (M3D_INDEX)-1U;
            am = model->numface;
            while(data < chunk) {
                k = *data++;
                n = k >> 4;
                k &= 15;
                if(!n) {
                    /* use material */
                    mi = (M3D_INDEX)-1U;
                    M3D_GETSTR(material);
                    if(material) {
                        for(j = 0; j < model->nummaterial; j++)
                            if(!strcmp(material, model->material[j].name)) {
                                mi = (M3D_INDEX)j;
                                break;
                            }
                        if(mi == (M3D_INDEX)-1U) model->errcode = M3D_ERR_MTRL;
                    }
                    continue;
                }
                if(n != 3) { M3D_LOG("Only triangle mesh supported for now"); model->errcode = M3D_ERR_UNKMESH; return model; }
                i = model->numface++;
                if(model->numface > am) {
                    am = model->numface + 4095;
                    model->face = (m3df_t*)M3D_REALLOC(model->face, am * sizeof(m3df_t));
                    if(!model->face) goto memerr;
                }
                memset(&model->face[i], 255, sizeof(m3df_t)); /* set all index to -1 by default */
                model->face[i].materialid = mi;
                for(j = 0; j < n; j++) {
                    /* vertex */
                    data = _m3d_getidx(data, model->vi_s, &model->face[i].vertex[j]);
                    /* texcoord */
                    if(k & 1)
                        data = _m3d_getidx(data, model->ti_s, &model->face[i].texcoord[j]);
                    /* normal */
                    if(k & 2)
                        data = _m3d_getidx(data, model->vi_s, &model->face[i].normal[j]);
                }
            }
            model->face = (m3df_t*)M3D_REALLOC(model->face, model->numface * sizeof(m3df_t));
        } else
        /* action */
        if(M3D_CHUNKMAGIC(data, 'A','C','T','N')) {
            M3D_LOG("Action");
            i = model->numaction++;
            model->action = (m3da_t*)M3D_REALLOC(model->action, model->numaction * sizeof(m3da_t));
            if(!model->action) goto memerr;
            a = &model->action[i];
            data += sizeof(m3dchunk_t);
            M3D_GETSTR(a->name);
            M3D_LOG(a->name);
            a->numframe = *((uint16_t*)data); data += 2;
            if(a->numframe < 1) {
                model->numaction--;
            } else {
                a->durationmsec = *((uint32_t*)data); data += 4;
                a->frame = (m3dfr_t*)M3D_MALLOC(a->numframe * sizeof(m3dfr_t));
                if(!a->frame) goto memerr;
                for(i = 0; data < chunk && i < a->numframe; i++) {
                    a->frame[i].msec = *((uint32_t*)data); data += 4;
                    a->frame[i].numtransform = 0; a->frame[i].transform = NULL;
                    data = _m3d_getidx(data, model->fi_s, &a->frame[i].numtransform);
                    if(a->frame[i].numtransform > 0) {
                        a->frame[i].transform = (m3dtr_t*)M3D_MALLOC(a->frame[i].numtransform * sizeof(m3dtr_t));
                        for(j = 0; j < a->frame[i].numtransform; j++) {
                            data = _m3d_getidx(data, model->bi_s, &a->frame[i].transform[j].boneid);
                            data = _m3d_getidx(data, model->vi_s, &a->frame[i].transform[j].pos);
                            data = _m3d_getidx(data, model->vi_s, &a->frame[i].transform[j].ori);
                        }
                    }
                }
            }
        } else {
            i = model->numunknown++;
            model->unknown = (m3dchunk_t**)M3D_REALLOC(model->unknown, model->numunknown * sizeof(m3dchunk_t*));
            if(!model->unknown) goto memerr;
            model->unknown[i] = (m3dchunk_t*)data;
        }
    }
    /* calculate normals, normalize skin weights, create bone/vertex cross-references and calculate transform matrices */
#ifdef M3D_ASCII
postprocess:
#endif
    if(model) {
#ifndef M3D_NONORMALS
        if(model->numface && model->face) {
            memset(&vn, 0, sizeof(m3dv_t));
            /* if they are missing, calculate triangle normals into a temporary buffer */
            for(i = numnorm = 0; i < model->numface; i++)
                if(model->face[i].normal[0] == -1U) {
                    v0 = &model->vertex[model->face[i].vertex[0]]; v1 = &model->vertex[model->face[i].vertex[1]];
                    v2 = &model->vertex[model->face[i].vertex[2]];
                    va.x = v1->x - v0->x; va.y = v1->y - v0->y; va.z = v1->z - v0->z;
                    vb.x = v2->x - v0->x; vb.y = v2->y - v0->y; vb.z = v2->z - v0->z;
                    vn.x = (va.y * vb.z) - (va.z * vb.y);
                    vn.y = (va.z * vb.x) - (va.x * vb.z);
                    vn.z = (va.x * vb.y) - (va.y * vb.x);
                    w = _m3d_rsq((vn.x * vn.x) + (vn.y * vn.y) + (vn.z * vn.z));
                    vn.x *= w; vn.y *= w; vn.z *= w;
                    norm = _m3d_addnorm(norm, &numnorm, &vn, &j);
                    if(!ni) {
                        ni = (M3D_INDEX*)M3D_MALLOC(model->numface * sizeof(M3D_INDEX));
                        if(!ni) goto memerr;
                    }
                    ni[i] = j;
                }
            if(ni && norm) {
                vi = (M3D_INDEX*)M3D_MALLOC(model->numvertex * sizeof(M3D_INDEX));
                if(!vi) goto memerr;
                /* for each vertex, take the average of the temporary normals and use that */
                for(i = 0, n = model->numvertex; i < n; i++) {
                    memset(&vn, 0, sizeof(m3dv_t));
                    for(j = 0; j < model->numface; j++)
                        for(k = 0; k < 3; k++)
                            if(model->face[j].vertex[k] == i) {
                                vn.x += norm[ni[j]].x;
                                vn.y += norm[ni[j]].y;
                                vn.z += norm[ni[j]].z;
                            }
                    w = _m3d_rsq((vn.x * vn.x) + (vn.y * vn.y) + (vn.z * vn.z));
                    vn.x *= w; vn.y *= w; vn.z *= w;
                    vn.skinid = -1U;
                    model->vertex = _m3d_addnorm(model->vertex, &model->numvertex, &vn, &vi[i]);
                }
                for(j = 0; j < model->numface; j++)
                    for(k = 0; k < 3; k++)
                        model->face[j].normal[k] = vi[model->face[j].vertex[k]];
                M3D_FREE(norm);
                M3D_FREE(ni);
                M3D_FREE(vi);
            }
        }
#endif
        if(model->numbone && model->bone && model->numskin && model->skin && model->numvertex && model->vertex) {
#ifndef M3D_NOWEIGHTS
            for(i = 0; i < model->numvertex; i++) {
                if(model->vertex[i].skinid < M3D_INDEXMAX) {
                    sk = &model->skin[model->vertex[i].skinid];
                    w = (M3D_FLOAT)0.0;
                    for(j = 0; j < M3D_NUMBONE && sk->boneid[j] != (M3D_INDEX)-1U && sk->weight[j] > (M3D_FLOAT)0.0; j++)
                        w += sk->weight[j];
                    for(j = 0; j < M3D_NUMBONE && sk->boneid[j] != (M3D_INDEX)-1U && sk->weight[j] > (M3D_FLOAT)0.0; j++) {
                        sk->weight[j] /= w;
                        b = &model->bone[sk->boneid[j]];
                        k = b->numweight++;
                        b->weight = (m3dw_t*)M3D_REALLOC(b->weight, b->numweight * sizeof(m3da_t));
                        if(!b->weight) goto memerr;
                        b->weight[k].vertexid = i;
                        b->weight[k].weight = sk->weight[j];
                    }
                }
            }
#endif
#ifndef M3D_NOANIMATION
            for(i = 0; i < model->numbone; i++) {
                b = &model->bone[i];
                if(model->bone[i].parent == (M3D_INDEX)-1U) {
                    _m3d_mat((M3D_FLOAT*)&b->mat4, &model->vertex[b->pos], &model->vertex[b->ori]);
                } else {
                    _m3d_mat((M3D_FLOAT*)&r, &model->vertex[b->pos], &model->vertex[b->ori]);
                    _m3d_mul((M3D_FLOAT*)&b->mat4, (M3D_FLOAT*)&model->bone[b->parent].mat4, (M3D_FLOAT*)&r);
                }
            }
            for(i = 0; i < model->numbone; i++)
                _m3d_inv((M3D_FLOAT*)&model->bone[i].mat4);
#endif
        }
    }
    return model;
}

/**
 * Calculates skeletons for animation frames, returns a working copy (should be freed after use)
 */
m3dtr_t *m3d_frame(m3d_t *model, M3D_INDEX actionid, M3D_INDEX frameid, m3dtr_t *skeleton)
{
    unsigned int i;
    M3D_INDEX s = frameid;
    m3dfr_t *fr;

    if(!model || !model->numbone || !model->bone || (actionid != (M3D_INDEX)-1U && (!model->action ||
        actionid >= model->numaction || frameid >= model->action[actionid].numframe))) {
            model->errcode = M3D_ERR_UNKFRAME;
            return skeleton;
    }
    model->errcode = M3D_SUCCESS;
    if(!skeleton) {
        skeleton = (m3dtr_t*)M3D_MALLOC(model->numbone * sizeof(m3dtr_t));
        if(!skeleton) {
            model->errcode = M3D_ERR_ALLOC;
            return NULL;
        }
        goto gen;
    }
    if(actionid == (M3D_INDEX)-1U || !frameid) {
gen:    s = 0;
        for(i = 0; i < model->numbone; i++) {
            skeleton[i].boneid = i;
            skeleton[i].pos = model->bone[i].pos;
            skeleton[i].ori = model->bone[i].ori;
        }
    }
    if(actionid < model->numaction && (frameid || !model->action[actionid].frame[0].msec)) {
        for(; s <= frameid; s++) {
            fr = &model->action[actionid].frame[s];
            for(i = 0; i < fr->numtransform; i++) {
                skeleton[fr->transform[i].boneid].pos = fr->transform[i].pos;
                skeleton[fr->transform[i].boneid].ori = fr->transform[i].ori;
            }
        }
    }
    return skeleton;
}

#ifndef M3D_NOANIMATION
/**
 * Returns interpolated animation-pose, a working copy (should be freed after use)
 */
m3db_t *m3d_pose(m3d_t *model, M3D_INDEX actionid, uint32_t msec)
{
    unsigned int i, j, l;
    M3D_FLOAT r[16], t, d;
    m3db_t *ret;
    m3dv_t *v, *p, *f;
    m3dtr_t *tmp;
    m3dfr_t *fr;

    if(!model || !model->numbone || !model->bone) {
        model->errcode = M3D_ERR_UNKFRAME;
        return NULL;
    }
    ret = (m3db_t*)M3D_MALLOC(model->numbone * sizeof(m3db_t));
    if(!ret) {
        model->errcode = M3D_ERR_ALLOC;
        return NULL;
    }
    memcpy(ret, model->bone, model->numbone * sizeof(m3db_t));
    for(i = 0; i < model->numbone; i++)
        _m3d_inv((M3D_FLOAT*)&ret[i].mat4);
    if(!model->action || actionid >= model->numaction) {
        model->errcode = M3D_ERR_UNKFRAME;
        return ret;
    }
    msec %= model->action[actionid].durationmsec;
    model->errcode = M3D_SUCCESS;
    fr = &model->action[actionid].frame[0];
    for(j = l = 0; j < model->action[actionid].numframe && model->action[actionid].frame[j].msec <= msec; j++) {
        fr = &model->action[actionid].frame[j];
        l = fr->msec;
        for(i = 0; i < fr->numtransform; i++) {
            ret[fr->transform[i].boneid].pos = fr->transform[i].pos;
            ret[fr->transform[i].boneid].ori = fr->transform[i].ori;
        }
    }
    if(l != msec) {
        model->vertex = (m3dv_t*)M3D_REALLOC(model->vertex, (model->numvertex + 2 * model->numbone) * sizeof(m3dv_t));
        if(!model->vertex) {
            free(ret);
            model->errcode = M3D_ERR_ALLOC;
            return NULL;
        }
        tmp = (m3dtr_t*)M3D_MALLOC(model->numbone * sizeof(m3dtr_t));
        if(tmp) {
            for(i = 0; i < model->numbone; i++) {
                tmp[i].pos = ret[i].pos;
                tmp[i].ori = ret[i].ori;
            }
            fr = &model->action[actionid].frame[j % model->action[actionid].numframe];
            t = l >= fr->msec ? (M3D_FLOAT)1.0 : (M3D_FLOAT)(msec - l) / (M3D_FLOAT)(fr->msec - l);
            for(i = 0; i < fr->numtransform; i++) {
                tmp[fr->transform[i].boneid].pos = fr->transform[i].pos;
                tmp[fr->transform[i].boneid].ori = fr->transform[i].ori;
            }
            for(i = 0, j = model->numvertex; i < model->numbone; i++) {
                /* LERP interpolation of position */
                if(ret[i].pos != tmp[i].pos) {
                    p = &model->vertex[ret[i].pos];
                    f = &model->vertex[tmp[i].pos];
                    v = &model->vertex[j];
                    v->x = p->x + t * (f->x - p->x);
                    v->y = p->y + t * (f->y - p->y);
                    v->z = p->z + t * (f->z - p->z);
                    ret[i].pos = j++;
                }
                /* NLERP interpolation of orientation (could have used SLERP, that's nicer, but slower) */
                if(ret[i].ori != tmp[i].ori) {
                    p = &model->vertex[ret[i].ori];
                    f = &model->vertex[tmp[i].ori];
                    v = &model->vertex[j];
                    d = (p->w * f->w + p->x * f->x + p->y * f->y + p->z * f->z < 0) ? (M3D_FLOAT)-1.0 : (M3D_FLOAT)1.0;
                    v->x = p->x + t * (d*f->x - p->x);
                    v->y = p->y + t * (d*f->y - p->y);
                    v->z = p->z + t * (d*f->z - p->z);
                    v->w = p->w + t * (d*f->w - p->w);
                    d = _m3d_rsq(v->w * v->w + v->x * v->x + v->y * v->y + v->z * v->z);
                    v->x *= d; v->y *= d; v->z *= d; v->w *= d;
                    ret[i].ori = j++;
                }
            }
            M3D_FREE(tmp);
        }
    }
    for(i = 0; i < model->numbone; i++) {
        if(ret[i].parent == (M3D_INDEX)-1U) {
            _m3d_mat((M3D_FLOAT*)&ret[i].mat4, &model->vertex[ret[i].pos], &model->vertex[ret[i].ori]);
        } else {
            _m3d_mat((M3D_FLOAT*)&r, &model->vertex[ret[i].pos], &model->vertex[ret[i].ori]);
            _m3d_mul((M3D_FLOAT*)&ret[i].mat4, (M3D_FLOAT*)&ret[ret[i].parent].mat4, (M3D_FLOAT*)&r);
        }
    }
    return ret;
}

#endif /* M3D_NOANIMATION */

#endif /* M3D_IMPLEMENTATION */

#if !defined(M3D_NODUP) && (!defined(M3D_NOIMPORTER) || defined(M3D_EXPORTER))
/**
 * Free the in-memory model
 */
void m3d_free(m3d_t *model)
{
    unsigned int i, j;

    if(!model) return;
#ifdef M3D_ASCII
    /* if model imported from ASCII, we have to free all strings as well */
    if(model->flags & M3D_FLG_FREESTR) {
        if(model->name) M3D_FREE(model->name);
        if(model->license) M3D_FREE(model->license);
        if(model->author) M3D_FREE(model->author);
        if(model->desc) M3D_FREE(model->desc);
        if(model->bone)
            for(i = 0; i < model->numbone; i++)
                if(model->bone[i].name)
                    M3D_FREE(model->bone[i].name);
        if(model->material)
            for(i = 0; i < model->nummaterial; i++)
                if(model->material[i].name)
                    M3D_FREE(model->material[i].name);
        if(model->action)
            for(i = 0; i < model->numaction; i++)
                if(model->action[i].name)
                    M3D_FREE(model->action[i].name);
        if(model->texture)
            for(i = 0; i < model->numtexture; i++)
                if(model->texture[i].name)
                    M3D_FREE(model->texture[i].name);
        if(model->unknown)
            for(i = 0; i < model->numunknown; i++)
                if(model->unknown[i])
                    M3D_FREE(model->unknown[i]);
    }
#endif
    if(model->flags & M3D_FLG_FREERAW) M3D_FREE(model->raw);

    if(model->tmap) M3D_FREE(model->tmap);
    if(model->bone) {
        for(i = 0; i < model->numbone; i++)
            if(model->bone[i].weight)
                M3D_FREE(model->bone[i].weight);
        M3D_FREE(model->bone);
    }
    if(model->skin) M3D_FREE(model->skin);
    if(model->vertex) M3D_FREE(model->vertex);
    if(model->face) M3D_FREE(model->face);
    if(model->material && !(model->flags & M3D_FLG_MTLLIB)) {
        for(i = 0; i < model->nummaterial; i++)
            if(model->material[i].prop) M3D_FREE(model->material[i].prop);
        M3D_FREE(model->material);
    }
    if(model->texture) {
        for(i = 0; i < model->numtexture; i++)
            if(model->texture[i].d) M3D_FREE(model->texture[i].d);
        M3D_FREE(model->texture);
    }
    if(model->action) {
        for(i = 0; i < model->numaction; i++) {
            if(model->action[i].frame) {
                for(j = 0; j < model->action[i].numframe; j++)
                    if(model->action[i].frame[j].transform) M3D_FREE(model->action[i].frame[j].transform);
                M3D_FREE(model->action[i].frame);
            }
        }
        M3D_FREE(model->action);
    }
    if(model->inlined) M3D_FREE(model->inlined);
    if(model->unknown) M3D_FREE(model->unknown);
    free(model);
}
#endif

#ifdef M3D_EXPORTER
typedef struct {
    char *str;
    uint32_t offs;
} m3dstr_t;

/* create unique list of strings */
static m3dstr_t *_m3d_addstr(m3dstr_t *str, uint32_t *numstr, char *s)
{
    uint32_t i;
    if(!s || !*s) return str;
    if(str) {
        for(i = 0; i < *numstr; i++)
            if(str[i].str == s || !strcmp(str[i].str, s)) return str;
    }
    str = (m3dstr_t*)M3D_REALLOC(str, ((*numstr) + 1) * sizeof(m3dstr_t));
    str[*numstr].str = s;
    str[*numstr].offs = 0;
    (*numstr)++;
    return str;
}

/* add strings to header */
m3dhdr_t *_m3d_addhdr(m3dhdr_t *h, m3dstr_t *s)
{
    int i;
    char *safe = _m3d_safestr(s->str, 0);
    i = strlen(safe);
    h = (m3dhdr_t*)M3D_REALLOC(h, h->length + i+1);
    if(!h) { M3D_FREE(safe); return NULL; }
    memcpy((uint8_t*)h + h->length, safe, i+1);
    s->offs = h->length - 16;
    h->length += i+1;
    M3D_FREE(safe);
    return h;
}

/* return offset of string */
static uint32_t _m3d_stridx(m3dstr_t *str, uint32_t numstr, char *s)
{
    uint32_t i;
    char *safe;
    if(!s || !*s) return 0;
    if(str) {
        safe = _m3d_safestr(s, 0);
        if(!safe) return 0;
        if(!*safe) {
            free(safe);
            return 0;
        }
        for(i = 0; i < numstr; i++)
            if(!strcmp(str[i].str, s)) {
                free(safe);
                return str[i].offs;
            }
        free(safe);
    }
    return 0;
}

/* compare two colors by HSV value */
_inline static int _m3d_cmapcmp(const void *a, const void *b)
{
    uint8_t *A = (uint8_t*)a,  *B = (uint8_t*)b;
    _register int m, vA, vB;
    /* get HSV value for A */
    m = A[2] < A[1]? A[2] : A[1]; if(A[0] < m) m = A[0];
    vA = A[2] > A[1]? A[2] : A[1]; if(A[0] > vA) vA = A[0];
    /* get HSV value for B */
    m = B[2] < B[1]? B[2] : B[1]; if(B[0] < m) m = B[0];
    vB = B[2] > B[1]? B[2] : B[1]; if(B[0] > vB) vB = B[0];
    return vA - vB;
}

/* create sorted list of colors */
static uint32_t *_m3d_addcmap(uint32_t *cmap, uint32_t *numcmap, uint32_t color)
{
    uint32_t i;
    if(cmap) {
        for(i = 0; i < *numcmap; i++)
            if(cmap[i] == color) return cmap;
    }
    cmap = (uint32_t*)M3D_REALLOC(cmap, ((*numcmap) + 1) * sizeof(uint32_t));
    for(i = 0; i < *numcmap && _m3d_cmapcmp(&color, &cmap[i]) > 0; i++);
    if(i < *numcmap) memmove(&cmap[i+1], &cmap[i], ((*numcmap) - i)*sizeof(uint32_t));
    cmap[i] = color;
    (*numcmap)++;
    return cmap;
}

/* look up a color and return its index */
static uint32_t _m3d_cmapidx(uint32_t *cmap, uint32_t numcmap, uint32_t color)
{
    uint32_t i;
    for(i = 0; i < numcmap; i++)
        if(cmap[i] == color) return i;
    return 0;
}

/* add vertex to list */
static m3dv_t *_m3d_addvrtx(m3dv_t *vrtx, uint32_t *numvrtx, m3dv_t *v, uint32_t *idx)
{
    uint32_t i;
    if(v->x == (M3D_FLOAT)-0.0) v->x = (M3D_FLOAT)0.0;
    if(v->y == (M3D_FLOAT)-0.0) v->y = (M3D_FLOAT)0.0;
    if(v->z == (M3D_FLOAT)-0.0) v->z = (M3D_FLOAT)0.0;
    if(v->w == (M3D_FLOAT)-0.0) v->w = (M3D_FLOAT)0.0;
    if(vrtx) {
        for(i = 0; i < *numvrtx; i++)
            if(!memcmp(&vrtx[i], v, sizeof(m3dv_t))) { *idx = i; return vrtx; }
    }
    vrtx = (m3dv_t*)M3D_REALLOC(vrtx, ((*numvrtx) + 1) * sizeof(m3dv_t));
    memcpy(&vrtx[*numvrtx], v, sizeof(m3dv_t));
    *idx = *numvrtx;
    (*numvrtx)++;
    return vrtx;
}

/* add texture map to list */
static m3dti_t *_m3d_addtmap(m3dti_t *tmap, uint32_t *numtmap, m3dti_t *t, uint32_t *idx)
{
    uint32_t i;
    if(tmap) {
        for(i = 0; i < *numtmap; i++)
            if(!memcmp(&tmap[i], t, sizeof(m3dti_t))) { *idx = i; return tmap; }
    }
    tmap = (m3dti_t*)M3D_REALLOC(tmap, ((*numtmap) + 1) * sizeof(m3dti_t));
    memcpy(&tmap[*numtmap], t, sizeof(m3dti_t));
    *idx = *numtmap;
    (*numtmap)++;
    return tmap;
}

/* add material to list */
static m3dm_t **_m3d_addmtrl(m3dm_t **mtrl, uint32_t *nummtrl, m3dm_t *m, uint32_t *idx)
{
    uint32_t i;
    if(mtrl) {
        for(i = 0; i < *nummtrl; i++)
            if(mtrl[i]->name == m->name || !strcmp(mtrl[i]->name, m->name)) { *idx = i; return mtrl; }
    }
    mtrl = (m3dm_t**)M3D_REALLOC(mtrl, ((*nummtrl) + 1) * sizeof(m3dm_t*));
    mtrl[*nummtrl] = m;
    *idx = *nummtrl;
    (*nummtrl)++;
    return mtrl;
}

/* add index to output */
static unsigned char *_m3d_addidx(unsigned char *out, char type, uint32_t idx) {
    switch(type) {
        case 1: *out++ = (uint8_t)(idx); break;
        case 2: *((uint16_t*)out) = (uint16_t)(idx); out += 2; break;
        case 4: *((uint32_t*)out) = (uint32_t)(idx); out += 4; break;
        /* case 0: case 8: break; */
    }
    return out;
}

/* round a vertex position */
static void _m3d_round(int quality, m3dv_t *src, m3dv_t *dst)
{
    _register int t;
    /* copy additional attributes */
    if(src != dst) memcpy(dst, src, sizeof(m3dv_t));
    /* round according to quality */
    switch(quality) {
        case M3D_EXP_INT8:
            t = src->x * 127; dst->x = (M3D_FLOAT)t / 127;
            t = src->y * 127; dst->y = (M3D_FLOAT)t / 127;
            t = src->z * 127; dst->z = (M3D_FLOAT)t / 127;
            t = src->w * 127; dst->w = (M3D_FLOAT)t / 127;
        break;
        case M3D_EXP_INT16:
            t = src->x * 32767; dst->x = (M3D_FLOAT)t / 32767;
            t = src->y * 32767; dst->y = (M3D_FLOAT)t / 32767;
            t = src->z * 32767; dst->z = (M3D_FLOAT)t / 32767;
            t = src->w * 32767; dst->w = (M3D_FLOAT)t / 32767;
        break;
    }
}

#ifdef M3D_ASCII
/* add a bone to ascii output */
static char *_m3d_prtbone(char *ptr, m3db_t *bone, M3D_INDEX numbone, M3D_INDEX parent, uint32_t level)
{
    uint32_t i, j;
    char *sn;

    if(level > M3D_BONEMAXLEVEL || !bone) return ptr;
    for(i = 0; i < numbone; i++) {
        if(bone[i].parent == parent) {
            for(j = 0; j < level; j++) *ptr++ = '/';
            sn = _m3d_safestr(bone[i].name, 0);
            ptr += sprintf(ptr, "%d %d %s\r\n", bone[i].pos, bone[i].ori, sn);
            M3D_FREE(sn);
            ptr = _m3d_prtbone(ptr, bone, numbone, i, level + 1);
        }
    }
    return ptr;
}
#endif

/**
 * Function to encode an in-memory model into on storage Model 3D format
 */
unsigned char *m3d_save(m3d_t *model, int quality, int flags, unsigned int *size)
{
#ifdef M3D_ASCII
    const char *ol;
    char *ptr;
#endif
    char vc_s, vi_s, si_s, ci_s, ti_s, bi_s, nb_s, sk_s, fi_s;
    char *sn = NULL, *sl = NULL, *sa = NULL, *sd = NULL;
    unsigned char *out = NULL, *z = NULL, weights[M3D_NUMBONE];
    unsigned int i, j, k, l, len, chunklen, *length;
    float scale = 0.0f, min_x, max_x, min_y, max_y, min_z, max_z;
    uint32_t idx, numcmap = 0, *cmap = NULL, numvrtx = 0, numtmap = 0, numbone = 0;
    uint32_t numskin = 0, numactn = 0, *actn = NULL, numstr = 0, nummtrl = 0, maxt = 0;
    m3dstr_t *str = NULL;
    m3dv_t *vrtx = NULL, vertex;
    m3dti_t *tmap = NULL, tcoord;
    m3db_t *bone = NULL;
    m3ds_t *skin = NULL;
    m3df_t *face = NULL;
    m3dhdr_t *h = NULL;
    m3dm_t *m, **mtrl = NULL;
    m3da_t *a;
    M3D_INDEX last;

    if(!model) {
        if(size) *size = 0;
        return NULL;
    }
    model->errcode = M3D_SUCCESS;
#ifdef M3D_ASCII
    if(flags & M3D_EXP_ASCII) quality = M3D_EXP_DOUBLE;
#endif
    /* collect array elements that are actually referenced */
    if(model->numface && model->face && !(flags & M3D_EXP_NOFACE)) {
        face = (m3df_t*)M3D_MALLOC(model->numface * sizeof(m3df_t));
        if(!face) goto memerr;
        memset(face, 255, model->numface * sizeof(m3df_t));
        last = (M3D_INDEX)-1U;
        for(i = 0; i < model->numface; i++) {
            face[i].materialid = (M3D_INDEX)-1U;
            if(!(flags & M3D_EXP_NOMATERIAL) && model->face[i].materialid != last) {
                last = model->face[i].materialid;
                if(last < model->nummaterial) {
                    mtrl = _m3d_addmtrl(mtrl, &nummtrl, &model->material[last], &face[i].materialid);
                    if(!mtrl) goto memerr;
                }
            }
            for(j = 0; j < 3; j++) {
                k = model->face[i].vertex[j];
                if(quality < M3D_EXP_FLOAT) {
                    _m3d_round(quality, &model->vertex[k], &vertex);
                    vrtx = _m3d_addvrtx(vrtx, &numvrtx, &vertex, &idx);
                } else
                    vrtx = _m3d_addvrtx(vrtx, &numvrtx, &model->vertex[k], &idx);
                if(!vrtx) goto memerr;
                face[i].vertex[j] = (M3D_INDEX)idx;
                if(!(flags & M3D_EXP_NOCMAP)) {
                    cmap = _m3d_addcmap(cmap, &numcmap, model->vertex[k].color);
                    if(!cmap) goto memerr;
                }
                k = model->face[i].normal[j];
                if(k < model->numvertex && !(flags & M3D_EXP_NONORMAL)) {
                    if(quality < M3D_EXP_FLOAT) {
                        _m3d_round(quality, &model->vertex[k], &vertex);
                        vrtx = _m3d_addnorm(vrtx, &numvrtx, &vertex, &idx);
                    } else
                        vrtx = _m3d_addnorm(vrtx, &numvrtx, &model->vertex[k], &idx);
                    if(!vrtx) goto memerr;
                    face[i].normal[j] = (M3D_INDEX)idx;
                }
                k = model->face[i].texcoord[j];
                if(k < model->numtmap) {
                    switch(quality) {
                        case M3D_EXP_INT8:
                            l = model->tmap[k].u * 255; tcoord.u = (M3D_FLOAT)l / 255;
                            l = model->tmap[k].v * 255; tcoord.v = (M3D_FLOAT)l / 255;
                        break;
                        case M3D_EXP_INT16:
                            l = model->tmap[k].u * 65535; tcoord.u = (M3D_FLOAT)l / 65535;
                            l = model->tmap[k].v * 65535; tcoord.v = (M3D_FLOAT)l / 65535;
                        break;
                        default:
                            tcoord.u = model->tmap[k].u;
                            tcoord.v = model->tmap[k].v;
                        break;
                    }
                    if(flags & M3D_EXP_FLIPTXTCRD)
                        tcoord.v = (M3D_FLOAT)1.0 - tcoord.v;
                    tmap = _m3d_addtmap(tmap, &numtmap, &tcoord, &idx);
                    if(!tmap) goto memerr;
                    face[i].texcoord[j] = (M3D_INDEX)idx;
                }
            }
            /* convert from CW to CCW */
            if(flags & M3D_EXP_IDOSUCK) {
                j = face[i].vertex[1];
                face[i].vertex[1] = face[i].vertex[2];
                face[i].vertex[2] = face[i].vertex[1];
                j = face[i].normal[1];
                face[i].normal[1] = face[i].normal[2];
                face[i].normal[2] = face[i].normal[1];
                j = face[i].texcoord[1];
                face[i].texcoord[1] = face[i].texcoord[2];
                face[i].texcoord[2] = face[i].texcoord[1];
            }
        }
    } else if(!(flags & M3D_EXP_NOMATERIAL)) {
        /* without a face, simply add all materials, because it can be an mtllib */
        nummtrl = model->nummaterial;
    }
    /* add colors to color map and texture names to string table */
    for(i = 0; i < nummtrl; i++) {
        m = !mtrl ? &model->material[i] : mtrl[i];
        str = _m3d_addstr(str, &numstr, m->name);
        if(!str) goto memerr;
        for(j = 0; j < mtrl[i]->numprop; j++) {
            if(!(flags & M3D_EXP_NOCMAP) && m->prop[j].type < 128) {
                for(l = 0; l < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); l++) {
                    if(m->prop[j].type == m3d_propertytypes[l].id && m3d_propertytypes[l].format == m3dpf_color) {
                        cmap = _m3d_addcmap(cmap, &numcmap, m->prop[j].value.color);
                        if(!cmap) goto memerr;
                        break;
                    }
                }
            }
            if(m->prop[j].type >= 128 && m->prop[j].value.textureid < model->numtexture &&
                model->texture[m->prop[j].value.textureid].name) {
                str = _m3d_addstr(str, &numstr, model->texture[m->prop[j].value.textureid].name);
                if(!str) goto memerr;
            }
        }
    }
    /* get bind-pose skeleton and skin */
    if(model->numbone && model->bone && !(flags & M3D_EXP_NOBONE)) {
        numbone = model->numbone;
        bone = (m3db_t*)M3D_MALLOC(model->numbone * sizeof(m3db_t));
        if(!bone) goto memerr;
        memset(bone, 0, model->numbone * sizeof(m3db_t));
        for(i = 0; i < model->numbone; i++) {
            bone[i].parent = model->bone[i].parent;
            bone[i].name = model->bone[i].name;
            str = _m3d_addstr(str, &numstr, bone[i].name);
            if(!str) goto memerr;
            if(quality < M3D_EXP_FLOAT) {
                _m3d_round(quality, &model->vertex[model->bone[i].pos], &vertex);
                vrtx = _m3d_addvrtx(vrtx, &numvrtx, &vertex, &k);
            } else
                vrtx = _m3d_addvrtx(vrtx, &numvrtx, &model->vertex[model->bone[i].pos], &k);
            if(!vrtx) goto memerr;
            bone[i].pos = (M3D_INDEX)k;
            if(quality < M3D_EXP_FLOAT) {
                _m3d_round(quality, &model->vertex[model->bone[i].ori], &vertex);
                vrtx = _m3d_addvrtx(vrtx, &numvrtx, &vertex, &k);
            } else
                vrtx = _m3d_addvrtx(vrtx, &numvrtx, &model->vertex[model->bone[i].ori], &k);
            if(!vrtx) goto memerr;
            bone[i].ori = (M3D_INDEX)k;
        }
    }
    /* actions, animated skeleton poses */
    if(model->numaction && model->action && !(flags & M3D_EXP_NOACTION)) {
        for(j = 0; j < model->numaction; j++) {
            a = &model->action[j];
            str = _m3d_addstr(str, &numstr, a->name);
            if(!str) goto memerr;
            if(a->numframe > 65535) a->numframe = 65535;
            for(i = 0; i < a->numframe; i++) {
                l = numactn;
                numactn += (a->frame[i].numtransform * 2);
                if(a->frame[i].numtransform > maxt)
                    maxt = a->frame[i].numtransform;
                actn = (uint32_t*)M3D_REALLOC(actn, numactn * sizeof(uint32_t));
                if(!actn) goto memerr;
                for(k = 0; k < a->frame[i].numtransform; k++) {
                    if(quality < M3D_EXP_FLOAT) {
                        _m3d_round(quality, &model->vertex[a->frame[i].transform[k].pos], &vertex);
                        vrtx = _m3d_addvrtx(vrtx, &numvrtx, &vertex, &actn[l++]);
                        if(!vrtx) goto memerr;
                        _m3d_round(quality, &model->vertex[a->frame[i].transform[k].ori], &vertex);
                        vrtx = _m3d_addvrtx(vrtx, &numvrtx, &vertex, &actn[l++]);
                    } else {
                        vrtx = _m3d_addvrtx(vrtx, &numvrtx, &model->vertex[a->frame[i].transform[k].pos], &actn[l++]);
                        if(!vrtx) goto memerr;
                        vrtx = _m3d_addvrtx(vrtx, &numvrtx, &model->vertex[a->frame[i].transform[k].ori], &actn[l++]);
                    }
                    if(!vrtx) goto memerr;
                }
            }
        }
    }
    /* normalize bounding cube and collect referenced skin records */
    if(numvrtx) {
        min_x = min_y = min_z = 1e10;
        max_x = max_y = max_z = -1e10;
        j = model->numskin && model->skin && !(flags & M3D_EXP_NOBONE);
        for(i = 0; i < numvrtx; i++) {
            if(j && model->numskin && model->skin && vrtx[i].skinid < M3D_INDEXMAX) {
                skin = _m3d_addskin(skin, &numskin, &model->skin[vrtx[i].skinid], &idx);
                if(!skin) goto memerr;
                vrtx[i].skinid = idx;
            }
            if(vrtx[i].skinid == (M3D_INDEX)-2U) continue;
            if(vrtx[i].x > max_x) max_x = vrtx[i].x;
            if(vrtx[i].x < min_x) min_x = vrtx[i].x;
            if(vrtx[i].y > max_y) max_y = vrtx[i].y;
            if(vrtx[i].y < min_y) min_y = vrtx[i].y;
            if(vrtx[i].z > max_z) max_z = vrtx[i].z;
            if(vrtx[i].z < min_z) min_z = vrtx[i].z;
        }
        if(min_x < 0.0f) min_x = -min_x;
        if(max_x < 0.0f) max_x = -max_x;
        if(min_y < 0.0f) min_y = -min_y;
        if(max_y < 0.0f) max_y = -max_y;
        if(min_z < 0.0f) min_z = -min_z;
        if(max_z < 0.0f) max_z = -max_z;
        scale = min_x;
        if(max_x > scale) scale = max_x;
        if(min_y > scale) scale = min_y;
        if(max_y > scale) scale = max_y;
        if(min_z > scale) scale = min_z;
        if(max_z > scale) scale = max_z;
        if(scale == 0.0f) scale = 1.0f;
        if(scale != 1.0f && !(flags & M3D_EXP_NORECALC)) {
            for(i = 0; i < numvrtx; i++) {
                if(vrtx[i].skinid == (M3D_INDEX)-2U) continue;
                vrtx[i].x /= scale;
                vrtx[i].y /= scale;
                vrtx[i].z /= scale;
            }
        }
    }
    /* if there's only one black color, don't store it */
    if(numcmap == 1 && cmap && !cmap[0]) numcmap = 0;
    /* at least 3 UV coordinate required for texture mapping */
    if(numtmap < 3 && tmap) numtmap = 0;
    /* meta info */
    sn = _m3d_safestr(model->name && *model->name ? model->name : (char*)"(noname)", 2);
    sl = _m3d_safestr(model->license ? model->license : (char*)"MIT", 2);
    sa = _m3d_safestr(model->author ? model->author : getenv("LOGNAME"), 2);
    if(!sn || !sl || !sa) {
memerr: if(face) M3D_FREE(face);
        if(cmap) M3D_FREE(cmap);
        if(tmap) M3D_FREE(tmap);
        if(mtrl) M3D_FREE(mtrl);
        if(vrtx) M3D_FREE(vrtx);
        if(bone) M3D_FREE(bone);
        if(skin) M3D_FREE(skin);
        if(actn) M3D_FREE(actn);
        if(sn) M3D_FREE(sn);
        if(sl) M3D_FREE(sl);
        if(sa) M3D_FREE(sa);
        if(sd) M3D_FREE(sd);
        if(out) M3D_FREE(out);
        if(str) M3D_FREE(str);
        if(h) M3D_FREE(h);
        M3D_LOG("Out of memory");
        model->errcode = M3D_ERR_ALLOC;
        return NULL;
    }
    if(model->scale > (M3D_FLOAT)0.0) scale = (float)model->scale;
    if(scale <= 0.0f) scale = 1.0f;
#ifdef M3D_ASCII
    if(flags & M3D_EXP_ASCII) {
        /* use CRLF to make model creators on Win happy... */
        sd = _m3d_safestr(model->desc, 1);
        if(!sd) goto memerr;
        ol = setlocale(LC_NUMERIC, NULL);
        setlocale(LC_NUMERIC, "C");
        /* header */
        len = 64 + strlen(sn) + strlen(sl) + strlen(sa) + strlen(sd);
        out = (unsigned char*)M3D_MALLOC(len);
        if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
        ptr = (char*)out;
        ptr += sprintf(ptr, "3dmodel %g\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n", scale,
            sn, sl, sa, sd);
        M3D_FREE(sn); M3D_FREE(sl); M3D_FREE(sa); M3D_FREE(sd);
        sn = sl = sa = sd = NULL;
        /* texture map */
        if(numtmap && tmap && !(flags & M3D_EXP_NOTXTCRD) && !(flags & M3D_EXP_NOFACE)) {
            ptr -= (uint64_t)out; len = (uint64_t)ptr + numtmap * 32 + 12;
            out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
            if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
            ptr += sprintf(ptr, "Textmap\r\n");
            for(i = 0; i < numtmap; i++)
                ptr += sprintf(ptr, "%g %g\r\n", tmap[i].u, tmap[i].v);
            ptr += sprintf(ptr, "\r\n");
        }
        /* vertex chunk */
        if(numvrtx && vrtx && !(flags & M3D_EXP_NOFACE)) {
            ptr -= (uint64_t)out; len = (uint64_t)ptr + numvrtx * 128 + 10;
            out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
            if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
            ptr += sprintf(ptr, "Vertex\r\n");
            for(i = 0; i < numvrtx; i++) {
                ptr += sprintf(ptr, "%g %g %g %g", vrtx[i].x, vrtx[i].y, vrtx[i].z, vrtx[i].w);
                if(!(flags & M3D_EXP_NOCMAP) && vrtx[i].color)
                    ptr += sprintf(ptr, " #%08x", vrtx[i].color);
                if(!(flags & M3D_EXP_NOBONE) && numbone && numskin && vrtx[i].skinid != (M3D_INDEX)-1U &&
                    vrtx[i].skinid != (M3D_INDEX)-2U) {
                    if(skin[vrtx[i].skinid].weight[0] == (M3D_FLOAT)1.0)
                        ptr += sprintf(ptr, " %d", skin[vrtx[i].skinid].boneid[0]);
                    else
                        for(j = 0; j < M3D_NUMBONE && skin[vrtx[i].skinid].boneid[j] != (M3D_INDEX)-1U &&
                            skin[vrtx[i].skinid].weight[j] > (M3D_FLOAT)0.0; j++)
                            ptr += sprintf(ptr, " %d:%g", skin[vrtx[i].skinid].boneid[j],
                                skin[vrtx[i].skinid].weight[j]);
                }
                ptr += sprintf(ptr, "\r\n");
            }
            ptr += sprintf(ptr, "\r\n");
        }
        /* bones chunk */
        if(numbone && bone && !(flags & M3D_EXP_NOBONE)) {
            ptr -= (uint64_t)out; len = (uint64_t)ptr + 9;
            for(i = 0; i < numbone; i++) {
                len += strlen(bone[i].name) + 128;
            }
            out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
            if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
            ptr += sprintf(ptr, "Bones\r\n");
            ptr = _m3d_prtbone(ptr, bone, numbone, (M3D_INDEX)-1U, 0);
            ptr += sprintf(ptr, "\r\n");
        }
        /* materials */
        if(nummtrl && !(flags & M3D_EXP_NOMATERIAL)) {
            for(j = 0; j < nummtrl; j++) {
                m = !mtrl ? &model->material[j] : mtrl[j];
                sn = _m3d_safestr(m->name, 0);
                if(!sn) { setlocale(LC_NUMERIC, ol); goto memerr; }
                ptr -= (uint64_t)out; len = (uint64_t)ptr + strlen(sn) + 12;
                for(i = 0; i < m->numprop; i++) {
                    if(m->prop[i].type < 128)
                        len += 32;
                    else if(m->prop[i].value.textureid < model->numtexture && model->texture[m->prop[i].value.textureid].name)
                        len += strlen(model->texture[m->prop[i].value.textureid].name) + 16;
                }
                out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
                if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
                ptr += sprintf(ptr, "Material %s\r\n", sn);
                M3D_FREE(sn); sn = NULL;
                for(i = 0; i < m->numprop; i++) {
                    k = 256;
                    if(m->prop[i].type >= 128) {
                        for(l = 0; l < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); l++)
                            if(m->prop[i].type == m3d_propertytypes[l].id) {
                                sn = m3d_propertytypes[l].key;
                                break;
                            }
                        if(!sn)
                            for(l = 0; l < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); l++)
                                if(m->prop[i].type - 128 == m3d_propertytypes[l].id) {
                                    sn = m3d_propertytypes[l].key;
                                    break;
                                }
                        k = sn ? m3dpf_map : 256;
                    } else {
                        for(l = 0; l < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); l++)
                            if(m->prop[i].type == m3d_propertytypes[l].id) {
                                sn = m3d_propertytypes[l].key;
                                k = m3d_propertytypes[l].format;
                                break;
                            }
                    }
                    switch(k) {
                        case m3dpf_color: ptr += sprintf(ptr, "%s #%08x\r\n", sn, m->prop[i].value.color); break;
                        case m3dpf_uint8:
                        case m3dpf_uint16:
                        case m3dpf_uint32: ptr += sprintf(ptr, "%s %d\r\n", sn, m->prop[i].value.num); break;
                        case m3dpf_float:  ptr += sprintf(ptr, "%s %g\r\n", sn, m->prop[i].value.fnum); break;
                        case m3dpf_map:
                            if(m->prop[i].value.textureid < model->numtexture &&
                                model->texture[m->prop[i].value.textureid].name) {
                                sl = _m3d_safestr(model->texture[m->prop[i].value.textureid].name, 0);
                                if(!sl) { setlocale(LC_NUMERIC, ol); goto memerr; }
                                if(*sl)
                                    ptr += sprintf(ptr, "map_%s %s\r\n", sn, sl);
                                M3D_FREE(sn); M3D_FREE(sl); sl = NULL;
                            }
                        break;
                    }
                    sn = NULL;
                }
                ptr += sprintf(ptr, "\r\n");
            }
        }
        /* mesh face */
        if(model->numface && face && !(flags & M3D_EXP_NOFACE)) {
            ptr -= (uint64_t)out; len = (uint64_t)ptr + model->numface * 128 + 6;
            last = (M3D_INDEX)-1U;
            if(!(flags & M3D_EXP_NOMATERIAL))
                for(i = 0; i < model->numface; i++) {
                    if(face[i].materialid != last) {
                        last = face[i].materialid;
                        if(last < nummtrl)
                            len += strlen(mtrl[last]->name);
                        len += 6;
                    }
                }
            out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
            if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
            ptr += sprintf(ptr, "Mesh\r\n");
            last = (M3D_INDEX)-1U;
            for(i = 0; i < model->numface; i++) {
                if(!(flags & M3D_EXP_NOMATERIAL) && face[i].materialid != last) {
                    last = face[i].materialid;
                    if(last < nummtrl) {
                        sn = _m3d_safestr(mtrl[last]->name, 0);
                        if(!sn) { setlocale(LC_NUMERIC, ol); goto memerr; }
                        ptr += sprintf(ptr, "use %s\r\n", sn);
                        M3D_FREE(sn); sn = NULL;
                    } else
                        ptr += sprintf(ptr, "use\r\n");
                }
                /* hardcoded triangles. Should be repeated as many times as the number of edges in polygon */
                for(j = 0; j < 3; j++) {
                    ptr += sprintf(ptr, "%s%d", j?" ":"", face[i].vertex[j]);
                    if(!(flags & M3D_EXP_NOTXTCRD) && (face[i].texcoord[j] != (M3D_INDEX)-1U))
                        ptr += sprintf(ptr, "/%d", face[i].texcoord[j]);
                    if(!(flags & M3D_EXP_NONORMAL) && (face[i].normal[j] != (M3D_INDEX)-1U))
                        ptr += sprintf(ptr, "%s/%d",
                            (flags & M3D_EXP_NOTXTCRD) || (face[i].texcoord[j] == (M3D_INDEX)-1U)? "/" : "",
                            face[i].normal[j]);
                }
                ptr += sprintf(ptr, "\r\n");
            }
            ptr += sprintf(ptr, "\r\n");
        }
        /* actions */
        if(model->numaction && model->action && numactn && actn && !(flags & M3D_EXP_NOACTION)) {
            l = 0;
            for(j = 0; j < model->numaction; j++) {
                a = &model->action[j];
                sn = _m3d_safestr(a->name, 0);
                if(!sn) { setlocale(LC_NUMERIC, ol); goto memerr; }
                ptr -= (uint64_t)out; len = (uint64_t)ptr + strlen(sn) + 48;
                for(i = 0; i < a->numframe; i++)
                    len += a->frame[i].numtransform * 128 + 8;
                out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
                if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
                ptr += sprintf(ptr, "Action %d %s\r\n", a->durationmsec, sn);
                M3D_FREE(sn); sn = NULL;
                if(a->numframe > 65535) a->numframe = 65535;
                for(i = 0; i < a->numframe; i++) {
                    ptr += sprintf(ptr, "frame %d\r\n", a->frame[i].msec);
                    for(k = 0; k < a->frame[i].numtransform; k++) {
                        ptr += sprintf(ptr, "%d %d %d\r\n", a->frame[i].transform[k].boneid, actn[l], actn[l + 1]);
                        l += 2;
                    }
                }
                ptr += sprintf(ptr, "\r\n");
            }
        }
        /* extra info */
        if(model->numunknown && (flags & M3D_EXP_EXTRA)) {
            for(i = 0; i < model->numunknown; i++) {
                if(model->unknown[i]->length < 9) continue;
                ptr -= (uint64_t)out; len = (uint64_t)ptr + 17 + model->unknown[i]->length * 3;
                out = (unsigned char*)M3D_REALLOC(out, len); ptr += (uint64_t)out;
                if(!out) { setlocale(LC_NUMERIC, ol); goto memerr; }
                ptr += sprintf(ptr, "Extra %c%c%c%c\r\n",
                    model->unknown[i]->magic[0] > ' ' ? model->unknown[i]->magic[0] : '_',
                    model->unknown[i]->magic[1] > ' ' ? model->unknown[i]->magic[1] : '_',
                    model->unknown[i]->magic[2] > ' ' ? model->unknown[i]->magic[2] : '_',
                    model->unknown[i]->magic[3] > ' ' ? model->unknown[i]->magic[3] : '_');
                for(j = 0; j < model->unknown[i]->length; j++)
                    ptr += sprintf(ptr, "%02x ", *((unsigned char *)model->unknown + sizeof(m3dchunk_t) + j));
                ptr--;
                ptr += sprintf(ptr, "\r\n\r\n");
            }
        }
        setlocale(LC_NUMERIC, ol);
        len = (uint64_t)ptr - (uint64_t)out;
        out = (unsigned char*)M3D_REALLOC(out, len + 1);
        if(!out) goto memerr;
        out[len] = 0;
    } else
#endif
    {
        /* stricly only use LF (newline) in binary */
        sd = _m3d_safestr(model->desc, 3);
        if(!sd) goto memerr;
        /* header */
        h = (m3dhdr_t*)M3D_MALLOC(sizeof(m3dhdr_t) + strlen(sn) + strlen(sl) + strlen(sa) + strlen(sd) + 4);
        if(!h) goto memerr;
        memcpy((uint8_t*)h, "HEAD", 4);
        h->length = sizeof(m3dhdr_t);
        h->scale = scale;
        i = strlen(sn); memcpy((uint8_t*)h + h->length, sn, i+1); h->length += i+1; M3D_FREE(sn);
        i = strlen(sl); memcpy((uint8_t*)h + h->length, sl, i+1); h->length += i+1; M3D_FREE(sl);
        i = strlen(sa); memcpy((uint8_t*)h + h->length, sa, i+1); h->length += i+1; M3D_FREE(sa);
        i = strlen(sd); memcpy((uint8_t*)h + h->length, sd, i+1); h->length += i+1; M3D_FREE(sd);
        sn = sl = sa = sd = NULL;
        len = 0;
        if(!bone) numbone = 0;
        if(skin)
            for(i = 0; i < numskin; i++) {
                for(j = k = 0; j < M3D_NUMBONE; j++)
                    if(skin[i].boneid[j] != (M3D_INDEX)-1U && skin[i].weight[j] > (M3D_FLOAT)0.0) k++;
                if(k > len) len = k;
            }
        else
            numskin = 0;
        if(model->inlined)
            for(i = 0; i < model->numinlined; i++) {
                if(model->inlined[i].name && *model->inlined[i].name && model->inlined[i].length > 0) {
                    str = _m3d_addstr(str, &numstr, model->inlined[i].name);
                    if(!str) goto memerr;
                }
            }
        if(str)
            for(i = 0; i < numstr; i++) {
                h = _m3d_addhdr(h, &str[i]);
                if(!h) goto memerr;
            }
        vc_s = quality == M3D_EXP_INT8? 1 : (quality == M3D_EXP_INT16? 2 : (quality == M3D_EXP_DOUBLE? 8 : 4));
        vi_s = numvrtx < 254 ? 1 : (numvrtx < 65534 ? 2 : 4);
        si_s = h->length - 16 < 254 ? 1 : (h->length - 16 < 65534 ? 2 : 4);
        ci_s = !numcmap || !cmap ? 0 : (numcmap < 254 ? 1 : (numcmap < 65534 ? 2 : 4));
        ti_s = !numtmap || !tmap ? 0 : (numtmap < 254 ? 1 : (numtmap < 65534 ? 2 : 4));
        bi_s = !numbone || !bone ? 0 : (numbone < 254 ? 1 : (numbone < 65534 ? 2 : 4));
        nb_s = len < 2 ? 1 : (len == 2 ? 2 : (len <= 4 ? 4 : 8));
        sk_s = !numbone || !numskin ? 0 : (numskin < 254 ? 1 : (numskin < 65534 ? 2 : 4));
        fi_s = maxt < 254 ? 1 : (maxt < 65534 ? 2 : 4);
        h->types =  (vc_s == 8 ? (3<<0) : (vc_s == 2 ? (1<<0) : (vc_s == 1 ? (0<<0) : (2<<0)))) |
                    (vi_s == 2 ? (1<<2) : (vi_s == 1 ? (0<<2) : (2<<2))) |
                    (si_s == 2 ? (1<<4) : (si_s == 1 ? (0<<4) : (2<<4))) |
                    (ci_s == 2 ? (1<<6) : (ci_s == 1 ? (0<<6) : (ci_s == 4 ? (2<<6) : (3<<6)))) |
                    (ti_s == 2 ? (1<<8) : (ti_s == 1 ? (0<<8) : (ti_s == 4 ? (2<<8) : (3<<8)))) |
                    (bi_s == 2 ? (1<<10): (bi_s == 1 ? (0<<10): (bi_s == 4 ? (2<<10) : (3<<10)))) |
                    (nb_s == 2 ? (1<<12): (nb_s == 1 ? (0<<12): (2<<12))) |
                    (sk_s == 2 ? (1<<14): (sk_s == 1 ? (0<<14): (sk_s == 4 ? (2<<14) : (3<<14)))) |
                    (fi_s == 2 ? (1<<16): (fi_s == 1 ? (0<<16): (2<<16))) ;
        len = h->length;
        /* color map */
        if(numcmap && cmap && ci_s < 4 && !(flags & M3D_EXP_NOCMAP)) {
            chunklen = 8 + numcmap * sizeof(uint32_t);
            h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
            if(!h) goto memerr;
            memcpy((uint8_t*)h + len, "CMAP", 4);
            *((uint32_t*)((uint8_t*)h + len + 4)) = chunklen;
            memcpy((uint8_t*)h + len + 8, cmap, chunklen - 8);
            len += chunklen;
        } else numcmap = 0;
        /* texture map */
        if(numtmap && tmap && !(flags & M3D_EXP_NOTXTCRD) && !(flags & M3D_EXP_NOFACE)) {
            chunklen = 8 + numtmap * vc_s * 2;
            h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
            if(!h) goto memerr;
            memcpy((uint8_t*)h + len, "TMAP", 4);
            *((uint32_t*)((uint8_t*)h + len + 4)) = chunklen;
            out = (uint8_t*)h + len + 8;
            for(i = 0; i < numtmap; i++) {
                switch(vc_s) {
                    case 1: *out++ = (uint8_t)(tmap[i].u * 255); *out++ = (uint8_t)(tmap[i].v * 255); break;
                    case 2:
                        *((uint16_t*)out) = (uint16_t)(tmap[i].u * 65535); out += 2;
                        *((uint16_t*)out) = (uint16_t)(tmap[i].v * 65535); out += 2;
                    break;
                    case 4:  *((float*)out) = tmap[i].u; out += 4;  *((float*)out) = tmap[i].v; out += 4; break;
                    case 8: *((double*)out) = tmap[i].u; out += 8; *((double*)out) = tmap[i].v; out += 8; break;
                }
            }
            out = NULL;
            len += chunklen;
        }
        /* vertex */
        if(numvrtx && vrtx) {
            chunklen = 8 + numvrtx * (ci_s + sk_s + 4 * vc_s);
            h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
            if(!h) goto memerr;
            memcpy((uint8_t*)h + len, "VRTS", 4);
            *((uint32_t*)((uint8_t*)h + len + 4)) = chunklen;
            out = (uint8_t*)h + len + 8;
            for(i = 0; i < numvrtx; i++) {
                switch(vc_s) {
                    case 1:
                        *out++ = (int8_t)(vrtx[i].x * 127);
                        *out++ = (int8_t)(vrtx[i].y * 127);
                        *out++ = (int8_t)(vrtx[i].z * 127);
                        *out++ = (int8_t)(vrtx[i].w * 127);
                    break;
                    case 2:
                        *((int16_t*)out) = (int16_t)(vrtx[i].x * 32767); out += 2;
                        *((int16_t*)out) = (int16_t)(vrtx[i].y * 32767); out += 2;
                        *((int16_t*)out) = (int16_t)(vrtx[i].z * 32767); out += 2;
                        *((int16_t*)out) = (int16_t)(vrtx[i].w * 32767); out += 2;
                    break;
                    case 4:
                        *((float*)out) = vrtx[i].x; out += 4;
                        *((float*)out) = vrtx[i].y; out += 4;
                        *((float*)out) = vrtx[i].z; out += 4;
                        *((float*)out) = vrtx[i].w; out += 4;
                    break;
                    case 8:
                        *((double*)out) = vrtx[i].x; out += 8;
                        *((double*)out) = vrtx[i].y; out += 8;
                        *((double*)out) = vrtx[i].z; out += 8;
                        *((double*)out) = vrtx[i].w; out += 8;
                    break;
                }
                idx = _m3d_cmapidx(cmap, numcmap, vrtx[i].color);
                switch(ci_s) {
                    case 1: *out++ = (uint8_t)(idx); break;
                    case 2: *((uint16_t*)out) = (uint16_t)(idx); out += 2; break;
                    case 4: *((uint32_t*)out) = vrtx[i].color; out += 4; break;
                }
                out = _m3d_addidx(out, sk_s, numbone && numskin ? vrtx[i].skinid : -1U);
            }
            out = NULL;
            len += chunklen;
        }
        /* bones chunk */
        if(numbone && bone && !(flags & M3D_EXP_NOBONE)) {
            i = 8 + bi_s + sk_s + numbone * (bi_s + si_s + 2*vi_s);
            chunklen = i + numskin * nb_s * (bi_s + 1);
            h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
            if(!h) goto memerr;
            memcpy((uint8_t*)h + len, "BONE", 4);
            length = (uint32_t*)((uint8_t*)h + len + 4);
            out = (uint8_t*)h + len + 8;
            out = _m3d_addidx(out, bi_s, numbone);
            out = _m3d_addidx(out, sk_s, numskin);
            for(i = 0; i < numbone; i++) {
                out = _m3d_addidx(out, bi_s, bone[i].parent);
                out = _m3d_addidx(out, si_s, _m3d_stridx(str, numstr, bone[i].name));
                out = _m3d_addidx(out, vi_s, bone[i].pos);
                out = _m3d_addidx(out, vi_s, bone[i].ori);
            }
            if(numskin && skin && sk_s) {
                for(i = 0; i < numskin; i++) {
                    memset(&weights, 0, nb_s);
                    for(j = 0; j < (uint32_t)nb_s && skin[i].boneid[j] != (M3D_INDEX)-1U &&
                        skin[i].weight[j] > (M3D_FLOAT)0.0; j++)
                            weights[j] = (uint8_t)(skin[i].weight[j] * 255);
                    switch(nb_s) {
                        case 1: weights[0] = 255; break;
                        case 2: *((uint16_t*)out) = *((uint16_t*)&weights[0]); out += 2; break;
                        case 4: *((uint32_t*)out) = *((uint32_t*)&weights[0]); out += 4; break;
                        case 8: *((uint64_t*)out) = *((uint64_t*)&weights[0]); out += 8; break;
                    }
                    for(j = 0; j < (uint32_t)nb_s && skin[i].boneid[j] != (M3D_INDEX)-1U &&
                        skin[i].weight[j] > (M3D_FLOAT)0.0; j++) {
                        out = _m3d_addidx(out, bi_s, skin[i].boneid[j]);
                        *length += bi_s;
                    }
                }
            }
            *length = (uint64_t)out - (uint64_t)((uint8_t*)h + len);
            out = NULL;
            len += *length;
        }
        /* materials */
        if(nummtrl && !(flags & M3D_EXP_NOMATERIAL)) {
            for(j = 0; j < nummtrl; j++) {
                m = !mtrl ? &model->material[j] : mtrl[j];
                chunklen = 12 + si_s + m->numprop * 5;
                h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
                if(!h) goto memerr;
                memcpy((uint8_t*)h + len, "MTRL", 4);
                length = (uint32_t*)((uint8_t*)h + len + 4);
                out = (uint8_t*)h + len + 8;
                out = _m3d_addidx(out, si_s, _m3d_stridx(str, numstr, m->name));
                for(i = 0; i < m->numprop; i++) {
                    if(m->prop[i].type >= 128) {
                        if(m->prop[i].value.textureid >= model->numtexture ||
                            !model->texture[m->prop[i].value.textureid].name) continue;
                        k = m3dpf_map;
                    } else {
                        for(k = 256, l = 0; l < sizeof(m3d_propertytypes)/sizeof(m3d_propertytypes[0]); l++)
                            if(m->prop[i].type == m3d_propertytypes[l].id) { k = m3d_propertytypes[l].format; break; }
                    }
                    if(k == 256) continue;
                    *out++ = m->prop[i].type;
                    switch(k) {
                        case m3dpf_color:
                            if(!(flags & M3D_EXP_NOCMAP)) {
                                idx = _m3d_cmapidx(cmap, numcmap, m->prop[i].value.color);
                                switch(ci_s) {
                                    case 1: *out++ = (uint8_t)(idx); break;
                                    case 2: *((uint16_t*)out) = (uint16_t)(idx); out += 2; break;
                                    case 4: *((uint32_t*)out) = (uint32_t)(m->prop[i].value.color); out += 4; break;
                                }
                            } else out--;
                        break;
                        case m3dpf_uint8:  *out++ = m->prop[i].value.num; break;
                        case m3dpf_uint16: *((uint16_t*)out) = m->prop[i].value.num; out += 2; break;
                        case m3dpf_uint32: *((uint32_t*)out) = m->prop[i].value.num; out += 4; break;
                        case m3dpf_float:  *((float*)out) = m->prop[i].value.fnum; out += 4; break;

                        case m3dpf_map:
                            idx = _m3d_stridx(str, numstr, model->texture[m->prop[i].value.textureid].name);
                            out = _m3d_addidx(out, si_s, idx);
                        break;
                    }
                }
                *length = (uint64_t)out - (uint64_t)((uint8_t*)h + len);
                len += *length;
                out = NULL;
            }
        }
        /* mesh face */
        if(model->numface && face && !(flags & M3D_EXP_NOFACE)) {
            chunklen = 8 + si_s + model->numface * (6 * vi_s + 3 * ti_s + si_s + 1);
            h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
            if(!h) goto memerr;
            memcpy((uint8_t*)h + len, "MESH", 4);
            length = (uint32_t*)((uint8_t*)h + len + 4);
            out = (uint8_t*)h + len + 8;
            last = (M3D_INDEX)-1U;
            for(i = 0; i < model->numface; i++) {
                if(!(flags & M3D_EXP_NOMATERIAL) && face[i].materialid != last) {
                    last = face[i].materialid;
                    if(last < nummtrl) {
                        idx = _m3d_stridx(str, numstr, !mtrl ? model->material[last].name : mtrl[last]->name);
                        if(idx) {
                            *out++ = 0;
                            out = _m3d_addidx(out, si_s, idx);
                        }
                    }
                }
                /* hardcoded triangles. */
                k = (3 << 4) |
                    (((flags & M3D_EXP_NOTXTCRD) || ti_s == 8 || (face[i].texcoord[0] == (M3D_INDEX)-1U &&
                    face[i].texcoord[1] == (M3D_INDEX)-1U && face[i].texcoord[2] == (M3D_INDEX)-1U)) ? 0 : 1) |
                    (((flags & M3D_EXP_NONORMAL) || (face[i].normal[0] == (M3D_INDEX)-1U &&
                    face[i].normal[1] == (M3D_INDEX)-1U && face[i].normal[2] == (M3D_INDEX)-1U)) ? 0 : 2);
                *out++ = k;
                for(j = 0; j < 3; j++) {
                    out = _m3d_addidx(out, vi_s, face[i].vertex[j]);
                    if(k & 1)
                        out = _m3d_addidx(out, ti_s, face[i].texcoord[j]);
                    if(k & 2)
                        out = _m3d_addidx(out, vi_s, face[i].normal[j]);
                }
            }
            *length = (uint64_t)out - (uint64_t)((uint8_t*)h + len);
            len += *length;
            out = NULL;
        }
        /* actions */
        if(model->numaction && model->action && numactn && actn && numbone && bone && !(flags & M3D_EXP_NOACTION)) {
            l = 0;
            for(j = 0; j < model->numaction; j++) {
                a = &model->action[j];
                chunklen = 14 + si_s + a->numframe * (4 + fi_s + maxt * (bi_s + 2 * vi_s));
                h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
                if(!h) goto memerr;
                memcpy((uint8_t*)h + len, "ACTN", 4);
                length = (uint32_t*)((uint8_t*)h + len + 4);
                out = (uint8_t*)h + len + 8;
                out = _m3d_addidx(out, si_s, _m3d_stridx(str, numstr, a->name));
                *((uint16_t*)out) = (uint16_t)(a->numframe); out += 2;
                *((uint32_t*)out) = (uint32_t)(a->durationmsec); out += 4;
                for(i = 0; i < a->numframe; i++) {
                    *((uint32_t*)out) = (uint32_t)(a->frame[i].msec); out += 4;
                    out = _m3d_addidx(out, fi_s, a->frame[i].numtransform);
                    for(k = 0; k < a->frame[i].numtransform; k++) {
                        out = _m3d_addidx(out, bi_s, a->frame[i].transform[k].boneid);
                        out = _m3d_addidx(out, vi_s, actn[l++]);
                        out = _m3d_addidx(out, vi_s, actn[l++]);
                    }
                }
                *length = (uint64_t)out - (uint64_t)((uint8_t*)h + len);
                len += *length;
                out = NULL;
            }
        }
        /* inlined assets */
        if(model->numinlined && model->inlined && (flags & M3D_EXP_INLINE)) {
            for(j = 0; j < model->numinlined; j++) {
                if(!model->inlined[j].name || !*model->inlined[j].name || !model->inlined[j].length)
                    continue;
                chunklen = 8 + si_s + model->inlined[j].length;
                h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
                if(!h) goto memerr;
                memcpy((uint8_t*)h + len, "ASET", 4);
                *((uint32_t*)((uint8_t*)h + len + 4)) = chunklen;
                out = (uint8_t*)h + len + 8;
                out = _m3d_addidx(out, si_s, _m3d_stridx(str, numstr, model->inlined[j].name));
                memcpy(out, model->inlined[j].data, model->inlined[j].length);
                out = NULL;
                len += chunklen;
            }
        }
        /* extra chunks */
        if(model->numunknown && model->unknown && (flags & M3D_EXP_EXTRA)) {
            for(j = 0; j < model->numunknown; j++) {
                if(!model->unknown[j] || model->unknown[j]->length < 8)
                    continue;
                chunklen = model->unknown[j]->length;
                h = (m3dhdr_t*)M3D_REALLOC(h, len + chunklen);
                if(!h) goto memerr;
                memcpy((uint8_t*)h + len, model->unknown[j], chunklen);
                len += chunklen;
            }
        }
        /* add end chunk */
        h = (m3dhdr_t*)M3D_REALLOC(h, len + 4);
        if(!h) goto memerr;
        memcpy((uint8_t*)h + len, "OMD3", 4);
        len += 4;
        /* zlib compress */
        if(!(flags & M3D_EXP_NOZLIB)) {
            z = stbi_zlib_compress((unsigned char *)h, len, (int*)&l, 9);
            if(z && l > 0 && l < len) { len = l; M3D_FREE(h); h = (m3dhdr_t*)z; }
        }
        /* add file header at the begining */
        len += 8;
        out = (unsigned char*)M3D_MALLOC(len);
        if(!out) goto memerr;
        memcpy(out, "3DMO", 4);
        *((uint32_t*)(out + 4)) = len;
        memcpy(out + 8, h, len - 8);
    }
    if(size) *size = out ? len : 0;
    if(face) M3D_FREE(face);
    if(cmap) M3D_FREE(cmap);
    if(tmap) M3D_FREE(tmap);
    if(mtrl) M3D_FREE(mtrl);
    if(vrtx) M3D_FREE(vrtx);
    if(bone) M3D_FREE(bone);
    if(skin) M3D_FREE(skin);
    if(actn) M3D_FREE(actn);
    if(str) M3D_FREE(str);
    if(h) M3D_FREE(h);
    return out;
}
#endif

#endif

#ifdef  __cplusplus
}
#ifdef M3D_CPPWRAPPER
#include <vector>
#include <string>
#include <memory>

/*** C++ wrapper class ***/
namespace M3D {
#ifdef M3D_IMPLEMENTATION

    class Model {
        public:
            m3d_t *model;

        public:
            Model() {
                this->model = (m3d_t*)malloc(sizeof(m3d_t)); memset(this->model, 0, sizeof(m3d_t));
            }
            Model(_unused const std::string &data, _unused m3dread_t ReadFileCB,
                _unused m3dfree_t FreeCB, _unused M3D::Model mtllib) {
#ifndef M3D_NOIMPORTER
                this->model = m3d_load((unsigned char *)data.data(), ReadFileCB, FreeCB, mtllib.model);
#else
                Model();
#endif
            }
            Model(_unused const std::vector<unsigned char> data, _unused m3dread_t ReadFileCB,
                _unused m3dfree_t FreeCB, _unused M3D::Model mtllib) {
#ifndef M3D_NOIMPORTER
                this->model = m3d_load((unsigned char *)&data[0], ReadFileCB, FreeCB, mtllib.model);
#else
                Model();
#endif
            }
            ~Model() { m3d_free(this->model); }

        public:
            m3d_t *getCStruct() { return this->model; }
            std::string getName() { return std::string(this->model->name); }
            void setName(std::string name) { this->model->name = (char*)name.c_str(); }
            std::string getLicense() { return std::string(this->model->license); }
            void setLicense(std::string license) { this->model->license = (char*)license.c_str(); }
            std::string getAuthor() { return std::string(this->model->author); }
            void setAuthor(std::string author) { this->model->author = (char*)author.c_str(); }
            std::string getDescription() { return std::string(this->model->desc); }
            void setDescription(std::string desc) { this->model->desc = (char*)desc.c_str(); }
            float getScale() { return this->model->scale; }
            void setScale(float scale) { this->model->scale = scale; }
            std::vector<uint32_t> getColorMap() { return this->model->cmap ? std::vector<uint32_t>(this->model->cmap,
                this->model->cmap + this->model->numcmap) : std::vector<uint32_t>(); }
            std::vector<m3dti_t> getTextureMap() { return this->model->tmap ? std::vector<m3dti_t>(this->model->tmap,
                this->model->tmap + this->model->numtmap) : std::vector<m3dti_t>(); }
            std::vector<m3dtx_t> getTextures() { return this->model->texture ? std::vector<m3dtx_t>(this->model->texture,
                this->model->texture + this->model->numtexture) : std::vector<m3dtx_t>(); }
            std::string getTextureName(int idx) { return idx >= 0 && (unsigned int)idx < this->model->numtexture ?
                std::string(this->model->texture[idx].name) : nullptr; }
            std::vector<m3db_t> getBones() { return this->model->bone ? std::vector<m3db_t>(this->model->bone, this->model->bone +
                this->model->numbone) : std::vector<m3db_t>(); }
            std::string getBoneName(int idx) { return idx >= 0 && (unsigned int)idx < this->model->numbone ?
                std::string(this->model->bone[idx].name) : nullptr; }
            std::vector<m3dm_t> getMaterials() { return this->model->material ? std::vector<m3dm_t>(this->model->material,
                this->model->material + this->model->nummaterial) : std::vector<m3dm_t>(); }
            std::string getMaterialName(int idx) { return idx >= 0 && (unsigned int)idx < this->model->nummaterial ?
                std::string(this->model->material[idx].name) : nullptr; }
            int getMaterialPropertyInt(int idx, int type) {
                    if (idx < 0 || (unsigned int)idx >= this->model->nummaterial || type < 0 || type >= 127 ||
                        !this->model->material[idx].prop) return -1;
                    for (int i = 0; i < this->model->material[idx].numprop; i++) {
                        if (this->model->material[idx].prop[i].type == type)
                            return this->model->material[idx].prop[i].value.num;
                    }
                    return -1;
                }
            uint32_t getMaterialPropertyColor(int idx, int type) { return this->getMaterialPropertyInt(idx, type); }
            float getMaterialPropertyFloat(int idx, int type) {
                    if (idx < 0 || (unsigned int)idx >= this->model->nummaterial || type < 0 || type >= 127 ||
                        !this->model->material[idx].prop) return -1.0f;
                    for (int i = 0; i < this->model->material[idx].numprop; i++) {
                        if (this->model->material[idx].prop[i].type == type)
                            return this->model->material[idx].prop[i].value.fnum;
                    }
                    return -1.0f;
                }
            m3dtx_t* getMaterialPropertyMap(int idx, int type) {
                    if (idx < 0 || (unsigned int)idx >= this->model->nummaterial || type < 128 || type > 255 ||
                        !this->model->material[idx].prop) return nullptr;
                    for (int i = 0; i < this->model->material[idx].numprop; i++) {
                        if (this->model->material[idx].prop[i].type == type)
                            return this->model->material[idx].prop[i].value.textureid < this->model->numtexture ?
                                &this->model->texture[this->model->material[idx].prop[i].value.textureid] : nullptr;
                    }
                    return nullptr;
                }
            std::vector<m3dv_t> getVertices() { return this->model->vertex ? std::vector<m3dv_t>(this->model->vertex,
                this->model->vertex + this->model->numvertex) : std::vector<m3dv_t>(); }
            std::vector<m3df_t> getFace() { return this->model->face ? std::vector<m3df_t>(this->model->face, this->model->face +
                this->model->numface) : std::vector<m3df_t>(); }
            std::vector<m3ds_t> getSkin() { return this->model->skin ? std::vector<m3ds_t>(this->model->skin, this->model->skin +
                this->model->numskin) : std::vector<m3ds_t>(); }
            std::vector<m3da_t> getActions() { return this->model->action ? std::vector<m3da_t>(this->model->action,
                this->model->action + this->model->numaction) : std::vector<m3da_t>(); }
            std::string getActionName(int aidx) { return aidx >= 0 && (unsigned int)aidx < this->model->numaction ?
                std::string(this->model->action[aidx].name) : nullptr; }
            unsigned int getActionDuration(int aidx) { return aidx >= 0 && (unsigned int)aidx < this->model->numaction ?
                this->model->action[aidx].durationmsec : 0; }
            std::vector<m3dfr_t> getActionFrames(int aidx) { return aidx >= 0 && (unsigned int)aidx < this->model->numaction ?
                std::vector<m3dfr_t>(this->model->action[aidx].frame, this->model->action[aidx].frame +
                this->model->action[aidx].numframe) : std::vector<m3dfr_t>(); }
            unsigned int getActionFrameTimestamp(int aidx, int fidx) { return aidx >= 0 && (unsigned int)aidx < this->model->numaction?
                    (fidx >= 0 && (unsigned int)fidx < this->model->action[aidx].numframe ?
                    this->model->action[aidx].frame[fidx].msec : 0) : 0; }
            std::vector<m3dtr_t> getActionFrameTransforms(int aidx, int fidx) {
                return aidx >= 0 && (unsigned int)aidx < this->model->numaction ? (
                    fidx >= 0 && (unsigned int)fidx < this->model->action[aidx].numframe ?
                    std::vector<m3dtr_t>(this->model->action[aidx].frame[fidx].transform,
                    this->model->action[aidx].frame[fidx].transform + this->model->action[aidx].frame[fidx].numtransform) :
                    std::vector<m3dtr_t>()) : std::vector<m3dtr_t>(); }
            std::vector<m3dtr_t> getActionFrame(int aidx, int fidx, std::vector<m3dtr_t> skeleton) {
                m3dtr_t *pose = m3d_frame(this->model, (unsigned int)aidx, (unsigned int)fidx,
                    skeleton.size() ? &skeleton[0] : nullptr);
                return std::vector<m3dtr_t>(pose, pose + this->model->numbone); }
            std::vector<m3db_t> getActionPose(int aidx, unsigned int msec) {
                m3db_t *pose = m3d_pose(this->model, (unsigned int)aidx, (unsigned int)msec);
                return std::vector<m3db_t>(pose, pose + this->model->numbone); }
            std::vector<m3di_t> getInlinedAssets() { return this->model->inlined ? std::vector<m3di_t>(this->model->inlined,
                this->model->inlined + this->model->numinlined) : std::vector<m3di_t>(); }
            std::vector<std::unique_ptr<m3dchunk_t>> getUnknowns() { return this->model->unknown ?
                std::vector<std::unique_ptr<m3dchunk_t>>(this->model->unknown,
                this->model->unknown + this->model->numunknown) : std::vector<std::unique_ptr<m3dchunk_t>>(); }
            std::vector<unsigned char> Save(_unused int quality, _unused int flags) {
#ifdef M3D_EXPORTER
                unsigned int size;
                unsigned char *ptr = m3d_save(this->model, quality, flags, &size);
                return ptr && size ? std::vector<unsigned char>(ptr, ptr + size) : std::vector<unsigned char>();
#else
                return std::vector<unsigned char>();
#endif
            }
    };

#else
    class Model {
        public:
            m3d_t *model;

        public:
            Model(const std::string &data, m3dread_t ReadFileCB, m3dfree_t FreeCB);
            Model(const std::vector<unsigned char> data, m3dread_t ReadFileCB, m3dfree_t FreeCB);
            Model();
            ~Model();

        public:
            m3d_t *getCStruct();
            std::string getName();
            void setName(std::string name);
            std::string getLicense();
            void setLicense(std::string license);
            std::string getAuthor();
            void setAuthor(std::string author);
            std::string getDescription();
            void setDescription(std::string desc);
            float getScale();
            void setScale(float scale);
            std::vector<uint32_t> getColorMap();
            std::vector<m3dti_t> getTextureMap();
            std::vector<m3dtx_t> getTextures();
            std::string getTextureName(int idx);
            std::vector<m3db_t> getBones();
            std::string getBoneName(int idx);
            std::vector<m3dm_t> getMaterials();
            std::string getMaterialName(int idx);
            int getMaterialPropertyInt(int idx, int type);
            uint32_t getMaterialPropertyColor(int idx, int type);
            float getMaterialPropertyFloat(int idx, int type);
            m3dtx_t* getMaterialPropertyMap(int idx, int type);
            std::vector<m3dv_t> getVertices();
            std::vector<m3df_t> getFace();
            std::vector<m3ds_t> getSkin();
            std::vector<m3da_t> getActions();
            std::string getActionName(int aidx);
            unsigned int getActionDuration(int aidx);
            std::vector<m3dfr_t> getActionFrames(int aidx);
            unsigned int getActionFrameTimestamp(int aidx, int fidx);
            std::vector<m3dtr_t> getActionFrameTransforms(int aidx, int fidx);
            std::vector<m3dtr_t> getActionFrame(int aidx, int fidx, std::vector<m3dtr_t> skeleton);
            std::vector<m3db_t> getActionPose(int aidx, unsigned int msec);
            std::vector<m3di_t> getInlinedAssets();
            std::vector<std::unique_ptr<m3dchunk_t>> getUnknowns();
            std::vector<unsigned char> Save(int quality, int flags);
    };

#endif /* impl */
}
#endif

#endif /* __cplusplus */

#endif
