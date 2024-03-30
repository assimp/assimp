/* SPDX-License-Identifier: Apache 2.0

 C API(C11) for TinyUSDZ
 This C API is primarily for bindings for other languages.
 Various features/manipulations are missing and not intended to use C API
 solely(at the moment).

 NOTE: Use `c_tinyusd` or `CTinyUSD` prefix(`z` is missing) in C API.
*/
#ifndef C_TINYUSD_H
#define C_TINYUSD_H

/*#include <assert.h>*/
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/*

   Common API design direction.


   - Frequently used type uses lower snake case(e.g. `c_tinyusd_string_t`)
   - For most of API, Return type is int(bool). 0 = failre, 1 = success.
   - For `***_new` API, return type is a pointer of new'ed object.
     - Use corresponding `***_free` to free a object.
   - Argument order: object(in or inout), ins, inouts, then outs

   Example

   ```
   c_tinyusd_string_t *s = c_tinyusd_string_new_empty();
   if (!s) {
     // err...
   }
   ...
   c_tinyusd_string_free(&s);
   ```

   ```
   CTinyUSDStage *pstage = c_tinyusd_stage_new();
   c_tinyusd_string_t *pwarn = c_tinyusd_string_new_empty();
   c_tinyusd_string_t *perr = c_tinyusd_string_new_empty();

   For losd API, pass memory allocated Stage object and string objects to args.

   int ret = c_tinyusd_load_usd_from_file("input.usd", pstage, pwarn, perr);

   c_tinyusd_stage_free(pstage);
   ```

*/

/*
 * TODO:
 *  - Provide dedicated string type for UTF-8 string?
 */

/*
 *  TODO: Use same export macro logic with C++ API?
 */
#if !defined(TINYUSDZ_EXPORT)

#if defined(TINYUSDZ_SHARED_LIBRARY)

#if defined(_MSC_VER)

#if defined(TINYUSDZ_COMPILE_LIBRARY)
#define C_TINYUSD_EXPORT __declspec(dllexport)
#else
#define C_TINYUSD_EXPORT __declspec(dllimport)
#endif

#else /* !_MSC_VER */

#if defined(TINYUSDZ_COMPILE_LIBRARY)
/* Assume non-msvc */
#define C_TINYUSD_EXPORT __attribute__((visibility("default")))
#else
#define C_TINYUSD_EXPORT __declspec(dllimport)
#endif

#endif /* _MSC_VER */

#else /* !TINYUSDZ_SHARED_LIBRARY */

#define C_TINYUSD_EXPORT

#endif /* TINYUSDZ_SHARED_LIBRARY */
#endif /* TINYUSDZ_EXPORT */

C_TINYUSD_EXPORT void *c_tinyusd_malloc(size_t nbytes);

/* Returns 0 when failed. */
C_TINYUSD_EXPORT int c_tinyusd_free(void *ptr);

/*
   NOTE: Current(2023.03) USD spec does not support 2D or multi-dim array,
   so set max_dim to 1.
 */
#define C_TINYUSD_MAX_DIM (1)

typedef enum {
  C_TINYUSD_FORMAT_UNKNOWN,
  C_TINYUSD_FORMAT_AUTO,  // auto detect based on file extension.
  C_TINYUSD_FORMAT_USDA,
  C_TINYUSD_FORMAT_USDC,
  C_TINYUSD_FORMAT_USDZ
} CTinyUSDFormat;

typedef enum {
  C_TINYUSD_AXIS_UNKNOWN,
  C_TINYUSD_AXIS_X,
  C_TINYUSD_AXIS_Y,
  C_TINYUSD_AXIS_Z,
} CTinyUSDAxis;

