# Special Characters (Geode mod, v0.1.2)

Type ε₀, ℵ₀, ω, Ω and other Greek/math symbols in Geometry Dash text fields.
Target: Geometry Dash 2.2081, Windows, Geode 5.11.2.

> **Not compiled or tested yet.** Written without the Geode SDK. The pure typing logic was unit-tested on its own and the font was rendered and checked, but the Geode hooks have never run in the game.

## How to type

Type a backslash followed by a code. The code turns into the symbol as soon as its last character is typed.

| Type | Get |
|------|-----|
| `\e0` or `\eps0` | ε₀ (epsilon naught) |
| `\a0` or `\aleph0` | ℵ₀ (aleph null) |
| `\w` | ω (lowercase omega) |
| `\W` | Ω (omega) |
| `\alpha \beta \gamma \delta \theta \lambda \mu \pi \sigma \phi` | α β γ δ θ λ μ π σ φ |
| `\inf` | ∞ |

More codes are one line each in `kCodes` in `main.cpp`. The prefix (default `\`) can be changed in the mod settings.

## What works

- **Typing:** a backslash plus a code is swapped for the symbol as soon as the code is complete. Only single keystrokes are converted, so pasting does not change text. Codes are always added at the end of the field, not at the cursor, so mid-text typing may still misplace the symbol. Unit-tested without the game.
- **Showing:** not working yet. Symbols are typed correctly but GD's fonts have no glyph for them, so they may appear as boxes or be missing.

v0.1.0 tried to draw symbols with a bundled fallback font by swapping the label's font file. That crashed GD on startup (`CCLabelBMFont::setFntFile` during label creation), so v0.1.2 removes it.

## Known limits

- Symbols are not drawn yet (see above).
- Only fields that already accept letters and numbers get the backslash; number-only fields (like ID boxes) are left alone.
- Fields that go through GD's normal text input and label code should work. Anything GD draws some other way (for example text baked into a sprite) won't.
- Symbols other than the ones listed are not in the fallback font.

## Build

Push this project to a GitHub repo. The workflow in `.github/workflows/build.yml` builds it on every push. Open the Actions tab, download **Build Output** from the finished run, and put the `.geode` file in your `geode/mods` folder.

To build locally instead: install the Geode SDK, set `GEODE_SDK`, then `cmake -B build` and `cmake --build build --config RelWithDebInfo`.

## Credits

The fallback font is rendered from DejaVu Sans Bold. DejaVu fonts are derived from Bitstream Vera and may be freely used and embedded (https://dejavu-fonts.github.io/License.html).
