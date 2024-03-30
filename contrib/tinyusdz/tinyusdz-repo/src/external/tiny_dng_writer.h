//
// TinyDNGWriter, single header only DNG writer in C++11.
//

/*
The MIT License (MIT)

Copyright (c) 2016 - 2020 Syoyo Fujita.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef TINY_DNG_WRITER_H_
#define TINY_DNG_WRITER_H_

#include <sstream>
#include <vector>
#include <cstring>

namespace tinydngwriter {

typedef enum {
  TIFFTAG_SUB_FILETYPE = 254,
  TIFFTAG_IMAGE_WIDTH = 256,
  TIFFTAG_IMAGE_LENGTH = 257,
  TIFFTAG_BITS_PER_SAMPLE = 258,
  TIFFTAG_COMPRESSION = 259,
  TIFFTAG_PHOTOMETRIC = 262,
  TIFFTAG_IMAGEDESCRIPTION = 270,
  TIFFTAG_STRIP_OFFSET = 273,
  TIFFTAG_SAMPLES_PER_PIXEL = 277,
  TIFFTAG_ROWS_PER_STRIP = 278,
  TIFFTAG_STRIP_BYTE_COUNTS = 279,
  TIFFTAG_PLANAR_CONFIG = 284,
  TIFFTAG_ORIENTATION = 274,

  TIFFTAG_XRESOLUTION = 282,  // rational
  TIFFTAG_YRESOLUTION = 283,  // rational
  TIFFTAG_RESOLUTION_UNIT = 296,

  TIFFTAG_SOFTWARE = 305,

  TIFFTAG_SAMPLEFORMAT = 339,

  // DNG extension
  TIFFTAG_CFA_REPEAT_PATTERN_DIM = 33421,
  TIFFTAG_CFA_PATTERN = 33422,

  TIFFTAG_DNG_VERSION = 50706,
  TIFFTAG_DNG_BACKWARD_VERSION = 50707,
  TIFFTAG_UNIQUE_CAMERA_MODEL = 50708,
  TIFFTAG_CHRROMA_BLUR_RADIUS = 50703,
  TIFFTAG_BLACK_LEVEL_REPEAT_DIM = 50713,
  TIFFTAG_BLACK_LEVEL = 50714,
  TIFFTAG_WHITE_LEVEL = 50717,
  TIFFTAG_COLOR_MATRIX1 = 50721,
  TIFFTAG_COLOR_MATRIX2 = 50722,
  TIFFTAG_CAMERA_CALIBRATION1 = 50723,
  TIFFTAG_CAMERA_CALIBRATION2 = 50724,
  TIFFTAG_ANALOG_BALANCE = 50727,
  TIFFTAG_AS_SHOT_NEUTRAL = 50728,
  TIFFTAG_AS_SHOT_WHITE_XY = 50729,
  TIFFTAG_CALIBRATION_ILLUMINANT1 = 50778,
  TIFFTAG_CALIBRATION_ILLUMINANT2 = 50779,
  TIFFTAG_EXTRA_CAMERA_PROFILES = 50933,
  TIFFTAG_PROFILE_NAME = 50936,
  TIFFTAG_AS_SHOT_PROFILE_NAME = 50934,
  TIFFTAG_DEFAULT_BLACK_RENDER = 51110,
  TIFFTAG_ACTIVE_AREA = 50829,
  TIFFTAG_FORWARD_MATRIX1 = 50964,
  TIFFTAG_FORWARD_MATRIX2 = 50965
} Tag;

// SUBFILETYPE(bit field)
static const int FILETYPE_REDUCEDIMAGE = 1;
static const int FILETYPE_PAGE = 2;
static const int FILETYPE_MASK = 4;

// PLANARCONFIG
static const int PLANARCONFIG_CONTIG = 1;
static const int PLANARCONFIG_SEPARATE = 2;

// COMPRESSION
// TODO(syoyo) more compressin types.
static const int COMPRESSION_NONE = 1;
static const int COMPRESSION_NEW_JPEG = 7;

// ORIENTATION
static const int ORIENTATION_TOPLEFT = 1;
static const int ORIENTATION_TOPRIGHT = 2;
static const int ORIENTATION_BOTRIGHT = 3;
static const int ORIENTATION_BOTLEFT = 4;
static const int ORIENTATION_LEFTTOP = 5;
static const int ORIENTATION_RIGHTTOP = 6;
static const int ORIENTATION_RIGHTBOT = 7;
static const int ORIENTATION_LEFTBOT = 8;

// RESOLUTIONUNIT
static const int RESUNIT_NONE = 1;
static const int RESUNIT_INCH = 2;
static const int RESUNIT_CENTIMETER = 2;

// PHOTOMETRIC
// TODO(syoyo): more photometric types.
static const int PHOTOMETRIC_WHITE_IS_ZERO = 0;  // For bilevel and grayscale
static const int PHOTOMETRIC_BLACK_IS_ZERO = 1;  // For bilevel and grayscale
static const int PHOTOMETRIC_RGB = 2;            // Default
static const int PHOTOMETRIC_CFA = 32803;        // DNG ext
static const int PHOTOMETRIC_LINEARRAW = 34892;  // DNG ext

// Sample format
static const int SAMPLEFORMAT_UINT = 1;  // Default
static const int SAMPLEFORMAT_INT = 2;
static const int SAMPLEFORMAT_IEEEFP = 3;  // floating point

struct IFDTag {
  unsigned short tag;
  unsigned short type;
  unsigned int count;
  unsigned int offset_or_value;
};
// 12 bytes.

class DNGImage {
 public:
  DNGImage();
  ~DNGImage() {}

  ///
  /// Optional: Explicitly specify endian.
  /// Must be called before calling other Set methods.
  ///
  void SetBigEndian(bool big_endian);

  ///
  /// Default = 0
  ///
  bool SetSubfileType(bool reduced_image = false, bool page = false,
                      bool mask = false);

  bool SetImageWidth(unsigned int value);
  bool SetImageLength(unsigned int value);
  bool SetRowsPerStrip(unsigned int value);
  bool SetSamplesPerPixel(unsigned short value);
  // Set bits for each samples
  bool SetBitsPerSample(const unsigned int num_samples,
                        const unsigned short *values);
  bool SetPhotometric(unsigned short value);
  bool SetPlanarConfig(unsigned short value);
  bool SetOrientation(unsigned short value);
  bool SetCompression(unsigned short value);
  bool SetSampleFormat(const unsigned int num_samples,
                       const unsigned short *values);
  bool SetXResolution(double value);
  bool SetYResolution(double value);
  bool SetResolutionUnit(const unsigned short value);

  ///
  /// Set arbitrary string for image description.
  /// Currently we limit to 1024*1024 chars at max.
  ///
  bool SetImageDescription(const std::string &ascii);

  ///
  /// Set arbitrary string for unique camera model name (not localized!).
  /// Currently we limit to 1024*1024 chars at max.
  ///
  bool SetUniqueCameraModel(const std::string &ascii);

  ///
  /// Set software description(string).
  /// Currently we limit to 4095 chars at max.
  ///
  bool SetSoftware(const std::string &ascii);

  bool SetActiveArea(const unsigned int values[4]);

  bool SetChromaBlurRadius(double value);

  /// Specify black level per sample.
  bool SetBlackLevel(const unsigned int num_samples, const unsigned short *values);

  /// Specify black level per sample (as rational values).
  bool SetBlackLevelRational(unsigned int num_samples, const double *values);

  /// Specify white level per sample.
  bool SetWhiteLevelRational(unsigned int num_samples, const double *values);

  /// Specify analog white balance from camera for raw values.
  bool SetAnalogBalance(const unsigned int plane_count, const double *matrix_values);

  /// Specify CFA repeating pattern dimensions.
  bool SetCFARepeatPatternDim(const unsigned short width, const unsigned short height);

  /// Specify black level repeating pattern dimensions.
  bool SetBlackLevelRepeatDim(const unsigned short width, const unsigned short height);

  bool SetCalibrationIlluminant1(const unsigned short value);
  bool SetCalibrationIlluminant2(const unsigned short value);

  /// Specify DNG version.
  bool SetDNGVersion(const unsigned char a, const unsigned char b, const unsigned char c, const unsigned char d);

  /// Specify transformation matrix (XYZ to reference camera native color space values, under the first calibration illuminant).
  bool SetColorMatrix1(const unsigned int plane_count, const double *matrix_values);

  /// Specify transformation matrix (XYZ to reference camera native color space values, under the second calibration illuminant).
  bool SetColorMatrix2(const unsigned int plane_count, const double *matrix_values);

  bool SetForwardMatrix1(const unsigned int plane_count, const double *matrix_values);
  bool SetForwardMatrix2(const unsigned int plane_count, const double *matrix_values);

  bool SetCameraCalibration1(const unsigned int plane_count, const double *matrix_values);
  bool SetCameraCalibration2(const unsigned int plane_count, const double *matrix_values);

  /// Specify CFA geometric pattern (left-to-right, top-to-bottom).
  bool SetCFAPattern(const unsigned int num_components, const unsigned char *values);

  /// Specify the selected white balance at time of capture, encoded as the coordinates of a perfectly neutral color in linear reference space values.
  bool SetAsShotNeutral(const unsigned int plane_count, const double *matrix_values);

  /// Specify the the selected white balance at time of capture, encoded as x-y chromaticity coordinates.
  bool SetAsShotWhiteXY(const double x, const double y);

  /// Set image data with packing (take 16-bit values and pack them to input_bpp values).
  bool SetImageDataPacked(const unsigned short *input_buffer, const int input_count, const unsigned int input_bpp, bool big_endian);

  /// Set image data.
  bool SetImageData(const unsigned char *data, const size_t data_len);

  /// Set image data.
  bool SetImageDataJpeg(const unsigned short *data, unsigned int width, unsigned int height, unsigned int bpp);

  /// Set custom field.
  bool SetCustomFieldLong(const unsigned short tag, const int value);
  bool SetCustomFieldULong(const unsigned short tag, const unsigned int value);

  size_t GetDataSize() const { return data_os_.str().length(); }

  size_t GetStripOffset() const { return data_strip_offset_; }
  size_t GetStripBytes() const { return data_strip_bytes_; }

  /// Write aux IFD data and strip image data to stream.
  bool WriteDataToStream(std::ostream *ofs) const;

  ///
  /// Write IFD to stream.
  ///
  /// @param[in] data_base_offset : Byte offset to data
  /// @param[in] strip_offset : Byte offset to image strip data
  ///
  /// TODO(syoyo): Support multiple strips
  ///
  bool WriteIFDToStream(const unsigned int data_base_offset,
                        const unsigned int strip_offset, std::ostream *ofs) const;

  std::string Error() const { return err_; }

 private:
  std::ostringstream data_os_;
  bool swap_endian_;
  bool dng_big_endian_;
  unsigned short num_fields_;
  unsigned int samples_per_pixels_;
  std::vector<unsigned short> bits_per_samples_;

  // TODO(syoyo): Support multiple strips
  size_t data_strip_offset_{0};
  size_t data_strip_bytes_{0};

  mutable std::string err_;  // Error message

  std::vector<IFDTag> ifd_tags_;
};

class DNGWriter {
 public:
  // TODO(syoyo): Use same endian setting with DNGImage.
  DNGWriter(bool big_endian);
  ~DNGWriter() {}

  ///
  /// Add DNGImage.
  /// It just retains the pointer of the image, thus
  /// application must not free resources until `WriteToFile` has been called.
  ///
  bool AddImage(const DNGImage *image) {
    images_.push_back(image);

    return true;
  }

  /// Write DNG to a file.
  /// Return error string to `err` when Write() returns false.
  /// Returns true upon success.
  bool WriteToFile(const char *filename, std::string *err) const;

 private:
  bool swap_endian_;
  bool dng_big_endian_;  // Endianness of DNG file.

  std::vector<const DNGImage *> images_;
};

}  // namespace tinydngwriter

#endif  // TINY_DNG_WRITER_H_

#ifdef TINY_DNG_WRITER_IMPLEMENTATION

//
// TIFF format resources.
//
// http://c0de517e.blogspot.jp/2013/07/tiny-hdr-writer.html
// http://paulbourke.net/dataformats/tiff/ and
// http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf
//

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>

// Undef if you want to use builtin function for clz
#if 0
#ifdef _MSC_VER
#include <intrin.h>
#endif
#endif


namespace tinydngwriter {

namespace detail {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// Begin liblj92, Lossless JPEG decode/encoder ------------------------------
//
// With fixes: https://github.com/ilia3101/MLV-App/pull/151

/*
lj92.c
(c) Andrew Baldwin 2014

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

enum LJ92_ERRORS {
  LJ92_ERROR_NONE = 0,
  LJ92_ERROR_CORRUPT = -1,
  LJ92_ERROR_NO_MEMORY = -2,
  LJ92_ERROR_BAD_HANDLE = -3,
  LJ92_ERROR_TOO_WIDE = -4
};

/*
 * Encode a grayscale image supplied as 16bit values within the given bitdepth
 * Read from tile in the image
 * Apply delinearization if given
 * Return the encoded lossless JPEG stream
 */
