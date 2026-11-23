# Install kcd1db

kcd1db supports the Windows x64 Steam build of Kingdom Come: Deliverance `1.9.8-404-504czj4` (BuildID `22623230`).

1. Close the game and extract `kcd1db.asi` from `kcd1db.zip` into `Bin/Win64`, next to `KingdomCome.exe`.
2. If `Bin/Win64` has no ASI loader, extract `dinput8.dll` from the x64 Ultimate ASI Loader archive into the same directory. Keep your existing loader if you already use one. [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
3. Launch the game through Steam. Check `kcd1db.log` in the game root for `LuaDB and DB registered` from the current launch.

To update, close the game and replace `kcd1db.asi`. To uninstall, close the game and remove `kcd1db.asi`. Keep `kcd1db.db` in the game root if you want to retain your Lua mods' data.

## Troubleshooting

- If `kcd1db.log` is missing, check that the ASI loader and `kcd1db.asi` are in `Bin/Win64` alongside `KingdomCome.exe`.
- If the log reports an unsupported `WHGame.dll`, check the displayed build number in the game menu. Report the build number and the relevant lines from `kcd1db.log` when requesting support for a new build.