/*
   NOTE: Use dedicated enum value for token[] and string[]
   (therse use C struct `c_tinyusd_token_vector` and `c_tinyusd_string_vector`
   respectively)

   Use C_TINYUSD_VALUE_1D_BIT for other numerical value type to represent 1D
   array.
*/
typedef enum {
  C_TINYUSD_VALUE_UNKNOWN,
  C_TINYUSD_VALUE_TOKEN,
  C_TINYUSD_VALUE_TOKEN_VECTOR, /* token[] */
  C_TINYUSD_VALUE_STRING,
  C_TINYUSD_VALUE_STRING_VECTOR, /* string[] */
  C_TINYUSD_VALUE_BOOL,
  C_TINYUSD_VALUE_HALF,
  C_TINYUSD_VALUE_HALF2,
  C_TINYUSD_VALUE_HALF3,
  C_TINYUSD_VALUE_HALF4,
  C_TINYUSD_VALUE_INT,
  C_TINYUSD_VALUE_INT2,
  C_TINYUSD_VALUE_INT3,
  C_TINYUSD_VALUE_INT4,
  C_TINYUSD_VALUE_UINT,
  C_TINYUSD_VALUE_UINT2,
  C_TINYUSD_VALUE_UINT3,
  C_TINYUSD_VALUE_UINT4,
  C_TINYUSD_VALUE_INT64,
  C_TINYUSD_VALUE_UINT64,
  C_TINYUSD_VALUE_FLOAT,
  C_TINYUSD_VALUE_FLOAT2,
  C_TINYUSD_VALUE_FLOAT3,
  C_TINYUSD_VALUE_FLOAT4,
  C_TINYUSD_VALUE_DOUBLE,
  C_TINYUSD_VALUE_DOUBLE2,
  C_TINYUSD_VALUE_DOUBLE3,
  C_TINYUSD_VALUE_DOUBLE4,
  C_TINYUSD_VALUE_QUATH,
  C_TINYUSD_VALUE_QUATF,
  C_TINYUSD_VALUE_QUATD,
  C_TINYUSD_VALUE_COLOR3H,
  C_TINYUSD_VALUE_COLOR3F,
  C_TINYUSD_VALUE_COLOR3D,
  C_TINYUSD_VALUE_COLOR4H,
  C_TINYUSD_VALUE_COLOR4F,
  C_TINYUSD_VALUE_COLOR4D,
  C_TINYUSD_VALUE_TEXCOORD2H,
  C_TINYUSD_VALUE_TEXCOORD2F,
  C_TINYUSD_VALUE_TEXCOORD2D,
  C_TINYUSD_VALUE_TEXCOORD3H,
  C_TINYUSD_VALUE_TEXCOORD3F,
  C_TINYUSD_VALUE_TEXCOORD3D,
  C_TINYUSD_VALUE_NORMAL3H,
  C_TINYUSD_VALUE_NORMAL3F,
  C_TINYUSD_VALUE_NORMAL3D,
  C_TINYUSD_VALUE_VECTOR3H,
  C_TINYUSD_VALUE_VECTOR3F,
  C_TINYUSD_VALUE_VECTOR3D,
  C_TINYUSD_VALUE_POINT3H,
  C_TINYUSD_VALUE_POINT3F,
  C_TINYUSD_VALUE_POINT3D,
  C_TINYUSD_VALUE_MATRIX2D,
  C_TINYUSD_VALUE_MATRIX3D,
  C_TINYUSD_VALUE_MATRIX4D,
  C_TINYUSD_VALUE_FRAME4D,
  C_TINYUSD_VALUE_DICTIONARY, /* tinyusdz::value::CustomData. VtDictionary equivalent in pxrUSD */
  C_TINYUSD_VALUE_END, /* terminator */
} CTinyUSDValueType;

/* assume the number of value types is less than 1024. */
#define C_TINYUSD_VALUE_1D_BIT (1 << 10)

/* NOTE: No `Geom` prefix to usdGeom prims in C API. */
typedef enum {
  C_TINYUSD_PRIM_UNKNOWN,
  C_TINYUSD_PRIM_MODEL,
  C_TINYUSD_PRIM_SCOPE,
  C_TINYUSD_PRIM_XFORM,
  C_TINYUSD_PRIM_MESH,
  C_TINYUSD_PRIM_GEOMSUBSET,
  C_TINYUSD_PRIM_MATERIAL,
  C_TINYUSD_PRIM_SHADER,
  C_TINYUSD_PRIM_CAMERA,
  C_TINYUSD_PRIM_SPHERE_LIGHT,
  C_TINYUSD_PRIM_DISTANT_LIGHT,
  C_TINYUSD_PRIM_RECT_LIGHT,
  /* TODO: Add more prim types... */
  C_TINYUSD_PRIM_END,
} CTinyUSDPrimType;

/*
 * Use lower snake case for string/token and value types.
 */

typedef uint16_t c_tinyusd_half_t;

/*
   Assume struct elements will be tightly packed in C11.
   TODO: Ensure struct elements are tightly packed.
 */
typedef struct {
  int x;
  int y;
} c_tinyusd_int2_t;

typedef struct {
  int x;
  int y;
  int z;
} c_tinyusd_int3_t;

typedef struct {
  int x;
  int y;
  int z;
  int w;
} c_tinyusd_int4_t;

typedef struct {
  uint32_t x;
  uint32_t y;
} c_tinyusd_uint2_t;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t z;
} c_tinyusd_uint3_t;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t w;
} c_tinyusd_uint4_t;

typedef struct {
  c_tinyusd_half_t x;
  c_tinyusd_half_t y;
} c_tinyusd_half2_t;

typedef struct {
  c_tinyusd_half_t x;
  c_tinyusd_half_t y;
  c_tinyusd_half_t z;
} c_tinyusd_half3_t;

typedef struct {
  c_tinyusd_half_t x;
  c_tinyusd_half_t y;
  c_tinyusd_half_t z;
  c_tinyusd_half_t w;
} c_tinyusd_half4_t;

