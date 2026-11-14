#pragma once

// The 26.3 chunk-section layout, as written by the official server: palette entries carry
// `id`/`properties` (the pre-26.3 keys `Name`/`Properties` are legacy), a state whose
// properties are all default is written as a bare string, and an unknown biome must not cost
// the whole section.
//
// See cmake/patches/libminecraft-file-26.x.patch.

namespace {

std::shared_ptr<CompoundTag> MakeSectionTag(int y, std::shared_ptr<ListTag> const &palette) {
  auto blocks = std::make_shared<CompoundTag>();
  blocks->set(u8"palette", palette);
  auto biomes = std::make_shared<CompoundTag>();
  auto biomePalette = std::make_shared<ListTag>(Tag::Type::End);
  biomePalette->push_back(std::make_shared<StringTag>(u8"minecraft:plains"));
  biomes->set(u8"palette", biomePalette);

  auto section = std::make_shared<CompoundTag>();
  section->set(u8"Y", std::make_shared<ByteTag>(static_cast<uint8_t>(y)));
  section->set(u8"block_states", blocks);
  section->set(u8"biomes", biomes);
  return section;
}

std::shared_ptr<ListTag> MakePalette(std::vector<std::shared_ptr<Tag>> const &entries) {
  auto palette = std::make_shared<ListTag>(Tag::Type::End);
  for (auto const &entry : entries) {
    palette->push_back(entry);
  }
  return palette;
}

std::shared_ptr<CompoundTag> MakeState(std::u8string const &key, std::u8string const &name) {
  auto state = std::make_shared<CompoundTag>();
  state->set(key, name);
  return state;
}

std::shared_ptr<mcfile::je::ChunkSection> ReadSection(std::shared_ptr<CompoundTag> const &tag, int dataVersion) {
  return je::chunksection::ChunkSection118::MakeChunkSection(tag.get(), dataVersion);
}

} // namespace

TEST_CASE("java-chunk-section-26.3") {
  int const dataVersion = 5023; // 26.3

  SUBCASE("bare-string palette") {
    // What the server writes for a section that is entirely air.
    auto tag = MakeSectionTag(0, MakePalette({std::make_shared<StringTag>(u8"minecraft:air")}));
    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    CHECK(section->y() == 0);
    auto block = section->blockAt(0, 0, 0);
    REQUIRE(block);
    CHECK(block->toString() == u8"minecraft:air");
    // A one-entry palette needs no `data` array: every voxel takes index 0.
    for (int x = 0; x < 16; x += 5) {
      for (int y = 0; y < 16; y += 5) {
        for (int z = 0; z < 16; z += 5) {
          auto b = section->blockAt(x, y, z);
          REQUIRE(b);
          CHECK(b->toString() == u8"minecraft:air");
        }
      }
    }
  }

  SUBCASE("id without properties") {
    auto tag = MakeSectionTag(-4, MakePalette({MakeState(u8"id", u8"minecraft:stone")}));
    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    CHECK(section->y() == -4);
    auto block = section->blockAt(3, 4, 5);
    REQUIRE(block);
    CHECK(block->toString() == je::Block::FromNameAndProperties(u8"minecraft:stone", dataVersion, {})->toString());
  }

  SUBCASE("id with properties") {
    auto grass = MakeState(u8"id", u8"minecraft:grass_block");
    auto props = std::make_shared<CompoundTag>();
    props->set(u8"snowy", u8"false");
    grass->set(u8"properties", props);
    auto tag = MakeSectionTag(0, MakePalette({grass}));
    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    auto block = section->blockAt(0, 0, 0);
    REQUIRE(block);
    std::map<std::u8string, std::u8string> expected{{u8"snowy", u8"false"}};
    CHECK(block->toString() == je::Block::FromNameAndProperties(u8"minecraft:grass_block", dataVersion, expected)->toString());
  }

  SUBCASE("block id under an empty key") {
    auto tag = MakeSectionTag(0, MakePalette({MakeState(u8"", u8"minecraft:dirt")}));
    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    auto block = section->blockAt(0, 0, 0);
    REQUIRE(block);
    CHECK(block->toString() == je::Block::FromNameAndProperties(u8"minecraft:dirt", dataVersion, {})->toString());
  }

  SUBCASE("legacy Name/Properties are still readable") {
    auto dirt = MakeState(u8"Name", u8"minecraft:dirt");
    auto props = std::make_shared<CompoundTag>();
    props->set(u8"snowy", u8"false");
    dirt->set(u8"Properties", props);
    auto tag = MakeSectionTag(0, MakePalette({dirt}));
    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    auto block = section->blockAt(0, 0, 0);
    REQUIRE(block);
    std::map<std::u8string, std::u8string> expected{{u8"snowy", u8"false"}};
    CHECK(block->toString() == je::Block::FromNameAndProperties(u8"minecraft:dirt", dataVersion, expected)->toString());
  }

  SUBCASE("26.x biomes are known") {
    // The patch adds the biomes that 26.2 and 26.3 introduced.
    CHECK(mcfile::biomes::Biome::FromName(u8"minecraft:sulfur_caves") == mcfile::biomes::minecraft::sulfur_caves);
    CHECK(mcfile::biomes::Biome::FromName(u8"minecraft:dappled_forest") == mcfile::biomes::minecraft::dappled_forest);
  }

  SUBCASE("unknown biome falls back to plains") {
    auto tag = MakeSectionTag(0, MakePalette({std::make_shared<StringTag>(u8"minecraft:air")}));
    auto biomes = tag->compoundTag(u8"biomes");
    REQUIRE(biomes);
    auto palette = std::make_shared<ListTag>(Tag::Type::End);
    palette->push_back(std::make_shared<StringTag>(u8"minecraft:biome_from_a_future_release"));
    biomes->set(u8"palette", palette);

    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    auto biome = section->biomeAt(0, 0, 0);
    REQUIRE(biome);
    CHECK(*biome == mcfile::biomes::minecraft::plains);
    // The block palette must survive an unknown biome.
    auto block = section->blockAt(0, 0, 0);
    REQUIRE(block);
    CHECK(block->toString() == u8"minecraft:air");
  }

  SUBCASE("multi-entry palette needs and uses data") {
    auto palette = MakePalette({
        std::make_shared<StringTag>(u8"minecraft:stone"),
        std::make_shared<StringTag>(u8"minecraft:dirt"),
    });
    auto tag = MakeSectionTag(0, palette);
    // A palette with more than one entry must carry the packed indices.
    CHECK(ReadSection(tag, dataVersion) == nullptr);

    // 4 bits per voxel: 4096 / 16 = 256 longs. All zero => every voxel takes entry 0.
    std::vector<int64_t> packed(256, 0);
    tag->compoundTag(u8"block_states")->set(u8"data", std::make_shared<LongArrayTag>(packed));
    auto section = ReadSection(tag, dataVersion);
    REQUIRE(section);
    auto stone = je::Block::FromNameAndProperties(u8"minecraft:stone", dataVersion, {});
    for (int i = 0; i < 16; i += 7) {
      auto block = section->blockAt(i, i, i);
      REQUIRE(block);
      CHECK(block->toString() == stone->toString());
    }
  }
}
