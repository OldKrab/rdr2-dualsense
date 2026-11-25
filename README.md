# DualSense for RDR2

Full DualSense support for Red Dead Redemption 2 on PC — adaptive triggers per weapon, voice-coil haptics,
dialogue you can feel (experimental), lightbar, speaker and microphone (beta), driven by what the game is actually
doing (ScriptHookRDR2 `.asi` mod).

Players: download it from **[Nexus Mods](https://www.nexusmods.com/reddeadredemption2/mods/11444)** or the
[GitHub release](https://github.com/OldKrab/rdr2-dualsense/releases/latest), and read `release/README.txt`.

## Repository layout
| Path | What |
|---|---|
| `mod/` | The mod (C++20, CMake + MinGW-w64). `mod/README.md`: build, install, feature table. |
| `mod/src/` | Controller I/O (HID, WASAPI haptics & speaker), trigger engine, game state (natives), features, voice/whistle. |
| `tools/trigger_lab/` | Python lab to design and test trigger effects and haptic recipes outside the game; `data/` holds `weapons.json` and `haptics.json` (`171 recipes`). |
| `tools/awc/` | Game sound bank tools (AWC decoding, sound / material name recovery, the speech voice table) and the read-only live-memory research scripts behind the dialogue haptics. |
| `tools/dev/` | Release script, ini updater, one-off patch scripts. |
| `release/` | Player README and mod page text. |

## Build
```powershell
cmake -S mod -B mod/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build mod/build
.\mod\build\dsr2_test.exe --selftest
.\mod\build\test_reload\dsr2_reload_test.exe
powershell -ExecutionPolicy Bypass -File tools/dev/make_release.ps1
```

## Contributors
- [TochaRude](https://github.com/TochaRude): the bow, lasso, fists, mounting, trains, weather, carrying and player LED modes of 1.4.0, the speaker's game sounds, and many fixes.

## License
MIT — see `LICENSE`. Uses nlohmann/json (MIT) and stb_vorbis (public domain).
