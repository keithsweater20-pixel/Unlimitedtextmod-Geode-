# Press Limit (Geode, GD 2.2081, Windows)

In the level editor, press the **Limit** button (top right), type a number, press **Make it**.
The mod places a block and triggers: after that many presses the block disappears.

Abbreviations: `E_0` epsilon naught, `l_w` lowercase omega, `a_n` aleph null, `O_w` Omega,
`^` Knuth up-arrow (`2^^5`), plus `+`, `*` and brackets.

Counters: GD item counters stop at 2147483647. The mod chains them: when counter 1 hits that, it
resets to 0 and counter 2 goes up by 1, and so on, up to 16 counters. E_0, l_w, a_n and O_w are
infinite, so they become the biggest chain possible and are never truly reached in a real level.

Untested in the real game: check the placed triggers in the editor. Unit test of the maths:
`g++ -std=c++17 tests/plan_test.cpp -o t && ./t`
