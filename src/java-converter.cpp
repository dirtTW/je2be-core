#include <je2be/java/converter.hpp>

#if defined(_MSC_VER)
#define NOMINMAX
#undef small
#include <windows.h>
#endif

#if defined(__GNUC__)
#include <fcntl.h>
#endif

#include <je2be/java/options.hpp>
#include <je2be/java/progress.hpp>

#include <atomic>

#include "_directory-iterator.hpp"
#include "_parallel.hpp"
#include "db/_concurrent-db.hpp"
#include "java/_context.hpp"
#include "java/_datapacks.hpp"
#include "java/_entity-store.hpp"
#include "java/_entity.hpp"
#include "java/_level.hpp"
#include "java/_region.hpp"
#include "java/_session-lock.hpp"
#include "java/_world-data.hpp"
#include "java/_world.hpp"

#include "_world-icon.hpp"

namespace je2be::java {

class Converter::Impl {
  Impl() = delete;

public:
  static Status Run(std::filesystem::path const &input, std::filesystem::path const &output, Options const &o, int concurrency, Progress *progress = nullptr) {
    using namespace std;
    namespace fs = std::filesystem;
    using namespace mcfile;

    SessionLock lock(input);
    if (!lock.lock()) {
      return JE2BE_ERROR;
    }

    double const numTotalChunks = GetTotalNumChunks(input, o);

    auto rootPath = output;
    auto dbPath = rootPath / "db";

    error_code ec;
    fs::create_directories(dbPath, ec);
    if (ec) {
      return JE2BE_ERROR_WHAT(ec.message());
    }

    auto data = Level::Read(o.getLevelDatFilePath(input));
    if (!data) {
      return JE2BE_ERROR;
    }
    // Java 26.3 stores game rules and difficulty outside Level.Data.GameRules.
    if (auto dataTag = data->compoundTag(u8"Data"); dataTag) {
      if (auto difficultySettings = dataTag->compoundTag(u8"difficulty_settings"); difficultySettings) {
        if (auto difficulty = difficultySettings->string(u8"difficulty"); difficulty) {
          int value = 2;
          if (*difficulty == u8"peaceful") {
            value = 0;
          } else if (*difficulty == u8"easy") {
            value = 1;
          } else if (*difficulty == u8"hard") {
            value = 3;
          }
          dataTag->set(u8"Difficulty", make_shared<mcfile::nbt::ByteTag>((int8_t)value));
        }
      }
      fs::path gameRulesPath = input / "data" / "minecraft" / "game_rules.dat";
      if (fs::is_regular_file(gameRulesPath)) {
        auto stream = make_shared<mcfile::stream::GzFileInputStream>(gameRulesPath);
        auto root = mcfile::nbt::CompoundTag::Read(stream, mcfile::Encoding::Java);
        if (root) {
          if (auto rules = root->compoundTag(u8"data"); rules) {
            static std::unordered_map<std::u8string, std::u8string> const names = {
                {u8"minecraft:command_block_output", u8"commandBlockOutput"},
                {u8"minecraft:command_blocks_work", u8"commandBlocksEnabled"},
                {u8"minecraft:advance_time", u8"doDaylightCycle"},
                {u8"minecraft:advance_weather", u8"doWeatherCycle"},
                {u8"minecraft:block_drops", u8"doTileDrops"},
                {u8"minecraft:entity_drops", u8"doEntityDrops"},
                {u8"minecraft:mob_drops", u8"doMobLoot"},
                {u8"minecraft:spawn_mobs", u8"doMobSpawning"},
                {u8"minecraft:immediate_respawn", u8"doImmediateRespawn"},
                {u8"minecraft:spawn_phantoms", u8"doInsomnia"},
                {u8"minecraft:limited_crafting", u8"doLimitedCrafting"},
                {u8"minecraft:fire_damage", u8"fireDamage"},
                {u8"minecraft:fall_damage", u8"fallDamage"},
                {u8"minecraft:drowning_damage", u8"drowningDamage"},
                {u8"minecraft:freeze_damage", u8"freezeDamage"},
                {u8"minecraft:keep_inventory", u8"keepInventory"},
                {u8"minecraft:locator_bar", u8"locatorBar"},
                {u8"minecraft:max_command_sequence_length", u8"maxCommandChainLength"},
                {u8"minecraft:mob_griefing", u8"mobGriefing"},
                {u8"minecraft:natural_health_regeneration", u8"naturalRegeneration"},
                {u8"minecraft:random_tick_speed", u8"randomTickSpeed"},
                {u8"minecraft:send_command_feedback", u8"sendCommandFeedback"},
                {u8"minecraft:show_death_messages", u8"showDeathMessages"},
                {u8"minecraft:respawn_radius", u8"spawnRadius"},
                {u8"minecraft:players_sleeping_percentage", u8"playersSleepingPercentage"},
                {u8"minecraft:projectiles_can_break_blocks", u8"projectilesCanBreakBlocks"},
                {u8"minecraft:pvp", u8"pvp"},
                {u8"minecraft:tnt_explodes", u8"tntExplodes"},
                {u8"minecraft:tnt_explosion_drop_decay", u8"tntexplosiondropdecay"},
                {u8"minecraft:reduced_debug_info", u8"reducedDebugInfo"},
            };
            auto legacyGameRules = make_shared<mcfile::nbt::CompoundTag>();
            for (auto const &it : *rules) {
              auto found = names.find(it.first);
              if (found == names.end()) {
                continue;
              }
              std::u8string value;
              switch (it.second->type()) {
              case mcfile::nbt::Tag::Type::Byte:
                if (auto b = dynamic_pointer_cast<mcfile::nbt::ByteTag>(it.second); b) {
                  value = b->fValue ? u8"true" : u8"false";
                }
                break;
              case mcfile::nbt::Tag::Type::Int:
                if (auto i = dynamic_pointer_cast<mcfile::nbt::IntTag>(it.second); i) {
                  value = mcfile::String::ToString(i->fValue);
                }
                break;
              case mcfile::nbt::Tag::Type::Long:
                if (auto i = dynamic_pointer_cast<mcfile::nbt::LongTag>(it.second); i) {
                  value = mcfile::String::ToString(i->fValue);
                }
                break;
              default:
                break;
              }
              if (!value.empty()) {
                legacyGameRules->set(found->second, make_shared<mcfile::nbt::StringTag>(value));
              }
            }
            dataTag->set(u8"GameRules", legacyGameRules);
          }
        }
      }
    }

    Level level = Level::ImportFromJava(*data);

    // Java 26.1+ moved world generation settings out of level.dat.
    fs::path worldGenSettingsPath = input / "data" / "minecraft" / "world_gen_settings.dat";
    if (fs::is_regular_file(worldGenSettingsPath)) {
      auto stream = make_shared<mcfile::stream::GzFileInputStream>(worldGenSettingsPath);
      auto root = mcfile::nbt::CompoundTag::Read(stream, mcfile::Encoding::Java);
      if (root) {
        auto settings = root->compoundTag(u8"data");
        if (settings) {
          if (auto seed = settings->int64(u8"seed")) {
            level.fRandomSeed = *seed;
          }
        }
      }
    }

    // Java 26.1+ moved weather out of level.dat.
    fs::path weatherPath = input / "data" / "minecraft" / "weather.dat";
    if (fs::is_regular_file(weatherPath)) {
      auto stream = make_shared<mcfile::stream::GzFileInputStream>(weatherPath);
      auto root = mcfile::nbt::CompoundTag::Read(stream, mcfile::Encoding::Java);
      if (root) {
        auto weather = root->compoundTag(u8"data");
        if (weather) {
          level.fRainLevel = weather->boolean(u8"raining", false) ? 1 : 0;
          level.fRainTime = weather->int32(u8"rain_time", level.fRainTime);
          level.fLightningLevel = weather->boolean(u8"thundering", false) ? 1 : 0;
          level.fLightningTime = weather->int32(u8"thunder_time", level.fLightningTime);
          auto clear = weather->int32(u8"clear_weather_time", 0);
          if (clear > 0) {
            level.fRainTime = clear;
            level.fLightningTime = clear;
          }
        }
      }
    }

    bool ok = Datapacks::Import(input, output);

    auto levelData = std::make_unique<LevelData>(input, o, level.fCurrentTick, level.fDifficulty, level.fCommandsEnabled, level.fGameType, level.fDataVersion);
    ConcurrentDb db(dbPath, concurrency, o.fDbTempDirectory);
    if (!db.valid()) {
      return JE2BE_ERROR;
    }

    auto localPlayerData = LocalPlayerData(*data, input, o, *levelData);
    if (localPlayerData) {
      auto k = mcfile::be::DbKey::LocalPlayer();
      if (auto st = db.put(k, *localPlayerData); !st.ok()) {
        return JE2BE_ERROR_PUSH(st);
      }
    }

    struct Work {
      Dimension fDim;
      shared_ptr<mcfile::je::Region> fRegion;

      Work(Dimension dim, shared_ptr<mcfile::je::Region> const &region) : fDim(dim), fRegion(region) {}
    };
    struct Result {
      map<mcfile::Dimension, shared_ptr<WorldData>> fData;

      void mergeInto(Result &out) const {
        for (auto const &it : fData) {
          if (!it.second) {
            continue;
          }
          if (auto found = out.fData.find(it.first); found != out.fData.end()) {
            if (found->second) {
              it.second->drain(*found->second);
            }
          } else {
            out.fData[it.first] = it.second;
          }
        }
      }
    };
    map<mcfile::Dimension, std::shared_ptr<EntityStore>> entityStores;
    vector<Work> works;
    for (auto dim : {Dimension::Overworld, Dimension::Nether, Dimension::End}) {
      if (!o.fDimensionFilter.empty()) [[unlikely]] {
        if (o.fDimensionFilter.find(dim) == o.fDimensionFilter.end()) {
          continue;
        }
      }
      auto entityStoreDir = mcfile::File::CreateTempDir(o.getTempDirectory());
      if (!entityStoreDir) {
        return JE2BE_ERROR;
      }
      auto entityStore = EntityStore::Open(*entityStoreDir);
      if (!entityStore) {
        return JE2BE_ERROR;
      }
      entityStores[dim].reset(entityStore);
      auto dir = o.getWorldDirectory(input, dim);
      mcfile::je::World world(dir);
      world.eachRegions([dim, &works](shared_ptr<mcfile::je::Region> const &region) {
        Work work(dim, region);
        works.push_back(work);
        return true;
      });
    }
    atomic_uint32_t done(0);
    atomic_bool abortSignal(false);
    atomic_uint64_t numConvertedChunks(0);
    LevelData const *ldPtr = levelData.get();
    auto [result, status] = Parallel::Reduce<Work, Result>(
        works,
        concurrency,
        Result(),
        [ldPtr, &db, progress, &done, numTotalChunks, &abortSignal, entityStores, o, &numConvertedChunks](Work const &work) -> pair<Result, Status> {
          auto found = entityStores.find(work.fDim);
          assert(found != entityStores.end());
          shared_ptr<EntityStore> entityStore = found->second;
          auto worldData = Region::Convert(
              work.fDim,
              work.fRegion,
              o,
              entityStore,
              *ldPtr,
              db,
              progress,
              done,
              numTotalChunks,
              abortSignal,
              numConvertedChunks);
          Result ret;
          ret.fData[work.fDim] = worldData;
          return make_pair(ret, Status::Ok());
        },
        [](Result const &from, Result &to) -> void {
          from.mergeInto(to);
        });
    if (!status.ok()) {
      return JE2BE_ERROR_PUSH(status);
    }
    u64 totalEntityChunks = 0;
    for (auto const &it : entityStores) {
      totalEntityChunks += it.second->fChunks.size();
    }
    atomic<u64> doneEntityChunks(0);
    for (auto const &it : result.fData) {
      mcfile::Dimension dim = it.first;

      it.second->drain(*levelData);
      auto entityStore = entityStores[dim];
      assert(entityStore);
      if (!entityStore) {
        continue;
      }

      if (auto st = World::PutWorldEntities(dim, db, entityStore, concurrency, progress, doneEntityChunks, totalEntityChunks); !st.ok()) {
        return JE2BE_ERROR_PUSH(st);
      }
    }
    if (progress) {
      u64 v = std::max<u64>(1, totalEntityChunks);
      if (!progress->reportEntityPostProcess({v, v})) {
        return JE2BE_ERROR;
      }
    }

    if (ok) {
      level.fCurrentTick = max(level.fCurrentTick, levelData->fMaxChunkLastUpdate);
      for (auto const &ex : levelData->fExperiments) {
        level.fExperiments[ex] = true;
      }
      level.fCheatsEnabled = levelData->fAllowCommand;
      ok = level.write(output / "level.dat");
      if (ok) {
        ConvertWorldIcon(input, output);
      }
      if (ok) {
        if (auto st = levelData->put(db, *data, levelData->fUuids); !st.ok()) {
          return JE2BE_ERROR_PUSH(st);
        }
      }
    }

    if (ok) {
      auto st = db.close([progress](Rational<u64> const &p) {
        if (!progress) {
          return true;
        }
        return progress->reportCompaction(p);
      });
      if (!st.ok()) {
        return JE2BE_ERROR_PUSH(st);
      }
    } else {
      db.abandon();
    }
    if (!ok) {
      return JE2BE_ERROR;
    }

    if (levelData->fError) {
      return Status(Status::ErrorData(*levelData->fError));
    } else {
      return Status::Ok();
    }
  }

