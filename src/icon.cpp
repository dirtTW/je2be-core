#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "_world-icon.hpp"

namespace je2be {

bool ConvertWorldIcon(std::filesystem::path const &javaWorld, std::filesystem::path const &bedrockWorld) {
  namespace fs = std::filesystem;
  fs::path src = javaWorld / "icon.png";
  std::error_code ec;
  if (!fs::is_regular_file(src, ec) || ec) {
    return false;
  }
  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char *pixels = stbi_load(src.string().c_str(), &width, &height, &channels, 3);
  if (!pixels) {
    return false;
  }
  fs::path dst = bedrockWorld / "world_icon.jpeg";
  bool ok = stbi_write_jpg(dst.string().c_str(), width, height, 3, pixels, 90) != 0;
  stbi_image_free(pixels);
  return ok;
}

} // namespace je2be
