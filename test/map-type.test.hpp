#pragma once

TEST_CASE("map-type") {
  CHECK(MapType::JavaItemIdFromBedrockDamage(23) == std::optional<std::u8string>(u8"minecraft:buried_ancient_city_map"));
  CHECK(MapType::JavaItemIdFromBedrockDamage(26) == std::optional<std::u8string>(u8"minecraft:warm_ocean_ruins_map"));
  CHECK(MapType::JavaItemIdFromBedrockDamage(15) == std::optional<std::u8string>(u8"minecraft:abandoned_camp_map"));
  CHECK_FALSE(MapType::JavaItemIdFromBedrockDamage(5).has_value());
  CHECK(MapType::BedrockDamageFromJavaTranslationKey(u8"filled_map.abandoned_camp") == std::optional<i16>(22));
}
