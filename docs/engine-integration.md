# KCD1 engine integration

The supported target is `Bin/Win64/WHGame.dll` from the Windows x64 Steam build `1.9.8-404-504czj4`, BuildID `22623230`. The PE timestamp is `0x69CCD815` and `SizeOfImage` is `0x039EB000`. All code addresses below are RVAs from the loaded `WHGame.dll` base. Offsets prefixed with `+` are byte offsets within objects or vtables.

| Entry | Anchor and behavior |
| --- | --- |
| `CCryAction` primary vtable `0x02734E30` | Locate the MSVC RTTI name `.?AVCCryAction@@`, its complete object locator for offset zero, and the primary vtable following its locator pointer. |
| Initialization `0x0186DDC4` | vtable `+0x48` at `0x02734E78`. This method uses the script-system pointer at `CCryAction +0x50` and runs `scripts/main.lua`. Lua registration occurs on this thread. |
| Shutdown `0x01895BB4` | vtable `+0x50` at `0x02734E80`. The plugin flushes pending global values at entry. |
| Listener registration `0x01892058` | `CCryAction` vtable `+0x338`. Follow the list populated by this method to the update, save and load dispatches. |
| Script system primary vtable `0x027D2488` | Locate `.?AVCScriptSystem@@` and use the table for the primary subobject. Check `ExecuteBuffer` at `+0x30`, `CreateTable` at `+0x68` and global assignment at `+0xF0`. |
| Save manager entry `0x00F12D64` | Hooked with MinHook. Returns the result of writing the serialized save. |
| Save helper vtable `0x026A2DD8` | Locate `.?AVC_CrySaveGameHelper@framework@wh@@`. Metadata is added at `+0x18` and the filename comes from `+0x40` (`0x006F85A0`). |
| Load helper vtable `0x026A2FD0` | Locate `.?AVC_CryLoadGameHelper@framework@wh@@`. Metadata is read at `+0x20` and the filename comes from `+0x48` (`0x006F8130`). |
| `lua_tolstring` `0x002B7EE4` | Trace string conversion from the script function handler. The handler points to the script system at `+0x10` and stores the argument offset at `+0x20`. The Lua state is at script system `+0x10`. The returned length covers the complete string and excludes its trailing C terminator. |

The framework save path at `0x01893C1C` calls `0x01A63678` at `0x01893E1B` and branches on its Boolean result. Trace the manager entry and verify that the `OnSaveGame` listener runs during serialization and that the manager returns after the final file write. The listener captures the local cache and writes a fresh `kcd1db_snapshot` ID into save metadata. The save hook commits the cache to SQLite after the manager returns success. The load path at `0x01886648` dispatches the load listener, which reads that ID to find the saved cache. A copied save carries its ID with it.

For a game update, identify the new executable build and rederive the above entries from RTTI, their callers, and references to `scripts/main.lua`. Recheck the original framework method pointers and save result flow before updating `src/Plugin.cpp`. If a Lua wrapper changes, validate flat and namespaced access on the actual `CScriptSystem` and `CScriptTable` objects. Test a fresh launch, two saves with different values, the older save after a restart, and both successful and failed writes to a reused path. [Manual tests](manual-testing.md)
