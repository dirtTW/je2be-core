#pragma once

#include "blocks-26.3-bedrock.data.hpp"

TEST_CASE("all 26.3 blocks map to a Bedrock 26.63 name") {
    je2be::DataVersion dataVersion(5023, 5023);
    for (auto const &c : kBlocks26Bedrock) {
        std::map<std::u8string, std::u8string> props(c.props.begin(), c.props.end());
        auto java = mcfile::je::Block::FromNameAndProperties(c.java, 5023, props);
        auto bedrock = je2be::java::BlockData::From(java, nullptr, dataVersion, {});
        INFO("java block: " << std::string(c.java.begin(), c.java.end()));
        REQUIRE(bedrock);
        CHECK(bedrock->string(u8"name", u8"") == c.bedrock);
    }
}