typedef struct {
  float x;
  float y;
} c_tinyusd_float2_t;

typedef struct {
  float x;
  float y;
  float z;
} c_tinyusd_float3_t;

typedef struct {
  float x;
  float y;
  float z;
  float w;
} c_tinyusd_float4_t;

typedef struct {
  double x;
  double y;
} c_tinyusd_double2_t;

typedef struct {
  double x;
  double y;
  double z;
} c_tinyusd_double3_t;

typedef struct {
  double x;
  double y;
  double z;
  double w;
} c_tinyusd_double4_t;

typedef struct {
  double m[4];
} c_tinyusd_matrix2d_t;

typedef struct {
  double m[9];
} c_tinyusd_matrix3d_t;

typedef struct {
  double m[16];
} c_tinyusd_matrix4d_t;

typedef struct {
  c_tinyusd_half_t imag[3];
  c_tinyusd_half_t real;
} c_tinyusd_quath_t;

typedef struct {
  float imag[3];
  float real;
} c_tinyusd_quatf_t;

typedef struct {
  double imag[3];
  double real;
} c_tinyusd_quatd_t;

typedef c_tinyusd_half3_t c_tinyusd_color3h_t;
typedef c_tinyusd_float3_t c_tinyusd_color3f_t;
typedef c_tinyusd_double3_t c_tinyusd_color3d_t;

typedef c_tinyusd_half4_t c_tinyusd_color4h_t;
typedef c_tinyusd_float4_t c_tinyusd_color4f_t;
typedef c_tinyusd_double4_t c_tinyusd_color4d_t;

typedef c_tinyusd_half3_t c_tinyusd_point3h_t;
typedef c_tinyusd_float3_t c_tinyusd_point3f_t;
typedef c_tinyusd_double3_t c_tinyusd_point3d_t;

typedef c_tinyusd_half3_t c_tinyusd_normal3h_t;
typedef c_tinyusd_float3_t c_tinyusd_normal3f_t;
typedef c_tinyusd_double3_t c_tinyusd_normal3d_t;

typedef c_tinyusd_half3_t c_tinyusd_vector3h_t;
typedef c_tinyusd_float3_t c_tinyusd_vector3f_t;
typedef c_tinyusd_double3_t c_tinyusd_vector3d_t;

typedef c_tinyusd_matrix4d_t c_tinyusd_frame4d_t;

typedef c_tinyusd_half2_t c_tinyusd_texcoord2h_t;
typedef c_tinyusd_float2_t c_tinyusd_texcoord2f_t;
typedef c_tinyusd_double2_t c_tinyusd_texcoord2d_t;

typedef c_tinyusd_half3_t c_tinyusd_texcoord3h_t;
typedef c_tinyusd_float3_t c_tinyusd_texcoord3f_t;
typedef c_tinyusd_double3_t c_tinyusd_texcoord3d_t;

typedef struct c_tinyusd_token_t c_tinyusd_token_t;

C_TINYUSD_EXPORT c_tinyusd_token_t *c_tinyusd_token_new(const char *str);

/* Duplicate token object. Return null when failed. */
C_TINYUSD_EXPORT c_tinyusd_token_t *c_tinyusd_token_dup(
    const c_tinyusd_token_t *tok);

/* Length of token string. equivalent to std::string::size. */
C_TINYUSD_EXPORT size_t c_tinyusd_token_size(const c_tinyusd_token_t *tok);

/*
   Get C char from a token.
   Returned char pointer is valid until `c_tinyusd_token` instance is free'ed.
 */
C_TINYUSD_EXPORT const char *c_tinyusd_token_str(const c_tinyusd_token_t *tok);

/*
   Free token
   Return 0 when failed to free.
 */
C_TINYUSD_EXPORT int c_tinyusd_token_free(c_tinyusd_token_t *tok);

/*   opaque pointer to `std::vector<tinyusd::value::token>`. */
typedef struct c_tinyusd_token_vector_t c_tinyusd_token_vector_t;

/*  New token vector(array) with size zero(empty) */

C_TINYUSD_EXPORT c_tinyusd_token_vector_t *c_tinyusd_token_vector_new_empty();

/*  New token vector(array) with given size `n` */

C_TINYUSD_EXPORT c_tinyusd_token_vector_t *c_tinyusd_token_vector_new(
    const size_t n, const char **toks);
C_TINYUSD_EXPORT int c_tinyusd_token_vector_free(c_tinyusd_token_vector_t *sv);

/*
   Returns number of elements.
   0 when empty or `tv` is invalid.
 */
C_TINYUSD_EXPORT size_t
c_tinyusd_token_vector_size(const c_tinyusd_token_vector_t *sv);

