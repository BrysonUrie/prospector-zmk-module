"""Converts stage1.gif, stage2.gif, stage3.gif into the pet's frames (C source).

    python3 scripts/sprites.py --gifs DIR --out pet_sprites.c

Needs Pillow (pip install pillow). A missing GIF gets a placeholder blob; with no
--gifs at all it regenerates the module's built-in placeholder sprites.

To use your own sprites, write the output to your zmk-config's config/ directory
(e.g. --out config/pet_sprites.c). The module builds that file instead of its
placeholder whenever it exists.

Frames are cropped to the sprite, shrunk to fit MAX_SIDE px, thinned to STAGE_BYTES,
composited onto the screen background and stored as RGB565; the widget draws them at 2x.
"""

import argparse
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageSequence

MODULE = Path(__file__).resolve().parent.parent
DEFAULT_OUT = MODULE / "boards/shields/prospector_adapter/src/pet_sprites.c"

BG = (0x1A, 0x1B, 0x26)  # must match THEME_BG in include/theme.h
SHADOW = (0x0B, 0x0C, 0x12)  # darkest point of the shadow under the pet
STAGES = 3
MAX_SIDE = 56  # drawn at 2x, so up to 112px on screen; keeps stage 3 clear of the level row
STAGE_BYTES = 80_000  # flash per stage; smaller sprites keep more frames
MIN_DURATION_MS = 20
SHADOW_WIDTH_PCT = 70  # of the drawn (2x) sprite width
SHADOW_HEIGHT = 10  # px, drawn at 1x below the feet


def load_gif(path):
    frames, durations = [], []
    with Image.open(path) as im:
        for f in ImageSequence.Iterator(im):
            frames.append(f.convert("RGBA"))
            durations.append(f.info.get("duration") or 100)
    return frames, durations


def placeholder(stage):
    """A plain blob that grows each stage and bobs; stands in until a GIF is added."""
    colors = [(0x9E, 0xCE, 0x6A), (0x7D, 0xCF, 0xFF), (0xBB, 0x9A, 0xF7)]
    radius = [14, 22, 30][stage]
    size = 64
    frames = []
    for bob in [0, 2, 3, 2]:
        im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        cx, cy = size // 2, size // 2 + bob
        d.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], fill=colors[stage])
        eye = max(2, radius // 6)
        for ex in (cx - radius // 3, cx + radius // 3):
            d.ellipse([ex - eye, cy - radius // 4 - eye, ex + eye, cy - radius // 4 + eye], fill=BG)
        frames.append(im)
    return frames, [180] * len(frames)


def sample(frames, durations, max_frames):
    """Keep at most max_frames frames; a kept frame lasts as long as the frames it replaces."""
    step = math.ceil(len(frames) / max_frames)
    kept, kept_durations = [], []
    for i in range(0, len(frames), step):
        kept.append(frames[i])
        kept_durations.append(sum(durations[i : i + step]))
    return kept, kept_durations


def fit(frames):
    """Crop every frame to the union of their visible pixels, then scale to MAX_SIDE."""
    box = None
    for f in frames:
        b = f.getchannel("A").getbbox()
        if b:
            box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3]))
    if box:
        frames = [f.crop(box) for f in frames]

    # Only shrink, so each stage keeps its own size and later stages look bigger.
    w, h = frames[0].size
    if max(w, h) <= MAX_SIDE:
        return frames
    scale = MAX_SIDE / max(w, h)
    size = (max(1, round(w * scale)), max(1, round(h * scale)))
    return [f.resize(size, Image.NEAREST) for f in frames]


def rgb565(frame):
    flat = Image.new("RGB", frame.size, BG)
    flat.paste(frame, mask=frame.getchannel("A"))
    out = bytearray()
    for r, g, b in flat.getdata():
        v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        out += bytes((v & 0xFF, v >> 8))
    return out


def shadow(sprite_w):
    """Lower half of an oval, darkest under the feet, fading to the background at the edges.

    Opaque (pre-blended onto BG) so it costs no alpha channel; drawn at 1x from the feet down,
    so the sprite's own background-filled rectangle never cuts through it.
    """
    w = max(8, 2 * sprite_w * SHADOW_WIDTH_PCT // 100)
    a, b = w / 2, SHADOW_HEIGHT
    im = Image.new("RGB", (w, SHADOW_HEIGHT), BG)
    px = im.load()
    for y in range(SHADOW_HEIGHT):
        for x in range(w):
            r2 = ((x + 0.5 - a) / a) ** 2 + ((y + 0.5) / b) ** 2
            t = max(0.0, 1 - r2) ** 1.6
            px[x, y] = tuple(round(BG[k] + (SHADOW[k] - BG[k]) * t) for k in range(3))
    return im.convert("RGBA")


def image_dsc(name, frame):
    w, h = frame.size
    return [
        f"static const uint8_t {name}_px[] = {{\n    {c_bytes(rgb565(frame))}\n}};",
        f"static const lv_image_dsc_t {name} = {{",
        "    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,",
        f"               .w = {w}, .h = {h}, .stride = {w * 2}}},",
        f"    .data_size = sizeof({name}_px),",
        f"    .data = {name}_px,",
        "};",
        "",
    ]


def c_bytes(data):
    rows = [", ".join(f"0x{b:02x}" for b in data[i : i + 16]) for i in range(0, len(data), 16)]
    return ",\n    ".join(rows)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gifs", type=Path, help="directory with stage1.gif, stage2.gif, stage3.gif")
    ap.add_argument("--out", type=Path, default=DEFAULT_OUT, help="C file to write (default: the module's placeholder)")
    args = ap.parse_args()

    parts = [
        "/* Generated by scripts/sprites.py. Do not edit. */",
        "#include <pet_sprites.h>",
        "",
    ]
    stage_entries = []
    for stage in range(STAGES):
        gif = args.gifs / f"stage{stage + 1}.gif" if args.gifs else None
        found = gif is not None and gif.exists()
        frames, durations = load_gif(gif) if found else placeholder(stage)
        source = gif.name if found else "placeholder"
        frames = fit(frames)
        w, h = frames[0].size
        frames, durations = sample(frames, durations, max(1, STAGE_BYTES // (w * h * 2)))

        names = []
        for i, f in enumerate(frames):
            name = f"s{stage}f{i}"
            names.append(name)
            parts += image_dsc(name, f)
        parts += image_dsc(f"s{stage}_shadow", shadow(w))
        ms = ", ".join(str(min(0xFFFF, max(MIN_DURATION_MS, d))) for d in durations)
        parts += [
            f"/* stage {stage + 1}: {source}, {len(frames)} frames, {w}x{h} */",
            f"static const lv_image_dsc_t *const s{stage}_frames[] = {{{', '.join('&' + n for n in names)}}};",
            f"static const uint16_t s{stage}_durations[] = {{{ms}}};",
            "",
        ]
        stage_entries.append(
            f"    {{.frame_count = {len(frames)}, .frames = s{stage}_frames, .durations_ms = s{stage}_durations,"
            f" .shadow = &s{stage}_shadow}},"
        )
        print(f"pet stage {stage + 1}: {source}, {len(frames)} frames, {w}x{h}")

    parts += ["const struct pet_sprite_stage pet_sprite_stages[PET_STAGE_COUNT] = {", *stage_entries, "};", ""]
    text = "\n".join(parts)
    if not args.out.exists() or args.out.read_text() != text:
        args.out.write_text(text)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
