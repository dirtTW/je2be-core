# je2be-core (dirtTW fork)

Fork of [kbinani/je2be-core](https://github.com/kbinani/je2be-core): the converter core used by
[DBOP world converter](https://dirttw.github.io/DBOP-Installtutorial/world-converter/).

It converts Minecraft worlds between Java Edition, Bedrock Edition, Xbox 360, and PS3 saves.
The core is C++20 and is built natively for the CLI tools and with Emscripten for the browser
build used by je2be-web.

Licensed **AGPL-3.0-or-later**. See `LICENSE`.

## Current state on `main`

### Java Edition input

- Reads Java worlds through **26.3 (data version 5023)**, including the 26.1 directory layout
  (`dimensions/minecraft/<dimension>`, `data/minecraft/...`, `players/data/...`) and the 26.3
  chunk-section block-palette shapes:
  - `{ Name, Properties }` (pre-26.3),
  - `{ id, properties }` (26.3),
  - a bare string entry,
  - a bare string palette list.
- Unknown block ids are preserved by name instead of being blanked.
- Unknown biome names no longer drop a whole 16×16×16 section; the section is kept and falls
  back to `plains`.
- `sulfur_caves` and `dappled_forest` are known Java biomes.

### Java Edition output

- Still writes **1.21.10 (`DataVersion = 4556`)**, the pre-26.1 directory layout
  (`region/`, `DIM-1/`, `DIM1/`), and the pre-26.3 `Name`/`Properties` palette keys.
- This is intentional for now: a world stamped with a newer `DataVersion` would make a 26.x
  client look under `dimensions/minecraft/overworld` and show an empty world. Every newer
  client upgrades a 1.21.10 world on load.

### Bedrock Edition output

- Writes **Bedrock 1.21.120**:
  - `StorageVersion = 10`
  - `NetworkVersion = 859`
  - `InventoryVersion = "1.21.120"`
  - block state version `18168865`
- Newer Bedrock clients upgrade a 1.21.120 world on load.
- Bedrock biome IDs for the 26.x Java biomes are current:
  - `sulfur_caves` → **194** (Bedrock 26.30+)
  - `dappled_forest` → **195** (Bedrock 26.50+)

### Xbox 360 / PS3

LCE save handling is present (`x2j`, `x2b`, `p2j`, `p2b` paths). Xbox 360 input is a single
`.bin` save; PS3 input is an extracted save directory with `GAMEDATA`.

### Player data and entities

- Java → Bedrock: local player data is written to the Bedrock `LocalPlayer` key, including
  root vehicle and shoulder riders.
- Bedrock → Java: the local player is written to `level.dat` (`Data.Player` on 1.21.10).
- Entities, block entities, maps, POI, and structures have converters, but coverage is not
  complete; round-trip tests compare the fixtures under `test/data/`.

### Dependency patch

`libminecraft-file` is pinned to `6f00d2c6` and patched by
`cmake/patches/libminecraft-file-26.x.patch` because the upstream headers do not read the
26.x chunk format yet. The patch covers:

- 26.3 palette keys and palette shapes,
- unknown biome fallback,
- the numeric Bedrock IDs above.

A second palette change (writing `id`/`properties` for 26.3 output) is **not** on `main` yet;
it belongs to the Java 26.3 writer work below.

## Build

Native (MSVC / CMake):

```bat
cmake -S . -B build-native
cmake --build build-native --config Release
```

The CLI examples are `example/j2b.cpp`, `b2j.cpp`, `x2j.cpp`, and `p2j.cpp`. The doctest suite
is `je2be-test`; the round-trip tests are `test/j2b2j.cpp` and the fixtures in `test/data/`.

WASM / browser payload is built through [je2be-web](https://github.com/kbinani/je2be-web);
see that repository's `build-wasm` workflow. The DBOP site's GitHub Actions workflow checks
out this fork and builds `core.js` + `core.worker.js` from it.

## TODO

1. **Write Java 26.3** (work in progress, not on `main`):
   - 26.1 layout: `dimensions/minecraft/<dimension>/{region,entities,poi}`,
     `data/minecraft/...`, `players/data/<uuid>.dat`.
   - 26.3 palette keys: `id` / `properties`, omitting `properties` for default states.
   - Split `level.dat` `Data.Player` into `players/data/<uuid>.dat` and write
     `Data.singleplayer_uuid` (int array).
   - Maps under `data/minecraft/maps/<id>.dat`; `last_id.dat`.
   - Regenerate `j2b2j` / `b2j2b` expectations for the new layout and `DataVersion = 5023`.
   - Add/repair round-trip tests for player data and falling blocks under the new layout.
2. **Write the latest Bedrock Edition** (26.60 / 26.63):
   - Update `StorageVersion`, `NetworkVersion`, `InventoryVersion`, `lastOpenedWithVersion`,
     `MinimumCompatibleClientVersion`, and the Bedrock block-state/chunk versions.
   - Use the latest Bedrock block palettes (`R26_60`, and 26.63 if it changes the format) so
     new blocks convert to real Bedrock identifiers instead of names that 1.21.120 lacks.
   - Add Bedrock 26.x biome IDs beyond 194/195 (e.g. `ice_caves` when it leaves preview).
3. **Block/item coverage**:
   - `libminecraft-file` has no ids for roughly the blocks and items added in 1.21.11–26.3.
     They currently convert by name. Add ids and heightmap classification for the ones that
     have a Bedrock counterpart.
4. **Player and entity completeness**:
   - Convert more than one Java player file; define behaviour for multiplayer/server worlds.
   - Audit entity, block-entity, and structure round-trips against 26.x fixtures.
5. **Tests and fixtures**:
   - Add a 26.3 Java world fixture and a Bedrock 26.60/26.63 fixture.
   - Keep the native test suite green and add the same checks to the WASM build.
   - Validate generated worlds with a dedicated 26.x client or server, not only with this
     library's reader.

## References

- [Chunker](https://github.com/HiveGamesOSS/Chunker) by Hive Games (MIT) is the reference
  implementation used for the latest Java/Bedrock layouts, version constants, and biome IDs.
  It is not bundled here.
- [Minecraft Wiki](https://minecraft.wiki/) is authoritative for Bedrock protocol versions and
  numeric biome IDs (for example `Biome/ID` and `Bedrock_Edition_26.63`).
- Upstream core: [kbinani/je2be-core](https://github.com/kbinani/je2be-core).
