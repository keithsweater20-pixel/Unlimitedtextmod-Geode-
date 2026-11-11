# Song Select Guard (Geode mod, v0.1.0)

Target: Geometry Dash 2.2081, Windows, Geode 5.11.2.

> **Untested.** It has not been run in the game. It is a guard, not a fix.

## What it does

Hooks `SongSelectNode::onSongMode(int)`. If any of the buttons, widget, label or arrays it uses is null, it skips the call and logs a warning. Otherwise it calls the original.

## What it doesn't do

- It doesn't find which pointer is actually null. The crash log only shows the crash inside `ButtonSprite::updateBGImage`, not which caller passed the null pointer.
- The real cause is probably another mod changing the song select buttons (the crash stack includes `alphalaneous.alphas-ui-pack` and `alk.better-touch-prio`). Disabling those mods may fix it at the source.
- If the crash comes from a null pointer that isn't one of the checked ones, this guard won't catch it.