  static std::optional<std::string> LocalPlayerData(CompoundTag const &tag, std::filesystem::path const &input, Options const &o, LevelData &ld) {
    using namespace std;
    using namespace mcfile::stream;

    auto data = tag.compoundTag(u8"Data");
    if (!data) {
      return std::nullopt;
    }
    auto playerJ = ResolveLocalPlayer(*data, input, o);
    if (!playerJ) {
      return std::nullopt;
    }

    WorldData wd(mcfile::Dimension::Overworld);
    Context ctx(ld.fJavaEditionMap, ld.fLodestones, ld.fUuids, wd, ld.fGameTick, ld.fDifficultyBedrock, ld.fAllowCommand, ld.fGameType);
    DataVersion dataVersion(ld.fDataVersion, ld.fDataVersion);
    auto playerB = Entity::LocalPlayer(*playerJ, ctx, dataVersion, {});
    if (!playerB) {
      return std::nullopt;
    }
    wd.drain(ld);

    LevelData::PlayerAttachedEntities pae;
    pae.fDim = playerB->fDimension;
    pae.fLocalPlayerUid = playerB->fUid;
    if (auto rootVehicle = playerJ->compoundTag(u8"RootVehicle"); rootVehicle) {
      if (auto entityJ = rootVehicle->compoundTag(u8"Entity"); entityJ) {
        if (auto entityB = Entity::From(*entityJ, ctx, dataVersion, {Entity::Flag::RootVehicle}); entityB.fEntity) {
          LevelData::VehicleAndPassengers vap;
          vap.fChunk = playerB->fChunk;
          vap.fVehicle = entityB.fEntity;
          vap.fPassengers.swap(entityB.fPassengers);
          pae.fVehicle = vap;

          auto vehicleUid = entityB.fEntity->int64(u8"UniqueID");
          if (vehicleUid) {
            playerB->fEntity->set(u8"RideID", Long(*vehicleUid));
          }
        }
      }
    }

    struct Rider {
      u8string fBedrockKey;
      int fLinkId;
      float fRotation;

      Rider(u8string const &bedrockKey, int linkId, float rotation) : fBedrockKey(bedrockKey), fLinkId(linkId), fRotation(rotation) {}
    };
    double const kDistanceToPlayer = 0.4123145064147021;        // distance between player and parrot.
    double const kAngleAgainstPlayerFacing = 104.0364421885305; // angle in degrees
    auto linksTag = List<Tag::Type::Compound>();
    static unordered_map<u8string, Rider> const keys = {
        {u8"ShoulderEntityLeft", Rider(u8"LeftShoulderRiderID", 0, -kAngleAgainstPlayerFacing)},
        {u8"ShoulderEntityRight", Rider(u8"RightShoulderPassengerID", 1, kAngleAgainstPlayerFacing)}};
    for (auto const &key : keys) {
      auto keyJ = key.first;
      auto keyB = key.second.fBedrockKey;
      auto linkId = key.second.fLinkId;
      auto angle = key.second.fRotation / 180.0f * std::numbers::pi;

      // player: [9.0087, 72.62, 1.96865] yaw: -174.737
      // riderLeft: [8.60121, 72.42, 2.03154]
      // riderRight: [9.39784, 72.42, 2.10492]
      // https://gyazo.com/e797402bff04e5c3db9bae4bd1730629
      auto shoulderEntity = playerJ->compoundTag(keyJ);
      if (!shoulderEntity) {
        continue;
      }
      auto pos = props::GetPos3f(*playerB->fEntity, u8"Pos");
      if (!pos) {
        continue;
      }
      auto rot = props::GetRotation(*playerJ, u8"Rotation");
      if (!rot) {
        continue;
      }
      float yaw = (rot->fYaw + 90.0) / 180.0 * std::numbers::pi;
      Pos2d facing = Pos2d(kDistanceToPlayer, 0).rotated(yaw);
      Pos2d rider = facing.rotated(angle);
      Pos2d riderPos2d = Pos2d(pos->fX, pos->fZ) + rider;
      Pos3f riderPos3f(riderPos2d.fX, pos->fY - 0.2, riderPos2d.fZ);
      // ground: 71 -> boat: 71.375 -> player: 72.62 -> parrot: 72.42
      // ground: 71 -> player: 72.62 -> parrot: 72.42
      if (auto riderB = Entity::From(*shoulderEntity, ctx, dataVersion, {Entity::Flag::ShoulderRider}); riderB.fEntity) {
        auto id = riderB.fEntity->int64(u8"UniqueID");
        if (id) {
          riderB.fEntity->set(u8"Pos", riderPos3f.toListTag());

          int cx = mcfile::Coordinate::ChunkFromBlock((int)floorf(riderPos3f.fX));
          int cz = mcfile::Coordinate::ChunkFromBlock((int)floorf(riderPos3f.fZ));
          Pos2i chunkPos(cx, cz);
          pae.fShoulderRiders.push_back(make_pair(chunkPos, riderB.fEntity));

          playerB->fEntity->set(keyB, Long(*id));

          auto link = Compound();
          link->set(u8"entityID", Long(*id));
          link->set(u8"linkID", Int(linkId));
          linksTag->push_back(link);
        }
      }
    }
    if (!linksTag->empty()) {
      playerB->fEntity->set(u8"LinksTag", linksTag);
    }
    if (pae.fVehicle || !pae.fShoulderRiders.empty()) {
      ld.fPlayerAttachedEntities = pae;
    }

    return CompoundTag::Write(*playerB->fEntity, mcfile::Encoding::LittleEndian);
  }

private:
  /// @brief The player entity to convert as the world's local player.
  ///
  /// Before 26.1 this was level.dat's Data.Player. 26.1 replaced that tag with a
  /// singleplayer_uuid reference and moved the entity itself into
  /// players/data/<uuid>.dat, so that file is read instead. A singleplayer world has
  /// exactly one player file; if several are present the most recently written one is
  /// used, which is the one the player last logged out with.
  static CompoundTagPtr ResolveLocalPlayer(CompoundTag const &levelDatData, std::filesystem::path const &input, Options const &o) {
    using namespace std;
    namespace fs = std::filesystem;
    using namespace mcfile::stream;

    if (auto player = levelDatData.compoundTag(u8"Player"); player) {
      return player;
    }

    error_code ec;
    auto dir = o.getPlayersDataDirectory(input);
    if (!fs::is_directory(dir, ec)) {
      return nullptr;
    }

    fs::path newest;
    fs::file_time_type newestTime{};
    for (DirectoryIterator itr(dir); itr.valid(); itr.next()) {
      if (!itr->is_regular_file()) {
        continue;
      }
      if (!itr->path().filename().string().ends_with(".dat")) {
        continue;
      }
      auto t = itr->last_write_time(ec);
      if (ec) {
        continue;
      }
      if (newest.empty() || t > newestTime) {
        newest = itr->path();
        newestTime = t;
      }
    }
    if (newest.empty()) {
      return nullptr;
    }

    auto s = make_shared<GzFileInputStream>(newest);
    auto player = CompoundTag::Read(s, mcfile::Encoding::Java);
    if (!player) {
      return nullptr;
    }
    return player;
  }

  static double GetTotalNumChunks(std::filesystem::path const &input, Options o) {
    namespace fs = std::filesystem;
    u32 num = 0;
    for (auto dim : {mcfile::Dimension::Overworld, mcfile::Dimension::Nether, mcfile::Dimension::End}) {
      auto dir = o.getWorldDirectory(input, dim) / "region";
      if (!fs::exists(dir)) {
        continue;
      }
      for (DirectoryIterator itr(dir); itr.valid(); itr.next()) {
        auto name = itr->path().filename().string();
        if (!name.starts_with("r.") || !name.ends_with(".mca")) {
          continue;
        }
        if (!itr->is_regular_file()) {
          continue;
        }
        num++;
      }
    }
    return num * 32.0 * 32.0;
  }
};

Status Converter::Run(std::filesystem::path const &input, std::filesystem::path const &output, Options const &o, int concurrency, Progress *progress) {
  return Impl::Run(input, output, o, concurrency, progress);
}

} // namespace je2be::java
