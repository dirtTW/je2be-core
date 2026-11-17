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
- Chunks whose status is `minecraft:initialize_light` are converted when they contain
  sections. 26.x leaves valid terrain and block entities there; skipping them lost most of
  a trial-chamber area in the local audit.
- Unknown block ids are preserved by name instead of being blanked.
- Unknown biome names no longer drop a whole 16×16×16 section; the section is kept and falls
  back to `plains`.
- `sulfur_caves` and `dappled_forest` are known Java biomes.

### Java Edition output

- Writes **26.3 (`DataVersion = 5023`)**:
  - 26.1 layout: `dimensions/minecraft/<dimension>/{region,entities,poi}` and
    `data/minecraft/...`.
  - 26.3 block palette keys: `id` / `properties` (properties are omitted for the default
    block state).
  - local player in `players/data/<uuid>.dat`, referenced by `Data.singleplayer_uuid`.
  - maps in `data/minecraft/maps/<id>.dat`, with `last_id.dat`.
- Unknown blocks still convert by name, so blocks newer than the Bedrock 1.26.63 palette
  may not render until the Bedrock tables catch up.

### Bedrock Edition output

- Writes **Bedrock 1.26.63**:
  - `StorageVersion = 10`
  - `NetworkVersion = 2225`
  - `InventoryVersion = "1.26.63"`
  - `MinimumCompatibleClientVersion` and `lastOpenedWithVersion` = `[1, 26, 63, 0, 0]`
  - block-state revision `18168865` (1.21.60.33, still current in 26.63)
  - subchunk block-storage version `9`, chunk version `42`
- 26.63 is the latest Bedrock release. Older clients cannot open this world; use the
  1.21.120 build of the converter for those clients.
- Bedrock biome IDs for the 26.x Java biomes are current:
  - `sulfur_caves` → **194** (Bedrock 26.30+)
  - `dappled_forest` → **195** (Bedrock 26.50+)

### Xbox 360 / PS3

LCE save handling is present (`x2j`, `x2b`, `p2j`, `p2b` paths). Xbox 360 input is a single
`.bin` save; PS3 input is an extracted save directory with `GAMEDATA`.

### Player data and entities

- Java → Bedrock: local player data is written to the Bedrock `LocalPlayer` key, including
  root vehicle and shoulder riders.
- Bedrock → Java: the local player is written to `players/data/<uuid>.dat` with a
  `Data.singleplayer_uuid` reference, because the Java output is now 26.3.
- Entities, block entities, maps, POI, and structures have converters, but coverage is not
  complete; round-trip tests compare the fixtures under `test/data/`.
- Legacy Java entity names are normalized on output (`minecart_chest` → `chest_minecart`,
  `falling_sand` → `falling_block`, and the furnace/hopper/TNT/rideable/command-block
  minecarts).
- The local 26.3 audit now matches source and Bedrock block entities by coordinate, except
  for one chest that is represented as the other half of a double chest. Sulfur cube body
  equipment, blank sign sides, `potent_sulfur`, world icon, and `levelname.txt` are covered.

### Dependency fork

`libminecraft-file` is pinned to
[dirtTW/libminecraft-file@f45bcee](https://github.com/dirtTW/libminecraft-file/commit/f45bceeac56827fc3d0799a24453e726a8c961f2),
a fork of kbinani's header-only library. See that fork's
[README](https://github.com/dirtTW/libminecraft-file#readme) for its current state and
limitations. The fork carries:

- 26.1/26.3 chunk palette reading (`id`/`properties` and string palettes),
- unknown-biome fallback and the Bedrock biome IDs above,
- the 120 Java 26.3 blocks in the id, name, from-name, and heightmap tables,
- the legacy flattening fixes from upstream PR #3 (typos, petrified slabs, axis:none logs,
  shrubs, and item renames),
- `Block::toCompoundTag` writing `id`/`properties` for 26.3 output.

## Build

Native (MSVC / CMake):

```bat
cmake -S . -B build-native
cmake --build build-native --config Release
```

The CLI examples are `example/j2b.cpp`, `b2j.cpp`, `x2j.cpp`, and `p2j.cpp`. The doctest suite
is `je2be-test`; the round-trip tests are `test/j2b2j.cpp` and the fixtures in `test/data/`.

The suite also checks all 120 Java 26.3 blocks at their default states against the Bedrock
26.63 names, and a real 26.3 world was converted and inspected locally.

WASM / browser payload is built through [je2be-web](https://github.com/kbinani/je2be-web);
see that repository's `build-wasm` workflow. The DBOP site's GitHub Actions workflow checks
out this fork and builds `core.js` + `core.worker.js` from it.

## TODO

1. **Block/item coverage**:
   - The 120 Java 26.3 blocks now have ids, Bedrock mappings, and heightmap entries. Bedrock
     sign and flower-pot variants go through the existing helpers: `poplar_sign` →
     `poplar_standing_sign`, and the potted plants → `flower_pot`.
   - Item-only additions (boats, etc.) still convert by name.
   - Add Bedrock 26.x biome IDs beyond 194/195 (e.g. `ice_caves` when it leaves preview).
2. **Player and entity completeness**:
   - Convert more than one Java player file; define behaviour for multiplayer/server worlds.
   - Audit entity, block-entity, and structure round-trips against 26.x fixtures.
3. **Tests and fixtures**:
   - Add a 26.3 Java world fixture and a Bedrock 26.63 fixture.
   - Keep the native test suite green and add the same checks to the WASM build.
   - Validate generated worlds with a dedicated 26.63 client or server, not only with this
     library's reader.
4. **Future Minecraft versions** (planned, not implemented):
   - Put Java/Bedrock version numbers and block-state revisions in one `data/versions.json`
     and generate `_versions.hpp` / `_data-version.hpp` from it.
   - Generate block, item, and entity/alias tables from the Java server `--reports` data and
     the Bedrock/Chunker identifier tables, then check the generated headers in CI.
   - Add a scheduled version-watch workflow that regenerates the tables, runs the suite, and
     opens a PR with a changelog. Semantic mappings still need manual review.

## References

- [Chunker](https://github.com/HiveGamesOSS/Chunker) by Hive Games (MIT) is the reference
  implementation used for the latest Java/Bedrock layouts, version constants, and biome IDs.
  It is not bundled here.
- [Minecraft Wiki](https://minecraft.wiki/) is authoritative for Bedrock protocol versions and
  numeric biome IDs (for example `Biome/ID` and `Bedrock_Edition_26.63`).
- Upstream core: [kbinani/je2be-core](https://github.com/kbinani/je2be-core).
