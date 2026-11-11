#!/usr/bin/env python3
"""Generates resources/specialFont.fnt + specialFont.png (a BMFont) from DejaVu Sans Bold.

DejaVu fonts are free to embed (Bitstream Vera license, see README).
Run:  python3 tools/make_font.py [path/to/DejaVuSans-Bold.ttf] [pixel size]
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

FONT = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
SIZE = int(sys.argv[2]) if len(sys.argv) > 2 else 32
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources")
NAME = "specialFont"

# Keep in sync with isSpecialCodepoint() in main.cpp.
CODEPOINTS = (
    list(range(0x20, 0x7F))                 # ASCII, so whole labels can use this font
    + list(range(0x391, 0x3AA))             # capital Greek (incl. Omega)
    + list(range(0x3B1, 0x3CA))             # lowercase Greek (incl. omega, epsilon)
    + list(range(0x2080, 0x208A))           # subscript digits (the "naught" in e0)
    + [0x2135, 0x221E]                      # aleph, infinity
)
CODEPOINTS = [c for c in CODEPOINTS if c != 0x3A2]  # unassigned

font = ImageFont.truetype(FONT, SIZE)
ascent, descent = font.getmetrics()
line_height = ascent + descent

# Refuse to ship a font that silently draws "missing glyph" boxes.
missing_mask = bytes(font.getmask("￿"))
for cp in CODEPOINTS:
    if cp > 0x20 and bytes(font.getmask(chr(cp))) == missing_mask:
        sys.exit(f"font has no glyph for U+{cp:04X}")

glyphs = []
for cp in CODEPOINTS:
    ch = chr(cp)
    left, top, right, bottom = font.getbbox(ch, anchor="ls")  # relative to baseline origin
    w, h = max(0, right - left), max(0, bottom - top)
    if h == 0:  # blank glyph (space): no ink, only an advance
        w, left, top = 0, 0, 0
    glyphs.append({
        "id": cp, "ch": ch, "l": left, "t": top, "w": w, "h": h,
        "adv": int(round(font.getlength(ch))),
    })

PAD = 2
cell_w = max(g["w"] for g in glyphs) + PAD * 2
cell_h = max(g["h"] for g in glyphs) + PAD * 2
tex_w = 1024
cols = tex_w // cell_w
rows = math.ceil(len(glyphs) / cols)
tex_h = 1
while tex_h < rows * cell_h:
    tex_h *= 2

alpha = Image.new("L", (tex_w, tex_h), 0)
draw = ImageDraw.Draw(alpha)
for i, g in enumerate(glyphs):
    g["x"] = (i % cols) * cell_w + PAD
    g["y"] = (i // cols) * cell_h + PAD
    if g["w"] and g["h"]:
        draw.text((g["x"] - g["l"], g["y"] - g["t"]), g["ch"], font=font, fill=255, anchor="ls")

# White glyphs with alpha, so GD can tint them like any other label.
rgba = Image.new("RGBA", (tex_w, tex_h), (255, 255, 255, 0))
rgba.putalpha(alpha)
os.makedirs(OUT_DIR, exist_ok=True)
rgba.save(os.path.join(OUT_DIR, f"{NAME}.png"), optimize=True)

lines = [
    f'info face="{NAME}" size={SIZE} bold=1 italic=0 charset="" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=1,1 outline=0',
    f"common lineHeight={line_height} base={ascent} scaleW={tex_w} scaleH={tex_h} pages=1 packed=0",
    f'page id=0 file="{NAME}.png"',
    f"chars count={len(glyphs)}",
]
for g in glyphs:
    lines.append(
        f"char id={g['id']} x={g['x']} y={g['y']} width={g['w']} height={g['h']} "
        f"xoffset={g['l']} yoffset={ascent + g['t']} xadvance={g['adv']} page=0 chnl=15"
    )
with open(os.path.join(OUT_DIR, f"{NAME}.fnt"), "w", newline="\n") as f:
    f.write("\n".join(lines) + "\n")

print(f"{len(glyphs)} glyphs, atlas {tex_w}x{tex_h}, lineHeight {line_height}, base {ascent}")