int lj92_encode(uint16_t *image, int width, int height, int bitdepth,
                int readLength, int skipLength, uint16_t *delinearize,
                int delinearizeLength, uint8_t **encoded, int *encodedLength);

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

//#define LJ92_DEBUG

/* Encoder implementation */

#if 0
uint32_t __inline clz32(uint32_t value) {
  unsigned long leading_zero = 0;

  if (_BitScanReverse(&leading_zero, value)) {
    return 31 - leading_zero;
  } else {
    // Same remarks as above
    return 32;
  }
}
#endif

// Very simple count leading zero implementation.
static int clz32(unsigned int x) {
  int n;
  if (x == 0) return 32;
  for (n = 0; ((x & 0x80000000) == 0); n++, x <<= 1)
    ;
  return n;
}

typedef struct _lje {
  uint16_t *image;
  int width;
  int height;
  int bitdepth;
  int readLength;
  int skipLength;
  uint16_t *delinearize;
  int delinearizeLength;
  uint8_t *encoded;
  int encodedWritten;
  int encodedLength;
  int hist[17];  // SSSS frequency histogram
  int bits[17];
  int huffval[17];
  u16 huffenc[17];
  u16 huffbits[17];
  int huffsym[17];
} lje;

int frequencyScan(lje *self) {
  // Scan through the tile using the standard type 6 prediction
  // Need to cache the previous 2 row in target coordinates because of tiling
  uint16_t *pixel = self->image;
  int pixcount = self->width * self->height;
  int scan = self->readLength;
  uint16_t *rowcache = (uint16_t *)calloc(1, self->width * 4);
  uint16_t *rows[2];
  rows[0] = rowcache;
  rows[1] = &rowcache[self->width];

  int col = 0;
  int row = 0;
  int Px = 0;
  int32_t diff = 0;
  int maxval = (1 << self->bitdepth);
  while (pixcount--) {
    uint16_t p = *pixel;
    if (self->delinearize) {
      if (p >= self->delinearizeLength) {
        free(rowcache);
        return LJ92_ERROR_TOO_WIDE;
      }
      p = self->delinearize[p];
    }
    if (p >= maxval) {
      free(rowcache);
      return LJ92_ERROR_TOO_WIDE;
    }
    rows[1][col] = p;

    if ((row == 0) && (col == 0))
      Px = 1 << (self->bitdepth - 1);
    else if (row == 0)
      Px = rows[1][col - 1];
    else if (col == 0)
      Px = rows[0][col];
    else
      Px = rows[0][col] + ((rows[1][col - 1] - rows[0][col - 1]) >> 1);
    diff = rows[1][col] - Px;
    int ssss = 32 - clz32(abs(diff));
    if (diff == 0) ssss = 0;
    self->hist[ssss]++;
    // printf("%d %d %d %d %d %d\n",col,row,p,Px,diff,ssss);
    pixel++;
    scan--;
    col++;
    if (scan == 0) {
      pixel += self->skipLength;
      scan = self->readLength;
    }
    if (col == self->width) {
      uint16_t *tmprow = rows[1];
      rows[1] = rows[0];
      rows[0] = tmprow;
      col = 0;
      row++;
    }
  }
#ifdef DEBUG
  int sort[17];
  for (int h = 0; h < 17; h++) {
    sort[h] = h;
    printf("%d:%d\n", h, self->hist[h]);
  }
#endif
  free(rowcache);
  return LJ92_ERROR_NONE;
}

