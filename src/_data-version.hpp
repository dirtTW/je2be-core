#pragma once

#include "enums/_chunk-conversion-mode.hpp"

namespace je2be {

// The Java Edition version a converted world is written as.
//
// This is deliberately *not* the newest version the converter can read.
// JavaDataVersions below goes up to 26.3, and the reader handles the 26.x world
// format (see Options::getWorldDirectory and cmake/patches/README.md), so a 26.x
// world can be converted. The writer, however, still produces the pre-26.1 world
// layout (region/entities/poi in the world root, plus DIM-1 and DIM1; see
// bedrock-converter.cpp) and the pre-26.3 block palette keys (Name/Properties;
// see PackBlockPalette in libminecraft-file). Stamping a world with a DataVersion
// newer than what was actually written makes the game look for region files in
// dimensions/<namespace>/<dimension>, not find them, and show an empty world, so
// the target stays at the newest release whose layout and palette the writer
// reproduces.
//
// Raising this means writing the 26.1 layout and the 26.3 palette, then
// regenerating the j2b2j/b2j2j test data for the new target.
constexpr int kJavaDataVersion = 4556;          // 1.21.10
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