C_TINYUSD_EXPORT int c_tinyusd_token_vector_clear(c_tinyusd_token_vector_t *sv);
C_TINYUSD_EXPORT int c_tinyusd_token_vector_resize(c_tinyusd_token_vector_t *sv,
                                                   const size_t n);

/*
   Return const string pointer for given index.
   Returns nullptr when index is out-of-range.
*/
C_TINYUSD_EXPORT const char *c_tinyusd_token_vector_str(
    const c_tinyusd_token_vector_t *sv, const size_t idx);

/*
   Replace `index`th token.
   Returns 0 when `sv` is invalid or `index` is out-of-range.
 */
C_TINYUSD_EXPORT int c_tinyusd_token_vector_replace(
    c_tinyusd_token_vector_t *sv, const size_t idx, const char *str);

/* opaque pointer to `std::string`.*/
typedef struct c_tinyusd_string_t c_tinyusd_string_t;

C_TINYUSD_EXPORT c_tinyusd_string_t *c_tinyusd_string_new_empty();

C_TINYUSD_EXPORT c_tinyusd_string_t *c_tinyusd_string_new(const char *str);

/* Length of string. equivalent to std::string::size. */
C_TINYUSD_EXPORT size_t c_tinyusd_string_size(const c_tinyusd_string_t *s);

/* Replace existing string with given `str`.
 * `c_tinyusd_string` object must be new'ed beforehand.
 * Return 0 when failed to set.
 */
C_TINYUSD_EXPORT int c_tinyusd_string_replace(c_tinyusd_string_t *s,
                                              const char *str);

/* Get C char(`std::string::c_str()`)
 * Returned char pointer is valid until `c_tinyusd_string` instance is free'ed.
 */
C_TINYUSD_EXPORT const char *c_tinyusd_string_str(const c_tinyusd_string_t *s);

C_TINYUSD_EXPORT int c_tinyusd_string_free(c_tinyusd_string_t *s);

typedef struct {
  void *data;  // opaque pointer to `std::vector<std::string>`.
} c_tinyusd_string_vector;

/*   New string vector(array) with given size `n` */
C_TINYUSD_EXPORT int c_tinyusd_string_vector_new_empty(
    c_tinyusd_string_vector *sv, const size_t n);

C_TINYUSD_EXPORT int c_tinyusd_string_vector_new(c_tinyusd_string_vector *sv,
                                                 const size_t n,
                                                 const char **strs);

/*
   Returns number of elements.
   0 when empty or `sv` is invalid.
 */
C_TINYUSD_EXPORT size_t
c_tinyusd_string_vector_size(const c_tinyusd_string_vector *sv);

C_TINYUSD_EXPORT int c_tinyusd_string_vector_clear(c_tinyusd_string_vector *sv);
C_TINYUSD_EXPORT int c_tinyusd_string_vector_resize(c_tinyusd_string_vector *sv,
                                                    const size_t n);

/*
   Return const string pointer for given index.
   Returns nullptr when index is out-of-range.
 */
C_TINYUSD_EXPORT const char *c_tinyusd_string_vector_str(
    const c_tinyusd_string_vector *sv, const size_t idx);

/*
   Replace `index`th string.
   Returns 0 when `sv` is invalid or `index` is out-of-range.
 */
C_TINYUSD_EXPORT int c_tinyusd_string_vector_replace(
    c_tinyusd_string_vector *sv, const size_t idx, const char *str);

C_TINYUSD_EXPORT int c_tinyusd_string_vector_free(c_tinyusd_string_vector *sv);

/*
   Return the name of Prim type(e.g. "Xform", "Mesh", ...).
   Return NULL for unsupported/unknown Prim type.
   Returned char pointer is valid until Prim is modified or deleted.
 */
C_TINYUSD_EXPORT const char *c_tinyusd_prim_type_name(
    CTinyUSDPrimType prim_type);

/*
   Return Builtin PrimType from a string.
   Returns C_TINYUSD_PRIM_UNKNOWN for invalid or unknown/unsupported Prim type
 */
CTinyUSDPrimType c_tinyusd_prim_type_from_string(const char *prim_type);

/*
   Returns name of ValueType.
   The pointer points to static Thread-local storage(so thread-safe), thus no
   need to free it.
 */
C_TINYUSD_EXPORT const char *c_tinyusd_value_type_name(
    CTinyUSDValueType value_type);

/*
   Return 1: Value type is numeric type(float3, int, ...). 0 otherwise(e.g. token, dictionary, ...)
 */
C_TINYUSD_EXPORT uint32_t c_tinyusd_value_type_is_numeric(CTinyUSDValueType value_type);

/*
   Returns sizeof(value_type);
   For non-numeric value type(e.g. STRING, TOKEN) and invalid enum value, it
   returns 0. NOTE: Returns 1 for bool type.
 */
C_TINYUSD_EXPORT uint32_t c_tinyusd_value_type_sizeof(CTinyUSDValueType value_type);

