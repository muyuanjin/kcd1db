# kcd1db

LuaDB for Kingdom Come: Deliverance. It provides SQLite storage for Lua mods with save-local and cross-save data.

## Install

Use the Windows x64 Steam build `1.9.8-404-504czj4` (Steam BuildID `22623230`). Put `kcd1db.asi` and the x64 Ultimate ASI Loader `dinput8.dll` next to `KingdomCome.exe` in `Bin/Win64`. Reuse the loader if it is already installed. Launch the game through Steam. [Install and troubleshoot](docs/install.md)

The game root holds `kcd1db.db` and `kcd1db.log`. A successful startup logs `LuaDB and DB registered`.

## Lua API

```lua
local db = DB.Create("MyMod")

db.L.progress = { chapter = 2, visited = true }
db.G.settings = { volume = 0.8 }
assert(db:Get("progress").chapter == 2)

db.L.progress = nil  -- Delete the value when saving the next snapshot.
```

- `DB.Create("MyMod")` and `DB("MyMod")` create a namespace. Values are encoded as JSON using the game's `Scripts/JSON/json.lua`.
- `Set`, `Get`, `Del`, `Exi`, and `All` operate on save-local data. The plugin commits a snapshot after a successful game save and restores the snapshot associated with the loaded save version. Keep `kcd1db.db` with your game-save backups if you want to restore their Lua data.
- `SetG`, `GetG`, `DelG`, `ExiG`, and `AllG` operate on cross-save data. The plugin writes pending changes during game updates and on shutdown.
- The raw `LuaDB` API uses the same method names and accepts booleans, numbers, and strings. `Set` and `SetG` return `false` for keys or string values containing a NUL byte. `LuaDB.Dump()` logs both caches to `kcd1db.log`.
- To make the native plugin optional, bundle `kcd1db_fake_db.lua` in your mod. It uses in-memory storage when the native plugin is absent. [Fake DB integration](docs/fake-db.md)

## Build

Use CMake 3.30+ and MSVC x64.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The plugin is built at `build/Release/kcd1db.asi`.

GitHub Actions builds checked-in code and packages `kcd1db.zip` and `kcd1db_fake_db.lua`. A `v*.*.*` tag publishes those files to GitHub Releases. [Build and release workflow](.github/workflows/build.yml)

## Compatibility

Engine addresses are pinned to the Steam `WHGame.dll` with PE timestamp `0x69CCD815` and image size `0x039EB000`. The plugin checks both fields and the original framework method pointers before replacing them.

Game saves created without a snapshot ID start with an empty save-local cache. This includes saves from the development build that indexed snapshots by filename. Global records are still loaded from `kcd1db.db`.

See the [manual test guide](docs/manual-testing.md) and [engine integration notes](docs/engine-integration.md) when validating a build or updating engine addresses.
