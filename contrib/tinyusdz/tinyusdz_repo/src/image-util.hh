// SPDX-License-Identifier: Apache 2.0
// Copyright 2023-Present, Light Transport Entertainment Inc.
//
// Image utilities.
// Currently sRGB color space conversion feature is provided.
//
// TODO
// - [ ] Image resize using stb_image_resize2
// - [ ] OIIO 3D LUT support through tinycolorio
//
#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace tinyusdz {

// forward decl
namespace value {
struct half;
};

///
/// [0, 255] => [0.0, 1.0]
///
bool u8_to_f32_image(const std::vector<uint8_t> &in_img,
    size_t width, size_t height, size_t channels, std::vector<float> *out_img);

///
/// Apply x' =  `scale_factor * x + bias`
/// Then u8 value is calculated as `255 * max(0.0, min(1.0, x'))`
///
bool f32_to_u8_image(const std::vector<float> &in_img,
    size_t width, size_t height, size_t channels, std::vector<uint8_t> *out_img,
    float scale_factor=1.0f,
    float bias=0.0f);

///
/// Convert fp32 image in linear space to 8bit image in sRGB color space.
///
/// @param[in] in_image Input image in linear color space. Image size =
/// [width_byte_stride/sizeof(float), height, channel_stride]
/// @param[in] width Width pixels
/// @param[in] height Height pixels
/// @param[in] chanels Pixel channels to apply conversion. must be less than or
/// equal to `channel_stride`
/// @param[in] chanel_stride channel stride. For example, channels=3 and
/// channel_stride=4 to convert RGB channel to sRGB but leave alpha channel
/// linear for RGBA image.
/// @param[out] out_image Image in sRGB colorspace. Image size is same with
/// `in_image`
///
/// @return true upon success. false when any parameter is invalid.
bool linear_f32_to_srgb_8bit(const std::vector<float> &in_img, size_t width,
                         size_t height, size_t channels, size_t channel_stride,
                         std::vector<uint8_t> *out_img);

///
/// Convert 8bit image in sRGB to fp32 image in linear sRGB color space.
///
/// @param[in] in_image Input image in sRGB color space. Image size =
/// [width_byte_stride, height, channel_stride]
/// @param[in] width Width pixels
/// @param[in] height Height pixels
/// @param[in] chanels Pixel channels to apply conversion. must be less than or
/// equal to `channel_stride`
/// @param[in] chanel_stride channel stride. For example, channels=3 and
/// channel_stride=4 to apply inverse sRGB convertion to RGB channel but apply
/// linear conversion to alpha channel for RGBA image.
/// @param[out] out_image Image in linear color space. Image size is same with
/// `in_image`
///
/// @return true upon success. false when any parameter is invalid.
bool srgb_8bit_to_linear_f32(const std::vector<uint8_t> &in_img, size_t width,
                         size_t height, size_t channels, size_t channel_stride,
                         std::vector<float> *out_img);

bool srgb_8bit_to_linear_8bit(const std::vector<uint8_t> &in_img, size_t width,
                         size_t height, size_t channels, size_t channel_stride,
                         std::vector<uint8_t> *out_img);

// Input texel value is transformed as: x' = in_img * scale_factor + bias for RGB
// alpha' = in_img * alpha_scale_factor + alpha_bias for alpha channel.
bool srgb_f32_to_linear_f32(const std::vector<float> &in_img, size_t width,
                         size_t height, size_t channels, size_t channel_stride,
                         std::vector<float> *out_img, const float scale_factor = 1.0f, const float bias = 0.0f, const float alpha_scale_factor = 1.0f, const float alpha_bias = 0.0f);

///
/// Convert 8bit image in Rec.709 to fp32 image in linear color space.
///
/// @param[in] in_img Input image in Rec.709 color space. Image size =
/// [width_byte_stride, height, channel_stride]
/// @param[in] width Width pixels
/// @param[in] width_byte_stride Width byte stride. 0 = Use `width` *
/// channel_stride
/// @param[in] height Height pixels
/// @param[in] chanels Pixel channels to apply conversion. must be less than or
/// equal to `channel_stride`
/// @param[in] chanel_stride channel stride. For example, channels=3 and
/// channel_stride=4 to apply inverse Rec.709 convertion to RGB channel but
/// apply linear conversion to alpha channel for RGBA image.
/// @param[out] out_image Image in linear color space. Image size is same with
/// `in_image`
///
/// @return true upon success. false when any parameter is invalid.
bool rec709_8bit_to_linear_f32(const std::vector<uint8_t> &in_img, size_t width,
                         size_t width_byte_stride, size_t height,
                         size_t channels, size_t channel_stride,
                         std::vector<float> *out_img);