void createEncodeTable(lje *self) {
  float freq[18];
  int codesize[18];
  int others[18];

  // Calculate frequencies
  float totalpixels = self->width * self->height;
  for (int i = 0; i < 17; i++) {
    freq[i] = (float)(self->hist[i]) / totalpixels;
#ifdef DEBUG
    printf("%d:%f\n", i, freq[i]);
#endif
    codesize[i] = 0;
    others[i] = -1;
  }
  codesize[17] = 0;
  others[17] = -1;
  freq[17] = 1.0f;

  float v1f, v2f;
  int v1, v2;

  while (1) {
    v1f = 3.0f;
    v1 = -1;
    for (int i = 0; i < 18; i++) {
      if ((freq[i] <= v1f) && (freq[i] > 0.0f)) {
        v1f = freq[i];
        v1 = i;
      }
    }
#ifdef DEBUG
    printf("v1:%d,%f\n", v1, v1f);
#endif
    v2f = 3.0f;
    v2 = -1;
    for (int i = 0; i < 18; i++) {
      if (i == v1) continue;
      if ((freq[i] < v2f) && (freq[i] > 0.0f)) {
        v2f = freq[i];
        v2 = i;
      }
    }
    if (v2 == -1) break;  // Done

    freq[v1] += freq[v2];
    freq[v2] = 0.0f;

    while (1) {
      codesize[v1]++;
      if (others[v1] == -1) break;
      v1 = others[v1];
    }
    others[v1] = v2;
    while (1) {
      codesize[v2]++;
      if (others[v2] == -1) break;
      v2 = others[v2];
    }
  }
  int *bits = self->bits;
  memset(bits, 0, sizeof(self->bits));
  for (int i = 0; i < 18; i++) {
    if (codesize[i] != 0) {
      bits[codesize[i]]++;
    }
  }
#ifdef DEBUG
  for (int i = 0; i < 17; i++) {
    printf("bits:%d,%d,%d\n", i, bits[i], codesize[i]);
  }
#endif
  int *huffval = self->huffval;
  int i = 1;
  int k = 0;
  int j;
  memset(huffval, 0, sizeof(self->huffval));
  while (i <= 32) {
    j = 0;
    while (j < 17) {
      if (codesize[j] == i) {
        huffval[k++] = j;
      }
      j++;
    }
    i++;
  }
#ifdef DEBUG
  for (i = 0; i < 17; i++) {
    printf("i=%d,huffval[i]=%x\n", i, huffval[i]);
  }
#endif
  int maxbits = 16;
  while (maxbits > 0) {
    if (bits[maxbits]) break;
    maxbits--;
  }
  u16 *huffenc = self->huffenc;
  u16 *huffbits = self->huffbits;
  int *huffsym = self->huffsym;
  memset(huffenc, 0, sizeof(self->huffenc));
  memset(huffbits, 0, sizeof(self->huffbits));
  memset(self->huffsym, 0, sizeof(self->huffsym));
  i = 0;
  int hv = 0;
  int rv = 0;
  int vl = 0;  // i
  // int hcode;
  int bitsused = 1;
  int sym = 0;
  // printf("%04x:%x:%d:%x\n",i,huffvals[hv],bitsused,1<<(maxbits-bitsused));
  while (i < 1 << maxbits) {
    if (bitsused > maxbits) {
      break;  // Done. Should never get here!
    }
    if (vl >= bits[bitsused]) {
      bitsused++;
      vl = 0;
      continue;
    }
    if (rv == 1 << (maxbits - bitsused)) {
      rv = 0;
      vl++;
      hv++;
      // printf("%04x:%x:%d:%x\n",i,huffvals[hv],bitsused,1<<(maxbits-bitsused));
      continue;
    }
    huffbits[sym] = bitsused;
    huffenc[sym++] = i >> (maxbits - bitsused);
    // printf("%d %d %d\n",i,bitsused,hcode);
    i += (1 << (maxbits - bitsused));
    rv = 1 << (maxbits - bitsused);
  }
  for (i = 0; i < 17; i++) {
    if (huffbits[i] > 0) {
      huffsym[huffval[i]] = i;
    }
#ifdef DEBUG
    printf("huffval[%d]=%d,huffenc[%d]=%x,bits=%d\n", i, huffval[i], i,
           huffenc[i], huffbits[i]);
#endif
    if (huffbits[i] > 0) {
      huffsym[huffval[i]] = i;
    }
  }
#ifdef DEBUG
  for (i = 0; i < 17; i++) {
    printf("huffsym[%d]=%d\n", i, huffsym[i]);
  }
#endif
}

void writeHeader(lje *self) {
  int w = self->encodedWritten;
  uint8_t *e = self->encoded;
  e[w++] = 0xff;
  e[w++] = 0xd8;  // SOI
  e[w++] = 0xff;
  e[w++] = 0xc3;  // SOF3
  // Write SOF
  e[w++] = 0x0;
  e[w++] = 11;  // Lf, frame header length
  e[w++] = self->bitdepth;
  e[w++] = self->height >> 8;
  e[w++] = self->height & 0xFF;
  e[w++] = self->width >> 8;
  e[w++] = self->width & 0xFF;
  e[w++] = 1;     // Components
  e[w++] = 0;     // Component ID
  e[w++] = 0x11;  // Component X/Y
  e[w++] = 0;     // Unused (Quantisation)
  e[w++] = 0xff;
  e[w++] = 0xc4;  // HUFF
  // Write HUFF
  int count = 0;
  for (int i = 0; i < 17; i++) {
    count += self->bits[i];
  }
  e[w++] = 0x0;
  e[w++] = 17 + 2 + count;  // Lf, frame header length
  e[w++] = 0;               // Table ID
  for (int i = 1; i < 17; i++) {
    e[w++] = self->bits[i];
  }
  for (int i = 0; i < count; i++) {
    e[w++] = self->huffval[i];
  }
  e[w++] = 0xff;
  e[w++] = 0xda;  // SCAN
  // Write SCAN
  e[w++] = 0x0;
  e[w++] = 8;  // Ls, scan header length
  e[w++] = 1;  // Components
  e[w++] = 0;  //
  e[w++] = 0;  //
  e[w++] = 6;  // Predictor
  e[w++] = 0;  //
  e[w++] = 0;  //
  self->encodedWritten = w;
}

void writePost(lje *self) {
  int w = self->encodedWritten;
  uint8_t *e = self->encoded;
  e[w++] = 0xff;
  e[w++] = 0xd9;  // EOI
  self->encodedWritten = w;
}

