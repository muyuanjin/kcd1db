# kcd1db on Nexus Mods

- Game — Kingdom Come: Deliverance
- Mod name — kcd1db - Lua Mod Persistence
- Category — Utilities
- Version — `0.1.0`
- Main file — `kcd1db.zip`, manual download. [File description](files/0.1.0.txt)
- Short description — LuaDB for the first Kingdom Come: Deliverance. Gives Lua mods SQLite storage for progress tied to each save and settings shared across saves. The download includes the x64 ASI loader.

## Description BBCode

```text
[b]Install kcd1db if a mod you use lists it as a requirement.[/b] It gives Lua mods persistent storage for progress and settings.

[size=4][b]Installation[/b][/size]
[list=1]
[*]Download [b]kcd1db.zip[/b] using Manual Download and close the game.
[*]Extract [b]kcd1db.asi[/b] and [b]dinput8.dll[/b] into [b]KingdomComeDeliverance/Bin/Win64[/b], next to [b]KingdomCome.exe[/b]. If you already use an ASI loader there, keep your existing dinput8.dll.
[*]Start the game through Steam. To check the installation, open [b]kcd1db.log[/b] in the game folder and look for [b]LuaDB and DB registered[/b].
[/list]
[b]Supported game build[/b] — Kingdom Come: Deliverance for Windows x64 on Steam, version 1.9.8-404-504czj4 (BuildID 22623230).

[size=4][b]What can mods save?[/b][/size]
[list]
[*][b]Per-save data[/b] such as quest progress, tracked items or the last outfit used. When you load an earlier save, the mod sees the data associated with that version of the save.
[*][b]Shared data[/b] such as preferences and hotkeys. Settings are available across playthroughs and survive a game restart.
[*][b]Lua tables[/b] such as a list of saved outfits or a group of settings. The [b]DB[/b] wrapper handles JSON serialization and gives each mod its own namespace.
[/list]

[size=4][b]For mod authors[/b][/size]
Create a database using a unique mod name. Use [b]db.L[/b] for per-save state and [b]db.G[/b] for shared settings.
[code]local db = DB.Create("MyMod")

db.L.progress = { stage = 2, visited = true }
db.G.settings = { volume = 0.8 }

local progress = db.L.progress
local settings = db.G.settings[/code]
To make kcd1db an optional dependency, bundle [b]kcd1db_fake_db.lua[/b] and load it before [b]DB.Create[/b]. The fallback stores values for the current Lua session. See the [url=https://github.com/muyuanjin/kcd1db#lua-api]Lua API reference[/url] and [url=https://github.com/muyuanjin/kcd1db/blob/main/docs/fake-db.md]fallback guide[/url].

[size=4][b]Backups, updates and troubleshooting[/b][/size]
[list]
[*]Back up [b]kcd1db.db[/b] from the game folder with your saves to preserve the associated mod data.
[*]To update, close the game and replace [b]kcd1db.asi[/b]. To uninstall, remove the ASI. Keep the database if you plan to return.
[*]If the module does not load, check the [url=https://github.com/muyuanjin/kcd1db/blob/main/docs/install.md]installation guide[/url]. Include the game build and relevant lines from [b]kcd1db.log[/b] when reporting an issue.
[/list]

[b]Credits[/b] — The ZIP includes [url=https://github.com/ThirteenAG/Ultimate-ASI-Loader]ThirteenAG's Ultimate ASI Loader[/url] for Windows x64. kcd1db uses SQLite, SQLiteCpp and MinHook. [url=https://github.com/muyuanjin/kcd1db]Source code[/url] · [url=https://github.com/muyuanjin/kcd1db/releases/latest/download/kcd1db-licenses.txt]License notices[/url]
```
