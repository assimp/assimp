// Copyright 2018 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "draco/io/texture_io.h"

#ifdef DRACO_TRANSCODER_SUPPORTED

#include "draco/io/file_utils.h"

namespace draco {

namespace {

StatusOr<std::unique_ptr<Texture>> CreateDracoTextureInternal(
    const std::vector<uint8_t> &image_data, SourceImage *out_source_image) {
  std::unique_ptr<Texture> draco_texture(new Texture());
  out_source_image->MutableEncodedData() = image_data;
  return std::move(draco_texture);
}

}  // namespace

StatusOr<std::unique_ptr<Texture>> ReadTextureFromFile(
    const std::string &file_name) {
  std::vector<uint8_t> image_data;
  if (!ReadFileToBuffer(file_name, &image_data)) {
    return Status(Status::IO_ERROR, "Unable to read input texture file.");
  }

  SourceImage source_image;
  DRACO_ASSIGN_OR_RETURN(auto texture,
                         CreateDracoTextureInternal(image_data, &source_image));
  source_image.set_filename(file_name);
  const std::string extension = LowercaseFileExtension(file_name);
  const std::string mime_type =
      "image/" + (extension == "jpg" ? "jpeg" : extension);
  source_image.set_mime_type(mime_type);
  texture->set_source_image(source_image);
  return texture;
}

StatusOr<std::unique_ptr<Texture>> ReadTextureFromBuffer(
    const uint8_t *buffer, size_t buffer_size, const std::string &mime_type) {
  SourceImage source_image;
  std::vector<uint8_t> image_data(buffer, buffer + buffer_size);
  DRACO_ASSIGN_OR_RETURN(auto texture,
                         CreateDracoTextureInternal(image_data, &source_image));
  source_image.set_mime_type(mime_type);
  texture->set_source_image(source_image);
  return texture;
}

Status WriteTextureToFile(const std::string &file_name,
                          const Texture &texture) {
  std::vector<uint8_t> buffer;
  DRACO_RETURN_IF_ERROR(WriteTextureToBuffer(texture, &buffer));

  if (!WriteBufferToFile(buffer.data(), buffer.size(), file_name)) {
    return Status(Status::DRACO_ERROR, "Failed to write image.");
  }

  return OkStatus();
}

Status WriteTextureToBuffer(const Texture &texture,
                            std::vector<uint8_t> *buffer) {
  // Copy data from the encoded source image if possible, otherwise load the
  // data from the source file.
  if (!texture.source_image().encoded_data().empty()) {
    *buffer = texture.source_image().encoded_data();
  } else if (!texture.source_image().filename().empty()) {
    if (!ReadFileToBuffer(texture.source_image().filename(), buffer)) {
      return Status(Status::IO_ERROR, "Unable to read input texture file.");
    }
  } else {
    return Status(Status::DRACO_ERROR, "Invalid source data for the texture.");
  }
  return OkStatus();
}

}  // namespace draco

#endif  // DRACO_TRANSCODER_SUPPORTED
