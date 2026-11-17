#pragma once

#include <string>

TEST_CASE("Bedrock 26.63 output constants") {
    CHECK(je2be::java::kNetworkVersion == 2225);
    CHECK(je2be::java::kStorageVersion == 10);
    CHECK(std::u8string(je2be::java::kInventoryVersion) == u8"1.26.63");
    CHECK(je2be::java::kBlockDataVersion == 18168865);
}

TEST_CASE("26.3 blocks map to Bedrock names") {
    je2be::DataVersion dataVersion(5023, 5023);

    auto cinnabar = mcfile::je::Block::FromNameAndProperties(u8"minecraft:cinnabar", 5023, {});
    auto beCinnabar = je2be::java::BlockData::From(cinnabar, nullptr, dataVersion, {});
    REQUIRE(beCinnabar);
    CHECK(beCinnabar->string(u8"name", u8"") == u8"minecraft:cinnabar");

    auto slab = mcfile::je::Block::FromNameAndProperties(u8"minecraft:black_concrete_slab", 5023, {{u8"type", u8"bottom"}});
    auto beSlab = je2be::java::BlockData::From(slab, nullptr, dataVersion, {});
    REQUIRE(beSlab);
    CHECK(beSlab->string(u8"name", u8"") == u8"minecraft:black_concrete_slab");

    auto sign = mcfile::je::Block::FromNameAndProperties(u8"minecraft:poplar_sign", 5023, {{u8"rotation", u8"0"}});
    auto beSign = je2be::java::BlockData::From(sign, nullptr, dataVersion, {});
    REQUIRE(beSign);
    CHECK(beSign->string(u8"name", u8"") == u8"minecraft:poplar_standing_sign");

    auto pot = mcfile::je::Block::FromNameAndProperties(u8"minecraft:potted_golden_dandelion", 5023, {});
    auto bePot = je2be::java::BlockData::From(pot, nullptr, dataVersion, {});
    REQUIRE(bePot);
    CHECK(bePot->string(u8"name", u8"") == u8"minecraft:flower_pot");
}
