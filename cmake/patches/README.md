# Dependency patches

## `libminecraft-file-26.x.patch`

Applied to [kbinani/libminecraft-file][lib] at the commit pinned in `CMakeLists.txt`
(`6f00d2c6`), which is the tip of its `master` at the time of writing. The repository has
no tags, so bumping the pin is not an option; the patch is carried here until the fix
lands upstream.

It exists because a Minecraft Java world saved by **26.1 or later cannot be read at all**
without it. Three separate things break:

1. **Chunk block palette (26.3, snapshot 7).** `ChunkSection118::MakeChunkSection` required
   a palette entry to be a compound with a `Name` string and an optional `Properties`
   compound. 26.3 renamed those keys to `id` and `properties`, made `properties` optional
   for the default block state, allowed the block resource location under an empty key, and
   allowed the whole palette to be a *list of strings*. The reader returned `nullptr` for
   any entry that did not match the old shape, and `nullptr` here means **the entire
   16×16×16 section is dropped** — a 26.3 world converted without this patch loses all of
   its blocks. The patch accepts both the old and the new shapes.

   Reference: the `Chunk_format` history entry for `26.3 dev=snap7` — "Block palette fields
   `Name` and `Properties` renamed to `id` and `properties`, respectively. The block
   properties may be elided if it is the default block state."

2. **Unknown biome names.** The biome palette is a list of strings; `Biome::FromName`
   `assert`s and returns `nullopt` for a name it does not know, and the caller treated that
   as "drop the section" — the same silent total data loss as above, triggered by a mere
   biome added after the header was written. 26.2 adds `sulfur_caves` and 26.3 adds
   `dappled_forest`, so every 26.2/26.3 world hits this. The patch keeps the section and
   falls back to `plains`.

3. **The biomes themselves.** `sulfur_caves` and `dappled_forest` are added to the biome
   enum, the name tables, and the Bedrock mapping. Bedrock has neither biome, so
   `mcfile::be::Biome::ToUint32` maps `sulfur_caves` to `dripstone_caves` (188) and
   `dappled_forest` to `forest` (4) — the nearest existing Bedrock biomes.

### Not covered

The patch makes 26.x worlds *readable*; it does not teach libminecraft-file the ~120 blocks
and items added in 1.21.11 → 26.3. Those blocks therefore resolve to
`mcfile::blocks::unknown` and are converted by name instead of by id, which je2be-core
supports (see the unknown-block handling in `src/java-block-data.cpp`). The practical
consequences are:

- JE → BE: a new block is written with its Java name and no states. Bedrock 1.21.120 has no
  such block, so it will not render. This is inherent — Bedrock has no poplar wood or wool
  stairs to map to.
- Block heightmaps: new blocks are not classified in `je2be/heightmaps.hpp`.

Adding the block ids is mechanical but needs the per-block heightmap classification, so it
is deliberately left as follow-up work.

[lib]: https://github.com/kbinani/libminecraft-file