void writeBody(lje *self) {
  // Scan through the tile using the standard type 6 prediction
  // Need to cache the previous 2 row in target coordinates because of tiling
  uint16_t *pixel = self->image;
  int pixcount = self->width * self->height;
  int scan = self->readLength;
  uint16_t *rowcache = (uint16_t *)calloc(1, self->width * 4);
  uint16_t *rows[2];
  rows[0] = rowcache;
  rows[1] = &rowcache[self->width];

  int col = 0;
  int row = 0;
  int Px = 0;
  int32_t diff = 0;
  int bitcount = 0;
  uint8_t *out = self->encoded;
  int w = self->encodedWritten;
  uint8_t next = 0;
  uint8_t nextbits = 8;
  while (pixcount--) {
    uint16_t p = *pixel;
    if (self->delinearize) p = self->delinearize[p];
    rows[1][col] = p;

    if ((row == 0) && (col == 0))
      Px = 1 << (self->bitdepth - 1);
    else if (row == 0)
      Px = rows[1][col - 1];
    else if (col == 0)
      Px = rows[0][col];
    else
      Px = rows[0][col] + ((rows[1][col - 1] - rows[0][col - 1]) >> 1);
    diff = rows[1][col] - Px;
    int ssss = 32 - clz32(abs(diff));
    if (diff == 0) ssss = 0;
    // printf("%d %d %d %d %d\n",col,row,Px,diff,ssss);

    // Write the huffman code for the ssss value
    int huffcode = self->huffsym[ssss];
    int huffenc = self->huffenc[huffcode];
    int huffbits = self->huffbits[huffcode];
    bitcount += huffbits + ssss;

    int vt = ssss > 0 ? (1 << (ssss - 1)) : 0;
    // printf("%d %d %d %d\n",rows[1][col],Px,diff,Px+diff);
#ifdef DEBUG
#endif
    if (diff < vt) diff += (1 << (ssss)) - 1;

    // Write the ssss
    while (huffbits > 0) {
      int usebits = huffbits > nextbits ? nextbits : huffbits;
      // Add top usebits from huffval to next usebits of nextbits
      int tophuff = huffenc >> (huffbits - usebits);
      next |= (tophuff << (nextbits - usebits));
      nextbits -= usebits;
      huffbits -= usebits;
      huffenc &= (1 << huffbits) - 1;
      if (nextbits == 0) {
        out[w++] = next;
        if (next == 0xff) out[w++] = 0x0;
        next = 0;
        nextbits = 8;
      }
    }
    // Write the rest of the bits for the value

    while (ssss > 0) {
      int usebits = ssss > nextbits ? nextbits : ssss;
      // Add top usebits from huffval to next usebits of nextbits
      int tophuff = diff >> (ssss - usebits);
      next |= (tophuff << (nextbits - usebits));
      nextbits -= usebits;
      ssss -= usebits;
      diff &= (1 << ssss) - 1;
      if (nextbits == 0) {
        out[w++] = next;
        if (next == 0xff) out[w++] = 0x0;
        next = 0;
        nextbits = 8;
      }
    }

    // printf("%d %d\n",diff,ssss);
    pixel++;
    scan--;
    col++;
    if (scan == 0) {
      pixel += self->skipLength;
      scan = self->readLength;
    }
    if (col == self->width) {
      uint16_t *tmprow = rows[1];
      rows[1] = rows[0];
      rows[0] = tmprow;
      col = 0;
      row++;
    }
  }
  // Flush the final bits
  if (nextbits < 8) {
    out[w++] = next;
    if (next == 0xff) out[w++] = 0x0;
  }
#ifdef DEBUG
  int sort[17];
  for (int h = 0; h < 17; h++) {
    sort[h] = h;
    printf("%d:%d\n", h, self->hist[h]);
  }
  printf("Total bytes: %d\n", bitcount >> 3);
#endif
  free(rowcache);
  self->encodedWritten = w;
}

/* Encoder
 * Read tile from an image and encode in one shot
 * Return the encoded data
 */
int lj92_encode(uint16_t *image, int width, int height, int bitdepth,
                int readLength, int skipLength, uint16_t *delinearize,
                int delinearizeLength, uint8_t **encoded, int *encodedLength) {
  int ret = LJ92_ERROR_NONE;

  lje *self = (lje *)calloc(sizeof(lje), 1);
  if (self == NULL) return LJ92_ERROR_NO_MEMORY;
  self->image = image;
  self->width = width;
  self->height = height;
  self->bitdepth = bitdepth;
  self->readLength = readLength;
  self->skipLength = skipLength;
  self->delinearize = delinearize;
  self->delinearizeLength = delinearizeLength;
  self->encodedLength = width * height * 3 + 200;
  self->encoded = (uint8_t*)malloc(self->encodedLength);
  if (self->encoded == NULL) {
    free(self);
    return LJ92_ERROR_NO_MEMORY;
  }
  // Scan through data to gather frequencies of ssss prefixes
  ret = frequencyScan(self);
  if (ret != LJ92_ERROR_NONE) {
    free(self->encoded);
    free(self);
    return ret;
  }
  // Create encoded table based on frequencies
  createEncodeTable(self);
  // Write JPEG head and scan header
  writeHeader(self);
  // Scan through and do the compression
  writeBody(self);
  // Finish
  writePost(self);
#ifdef DEBUG
  printf("written:%d\n", self->encodedWritten);
#endif
  self->encoded = (uint8_t*)realloc(self->encoded, self->encodedWritten);
  self->encodedLength = self->encodedWritten;
  *encoded = self->encoded;
  *encodedLength = self->encodedLength;

  free(self);

  return ret;
}

// End liblj92 ---------------------------------------------------------

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace detail

#ifdef __clang__
#pragma clang diagnostic push
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#endif

//
// TinyDNGWriter stores IFD table in the end of file so that offset to
// image data can be easily computed.
//
// +----------------------+
// |    header            |
// +----------------------+
// |                      |
// |  image & meta 0      |
// |                      |
// +----------------------+
// |                      |
// |  image & meta 1      |
// |                      |
// +----------------------+
//    ...
// +----------------------+
// |                      |
// |  image & meta N      |
// |                      |
// +----------------------+
// |                      |
// |  IFD 0               |
// |                      |
// +----------------------+
// |                      |
// |  IFD 1               |
// |                      |
// +----------------------+
//    ...
// +----------------------+
// |                      |
// |  IFD 2               |
// |                      |
// +----------------------+
//

// From tiff.h
typedef enum {
  TIFF_NOTYPE = 0,     /* placeholder */
  TIFF_BYTE = 1,       /* 8-bit unsigned integer */
  TIFF_ASCII = 2,      /* 8-bit bytes w/ last byte null */
  TIFF_SHORT = 3,      /* 16-bit unsigned integer */
  TIFF_LONG = 4,       /* 32-bit unsigned integer */
  TIFF_RATIONAL = 5,   /* 64-bit unsigned fraction */
  TIFF_SBYTE = 6,      /* !8-bit signed integer */
  TIFF_UNDEFINED = 7,  /* !8-bit untyped data */
  TIFF_SSHORT = 8,     /* !16-bit signed integer */
  TIFF_SLONG = 9,      /* !32-bit signed integer */
  TIFF_SRATIONAL = 10, /* !64-bit signed fraction */
  TIFF_FLOAT = 11,     /* !32-bit IEEE floating point */
  TIFF_DOUBLE = 12,    /* !64-bit IEEE floating point */
  TIFF_IFD = 13,       /* %32-bit unsigned integer (offset) */
  TIFF_LONG8 = 16,     /* BigTIFF 64-bit unsigned integer */
  TIFF_SLONG8 = 17,    /* BigTIFF 64-bit signed integer */
  TIFF_IFD8 = 18       /* BigTIFF 64-bit unsigned integer (offset) */
} DataType;

const static int kHeaderSize = 8;  // TIFF header size.

