#pragma once

#include "enums/_chunk-conversion-mode.hpp"

namespace je2be {

// The Java Edition version a converted world is written as. The writer emits the
// 26.1 directory layout (dimensions/minecraft/<dimension>, players/data and
// data/minecraft) and the 26.3 block palette keys (id/properties), so this can
// track the newest supported release.
constexpr int kJavaDataVersion = 5023;          // 26.3
constexpr int kJavaDataVersionMaxLegacy = 2730; // 1.17.1

constexpr int kJavaDataVersionComponentIntroduced = 3819; // 24w09a

struct DataVersion {
  int32_t const fSource;
  int32_t const fTarget;

  static int32_t TargetVersionByMode(ChunkConversionMode mode) {
    switch (mode) {
    case ChunkConversionMode::Legacy:
      return kJavaDataVersionMaxLegacy;
    case ChunkConversionMode::CavesAndCliffs2:
    default:
      return kJavaDataVersion;
    }
  }

  DataVersion(int32_t source, ChunkConversionMode mode) : fSource(source), fTarget(TargetVersionByMode(mode)) {}
  DataVersion(int32_t source, int32_t target) : fSource(source), fTarget(target) {}
};

} // namespace je2be
