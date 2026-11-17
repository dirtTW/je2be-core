#pragma once

#include <mcfile/blocks/from-name.hpp>
#include <mcfile/blocks/name.hpp>
#include <mcfile/je/heightmaps.hpp>

#include <array>
#include <string>

static std::array<char8_t const *, 120> const kBlocks26 = {
    u8"minecraft:black_concrete_slab",
    u8"minecraft:black_concrete_stairs",
    u8"minecraft:black_wool_slab",
    u8"minecraft:black_wool_stairs",
    u8"minecraft:blue_concrete_slab",
    u8"minecraft:blue_concrete_stairs",
    u8"minecraft:blue_wool_slab",
    u8"minecraft:blue_wool_stairs",
    u8"minecraft:brown_concrete_slab",
    u8"minecraft:brown_concrete_stairs",
    u8"minecraft:brown_wool_slab",
    u8"minecraft:brown_wool_stairs",
    u8"minecraft:chiseled_cinnabar",
    u8"minecraft:chiseled_sulfur",
    u8"minecraft:cinnabar",
    u8"minecraft:cinnabar_brick_slab",
    u8"minecraft:cinnabar_brick_stairs",
    u8"minecraft:cinnabar_brick_wall",
    u8"minecraft:cinnabar_bricks",
    u8"minecraft:cinnabar_slab",
    u8"minecraft:cinnabar_stairs",
    u8"minecraft:cinnabar_wall",
    u8"minecraft:cyan_concrete_slab",
    u8"minecraft:cyan_concrete_stairs",
    u8"minecraft:cyan_wool_slab",
    u8"minecraft:cyan_wool_stairs",
    u8"minecraft:golden_dandelion",
    u8"minecraft:gray_concrete_slab",
    u8"minecraft:gray_concrete_stairs",
    u8"minecraft:gray_wool_slab",
    u8"minecraft:gray_wool_stairs",
    u8"minecraft:green_concrete_slab",
    u8"minecraft:green_concrete_stairs",
    u8"minecraft:green_wool_slab",
    u8"minecraft:green_wool_stairs",
    u8"minecraft:light_blue_concrete_slab",
    u8"minecraft:light_blue_concrete_stairs",
    u8"minecraft:light_blue_wool_slab",
    u8"minecraft:light_blue_wool_stairs",
    u8"minecraft:light_gray_concrete_slab",
    u8"minecraft:light_gray_concrete_stairs",
    u8"minecraft:light_gray_wool_slab",
    u8"minecraft:light_gray_wool_stairs",
    u8"minecraft:lime_concrete_slab",
    u8"minecraft:lime_concrete_stairs",
    u8"minecraft:lime_wool_slab",
    u8"minecraft:lime_wool_stairs",
    u8"minecraft:magenta_concrete_slab",
    u8"minecraft:magenta_concrete_stairs",
    u8"minecraft:magenta_wool_slab",
    u8"minecraft:magenta_wool_stairs",
    u8"minecraft:orange_concrete_slab",
    u8"minecraft:orange_concrete_stairs",
    u8"minecraft:orange_poplar_leaves",
    u8"minecraft:orange_wool_slab",
    u8"minecraft:orange_wool_stairs",
    u8"minecraft:pink_concrete_slab",
    u8"minecraft:pink_concrete_stairs",
    u8"minecraft:pink_wool_slab",
    u8"minecraft:pink_wool_stairs",
    u8"minecraft:polished_cinnabar",
    u8"minecraft:polished_cinnabar_slab",
    u8"minecraft:polished_cinnabar_stairs",
    u8"minecraft:polished_cinnabar_wall",
    u8"minecraft:polished_sulfur",
    u8"minecraft:polished_sulfur_slab",
    u8"minecraft:polished_sulfur_stairs",
    u8"minecraft:polished_sulfur_wall",
    u8"minecraft:poplar_button",
    u8"minecraft:poplar_door",
    u8"minecraft:poplar_fence",
    u8"minecraft:poplar_fence_gate",
    u8"minecraft:poplar_hanging_sign",
    u8"minecraft:poplar_log",
    u8"minecraft:poplar_planks",
    u8"minecraft:poplar_pressure_plate",
    u8"minecraft:poplar_sapling",
    u8"minecraft:poplar_shelf",
    u8"minecraft:poplar_sign",
    u8"minecraft:poplar_slab",
    u8"minecraft:poplar_stairs",
    u8"minecraft:poplar_trapdoor",
    u8"minecraft:poplar_wall_hanging_sign",
    u8"minecraft:poplar_wall_sign",
    u8"minecraft:poplar_wood",
    u8"minecraft:potent_sulfur",
    u8"minecraft:potted_golden_dandelion",
    u8"minecraft:potted_poplar_sapling",
    u8"minecraft:purple_concrete_slab",
    u8"minecraft:purple_concrete_stairs",
    u8"minecraft:purple_wool_slab",
    u8"minecraft:purple_wool_stairs",
    u8"minecraft:red_concrete_slab",
    u8"minecraft:red_concrete_stairs",
    u8"minecraft:red_poplar_leaves",
    u8"minecraft:red_shrub",
    u8"minecraft:red_wool_slab",
    u8"minecraft:red_wool_stairs",
    u8"minecraft:shelf_mushroom",
    u8"minecraft:straw_bed",
    u8"minecraft:stripped_poplar_log",
    u8"minecraft:stripped_poplar_wood",
    u8"minecraft:sulfur",
    u8"minecraft:sulfur_brick_slab",
    u8"minecraft:sulfur_brick_stairs",
    u8"minecraft:sulfur_brick_wall",
    u8"minecraft:sulfur_bricks",
    u8"minecraft:sulfur_slab",
    u8"minecraft:sulfur_spike",
    u8"minecraft:sulfur_stairs",
    u8"minecraft:sulfur_wall",
    u8"minecraft:white_concrete_slab",
    u8"minecraft:white_concrete_stairs",
    u8"minecraft:white_wool_slab",
    u8"minecraft:white_wool_stairs",
    u8"minecraft:yellow_concrete_slab",
    u8"minecraft:yellow_concrete_stairs",
    u8"minecraft:yellow_poplar_leaves",
    u8"minecraft:yellow_wool_slab",
    u8"minecraft:yellow_wool_stairs",
};