/*
   Returns the number of components of given value_type;
   For example, 3 for C_TINYUSD_VALUE_POINT3F.
   For non-numeric value type(e.g. STRING, TOKEN), it returns 0.
   For scalar type, it returns 1.
 */
C_TINYUSD_EXPORT uint32_t
c_tinyusd_value_type_components(CTinyUSDValueType value_type);

/*  opaque pointer to tinyusdz::value::Value */
typedef struct CTinyUSDValue CTinyUSDValue;

/* Return value type enum.
   Returns C_TINYUSD_VALUE_UNKNOWN when `value` is nullptr or invalid.
 */
C_TINYUSD_EXPORT CTinyUSDValueType c_tinyusd_value_type(const CTinyUSDValue *value);

/*
  New Value with null(empty) value.
 */
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_null();

C_TINYUSD_EXPORT int c_tinyusd_value_free(CTinyUSDValue *val);

/*
   Get string representation of Value content(pprint).

   Return 0 upon error.
*/
C_TINYUSD_EXPORT int c_tinyusd_value_to_string(const CTinyUSDValue *val,
                                               c_tinyusd_string_t *str);

/* Free Value.
   Internally calls `c_tinyusd_buffer_free` to free buffer associated with this
   Value.
 */
C_TINYUSD_EXPORT int c_tinyusd_value_free(CTinyUSDValue *val);

/*
   New Value with token type.
   NOTE: token data are copied. So it is safe to free token after calling this
   function.
*/
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_token(
    const c_tinyusd_token_t *val);

/*
   New Value with string type.
   NOTE: string data are copied. So it is safe to free string after calling this
   function.
 */
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_string(
    const c_tinyusd_string_t *val);

