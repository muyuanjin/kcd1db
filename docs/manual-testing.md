# Manual tests

Use the Windows x64 Steam build `1.9.8-404-504czj4` (BuildID `22623230`) and start in a disposable playline where the game allows saving. Back up the test playline and `kcd1db.db` with the game closed if you plan to reuse them. Check the game-root `kcd1db.log` for `LuaDB and DB registered` from this launch.

Open the game console with `~`. Paste each line starting with `#` as one command and press Enter. `KCD1DB … PASS` appears in the console and in the game-root `kcd.log` only after all assertions on the line pass.

## Save-local and global data

If **Save Game** is unavailable because the character has no Saviour Schnapps, run `wh_sys_NoSavePotion 1` in the console before saving.

1. Run the first command and save through the game menu. Note the time of this save so you can load it as A.

   ```lua
   #local d = DB("KCD1DBManual"); assert(d:Set("progress", {score = 101})); assert(d:SetG("setting", 1)); assert(d.L.progress.score == 101); System.LogAlways("KCD1DB A READY")
   ```

2. Run the next command and create a second save B. Keep A. The test uses `DB.Del` to check that a deleted field does not follow a different snapshot.

   ```lua
   #local d = DB("KCD1DBManual"); assert(d:Del("progress")); assert(d:Set("other", {score = 202})); assert(d:SetG("setting", 2)); System.LogAlways("KCD1DB B READY")
   ```

3. Load A and run the next command. The A snapshot should restore `progress` and replace the live save-local cache. The global setting should stay at `2`.

   ```lua
   #local d = DB("KCD1DBManual"); local p = d.L.progress; assert(p and p.score == 101); assert(d.L.other == nil); assert(d.G.setting == 2); System.LogAlways("KCD1DB A PASS")
   ```

4. Quit the game normally, relaunch, load B, and check both caches again.

   ```lua
   #local d = DB("KCD1DBManual"); local p = d.L.other; assert(p and p.score == 202); assert(d.L.progress == nil); assert(d.G.setting == 2); System.LogAlways("KCD1DB RESTART PASS")
   ```

5. Start a new game from the menu after loading B. At the first playable point, check that the new playline starts with an empty save-local cache.

   ```lua
   #local d = DB("KCD1DBManual"); assert(d.L.progress == nil and d.L.other == nil); assert(d.G.setting == 2); System.LogAlways("KCD1DB NEW GAME PASS")
   ```

## Reused exit save

1. In the test playline, set the first values using the console and choose **Save and Quit**. When the game exits, copy the playline's `exit.whs` to a backup file outside the saves directory. On a default Windows installation, playlines are under `%USERPROFILE%\Saved Games\kingdomcome\saves`.

   ```lua
   #local d = DB("KCD1DBManual"); d.L.exitMarker = 101; d.L.oldOnly = true; d.L.newOnly = nil
   ```

2. Relaunch and continue from the exit save in the same playline. Check the first value, assign the new values, and **Save and Quit** again to the same path.

   ```lua
   #local d = DB("KCD1DBManual"); assert(d.L.exitMarker == 101); d.L.exitMarker = 202; d.L.oldOnly = nil; d.L.newOnly = true
   ```

3. Relaunch and continue from that exit save. Check the second snapshot, including the deletion. Quit through the **Quit** action and restore the backed-up `exit.whs` to its original path.

   ```lua
   #local d = DB("KCD1DBManual"); assert(d.L.exitMarker == 202); assert(d.L.oldOnly == nil and d.L.newOnly == true); System.LogAlways("KCD1DB OVERWRITE PASS")
   ```

4. Relaunch and load the restored exit save. This command should pass with the original marker and the original set of keys.

   ```lua
   #local d = DB("KCD1DBManual"); assert(d.L.exitMarker == 101); assert(d.L.oldOnly == true and d.L.newOnly == nil); System.LogAlways("KCD1DB RESTORED EXIT SAVE PASS")
   ```

## Failed overwrite

Load the test playline's successful exit save and change `exitMarker` to a different value. Leave the backup outside the saves directory. In a separate PowerShell window, use the actual path of `exit.whs` and hold it open for reading while denying writes and deletion.

```powershell
$exitSave = "$env:USERPROFILE\Saved Games\kingdomcome\saves\Playline0\exit.whs"
$lockedSave = [IO.File]::Open($exitSave, 'Open', 'Read', 'Read')
try {
    Read-Host 'Press Enter after the overwrite fails or the game crashes'
} finally {
    $lockedSave.Dispose()
}
```

Attempt **Save and Quit**. Steam build `22623230` can crash inside `WHGame.dll` when Windows denies the write, including with the ASI removed. If the save function returns failure, `kcd1db.log` reports `Discarded snapshot after failed save`. If the engine crashes before returning, a snapshot for the attempted save can remain in SQLite. After releasing the lock, check that the original file matches the backup. Reload the preserved save and verify that `exitMarker` still has its previously saved value.

## SQLite write failure

Back up the test playline's `exit.whs` and load it. Change `exitMarker` and use a separate SQLite shell on the game-root `kcd1db.db` to run `BEGIN IMMEDIATE;`. Attempt **Save and Quit** while that transaction is open. The game should report a save failure and `kcd1db.log` should show `Save data write failed`. Check that `exit.whs` still matches the backup. Run `ROLLBACK;` in the SQLite shell and retry the save. The new exit save should restore the updated marker after a restart.

## Raw Lua string arguments

Run these commands with the native plugin and repeat them with the in-memory fallback. The first seeds an ordinary key and verifies that a NUL in either argument prevents the write. The second checks that access through a NUL-containing key cannot read or delete the ordinary key.

```lua
#local t = LuaDB; local k = "KCD1DBManualRaw"; local z = string.char(0); assert(t.Set(k, "keep")); assert(t.Set(k .. z .. "suffix", "wrong") == false); assert(t.Set(k, "x" .. z .. "y") == false); assert(t.Get(k) == "keep")
#local t = LuaDB; local k = "KCD1DBManualRaw"; local z = k .. string.char(0) .. "suffix"; assert(t.Get(z) == nil); assert(t.Exi(z) == false); assert(t.Del(z) == false); assert(t.Get(k) == "keep"); assert(t.Del(k)); System.LogAlways("KCD1DB RAW STRING PASS")
```

## Fake DB and reserved methods

Bundle the script and load it from a mod as described in the [Fake DB integration guide](fake-db.md). With the ASI removed from `Bin/Win64`, start the game and check `KCD1DB.backend == "memory"`. Write a local and a global key, check that both are readable, then restart and check that both keys are absent. Repeat with the ASI installed and check `KCD1DB.backend == "sqlite"`.

With each backend, run this command to exercise assignment to reserved methods and verify normal data assignment still works.

```lua
#local d = DB("KCD1DBManual"); assert(not pcall(function() DB.Get = 42 end)); assert(not pcall(function() d.Get = 42 end)); d.L.methodProbe = 17; assert(d:Get("methodProbe") == 17); d.L.methodProbe = nil; System.LogAlways("KCD1DB METHOD PROTECTION PASS")
```