// floating point to integer rational value conversion
// https://stackoverflow.com/questions/51142275/exact-value-of-a-floating-point-number-as-a-rational
//
// Return error flag
static int DoubleToRational(double x, double *numerator, double *denominator) {
  if (!std::isfinite(x)) {
    *numerator = *denominator = 0.0;
    if (x > 0.0) *numerator = 1.0;
    if (x < 0.0) *numerator = -1.0;
    return 1;
  }

  // TIFF Rational use two uint32's, so reduce the bits
  int bdigits = FLT_MANT_DIG;
  int expo;
  *denominator = 1.0;
  *numerator = std::frexp(x, &expo) * std::pow(2.0, bdigits);
  expo -= bdigits;
  if (expo > 0) {
    *numerator *= std::pow(2.0, expo);
  } else if (expo < 0) {
    expo = -expo;
    if (expo >= FLT_MAX_EXP - 1) {
      *numerator /= std::pow(2.0, expo - (FLT_MAX_EXP - 1));
      *denominator *= std::pow(2.0, FLT_MAX_EXP - 1);
      return fabs(*numerator) < 1.0;
    } else {
      *denominator *= std::pow(2.0, expo);
    }
  }

  while ((std::fabs(*numerator) > 0.0) &&
         (std::fabs(std::fmod(*numerator, 2)) <
          std::numeric_limits<double>::epsilon()) &&
         (std::fabs(std::fmod(*denominator, 2)) <
          std::numeric_limits<double>::epsilon())) {
    *numerator /= 2.0;
    *denominator /= 2.0;
  }
  return 0;
}

static inline bool IsBigEndian() {
  unsigned int i = 0x01020304;
  char c[4];
  memcpy(c, &i, 4);
  return (c[0] == 1);
}

static void swap2(unsigned short *val) {
  unsigned short tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[1];
  dst[1] = src[0];
}

static void swap4(unsigned int *val) {
  unsigned int tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
}

static void swap8(uint64_t *val) {
  uint64_t tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[7];
  dst[1] = src[6];
  dst[2] = src[5];
  dst[3] = src[4];
  dst[4] = src[3];
  dst[5] = src[2];
  dst[6] = src[1];
  dst[7] = src[0];
}

static void Write1(const unsigned char c, std::ostringstream *out) {
  unsigned char value = c;
  out->write(reinterpret_cast<const char *>(&value), 1);
}

static void Write2(const unsigned short c, std::ostringstream *out,
                   const bool swap_endian) {
  unsigned short value = c;
  if (swap_endian) {
    swap2(&value);
  }

  out->write(reinterpret_cast<const char *>(&value), 2);
}

static void Write4(const unsigned int c, std::ostringstream *out,
                   const bool swap_endian) {
  unsigned int value = c;
  if (swap_endian) {
    swap4(&value);
  }

  out->write(reinterpret_cast<const char *>(&value), 4);
}

static bool WriteTIFFTag(const unsigned short tag, const unsigned short type,
                         const unsigned int count, const unsigned char *data,
                         std::vector<IFDTag> *tags_out,
                         std::ostringstream *data_out) {
  assert(sizeof(IFDTag) ==
         12);  // FIXME(syoyo): Use static_assert for C++11 compiler

  IFDTag ifd;
  ifd.tag = tag;
  ifd.type = type;
  ifd.count = count;

  size_t typesize_table[] = {1, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8, 4};

  size_t len = count * (typesize_table[(type) < 14 ? (type) : 0]);
  if (len > 4) {
    assert(data_out);
    if (!data_out) {
      return false;
    }

    // Store offset value.

    unsigned int offset =
        static_cast<unsigned int>(data_out->tellp()) + kHeaderSize;
    ifd.offset_or_value = offset;

    data_out->write(reinterpret_cast<const char *>(data),
                    static_cast<std::streamsize>(len));

  } else {
    ifd.offset_or_value = 0;

    // less than 4 bytes = store data itself.
    if (len == 1) {
      unsigned char value = *(data);
      memcpy(&(ifd.offset_or_value), &value, sizeof(unsigned char));
    } else if (len == 2) {
      unsigned short value = *(reinterpret_cast<const unsigned short *>(data));
      memcpy(&(ifd.offset_or_value), &value, sizeof(unsigned short));
    } else if (len == 4) {
      unsigned int value = *(reinterpret_cast<const unsigned int *>(data));
      ifd.offset_or_value = value;
    } else {
      assert(0);
    }
  }

  tags_out->push_back(ifd);

  return true;
}

static bool WriteTIFFVersionHeader(std::ostringstream *out, bool big_endian) {
  // TODO(syoyo): Support BigTIFF?

  // 4d 4d = Big endian. 49 49 = Little endian.
  if (big_endian) {
    Write1(0x4d, out);
    Write1(0x4d, out);
    Write1(0x0, out);
    Write1(0x2a, out);  // Tiff version ID
  } else {
    Write1(0x49, out);
    Write1(0x49, out);
    Write1(0x2a, out);  // Tiff version ID
    Write1(0x0, out);
  }

  return true;
}

DNGImage::DNGImage()
    : dng_big_endian_(true),
      num_fields_(0),
      samples_per_pixels_(0),
      data_strip_offset_{0},
      data_strip_bytes_{0} {
  swap_endian_ = (IsBigEndian() != dng_big_endian_);
}

void DNGImage::SetBigEndian(bool big_endian) {
  dng_big_endian_ = big_endian;
  swap_endian_ = (IsBigEndian() != dng_big_endian_);
}

bool DNGImage::SetSubfileType(bool reduced_image, bool page, bool mask) {
  unsigned int count = 1;

  unsigned int bits = 0;
  if (reduced_image) {
    bits |= FILETYPE_REDUCEDIMAGE;
  }
  if (page) {
    bits |= FILETYPE_PAGE;
  }
  if (mask) {
    bits |= FILETYPE_MASK;
  }

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_SUB_FILETYPE), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&bits), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageWidth(const unsigned int width) {
  unsigned int count = 1;

  unsigned int data = width;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_IMAGE_WIDTH), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageLength(const unsigned int length) {
  unsigned int count = 1;

  const unsigned int data = length;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_IMAGE_LENGTH), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetRowsPerStrip(const unsigned int rows) {
  if (rows == 0) {
    return false;
  }

  unsigned int count = 1;

  const unsigned int data = rows;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_ROWS_PER_STRIP), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetSamplesPerPixel(const unsigned short value) {
  if (value > 4) {
    {
      std::stringstream ss;
      ss << "Samples per pixel must be less than or equal to 4, but got " << value << ".\n";
      err_ += ss.str();
    }
    return false;
  }

  unsigned int count = 1;

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_SAMPLES_PER_PIXEL), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    err_ += "Failed to write `TIFFTAG_SAMPLES_PER_PIXEL` tag.\n";
    return false;
  }

  samples_per_pixels_ = value;  // Store SPP for later use.

  num_fields_++;
  return true;
}