TEST_CASE("26.3 blocks resolve by name and id") {
    using namespace mcfile::blocks;
    for (char8_t const *name : kBlocks26) {
        BlockId id = FromName(std::u8string(name), 5023);
        REQUIRE(id != mcfile::blocks::unknown);
        CHECK(Name(id, 5023) == std::u8string(name));
    }
}

TEST_CASE("26.3 block characteristics and heightmaps") {
    using namespace mcfile::blocks;
    auto id = [](char8_t const *name) { return FromName(std::u8string(name), 5023); };

    auto slab = impl::BlockCharacteristic(id(u8"minecraft:black_concrete_slab"));
    REQUIRE(slab.has_value());
    CHECK_FALSE(slab->opaque);
    auto planks = impl::BlockCharacteristic(id(u8"minecraft:poplar_planks"));
    REQUIRE(planks.has_value());
    CHECK(planks->opaque);

    CHECK(mcfile::Heightmap::IsMotionBlockingById(id(u8"minecraft:black_concrete_slab")));
    CHECK(mcfile::Heightmap::IsMotionBlockingById(id(u8"minecraft:orange_poplar_leaves")));
    CHECK_FALSE(mcfile::Heightmap::IsMotionBlockingNoLeavesById(id(u8"minecraft:orange_poplar_leaves")));
    CHECK_FALSE(mcfile::Heightmap::IsMotionBlockingById(id(u8"minecraft:poplar_sapling")));
    CHECK_FALSE(mcfile::Heightmap::IsMotionBlockingById(id(u8"minecraft:poplar_button")));
    CHECK(mcfile::Heightmap::IsWorldSurfaceById(id(u8"minecraft:poplar_sapling")));
    CHECK(mcfile::Heightmap::IsWorldSurfaceById(id(u8"minecraft:shelf_mushroom")));
    CHECK(mcfile::Heightmap::IsOceanFloorById(id(u8"minecraft:cinnabar_bricks")));
    CHECK(mcfile::Heightmap::IsMotionBlockingById(id(u8"minecraft:sulfur_spike")));
    CHECK(mcfile::Heightmap::IsMotionBlockingById(id(u8"minecraft:straw_bed")));
}
