# Repository Guidelines

## Project Structure

- `src/Plugin.cpp` contains the Windows ASI entry point, game hooks, and native `LuaDB` bindings. Engine addresses are tied to the supported Steam build listed in `README.md`.
- `src/LuaDB.cpp` and `src/LuaDB.h` implement SQLite persistence for save-local snapshots and cross-save data.
- `src/db_lua.h` embeds the public `DB` wrapper. `dist/kcd1db_fake_db.lua` is the standalone in-memory fallback for mods. Review both when changing shared Lua API behavior, and update the examples in `README.md` when usage changes.

## Build and Run

Use CMake 3.30+ and MSVC x64. Run these commands from the repository root.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The first command configures the project and fetches SQLiteCpp and MinHook. The second produces `build/Release/kcd1db.asi`. Install it alongside the x64 Ultimate ASI Loader (`dinput8.dll`) and `KingdomCome.exe` in the game's `Bin/Win64`, then launch through Steam.

## Coding Style

Use four spaces in C++ and Lua. Follow the existing C++20 style with braces on separate lines for multiline blocks. Use `snake_case` for internal helpers, `kPascalCase` for named constants in the plugin, and `m_` for `LuaDB` data members. Keep public API casing consistent with methods such as `DB.Create` and `SetG`. CMake enables MSVC's `/W4` warnings.

## Manual Testing

Use the [manual test guide](docs/manual-testing.md) for changes to save snapshots, global persistence, or the Fake DB wrapper. When changing engine addresses, use the supported build and trace the calls described in the [engine integration notes](docs/engine-integration.md).

## Commits and Pull Requests

Use short, imperative commit subjects such as `Fix save-local snapshot loading`. In the pull request, describe the affected behavior, link any related issue, and include the game build and manual checks performed for runtime changes.

## Nexus Mods Publishing

Use the [kcd1db Nexus publishing skill](.agents/skills/kcd1-nexus-publishing/SKILL.md) for first releases, file updates, upload configuration, and mod page maintenance.
