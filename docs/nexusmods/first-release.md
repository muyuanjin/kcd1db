# kcd1db on Nexus Mods

- Game — Kingdom Come: Deliverance
- Mod name — kcd1db - Lua Mod Persistence
- Suggested category — Modders Resources
- Version — `0.1.0`
- Main file — `kcd1db.zip`, manual download. [File description](files/0.1.0.txt)
- Cover — [kcd1db cover image](cover.png)
- Summary — SQLite storage for Kingdom Come: Deliverance Lua mods, with data tied to individual saves and settings shared across saves.

## Description BBCode

```text
kcd1db gives Lua mods for Kingdom Come: Deliverance persistent storage through the DB and LuaDB APIs. Mods can keep progress with individual saves and share settings across saves. Install it when a mod lists kcd1db as a requirement.

[b]Compatibility[/b]
Windows x64 Steam build 1.9.8-404-504czj4 (BuildID 22623230).

[b]Installation[/b]
[list=1]
[*]Close the game. Extract kcd1db.asi into Bin/Win64 beside KingdomCome.exe.
[*]Install the x64 Ultimate ASI Loader from [url=https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases]its Releases page[/url]. Place dinput8.dll beside KingdomCome.exe. Reuse the loader if it is already installed.
[*]Launch the game through Steam. The game-root kcd1db.log should report “LuaDB and DB registered”.
[/list]

[b]Backups and updates[/b]
Keep kcd1db.db with your save backups to preserve mod data. To update, close the game and replace kcd1db.asi. To uninstall, close the game and remove the ASI. Keep the database if you want to retain mod data.

[b]For mod authors[/b]
Use DB.Create("MyMod") for a mod namespace. Store save-local values in db.L and cross-save values in db.G. The optional Fake DB script provides in-memory storage for the current session when the ASI is absent. See the [url=https://github.com/muyuanjin/kcd1db#lua-api]Lua API examples[/url] and [url=https://github.com/muyuanjin/kcd1db/releases]GitHub downloads[/url].
```
