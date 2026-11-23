# Optional Fake DB

Bundle `kcd1db_fake_db.lua` from the `dist/` directory or the release assets in your mod's PAK. Use a directory unique to your mod and load the file from your Lua entry script before creating a database. Include kcd1db's MIT license notice with your mod.

```lua
Script.ReloadScript("Scripts/MyMod/kcd1db_fake_db.lua")
local db = DB.Create("MyMod")
```

With the plugin present, `DB` uses SQLite. Otherwise, `DB` uses memory for the current Lua session. Both `L` and `G` reset when the game process ends. The in-memory `L` cache is shared across save switches in the same session.

`DB` uses the game's `Scripts/JSON/json.lua` to encode values. `DB.Set("x", nil)` stores JSON `null`, so `DB.Get("x")` returns `nil` and `DB.Exi("x")` returns `true`. Use `DB.Del("x")` or `db.L.x = nil` to delete a key.

Check `KCD1DB.persistent` if your mod needs to indicate whether a value will survive a restart. `KCD1DB.native`, `KCD1DB.fake` and `KCD1DB.backend` identify the active backend.