///
/// Convert fp16 image in Display P3(P3-D65) to fp32 image in linear Display P3 color space.
/// 
/// The conversion is identical to sRGB -> linear sRGB, since Display P3 uses same gamma curve(transfer function) with sRGB.
///
/// Input value is scaled by x' = x * scale + bias.
///
/// @param[in] in_img Input image in Display P3 color space. Image size =
/// [width_byte_stride, height, channel_stride]
/// @param[in] width Width pixels
/// @param[in] height Height pixels
/// @param[in] chanels Pixel channels to apply conversion. must be less than or
/// equal to `channel_stride`
/// @param[in] chanel_stride channel stride. For example, channels=3 and
/// channel_stride=4 to apply inverse gamma correction to RGB channel but apply
/// linear conversion to alpha channel for RGBA image.
/// @param[out] out_img Image in linear Display P3 color space. Image size is same with
/// `in_image`
/// @param[in] scale texel scale factor(RGB)
/// @param[in] bias texel bias(RGB)
/// @param[in] alpha_scale texel scale factor(alpha)
/// @param[in] alpha_bias texel bias(alpha)
///
/// @return true upon success. false when any parameter is invalid.
bool displayp3_f16_to_linear_f32(const std::vector<value::half> &in_img, size_t width,
                         size_t height, size_t channels, size_t channel_stride,
                         std::vector<float> *out_img, const float scale_factor = 1.0f, const float bias = 0.0f, const float alpha_scale_factor = 1.0f, const float alpha_bias = 0.0f);

///
/// Convert fp32 image in linear Display P3 color space to 10bit Display P3(10 bit for RGB, 2 bit for alpha, 32bit in total)
/// Apply gamma curve + quantize values.
///
/// Input image must be Mono, LA(Luminance + Alpha), RGB or RGBA.
///
/// Monochrome image is converted to RGB
///
/// When alpha channel is supplied, alpha value in [0.0, 1.0] is quantized to 2bit(~0.25 = 0, ~0.5 = 1, ~0.75 = 2, 0.75+ = 3)
///
/// @param[in] in_img Input image in linear Display P3 color space. 
/// @param[in] width Width pixels
/// @param[in] height Height pixels
/// @param[in] channels 1 = mono, 2 = luminance(mono) + alpha, 3 = RGB, 4 = RGBA
/// @param[out] out_img Image in linear Display P3 color space. Image size is same with
/// `in_image`
bool linear_f32_to_displayp3_u10(const std::vector<float> &in_img, size_t width,
                         size_t height, size_t channels,
                         std::vector<uint32_t> *out_img);

///
/// Convert linear Display P3 color space to linear sRGB color space.
///
/// Input image must be RGB or RGBA.
/// (TODO: Support Mono, Lumi/Alpha)
///
/// When alpha channel is supplied, no conversion applied to alpha value.
///
/// @param[in] in_img Input image in linear Display P3 color space. 
/// @param[in] width Width pixels
/// @param[in] height Height pixels
/// @param[in] channels 3 = RGB, 4 = RGBA
/// @param[out] out_img Image in linear sRGB color space. Image size is same with
/// `in_image`
bool linear_displayp3_to_linear_sRGB(const std::vector<float> &in_img, size_t width,
                         size_t height, size_t channels,
                         std::vector<float> *out_img);

///
/// Convert linear sRGB color space to linear Display P3 color space.
///
/// Input image must be RGB or RGBA.
/// (TODO: Support Mono, Lumi/Alpha)
///
/// When alpha channel is supplied, no conversion applied to alpha value.
///
/// @param[in] in_img Input image in linear sRGB color space. 
/// @param[in] width Width pixels
/// @param[in] height Height pixels
/// @param[in] channels 3 = RGB, 4 = RGBA
/// @param[out] out_img Image in linear Display P3 color space. Image size is same with
/// `in_image`
bool linear_sRGB_to_linear_displayp3(const std::vector<float> &in_img, size_t width,
                         size_t height, size_t channels,
                         std::vector<float> *out_img);

///
/// Resize fp32 image in linear color space.
///
/// @param[in] in_image Input image in linear color space.
/// @param[in] src_width Source image width pixels
/// @param[in] src_width_byte_stride Source image width byte stride. 0 = Use
/// `src_width` * channels
/// @param[in] src_height Source image height pixels
/// @param[in] dest_width Dest image width pixels
/// @param[in] dest_width_byte_stride Dest image width byte stride. 0 = Use
/// `src_width` * channels
/// @param[in] src_height Dest image height pixels
//
/// @param[in] chanels Pixel channels both src and dest image.
/// @param[out] dest_img Resized image in linear color space(memory is allocated
/// inside this function).
///
/// @return true upon success. false when any parameter is invalid.
bool resize_image_f32(const std::vector<float> &src_img, size_t src_width,
                      size_t src_width_byte_stride, size_t src_height,

                      size_t dest_width, size_t dest_width_byte_stride,
                      size_t dest_height,

                      size_t channels, std::vector<float> *dest_img);

///
/// Resize uint8 image in sRGB color space.
///
/// @param[in] in_image Input image in sRGB color space.
/// @param[in] src_width Source image width pixels
/// @param[in] src_width_byte_stride Source image width byte stride. 0 = Use
/// `src_width` * channels
/// @param[in] src_height Source image height pixels
/// @param[in] dest_width Dest image width pixels
/// @param[in] dest_width_byte_stride Dest image width byte stride. 0 = Use
/// `src_width` * channels
/// @param[in] src_height Dest image height pixels
//
/// @param[in] chanels Pixel channels both src and dest image.
/// @param[out] dest_img Resized image in sRGB color space(memory is allocated
/// inside this function).
///
/// @return true upon success. false when any parameter is invalid.
bool resize_image_u8_srgb(const std::vector<uint8_t> &src_img, size_t src_width,
                          size_t src_width_byte_stride, size_t src_height,

                          size_t dest_width, size_t dest_width_byte_stride,
                          size_t dest_height,

                          size_t channels, std::vector<uint8_t> *dest_img);

}  // namespace tinyusdz
