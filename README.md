# Special Characters (Geode mod, v0.1.0)

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

## Why two halves

1. **Typing.** GD text fields only accept characters on an allow list, so the symbols can't be typed directly. The mod watches what you type and swaps a finished code for the real symbol. In fields that clearly take text, it also lets the backslash through.
2. **Showing.** GD's fonts contain no Greek or math glyphs, so a symbol would simply be invisible. Any label containing one of the supported symbols is drawn with a bundled fallback font (`specialFont`, made from DejaVu Sans Bold). Labels without symbols are untouched.

## Known limits

- **The fallback font looks different** from GD's own fonts (different style, and the size may be a bit off). A label containing a symbol is drawn entirely in the fallback font. I couldn't tune the size without the game. To try another size: `python3 tools/make_font.py "" 28` (needs Pillow and the DejaVu font).
- Only fields that already accept letters and numbers get the backslash; number-only fields (like ID boxes) are left alone.
- Level text objects and fields that go through GD's normal text input and label code should work. Anything GD draws some other way (for example text baked into a sprite) won't.
- Symbols other than the ones listed are not in the fallback font.

## Build

Push this project to a GitHub repo. The workflow in `.github/workflows/build.yml` builds it on every push. Open the Actions tab, download **Build Output** from the finished run, and put the `.geode` file in your `geode/mods` folder.

To build locally instead: install the Geode SDK, set `GEODE_SDK`, then `cmake -B build` and `cmake --build build --config RelWithDebInfo`.

## Credits

The fallback font is rendered from DejaVu Sans Bold. DejaVu fonts are derived from Bitstream Vera and may be freely used and embedded (https://dejavu-fonts.github.io/License.html).
