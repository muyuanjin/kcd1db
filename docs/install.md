# Install kcd1db

kcd1db supports the Windows x64 Steam build of Kingdom Come: Deliverance `1.9.8-404-504czj4` (BuildID `22623230`).

1. Close the game and open `KingdomComeDeliverance/Bin/Win64`, the folder containing `KingdomCome.exe`.
2. Extract both files from `kcd1db.zip` into that folder. If it already contains `dinput8.dll`, keep the existing loader and extract `kcd1db.asi`.
3. Launch the game through Steam. Check `kcd1db.log` in the game root for `LuaDB and DB registered` from the current launch.

To update, close the game and replace `kcd1db.asi`. To uninstall, close the game and remove `kcd1db.asi`. Keep `kcd1db.db` in the game root if you want to retain your Lua mods' data.

The ZIP bundles [Ultimate ASI Loader for Windows x64](https://github.com/ThirteenAG/Ultimate-ASI-Loader). License notices for the distributed binaries are available as `kcd1db-licenses.txt` with the [GitHub Releases](https://github.com/muyuanjin/kcd1db/releases).

## Troubleshooting

- If `kcd1db.log` is missing, check that the ASI loader and `kcd1db.asi` are in `Bin/Win64` alongside `KingdomCome.exe`.
- If the log reports an unsupported `WHGame.dll`, check the displayed build number in the game menu. Report the build number and the relevant lines from `kcd1db.log` when requesting support for a new build.
