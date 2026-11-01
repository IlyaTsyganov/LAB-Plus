# Native Eternal UI 2 font

The native HP/GK and screen HUD font uses the bundled `Assets/fonts/EternalUi2Regular.ttf`, not a hand-drawn digit approximation.

- Family/style: Eternal UI 2 / Regular.
- TTF SHA-256: `da163693bafcbabbb720b2c28d9f08a655f736a9fb5a17b238f3d9b798583838`.
- Bake: 18 px, Pillow 12.2.0 / FreeType 2.14.3, BASIC layout with additive advances.
- Metrics: ascent 18, descent 5, line height 23. Digits have a 13 px ink height.
- Coverage: exact 8-bit grayscale, ASCII 32–126, a 256×256 R8 atlas with two transparent padding pixels between glyphs.

`Native/CollisionViewer/tools/bake_eternal_ui2_font.py --verify` reconstructs the atlas from the TTF and checks the generated `eternal_ui2_font_data.h`. `--preview build/eternal-ui2-atlas.png` also saves an atlas preview and source metadata.

The baked coverage is run-length encoded in the native source and decoded once. Runtime builds require neither Pillow nor FreeType and do not load or rasterize the TTF per frame.

`eternalUi2TextWidth` and `appendEternalUi2GlyphQuads` share the same glyph advances. Quads use screen coordinates with a descending Y axis and a baseline; the HP renderer flips Y into its camera-facing world basis. The GPU receives one textured quad per glyph plus its optional shadow. Vertex colors use straight RGBA; the fragment shader multiplies atlas red-channel coverage into alpha exactly once before SRC_ALPHA / ONE_MINUS_SRC_ALPHA blending. The world-depth check remains separate.

## CPU construction measurements

The focused native benchmark uses reusable buffers, the same current/max HP strings, and one active GK countdown for every three enemies. Maximum duration is omitted in both versions to isolate the font implementation. These are CPU geometry-construction measurements, not measured game FPS.

| Enemies | Old bitmap vertices | Eternal UI 2 vertices | Old median | New median |
| --- | ---: | ---: | ---: | ---: |
| 3 | 3,222 | 378 | 26.6 µs | 9.5 µs |
| 50 | 53,376 | 6,252 | 493.1 µs | 164.5 µs |
| 200 | 214,476 | 25,152 | 2,044.0 µs | 726.2 µs |

The logs are `build/health-font-baseline.txt` and `build/health-font-eternal-ui2.txt`. `HealthBarRenderTests.exe --benchmark-max` adds validated maximum GK durations for the final layout. The CPU projection preview `build/health-bar-render-preview.png` samples the actual atlas with bilinear coverage and straight-alpha blending; it is not a game screenshot.
