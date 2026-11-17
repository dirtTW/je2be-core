#pragma once

#include <filesystem>
#include <string>

#include <mcfile/dimension.hpp>

namespace je2be::bedrock {

char8_t const *const kVersionString = u8"26.3";
constexpr int kLevelVersion = 19133;

inline std::filesystem::path JavaDimensionDirectory(std::filesystem::path const &root, mcfile::Dimension dim) {
  std::string name = "overworld";
  switch (dim) {
  case mcfile::Dimension::Nether:
    name = "the_nether";
    break;
  case mcfile::Dimension::End:
    name = "the_end";
    break;
  default:
    break;
  }
  return root / "dimensions" / "minecraft" / name;
}

inline std::filesystem::path JavaMinecraftDataDirectory(std::filesystem::path const &root) {
  return root / "data" / "minecraft";
}

inline std::filesystem::path JavaMapsDirectory(std::filesystem::path const &root) {
  return JavaMinecraftDataDirectory(root) / "maps";
}

inline std::filesystem::path JavaPlayersDataDirectory(std::filesystem::path const &root) {
  return root / "players" / "data";
}

} // namespace je2be::bedrock