/*
   New Value with specific type.
   NOTE: Datas are copied.
   Returns 1 upon success, 0 failed.
 */
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_int(int val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_int2(c_tinyusd_int2_t val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_int3(c_tinyusd_int3_t val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_int4(c_tinyusd_int4_t val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_float(float val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_float2(
    c_tinyusd_float2_t val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_float3(
    c_tinyusd_float3_t val);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_float4(
    c_tinyusd_float4_t val);
/*   TODO: List up other types... */

/* Check if the content of Value is the type of `value_type` */
C_TINYUSD_EXPORT int c_tinyusd_value_is_type(const CTinyUSDValue *value, CTinyUSDValueType value_type);

/*
   Get the actual value in CTinyUSDValue by specifying the type.
   NOTE: Datas are copied.
   Returns 1 upon success, 0 failed(e.g. Value is invalid, type mismatch).
 */
C_TINYUSD_EXPORT int c_tinyusd_value_as_int(const CTinyUSDValue *value, int *val);
C_TINYUSD_EXPORT int c_tinyusd_value_as_int2(const CTinyUSDValue *value, c_tinyusd_int2_t *val);
C_TINYUSD_EXPORT int c_tinyusd_value_as_int3(const CTinyUSDValue *value, c_tinyusd_int3_t *val);
C_TINYUSD_EXPORT int c_tinyusd_value_as_int4(const CTinyUSDValue *value, c_tinyusd_int4_t *val);

C_TINYUSD_EXPORT int c_tinyusd_value_as_float(const CTinyUSDValue *value, float *val);
C_TINYUSD_EXPORT int c_tinyusd_value_as_float2(const CTinyUSDValue *value, c_tinyusd_float2_t *val);
C_TINYUSD_EXPORT int c_tinyusd_value_as_float3(const CTinyUSDValue *value, c_tinyusd_float3_t *val);
C_TINYUSD_EXPORT int c_tinyusd_value_as_float4(const CTinyUSDValue *value, c_tinyusd_float4_t *val);


/*   TODO: List up other types... */


/*
   New Value with 1D array ofspecific type.
   NOTE: Array data is copied.
 */
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_int(uint64_t n,
                                                              const int *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_int2(
    uint64_t n, const c_tinyusd_int2_t *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_int3(
    uint64_t n, const c_tinyusd_int3_t *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_int4(
    uint64_t n, const c_tinyusd_int4_t *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_float(uint64_t n,
                                                                const float *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_float2(
    uint64_t n, const c_tinyusd_float2_t *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_float3(
    uint64_t n, const c_tinyusd_float3_t *vals);
C_TINYUSD_EXPORT CTinyUSDValue *c_tinyusd_value_new_array_float4(
    uint64_t n, const c_tinyusd_float4_t *vals);
/*   TODO: List up other types... */

/* opaque pointer to tinyusdz::Path */
typedef struct CTinyUSDPath CTinyUSDPath;

/* opaque pointer to tinyusdz::Property */
typedef struct CTinyUSDProperty CTinyUSDProperty;

/* opaque pointer to tinyusdz::Relationship */
typedef struct CTinyUSDRelationship CTinyUSDRelationship;

CTinyUSDRelationship *c_tinyusd_relationsip_new(uint32_t n,
                                                const char **targetPaths);

C_TINYUSD_EXPORT int c_tinyusd_relationsip_free(CTinyUSDRelationship *rel);

C_TINYUSD_EXPORT int c_tinyusd_relationsip_is_blocked(
    const CTinyUSDRelationship *rel);

/* Returns 0 = declaration only(e.g. `rel myrel`) */
C_TINYUSD_EXPORT uint32_t
c_tinyusd_relationsip_num_targetPaths(const CTinyUSDRelationship *rel);

/*
   Get i'th targetPath
   Returned `targetPath` is just a reference, so no need to free it.
 */
C_TINYUSD_EXPORT int c_tinyusd_relationsip_get_targetPath(
    const CTinyUSDRelationship *rel, uint32_t i, CTinyUSDPath *targetPath);

/*   opaque pointer to tinyusdz::Attribute */
typedef struct CTinyUSDAttribute CTinyUSDAttribute;

C_TINYUSD_EXPORT int c_tinyusd_attribute_connection_set(
    CTinyUSDAttribute *attr, const CTinyUSDPath *connectionPath);

C_TINYUSD_EXPORT int c_tinyusd_attribute_connections_set(
    CTinyUSDAttribute *attr, uint32_t n, const CTinyUSDPath *connectionPaths);

C_TINYUSD_EXPORT int c_tinyusd_attribute_meta_set(
    CTinyUSDAttribute *attr, const char *meta_name, const CTinyUSDValue *value);

/*
   Get metadata value.
   Returns 0 when `attr` is nullptr.
   Returns -1 when requested metadata is not authored.
   `value` is just a pointer so no need to free it(the pointer is valid until `attr` is modified/deleted)
 */
C_TINYUSD_EXPORT int c_tinyusd_attribute_meta_get(
    CTinyUSDAttribute *attr, const char *meta_name, const CTinyUSDValue **value);


#if 0
   Get i'th targetPaths
C_TINYUSD_EXPORT int c_tinyusd_attribute_connection_get(CTinyUSDAttribute *attr, uint32_t n, const CTinyUSDPath *connectionPaths);
#endif

C_TINYUSD_EXPORT int c_tinyusd_property_new(CTinyUSDProperty *prop);
C_TINYUSD_EXPORT int c_tinyusd_property_new_attribute(
    CTinyUSDProperty *prop, const CTinyUSDAttribute *attr);
C_TINYUSD_EXPORT int c_tinyusd_property_new_relationship(
    CTinyUSDProperty *prop, const CTinyUSDRelationship *rel);
C_TINYUSD_EXPORT int c_tinyusd_property_free(CTinyUSDProperty *prop);

C_TINYUSD_EXPORT int c_tinyusd_property_set_attribute(
    CTinyUSDProperty *prop, const CTinyUSDAttribute *attr);
C_TINYUSD_EXPORT int c_tinyusd_property_set_relationship(
    CTinyUSDProperty *prop, const CTinyUSDRelationship *rel);

C_TINYUSD_EXPORT int c_tinyusd_property_is_attribute(const CTinyUSDProperty *prop);
C_TINYUSD_EXPORT int c_tinyusd_property_is_attribute_connection(
    const CTinyUSDProperty *prop);
C_TINYUSD_EXPORT int c_tinyusd_property_is_relationship(const CTinyUSDProperty *prop);

C_TINYUSD_EXPORT int c_tinyusd_property_is_custom(const CTinyUSDProperty *prop);
C_TINYUSD_EXPORT int c_tinyusd_property_is_varying(const CTinyUSDProperty *prop);

typedef struct CTinyUSDPrim CTinyUSDPrim;

/*
   Create Prim with name.
   Will create a builtin Prim when `prim_type` is a builtin Prim name(e.g.
   "Xform")
   Otherwise create a Model Prim(Generic Prim).
   Return nullptr when `prim_type` is null or `prim_type` contains invalid
   character (A character which cannot be used for Prim type name(e.g. '%') and
   fills `err` with error message.
   (App need to free `err` after using it.)
 */
CTinyUSDPrim *c_tinyusd_prim_new(const char *prim_type,
                                 c_tinyusd_string_t *err);

/* Create Prim with builtin Prim type.
   Returns nullptr when invalid `prim_type` enum value is provided.
 */

CTinyUSDPrim *c_tinyusd_prim_new_builtin(CTinyUSDPrimType prim_type);

C_TINYUSD_EXPORT int c_tinyusd_prim_to_string(const CTinyUSDPrim *prim,
                                              c_tinyusd_string_t *str);

C_TINYUSD_EXPORT int c_tinyusd_prim_free(CTinyUSDPrim *prim);

/* Prim type as a const char pointer.
   Returns nullptr when `prim` is invalid */
C_TINYUSD_EXPORT const char *c_tinyusd_prim_type(const CTinyUSDPrim *prim);

/*
   Return the element name of Prim(e.g. "root", "pbr", "xform0").
   Return NULL when input `prim` is invalid.
   Returned char pointer is valid until Prim is modified or deleted.
 */
C_TINYUSD_EXPORT const char *c_tinyusd_prim_element_name(
    const CTinyUSDPrim *prim);


/*
   Get list of property names as token array.

   @param[in] prim Prim
   @param[out] prop_names Property names. Please initialize this instance using
   `c_tinyusd_token_vector_new` beforehand.

   @return 1 upon success(even when len(property names) == 0). 0 failure.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_get_property_names(
    const CTinyUSDPrim *prim, c_tinyusd_token_vector_t *prop_names_out);

/*
   Get Prim's property. Returns 0 when property `prop_name` does not exist in
   the Prim. `prop` just holds pointer to corresponding C++ Property instance,
   so no free operation required.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_property_get(const CTinyUSDPrim *prim,
                                                 const char *prop_name,
                                                 CTinyUSDProperty *prop);

/*
   Add property to the Prim.
   It copies the content of `prop`, so please free `prop` after this add
   operation. Returns 0 when the operation failed(`err` will be returned. Please
   free `err` after using it)
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_property_add(CTinyUSDPrim *prim,
                                                 const char *prop_name,
                                                 const CTinyUSDProperty *prop,
                                                 c_tinyusd_string_t *err);

/*
   Delete a property in the Prim.
   Returns 0 when `prop_name` does not exist in the prim.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_property_del(CTinyUSDPrim *prim,
                                                 const char *prop_name);

/*
   Set Prim metadatum.
   Return 0 when Value type mismatch for builtin metadata.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_meta_set(CTinyUSDPrim *prim,
                                             const char *meta_name,
                                             const CTinyUSDValue *value);

/*
   Get Prim metadatum.
   Return 0 when requested metadatum is not authord or invalid.
   Returned `value` is just a pointer, so no need to free it(and the pointer is valid unless `prim` is modified/deleted.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_meta_get(CTinyUSDPrim *prim,
                                             const char *meta_name,
                                             const CTinyUSDValue **value);

/*
   Check if requested metadatum is authored.
   Return 1: authored. 0 not authored.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_meta_authored(CTinyUSDPrim *prim,
                                             const char *meta_name);

/*
   Append Prim to `prim`'s children. child Prim object is *COPIED*.
   So need to free child Prim after the append_child operation.
 */

C_TINYUSD_EXPORT int c_tinyusd_prim_append_child(CTinyUSDPrim *prim,
                                                 CTinyUSDPrim *child);

/*
   Append Prim to `prim`'s children. child Prim object is moved(in C++ meaning).
   So no need to free child Prim(and `child` pointer is invalid after calling
   this function. Usefull if a Prim is a large object(e.g. GeomMesh with 100M
   vertices)
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_append_child_move(CTinyUSDPrim *prim,
                                                      CTinyUSDPrim *child);

/*
   Delete child at `child_index` position from a Prim.
   Return 0 when `child_index` is out-of-range.
 */
C_TINYUSD_EXPORT int c_tinyusd_prim_del_child(CTinyUSDPrim *prim,
                                              uint64_t child_index);

/*
   Return the number of child Prims in this Prim.

   Return 0 when `prim` is invalid or nullptr.
*/
C_TINYUSD_EXPORT uint64_t c_tinyusd_prim_num_children(const CTinyUSDPrim *prim);

/*
   Get a child Prim of specified child_index.

   Child's conent is just a pointer, so please do not call Prim
   deleter(`c_tinyusd_prim_free`) to it. (Please use `c_tinyusd_prim_del_child`
   if you want to remove a child Prim)

   Also the content(pointer) is valid unless the `prim`'s children is
   preserved(i.e., child is not deleted/added)

   Return 0 when `child_index` is out-of-range.
*/
C_TINYUSD_EXPORT int c_tinyusd_prim_get_child(const CTinyUSDPrim *prim,
                                              uint64_t child_index,
                                              const CTinyUSDPrim **child_prim);

/* opaque pointer to tinyusd::Stage */
typedef struct CTinyUSDStage CTinyUSDStage;

C_TINYUSD_EXPORT CTinyUSDStage *c_tinyusd_stage_new();
C_TINYUSD_EXPORT int c_tinyusd_stage_to_string(const CTinyUSDStage *stage,
                                               c_tinyusd_string_t *str);
C_TINYUSD_EXPORT int c_tinyusd_stage_free(CTinyUSDStage *stage);

/*
   Callback function for Stage's root Prim traversal.
   Return 1 for success, Return 0 to stop traversal futher.
 */
typedef int (*CTinyUSDTraversalFunction)(const CTinyUSDPrim *prim,
                                         const CTinyUSDPath *path);

/*
   Traverse root Prims in the Stage and invoke callback function for each Prim.

   @param[in] stage Stage.
   @param[in] callbacl_fun Callback function.
   @param[out] err Optional. error message.

   @return 1 upon success. 0 when failed(and `err` will be set).

   When providing `err`, it must be created with `c_tinyusd_string_new` before
   calling this `c_tinyusd_stage_traverse` function, and an App must free it by
   calling `c_tinyusd_string_free` after using it.
 */
C_TINYUSD_EXPORT int c_tinyusd_stage_traverse(
    const CTinyUSDStage *stage, CTinyUSDTraversalFunction callback_fun,
    c_tinyusd_string_t *err);

/*
   Detect file format of input file.
 */
CTinyUSDFormat c_tinyusd_detect_format(const char *filename);

C_TINYUSD_EXPORT int c_tinyusd_is_usd_file(const char *filename);
C_TINYUSD_EXPORT int c_tinyusd_is_usda_file(const char *filename);
C_TINYUSD_EXPORT int c_tinyusd_is_usdc_file(const char *filename);
C_TINYUSD_EXPORT int c_tinyusd_is_usdz_file(const char *filename);

/*
 * wide char version. especially for Windows UTF-16 filename.
 */
CTinyUSDFormat c_tinyusd_detect_format_w(const wchar_t *filename);

C_TINYUSD_EXPORT int c_tinyusd_is_usd_file_w(const wchar_t *filename);
C_TINYUSD_EXPORT int c_tinyusd_is_usda_file_w(const wchar_t *filename);
C_TINYUSD_EXPORT int c_tinyusd_is_usdc_file_w(const wchar_t *filename);
C_TINYUSD_EXPORT int c_tinyusd_is_usdz_file_w(const wchar_t *filename);

C_TINYUSD_EXPORT int c_tinyusd_is_usd_memory(const uint8_t *addr,
                                             const size_t nbytes);
C_TINYUSD_EXPORT int c_tinyusd_is_usda_memory(const uint8_t *addr,
                                              const size_t nbytes);
C_TINYUSD_EXPORT int c_tinyusd_is_usdc_memory(const uint8_t *addr,
                                              const size_t nbytes);
C_TINYUSD_EXPORT int c_tinyusd_is_usdz_memory(const uint8_t *addr,
                                              const size_t nbytes);

/*
 * Return 1 upon success. 0 when failed.
 */
C_TINYUSD_EXPORT int c_tinyusd_load_usd_from_file(const char *filename,
                                                  CTinyUSDStage *stage,
                                                  c_tinyusd_string_t *warn,
                                                  c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usda_from_file(const char *filename,
                                                   CTinyUSDStage *stage,
                                                   c_tinyusd_string_t *warn,
                                                   c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usdc_from_file(const char *filename,
                                                   CTinyUSDStage *stage,
                                                   c_tinyusd_string_t *warn,
                                                   c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usdz_from_file(const char *filename,
                                                   CTinyUSDStage *stage,
                                                   c_tinyusd_string_t *warn,
                                                   c_tinyusd_string_t *err);

/*
 * wide char version. especially for Windows UTF-16 filename.
 */
C_TINYUSD_EXPORT int c_tinyusd_load_usd_from_file_w(const wchar_t *filename,
                                                    CTinyUSDStage *stage,
                                                    c_tinyusd_string_t *warn,
                                                    c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usda_from_file_w(const wchar_t *filename,
                                                     CTinyUSDStage *stage,
                                                     c_tinyusd_string_t *warn,
                                                     c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usdc_from_file_w(const wchar_t *filename,
                                                     CTinyUSDStage *stage,
                                                     c_tinyusd_string_t *warn,
                                                     c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usdz_from_file_w(const wchar_t *filename,
                                                     CTinyUSDStage *stage,
                                                     c_tinyusd_string_t *warn,
                                                     c_tinyusd_string_t *err);

C_TINYUSD_EXPORT int c_tinyusd_load_usd_from_memory(const uint8_t *addr,
                                                    const size_t nbytes,
                                                    c_tinyusd_string_t *warn,
                                                    c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usda_from_memory(const uint8_t *addr,
                                                     const size_t nbytes,
                                                     c_tinyusd_string_t *warn,
                                                     c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usdc_from_memory(const uint8_t *addr,
                                                     const size_t nbytes,
                                                     c_tinyusd_string_t *warn,
                                                     c_tinyusd_string_t *err);
C_TINYUSD_EXPORT int c_tinyusd_load_usdz_from_memory(const uint8_t *addr,
                                                     const size_t nbytes,
                                                     c_tinyusd_string_t *warn,
                                                     c_tinyusd_string_t *err);

#ifdef __cplusplus
}
#endif

#endif  // C_TINYUSD_H
