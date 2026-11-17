#pragma once

TEST_CASE("shoulder-riders") {
  fs::path thisFile(__FILE__);

  // The situation is like this: https://gyazo.com/fe4669d4e655e80282484d961a17b167
  auto original = thisFile.parent_path() / "data" / "shoulder-riders";
  auto tmp = mcfile::File::CreateTempDir(fs::temp_directory_path());

  defer {
    fs::remove_all(*tmp);
  };

  auto be = *tmp / "be";
  fs::create_directories(be);
  je2be::java::Options optToBe;
  optToBe.fDimensionFilter.insert(mcfile::Dimension::Overworld);
  optToBe.fChunkFilter.insert(Pos2i(0, 0));
  CHECK(je2be::java::Converter::Run(original, be, optToBe, 1).ok());

  auto je = *tmp / "je";
  fs::create_directories(je);
  je2be::bedrock::Options optToJe;
  optToJe.fDimensionFilter.insert(mcfile::Dimension::Overworld);
  optToJe.fChunkFilter.insert(Pos2i(0, 0));
  CHECK(je2be::bedrock::Converter::Run(be, je, optToJe, 1).ok());

  auto playersDir = je / "players" / "data";
  fs::path playerFile;
  for (auto const &it : fs::directory_iterator(playersDir)) {
    if (it.is_regular_file() && it.path().filename().string().ends_with(".dat")) {
      playerFile = it.path();
      break;
    }
  }
  CHECK(!playerFile.empty());
  auto stream = make_shared<mcfile::stream::GzFileInputStream>(playerFile);
  auto player = CompoundTag::Read(stream, Encoding::Java);
  CHECK(player);

  CHECK(player->compoundTag(u8"ShoulderEntityLeft"));
  CHECK(player->compoundTag(u8"ShoulderEntityRight"));
  CHECK(player->compoundTag(u8"RootVehicle"));
}
