# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

DUNGEON_AI is a single-player, turn-based, procedurally generated dungeon-crawl roguelike (tile graphics, not ASCII), written in C++17 against SDL3, with a hand-rolled entity-component-system. See the **Dungeon Codex** below for a full architecture/status review — this file is meant to be enough to orient quickly, not a replacement for it.

## Build & run

- **Windows / Visual Studio only.** `DUNGEON_AI.sln` / `DUNGEON_AI.vcxproj` is the only build definition — there is no CMakeLists.txt. Build output goes to `ouput\` (that's the actual directory name in the `.vcxproj`'s `OutDir`, typo and all — don't "fix" the typo without also updating the project file's `None Include` entries that point at `ouput\assets\*.json`).
- Dependencies are vendored, not fetched: SDL3 / SDL3_image / SDL3_ttf under `libs/`, nlohmann/json under `third_party/nlohmann`.
- Runtime config is JSON at `ouput/assets/config.json` (display, gameplay tuning, asset paths, logging), loaded via `GameConfig`. Enemy definitions: `ouput/assets/enemies.json`. Sprite sheet config: `ouput/assets/sprites.json`.
- Optional CLI arg: `--seed <n>` for a deterministic dungeon seed (see `main.cpp`).
- **No test suite and no CI exist.** The only verification available is building in Visual Studio and playing it.
- To sanity-check a change without Visual Studio (syntax only, not a real build — this is what was used to validate changes before a Windows build was available), from repo root:
  ```
  g++ -std=c++17 -fsyntax-only -fpermissive \
    -Ilibs/SDL3-3.2.20/include -Ilibs/SDL3_image-3.2.4/include -Ilibs/SDL3_ttf-3.1.0/include \
    -Ithird_party -Isrc  src/path/to/file.cpp
  ```
  Three pre-existing errors show up this way and are expected (silent under MSVC): missing `<cstdio>` in `UILayout.h` (`snprintf`), missing `<algorithm>` in `Components.h` (`std::find`, affects anything including `RenderSystem.cpp`/`SpriteUpdateSystem.cpp`), and an illegal out-of-class re-qualification in `Map.h` (`void Map::dump_sprite_grid_to_log(...)` written inside the class body). Don't treat these as caused by your change unless you touched those files.

## Architecture

- **`Game` (src/core/Game.h/.cpp) is a thin coordinator**, not where logic lives. It delegates: `GameBootstrap` (startup/wiring), `InputController` (input), `HudRenderer` (rendering), `LevelTransitionSystem` (stairs/depth changes), `TurnManager` (turn state). This is the result of a Feb 2026 refactor out of a 1000+ line `main.cpp` and a God-object `Game` class — when extending a responsibility, extend the relevant delegate, don't grow `Game` itself back out.
- **ECS core is in `src/ecs/`** (`World`, `EntityManager`, `ComponentManager`, `ComponentArray`, `System`). Entities are bare integer IDs. `Entity(0)` is *both* the real ID of the first entity created (in practice, the player) *and* an ad-hoc "no entity" sentinel used in a few places — this works by convention/luck of creation order, not by design; be careful introducing new "no entity" checks.
- **`World::update(dt)` runs every system once, but it's called from two different places for two different reasons:** once per turn, from `Game::update()`'s enemy-turn block (turn-based logic), and unconditionally every rendered frame, from `Game::render()` (because the render systems — `RenderSystem`, `MapRenderSystem`, `SpriteUpdateSystem` — live in the same systems vector and must run every frame to draw). **Any system whose effect is meant to happen once per turn must not put that logic in `update()`** — it will fire ~60x more often than intended. Follow the pattern in `MagicSystem`: `update()` is a no-op with a comment explaining why, and the real per-turn work lives in an explicitly-named method (`regenerate_mana()`) called directly from `Game::update()`'s per-turn block.
- **Ownership of `TileVisibility`** (fog-of-war/FOV state) belongs to `World` via `unique_ptr`. Everything else (`Game`, `LevelTransitionSystem`, `InputController`, render systems) holds only a non-owning raw `TileVisibility*` obtained from `World::get_tile_visibility()`. Never `delete` that pointer or replace it with raw `new` — go through `World::set_tile_visibility()` so `World` stays the single owner. (This was a double-free, fixed in commit `416edae`; the pattern exists specifically to avoid reintroducing it.)
- **Procedural generation** (`src/world/DungeonManager.h`, `MapGenerators.cpp`): `DungeonManager` picks a generator by `depth % 3` — room/corridor, Larn-style maze, or cellular-automata cave. The cave generator registers the *entire level as one `Room`*, so any code assuming `rooms.size() > 1` (stair placement, enemy placement) needs a single-room special case — see `DungeonManager::find_floor_positions()` for the pattern already in use. Generated levels are cached per depth in `LevelCache`, in memory only, for the current run — nothing persists across a restart.
- **Player HP has two components and only one is real.** `CombatStats.current_hp` is what combat, potions, and spells actually mutate. A separate `Health` component exists but nothing keeps it in sync — don't read `Health` for player alive/dead logic; it's stale. This is the current biggest correctness gap (the death check reads `Health`, so it never fires — see Known gaps).
- **Content is data-driven where it exists, hardcoded where it doesn't.** Enemies load from `enemies.json` (currently only `rat`/`goblin`/`orc`). Spells are hardcoded in `src/magic/SpellDatabase.h` (13 defined, but the player only ever learns 2 at game start — there's no in-game way to learn the rest yet). Equipment has a full 8-slot model and a working equip UI, but no code anywhere ever spawns an `EquippableItem` — the mechanism has no content to act on.

## Known gaps / where to pick up work

Full system-by-system status (Working/Partial/Stub/Broken/Missing) and a phased roadmap live in **[Dungeon Codex](https://claude.ai/code/artifact/8acc840a-6d1a-4b2f-b017-bc89a37fca2a)** — a Claude-authored review artifact, kept as the canonical reference for project state and planning. Re-read it at the start of roadmap work; ask to have it re-published if it drifts from the code. Headline items:

- **Player death doesn't produce a game-over state** (see the `Health`/`CombatStats` split above). This is the top-priority gap — most of the rest of the roadmap assumes the game loop can actually end.
- No win condition, no main menu, no save/load across restarts, no audio.
- FOV is a plain circle that sees through walls; a working recursive-shadowcast implementation already exists in `TileVisibility` but is unused.
- Status effects (cure-poison, haste) are message-only stubs with no mechanical effect.

## Repo hygiene notes

- `ouput/` (sic) is the real build output directory and has build artifacts, a runtime `game_log.txt`, and a large vendored font source tree checked into git — expected, not something to "clean up" reflexively as part of an unrelated change.
- `src.zip` at the repo root is a stale, non-canonical backup of part of `src/` — `src/` is the source of truth.
- License is GPL-3.0.
