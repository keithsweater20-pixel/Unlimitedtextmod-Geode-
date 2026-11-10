# FPS Optimizer (Geode mod, v0.1.0)

Target: Geometry Dash 2.208, Windows.

> This version has **not been compiled or tested**. It was written without access to the Geode SDK. Expect to fix a few hook signatures on the first build.

## Build

1. Install the Geode SDK and set the `GEODE_SDK` environment variable.
2. Check `"geode"` in `mod.json` matches your installed Geode version.
3. `cmake -B build` then `cmake --build build --config RelWithDebInfo`.
4. Put the resulting `.geode` file in your Geode `mods` folder.

## What works in v0.1.0

- **Max FPS / Min FPS / Target FPS** settings. Max is stored as a signed 32-bit value (largest 2147483647). 0 or anything at or above 100000 means no cap.
- **Adaptive optimization**: switches savings up when FPS falls below the floor (Min FPS, or Target FPS if Min is 0) and eases them off again with a buffer so it doesn't flicker.
- **Disable particles** and **Disable trails** (manual toggles, and the two adaptive levels).
- **Force FPS (experimental)**: strongest savings at once, 5 warnings, cancelling any of them turns it back off, always starts off each launch.
- **Extreme mode**: only active while Force FPS is on. Removes the cap.
- **FPS overlay** with real FPS, frame time and savings level.
- **Fake FPS display** (display only, optional "(fake)" tag). Presets: eps0, omega, knux, inf, 9001, NaN, or any custom text. The game's fonts lack some Greek symbols, so presets use plain letters.

## The 30 options

| # | Option | Status |
|---|--------|--------|
| 1 | Cull off-screen objects | planned |
| 2 | Throttle off-screen group updates | planned |
| 3 | Throttle move/rotate/scale triggers | planned |
| 4 | Skip hidden or transparent objects | planned |
| 5 | Pause off-screen animations | planned |
| 6 | Lower collision-check frequency for decoration | planned |
| 7 | Throttle pulse and color triggers | planned |
| 8 | Skip invisible helper objects | planned |
| 9 | Reduce spawn-trigger overhead | planned |
| 10 | Cap per-frame object updates | planned |
| 11 | Disable particles | **done** |
| 12 | Reduce particle count | planned |
| 13 | Disable glow | planned |
| 14 | Disable additive-blend effects | planned |
| 15 | Disable shader effects | planned |
| 16 | Lower render scale | planned |
| 17 | Disable background and ground animation | planned |
| 18 | Disable trails | **done** (icon trails only, not wave) |
| 19 | Simplify player icon effects | planned |
| 20 | Reduce transparency overdraw | planned |
| 21 | Frame pacing | planned |
| 22 | Smart VSync handling | planned |
| 23 | Adaptive optimization | **done** |
| 24 | Disable screen shake and camera effects | planned |
| 25 | Reduce menu and UI redraws | planned |
| 26 | Preload textures | planned |
| 27 | Lower texture quality | planned |
| 28 | Disable audio visualizer processing | planned |
| 29 | Free unused sprite and level memory | planned |
| 30 | Disable popup animations | planned |

Also planned: frame-time bottleneck readout (CPU vs GPU), a panic-off key for Force FPS, and the jump improvement feature (currently an inert placeholder setting).

## Notes

- To go above your monitor's refresh rate you also need VSync off in GD's video settings.
- Real FPS is always what the optimizer uses. The fake FPS text never feeds into it.
- Extremely high frame rates can cause physics glitches in GD, so don't use Extreme mode in anything you care about.
