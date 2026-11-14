#pragma once

#include <je2be/enums/level-directory-structure.hpp>
#include <je2be/pos2.hpp>

namespace je2be::java {

class Options {
public:
  LevelDirectoryStructure fLevelDirectoryStructure = LevelDirectoryStructure::Vanilla;
  std::unordered_set<mcfile::Dimension> fDimensionFilter;
  std::unordered_set<Pos2i, Pos2iHasher> fChunkFilter;
  std::optional<std::filesystem::path> fTempDirectory;
  std::optional<std::filesystem::path> fDbTempDirectory;

  /// @brief Folder name a dimension uses once it lives under dimensions/<namespace>/.
  ///
  /// Minecraft 26.1 moved every dimension out of the world root and named the folders
  /// after the dimension ("overworld", "the_nether", "the_end") instead of the old
  /// "DIM-1"/"DIM1" scheme.
  static std::filesystem::path ModernDimensionName(mcfile::Dimension dim) {
    using namespace mcfile;
    switch (dim) {
    case Dimension::Nether:
      return "the_nether";
    case Dimension::End:
      return "the_end";
    case Dimension::Overworld:
    default:
      return "overworld";
    }
  }

  std::filesystem::path getWorldDirectory(std::filesystem::path const &root, mcfile::Dimension dim) const {
    using namespace mcfile;
    namespace fs = std::filesystem;
    switch (fLevelDirectoryStructure) {
    case LevelDirectoryStructure::Paper: {
      switch (dim) {
      case Dimension::Nether:
        return root / "world_nether" / "DIM-1";
      case Dimension::End:
        return root / "world_the_end" / "DIM1";
      case Dimension::Overworld:
      default:
        return root / "world";
      }
      break;
    }
    case LevelDirectoryStructure::Vanilla:
    default: {
      // 26.1 keeps each dimension's region/entities/poi under dimensions/minecraft/<name>.
      // Fall back to the pre-26.1 layout when that folder is absent, so both world
      // versions convert with the same build.
      std::error_code ec;
      fs::path modern = root / "dimensions" / "minecraft" / ModernDimensionName(dim);
      if (fs::is_directory(modern, ec)) {
        return modern;
      }
      switch (dim) {
      case Dimension::Nether:
        return root / "DIM-1";
      case Dimension::End:
        return root / "DIM1";
      case Dimension::Overworld:
      default:
        return root;
      }
      break;
    }
    }
  }

  std::filesystem::path getDataDirectory(std::filesystem::path const &root) const {
    std::filesystem::path base;
    switch (fLevelDirectoryStructure) {
    case LevelDirectoryStructure::Paper:
      base = root / "world";
      break;
    case LevelDirectoryStructure::Vanilla:
    default:
      base = root;
      break;
    }
    // 26.1 namespaces the data folder: data/scoreboard.dat became data/minecraft/scoreboard.dat.
    std::error_code ec;
    auto namespaced = base / "data" / "minecraft";
    if (std::filesystem::is_directory(namespaced, ec)) {
      return namespaced;
    }
    return base / "data";
  }

  /// @brief Directory holding the map_<id>.dat files.
  ///
  /// 26.1 moved maps into a "maps" subfolder of the data directory and renamed the files
  /// from "map_<id>.dat" to "<id>.dat".
  std::filesystem::path getMapDirectory(std::filesystem::path const &root) const {
    auto dataDir = getDataDirectory(root);
    std::error_code ec;
    auto maps = dataDir / "maps";
    if (std::filesystem::is_directory(maps, ec)) {
      return maps;
    }
    return dataDir;
  }

  std::filesystem::path getMapFilePath(std::filesystem::path const &root, int32_t mapId) const {
    auto dir = getMapDirectory(root);
    if (dir.filename() == "maps") {
      return dir / (std::to_string(mapId) + ".dat");
    }
    return dir / ("map_" + std::to_string(mapId) + ".dat");
  }

  /// @brief Directory holding the player data files.
  ///
  /// 26.1 moved player storage into a "players" subfolder: playerdata became
  /// players/data, advancements became players/advancements and stats became
  /// players/stats.
  std::filesystem::path getPlayersDataDirectory(std::filesystem::path const &root) const {
    std::filesystem::path base;
    switch (fLevelDirectoryStructure) {
    case LevelDirectoryStructure::Paper:
      base = root / "world";
      break;
    case LevelDirectoryStructure::Vanilla:
    default:
      base = root;
      break;
    }
    std::error_code ec;
    auto modern = base / "players" / "data";
    if (std::filesystem::is_directory(modern, ec)) {
      return modern;
    }
    return base / "playerdata";
  }

  std::filesystem::path getLevelDatFilePath(std::filesystem::path const &root) const {
    switch (fLevelDirectoryStructure) {
    case LevelDirectoryStructure::Paper:
      return root / "world" / "level.dat";
    case LevelDirectoryStructure::Vanilla:
    default:
      return root / "level.dat";
    }
  }

  std::filesystem::path getTempDirectory() const {
    return fTempDirectory ? *fTempDirectory : std::filesystem::temp_directory_path();
  }
};

} // namespace je2be::java