bool DNGImage::SetBitsPerSample(const unsigned int num_samples,
                                const unsigned short *values) {
  // `SetSamplesPerPixel()` must be called in advance and SPP shoud be equal to
  // `num_samples`.

  if (samples_per_pixels_ == 0) {
    err_ += "SetSamplesPerPixel() must be called before SetBitsPerSample().\n";
    return false;
  }

  if ((num_samples == 0) || (num_samples > 4)) {
    std::stringstream ss;
    ss << "Invalid number of samples: " << num_samples << "\n";
    err_ += ss.str();
    return false;
  } else if (num_samples != samples_per_pixels_) {
    std::stringstream ss;
    ss << "Samples per pixel mismatch. " << num_samples << " is given for SetBitsPerSample(), but SamplesPerPixel is set to " << samples_per_pixels_ << "\n";
    err_ += ss.str();
    return false;
  } else {
    // ok
  }

  unsigned short bps = values[0];

  std::vector<unsigned short> vs(num_samples);
  for (size_t i = 0; i < vs.size(); i++) {
    // FIXME(syoyo): Currently bps must be same for all samples
    if (bps != values[i]) {
      err_ += "BitsPerSample must be same among samples at the moment.\n";
      return false;
    }

    vs[i] = values[i];

    // TODO(syoyo): Swap values when writing IFD tag, not here.
    if (swap_endian_) {
      swap2(&vs[i]);
    }
  }

  unsigned int count = num_samples;

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_BITS_PER_SAMPLE),
                          TIFF_SHORT, count,
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  // Store BPS for later use.
  bits_per_samples_.resize(num_samples);
  for (size_t i = 0; i < num_samples; i++) {
    bits_per_samples_[i] = values[i];
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetPhotometric(const unsigned short value) {
  if ((value == PHOTOMETRIC_LINEARRAW) ||
      (value == PHOTOMETRIC_CFA) ||
      (value == PHOTOMETRIC_RGB) ||
      (value == PHOTOMETRIC_WHITE_IS_ZERO) ||
      (value == PHOTOMETRIC_BLACK_IS_ZERO)) {
    // OK
  } else {
    return false;
  }

  unsigned int count = 1;

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_PHOTOMETRIC), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetPlanarConfig(const unsigned short value) {
  unsigned int count = 1;

  if ((value == PLANARCONFIG_CONTIG) || (value == PLANARCONFIG_SEPARATE)) {
    // OK
  } else {
    return false;
  }

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_PLANAR_CONFIG), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCompression(const unsigned short value) {
  unsigned int count = 1;

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_COMPRESSION), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetSampleFormat(const unsigned int num_samples,
                               const unsigned short *values) {
  // `SetSamplesPerPixel()` must be called in advance
  if ((num_samples > 0) && (num_samples == samples_per_pixels_)) {
    // OK
  } else {
    err_ += "SetSamplesPerPixel() must be called before SetSampleFormat().\n";
    return false;
  }

  unsigned short format = values[0];

  std::vector<unsigned short> vs(num_samples);
  for (size_t i = 0; i < vs.size(); i++) {
    // FIXME(syoyo): Currently format must be same for all samples
    if (format != values[i]) {
      err_ += "SampleFormat must be same among samples at the moment.\n";
      return false;
    }

    if ((format == SAMPLEFORMAT_UINT) || (format == SAMPLEFORMAT_INT) ||
        (format == SAMPLEFORMAT_IEEEFP)) {
      // OK
    } else {
      err_ += "Invalid format value specified for SetSampleFormat().\n";
      return false;
    }

    vs[i] = values[i];

    // TODO(syoyo): Swap values when writing IFD tag, not here.
    if (swap_endian_) {
      swap2(&vs[i]);
    }
  }

  unsigned int count = num_samples;

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_SAMPLEFORMAT),
                          TIFF_SHORT, count,
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetOrientation(const unsigned short value) {
  unsigned int count = 1;

  if ((value == ORIENTATION_TOPLEFT) || (value == ORIENTATION_TOPRIGHT) ||
      (value == ORIENTATION_BOTRIGHT) || (value == ORIENTATION_BOTLEFT) ||
      (value == ORIENTATION_LEFTTOP) || (value == ORIENTATION_RIGHTTOP) ||
      (value == ORIENTATION_RIGHTBOT) || (value == ORIENTATION_LEFTBOT)) {
    // OK
  } else {
    return false;
  }

  const unsigned int data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_ORIENTATION), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetBlackLevel(const unsigned int num_components,
                             const unsigned short *values) {
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_BLACK_LEVEL), TIFF_SHORT, num_components,
      reinterpret_cast<const unsigned char *>(values), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetBlackLevelRational(unsigned int num_samples,
                                     const double *values) {
  // `SetSamplesPerPixel()` must be called in advance and SPP shoud be equal to
  // `num_samples`.
  if ((num_samples > 0) && (num_samples == samples_per_pixels_)) {
    // OK
  } else {
    return false;
  }

  std::vector<unsigned int> vs(num_samples * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }

  unsigned int count = num_samples;

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_BLACK_LEVEL),
                          TIFF_RATIONAL, count,
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetWhiteLevelRational(unsigned int num_samples,
                                     const double *values) {
  // `SetSamplesPerPixel()` must be called in advance and SPP shoud be equal to
  // `num_samples`.
  if ((num_samples > 0) && (num_samples == samples_per_pixels_)) {
    // OK
  } else {
    return false;
  }

  std::vector<unsigned int> vs(num_samples * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }

  unsigned int count = num_samples;

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_WHITE_LEVEL),
                          TIFF_RATIONAL, count,
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetXResolution(const double value) {
  double numerator, denominator;
  if (DoubleToRational(value, &numerator, &denominator) != 0) {
    // Couldn't represent fp value as integer rational value.
    return false;
  }

  unsigned int data[2];
  data[0] = static_cast<unsigned int>(numerator);
  data[1] = static_cast<unsigned int>(denominator);

  // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
  if (swap_endian_) {
    swap4(&data[0]);
    swap4(&data[1]);
  }

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_XRESOLUTION), TIFF_RATIONAL, 1,
      reinterpret_cast<const unsigned char *>(data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetYResolution(const double value) {
  double numerator, denominator;
  if (DoubleToRational(value, &numerator, &denominator) != 0) {
    // Couldn't represent fp value as integer rational value.
    return false;
  }

  unsigned int data[2];
  data[0] = static_cast<unsigned int>(numerator);
  data[1] = static_cast<unsigned int>(denominator);

  // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
  if (swap_endian_) {
    swap4(&data[0]);
    swap4(&data[1]);
  }

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_YRESOLUTION), TIFF_RATIONAL, 1,
      reinterpret_cast<const unsigned char *>(data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetResolutionUnit(const unsigned short value) {
  unsigned int count = 1;

  if ((value == RESUNIT_NONE) || (value == RESUNIT_INCH) ||
      (value == RESUNIT_CENTIMETER)) {
    // OK
  } else {
    return false;
  }

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_RESOLUTION_UNIT), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageDescription(const std::string &ascii) {
  unsigned int count =
      static_cast<unsigned int>(ascii.length() + 1);  // +1 for '\0'

  if (count < 2) {
    // empty string
    return false;
  }

  if (count > (1024 * 1024)) {
    // too large
    return false;
  }

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_IMAGEDESCRIPTION),
                          TIFF_ASCII, count,
                          reinterpret_cast<const unsigned char *>(ascii.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetUniqueCameraModel(const std::string &ascii) {
  unsigned int count =
      static_cast<unsigned int>(ascii.length() + 1);  // +1 for '\0'

  if (count < 2) {
    // empty string
    return false;
  }

  if (count > (1024 * 1024)) {
    // too large
    return false;
  }

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_UNIQUE_CAMERA_MODEL),
                          TIFF_ASCII, count,
                          reinterpret_cast<const unsigned char *>(ascii.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetSoftware(const std::string &ascii) {
  unsigned int count =
      static_cast<unsigned int>(ascii.length() + 1);  // +1 for '\0'

  if (count < 2) {
    // empty string
    return false;
  }

  if (count > 4096) {
    // too large
    return false;
  }

  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_SOFTWARE),
                          TIFF_ASCII, count,
                          reinterpret_cast<const unsigned char *>(ascii.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}


bool DNGImage::SetActiveArea(const unsigned int values[4]) {
  unsigned int count = 4;

  const unsigned int *data = values;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_ACTIVE_AREA), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetDNGVersion(const unsigned char a,
                             const unsigned char b,
                             const unsigned char c,
                             const unsigned char d) {
  unsigned char data[4] = {a, b, c, d};

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_DNG_VERSION), TIFF_BYTE, 4,
      reinterpret_cast<const unsigned char *>(data),
      &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetColorMatrix1(const unsigned int plane_count,
                               const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * 3 * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_COLOR_MATRIX1),
                          TIFF_SRATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetColorMatrix2(const unsigned int plane_count,
                               const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * 3 * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_COLOR_MATRIX2),
                          TIFF_SRATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetForwardMatrix1(const unsigned int plane_count,
                                 const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * 3 * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_FORWARD_MATRIX1),
                          TIFF_SRATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetForwardMatrix2(const unsigned int plane_count,
                                 const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * 3 * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_FORWARD_MATRIX2),
                          TIFF_SRATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCameraCalibration1(const unsigned int plane_count,
                                     const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * plane_count * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_CAMERA_CALIBRATION1),
                          TIFF_SRATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCameraCalibration2(const unsigned int plane_count,
                                     const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * plane_count * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_CAMERA_CALIBRATION2),
                          TIFF_SRATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetAnalogBalance(const unsigned int plane_count,
                                const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_ANALOG_BALANCE),
                          TIFF_RATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCFARepeatPatternDim(const unsigned short width,
                                      const unsigned short height) {
  unsigned short data[2] = {width, height};

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_CFA_REPEAT_PATTERN_DIM), TIFF_SHORT, 2,
      reinterpret_cast<const unsigned char *>(data),
      &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetBlackLevelRepeatDim(const unsigned short width,
                                      const unsigned short height) {
  unsigned short data[2] = {width, height};

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_BLACK_LEVEL_REPEAT_DIM), TIFF_SHORT, 2,
      reinterpret_cast<const unsigned char *>(data),
      &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCalibrationIlluminant1(const unsigned short value) {
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_CALIBRATION_ILLUMINANT1), TIFF_SHORT, 1,
      reinterpret_cast<const unsigned char *>(&value),
      &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCalibrationIlluminant2(const unsigned short value) {
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_CALIBRATION_ILLUMINANT2), TIFF_SHORT, 1,
      reinterpret_cast<const unsigned char *>(&value),
      &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCFAPattern(const unsigned int num_components,
                             const unsigned char *values) {
  if ((values == NULL) || (num_components < 1)) {
    return false;
  }

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_CFA_PATTERN), TIFF_BYTE, num_components,
      reinterpret_cast<const unsigned char *>(values),
      &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetAsShotNeutral(const unsigned int plane_count,
                                const double *matrix_values) {
  std::vector<unsigned int> vs(plane_count * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(matrix_values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_AS_SHOT_NEUTRAL),
                          TIFF_RATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetAsShotWhiteXY(const double x, const double y) {
  const double values[2] = {x, y};
  std::vector<unsigned int> vs(2 * 2);
  for (size_t i = 0; i * 2 < vs.size(); i++) {
    double numerator, denominator;
    if (DoubleToRational(values[i], &numerator, &denominator) != 0) {
      // Couldn't represent fp value as integer rational value.
      return false;
    }

    vs[2 * i + 0] = static_cast<unsigned int>(numerator);
    vs[2 * i + 1] = static_cast<unsigned int>(denominator);

    // TODO(syoyo): Swap rational value(8 bytes) when writing IFD tag, not here.
    if (swap_endian_) {
      swap4(&vs[2 * i + 0]);
      swap4(&vs[2 * i + 1]);
    }
  }
  bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_AS_SHOT_WHITE_XY),
                          TIFF_RATIONAL, uint32_t(vs.size() / 2),
                          reinterpret_cast<const unsigned char *>(vs.data()),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageDataPacked(const unsigned short *input_buffer, const int input_count, const unsigned int input_bpp, bool big_endian)
{

#ifndef ROL32
#define ROL32(v,a) ((v) << (a) | (v) >> (32-(a)))
#endif

#ifndef ROL16
#define ROL16(v,a) ((v) << (a) | (v) >> (16-(a)))
#endif

  if (input_count <= 0) {
    return false;
  }

  if (input_bpp > 16)
    return false;

  unsigned int bits_free = 16 - input_bpp;
  const unsigned short *unpacked_bits = input_buffer;

  std::vector<unsigned short> output(static_cast<size_t>(input_count));
  unsigned short *packed_bits = output.data();

  packed_bits[0] = static_cast<unsigned short>(unpacked_bits[0] << bits_free);
  for (unsigned int pixel_index = 1; pixel_index < static_cast<unsigned int>(input_count); pixel_index++)
  {
    unsigned int bits_offset = (pixel_index * bits_free) % 16;
    unsigned int bits_to_rol = bits_free + bits_offset + (bits_offset > 0) * 16;

    unsigned int data = ROL32(static_cast<unsigned int>(unpacked_bits[pixel_index]), bits_to_rol);
    *(reinterpret_cast<unsigned int *>(packed_bits)) = (*(reinterpret_cast<unsigned int *>(packed_bits)) & 0x0000FFFF) | data;

    if(bits_offset > 0 && bits_offset <= input_bpp)
    {
      if(big_endian)
        *(reinterpret_cast<unsigned short *>(packed_bits)) = static_cast<unsigned short>(ROL16(*(reinterpret_cast<unsigned short *>(packed_bits)), 8));

      ++packed_bits;
    }
  }

  return SetImageData(reinterpret_cast<unsigned char*>(output.data()), output.size() * sizeof(unsigned short));

#undef ROL32
#undef ROL16
}

bool DNGImage::SetImageData(const unsigned char *data, const size_t data_len) {
  if ((data == NULL) || (data_len < 1)) {
    return false;
  }

  data_strip_offset_ = size_t(data_os_.tellp());
  data_strip_bytes_ = data_len;

  data_os_.write(reinterpret_cast<const char *>(data),
                 static_cast<std::streamsize>(data_len));

  // NOTE: STRIP_OFFSET tag will be written at `WriteIFDToStream()`.

  {
    unsigned int count = 1;
    unsigned int bytes = static_cast<unsigned int>(data_len);

    bool ret = WriteTIFFTag(
        static_cast<unsigned short>(TIFFTAG_STRIP_BYTE_COUNTS), TIFF_LONG,
        count, reinterpret_cast<const unsigned char *>(&bytes), &ifd_tags_,
        NULL);

    if (!ret) {
      return false;
    }

    num_fields_++;
  }

  return true;
}

bool DNGImage::SetImageDataJpeg(const unsigned short *data, unsigned int width,
                                unsigned int height, unsigned int bpp) {
  if ((data == NULL) || (height % 2 == 1) || (width % 2 == 1)) {
    return false;
  }

  uint8_t *compressed = NULL;
  int output_buffer_size = 0;

  // Width x2 to move each second line
  // -----------
  // Before:
  //
  // GRGRGR...
  // BGBGBG...
  // GRGRGR...
  // BGBGBG...
  // -----------
  // After:
  //
  // GRGRGR...BGBGBG...
  // GRGRGR...BGBGBG...
  // -----------
  int new_width = int(width * 2);
  int new_height = int(height / 2);

  // Encode image
  int ret = detail::lj92_encode(const_cast<unsigned short *>(data), new_width, new_height, int(bpp),
                        new_width * new_height, 0, NULL, 0, &compressed,
                        &output_buffer_size);

  if (ret != detail::LJ92_ERROR_NONE)
	  return false;

  bool sid_res = SetImageData(compressed, size_t(output_buffer_size));

  if (compressed)
	  free(compressed);

  return sid_res;
}

bool DNGImage::SetCustomFieldLong(const unsigned short tag, const int value) {
  unsigned int count = 1;

  // TODO(syoyo): Check if `tag` value does not conflict with existing TIFF tag
  // value.

  bool ret = WriteTIFFTag(tag, TIFF_SLONG, count,
                          reinterpret_cast<const unsigned char *>(&value),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCustomFieldULong(const unsigned short tag,
                                   const unsigned int value) {
  unsigned int count = 1;

  // TODO(syoyo): Check if `tag` value does not conflict with existing TIFF tag
  // value.

  bool ret = WriteTIFFTag(tag, TIFF_LONG, count,
                          reinterpret_cast<const unsigned char *>(&value),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

static bool IFDComparator(const IFDTag &a, const IFDTag &b) {
  return (a.tag < b.tag);
}

bool DNGImage::WriteDataToStream(std::ostream *ofs) const {
  if ((data_os_.str().length() == 0)) {
    err_ += "Empty IFD data and image data.\n";
    return false;
  }

  if (bits_per_samples_.empty()) {
    err_ += "BitsPerSample is not set\n";
    return false;
  }

  for (size_t i = 0; i < bits_per_samples_.size(); i++) {
    if (bits_per_samples_[i] == 0) {
      err_ += std::to_string(i) + "'th BitsPerSample is zero";
      return false;
    }
  }

  if (samples_per_pixels_ == 0) {
    err_ += "SamplesPerPixels is not set or zero.";
    return false;
  }

  std::vector<uint8_t> data(data_os_.str().length());
  memcpy(data.data(), data_os_.str().data(), data.size());

  if (data_strip_bytes_ == 0) {
    // May ok?.
  } else {
    // FIXME(syoyo): Assume all channels use sample bps
    uint32_t bps = bits_per_samples_[0];

    // We may need to swap endian for pixel data.
    if (swap_endian_) {
      if (bps == 16) {
        size_t n = data_strip_bytes_ / sizeof(uint16_t);
        uint16_t *ptr =
            reinterpret_cast<uint16_t *>(data.data() + data_strip_offset_);

        for (size_t i = 0; i < n; i++) {
          swap2(&ptr[i]);
        }

      } else if (bps == 32) {
        size_t n = data_strip_bytes_ / sizeof(uint32_t);
        uint32_t *ptr =
            reinterpret_cast<uint32_t *>(data.data() + data_strip_offset_);

        for (size_t i = 0; i < n; i++) {
          swap4(&ptr[i]);
        }

      } else if (bps == 64) {
        size_t n = data_strip_bytes_ / sizeof(uint64_t);
        uint64_t *ptr =
            reinterpret_cast<uint64_t *>(data.data() + data_strip_offset_);

        for (size_t i = 0; i < n; i++) {
          swap8(&ptr[i]);
        }
      }
    }
  }

  ofs->write(reinterpret_cast<const char *>(data.data()),
             static_cast<std::streamsize>(data.size()));

  return true;
}

bool DNGImage::WriteIFDToStream(const unsigned int data_base_offset,
                                const unsigned int strip_offset,
                                std::ostream *ofs) const {
  if ((num_fields_ == 0) || (ifd_tags_.size() < 1)) {
    err_ += "No TIFF Tags.\n";
    return false;
  }

  // add STRIP_OFFSET tag and sort IFD tags.
  std::vector<IFDTag> tags = ifd_tags_;
  {
    // For STRIP_OFFSET we need the actual offset value to data(image),
    // thus write STRIP_OFFSET here.
    unsigned int offset = strip_offset + kHeaderSize;
    IFDTag ifd;
    ifd.tag = TIFFTAG_STRIP_OFFSET;
    ifd.type = TIFF_LONG;
    ifd.count = 1;
    ifd.offset_or_value = offset;
    tags.push_back(ifd);
  }

  // TIFF expects IFD tags are sorted.
  std::sort(tags.begin(), tags.end(), IFDComparator);

  std::ostringstream ifd_os;

  unsigned short num_fields = static_cast<unsigned short>(tags.size());

  Write2(num_fields, &ifd_os, swap_endian_);

  {
    size_t typesize_table[] = {1, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8, 4};

    for (size_t i = 0; i < tags.size(); i++) {
      const IFDTag &ifd = tags[i];
      Write2(ifd.tag, &ifd_os, swap_endian_);
      Write2(ifd.type, &ifd_os, swap_endian_);
      Write4(ifd.count, &ifd_os, swap_endian_);

      size_t len =
          ifd.count * (typesize_table[(ifd.type) < 14 ? (ifd.type) : 0]);
      if (len > 4) {
        // Store offset value.
        unsigned int ifd_offt = ifd.offset_or_value + data_base_offset;
        Write4(ifd_offt, &ifd_os, swap_endian_);
      } else {
        // less than 4 bytes = store data itself.

        if (len == 1) {
          const unsigned char value =
              *(reinterpret_cast<const unsigned char *>(&ifd.offset_or_value));
          Write1(value, &ifd_os);
          unsigned char pad = 0;
          Write1(pad, &ifd_os);
          Write1(pad, &ifd_os);
          Write1(pad, &ifd_os);
        } else if (len == 2) {
          const unsigned short value =
              *(reinterpret_cast<const unsigned short *>(&ifd.offset_or_value));
          Write2(value, &ifd_os, swap_endian_);
          const unsigned short pad = 0;
          Write2(pad, &ifd_os, swap_endian_);
        } else if (len == 4) {
          const unsigned int value =
              *(reinterpret_cast<const unsigned int *>(&ifd.offset_or_value));
          Write4(value, &ifd_os, swap_endian_);
        } else {
          assert(0);
        }
      }
    }

    ofs->write(ifd_os.str().c_str(),
               static_cast<std::streamsize>(ifd_os.str().length()));
  }

  return true;
}

// -------------------------------------------

DNGWriter::DNGWriter(bool big_endian) : dng_big_endian_(big_endian) {
  swap_endian_ = (IsBigEndian() != dng_big_endian_);
}

bool DNGWriter::WriteToFile(const char *filename, std::string *err) const {
  std::ofstream ofs(filename, std::ostream::binary);

  if (!ofs) {
    if (err) {
      (*err) = "Failed to open file.\n";
    }

    return false;
  }

  std::ostringstream header;
  bool ret = WriteTIFFVersionHeader(&header, dng_big_endian_);
  if (!ret) {
    if (err) {
      (*err) = "Failed to write TIFF version header.\n";
    }
    return false;
  }

  if (images_.size() == 0) {
    if (err) {
      (*err) = "No image added for writing.\n";
    }

    return false;
  }

  // 1. Compute offset and data size(exclude TIFF header bytes)
  size_t data_len = 0;
  size_t strip_offset = 0;
  std::vector<size_t> data_offset_table;
  std::vector<size_t> strip_offset_table;
  for (size_t i = 0; i < images_.size(); i++) {
    strip_offset = data_len + images_[i]->GetStripOffset();
    data_offset_table.push_back(data_len);
    strip_offset_table.push_back(strip_offset);
    data_len += images_[i]->GetDataSize();
  }

  // 2. Write offset to ifd table.
  const unsigned int ifd_offset =
      kHeaderSize + static_cast<unsigned int>(data_len);
  Write4(ifd_offset, &header, swap_endian_);

  assert(header.str().length() == 8);

  // std::cout << "ifd_offset " << ifd_offset << std::endl;
  // std::cout << "data_len " << data_os_.str().length() << std::endl;
  // std::cout << "ifd_len " << ifd_os_.str().length() << std::endl;
  // std::cout << "swap endian " << swap_endian_ << std::endl;

  // 3. Write header
  ofs.write(header.str().c_str(),
            static_cast<std::streamsize>(header.str().length()));

  // 4. Write image and meta data
  // TODO(syoyo): Write IFD first, then image/meta data
  for (size_t i = 0; i < images_.size(); i++) {
    bool ok = images_[i]->WriteDataToStream(&ofs);
    if (!ok) {
      if (err) {
        std::stringstream ss;
        ss << "Failed to write data at image[" << i << "]. err = " << images_[i]->Error() << "\n";
        (*err) += ss.str();
      }
      return false;
    }
  }

  // 5. Write IFD entries;
  for (size_t i = 0; i < images_.size(); i++) {
    bool ok = images_[i]->WriteIFDToStream(
        static_cast<unsigned int>(data_offset_table[i]),
        static_cast<unsigned int>(strip_offset_table[i]), &ofs);
    if (!ok) {
      if (err) {
        std::stringstream ss;
        ss << "Failed to write IFD at image[" << i << "]. err = " << images_[i]->Error() << "\n";
        (*err) += ss.str();
      }
      return false;
    }

    unsigned int next_ifd_offset =
        static_cast<unsigned int>(ofs.tellp()) + sizeof(unsigned int);

    if (i == (images_.size() - 1)) {
      // Write zero as IFD offset(= end of data)
      next_ifd_offset = 0;
    }

    if (swap_endian_) {
      swap4(&next_ifd_offset);
    }

    ofs.write(reinterpret_cast<const char *>(&next_ifd_offset), 4);
  }

  return true;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace tinydngwriter

#endif  // TINY_DNG_WRITER_IMPLEMENTATION
