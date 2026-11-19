# Draws TrickLogic2's icon, original art: an open book and a magnifying glass,
# at 1024x1024. icon.png is it scaled to 256x256 with Lanczos, and
# macos/TrickLogic2.icns is made from it with iconutil.
#   python3 packaging/make_icon.py icon-1024.png
import sys
from PIL import Image, ImageDraw, ImageFilter
N = 1024; SS = 2; W = N * SS
img = Image.new('RGBA', (W, W), (0, 0, 0, 0))
# Background: rounded square, vertical burgundy gradient.
bg = Image.new('RGBA', (W, W))
top, bottom = (128, 22, 34), (52, 8, 16)
d = ImageDraw.Draw(bg)
for y in range(W):
    t = y / (W - 1)
    d.line([(0, y), (W, y)], fill=tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3)) + (255,))
mask = Image.new('L', (W, W), 0)
m = 100 * SS
ImageDraw.Draw(mask).rounded_rectangle([m, m, W - m, W - m], radius=180 * SS, fill=255)
img.paste(bg, (0, 0), mask)
d = ImageDraw.Draw(img)
s = SS
# Open book: two pages meeting at a spine, slightly fanned.
cx, base = 512 * s, 700 * s
cream, shade, line = (246, 236, 214, 255), (220, 204, 176, 255), (150, 120, 90, 255)
left = [(cx, 360 * s), (250 * s, 320 * s), (250 * s, base - 40 * s), (cx, base)]
right = [(cx, 360 * s), (774 * s, 320 * s), (774 * s, base - 40 * s), (cx, base)]
d.polygon(left, fill=cream); d.polygon(right, fill=shade)
d.line([(cx, 360 * s), (cx, base)], fill=line, width=6 * s)
for i in range(5):
    y = 400 * s + i * 52 * s
    d.line([(295 * s, y - 8 * s), (470 * s, y + 8 * s)], fill=line, width=7 * s)
    d.line([(555 * s, y + 8 * s), (729 * s, y - 8 * s)], fill=line, width=7 * s)
# Magnifying glass over the right page.
gold, gold_dark = (232, 186, 82, 255), (160, 112, 40, 255)
gx, gy, r = 610 * s, 560 * s, 150 * s
d.line([(gx + 100 * s, gy + 100 * s), (gx + 250 * s, gy + 250 * s)], fill=gold_dark, width=64 * s)
d.line([(gx + 100 * s, gy + 100 * s), (gx + 245 * s, gy + 245 * s)], fill=gold, width=46 * s)
lens = Image.new('RGBA', (W, W), (0, 0, 0, 0))
ImageDraw.Draw(lens).ellipse([gx - r, gy - r, gx + r, gy + r], fill=(255, 255, 255, 70))
img.alpha_composite(lens)
d = ImageDraw.Draw(img)
d.ellipse([gx - r, gy - r, gx + r, gy + r], outline=gold_dark, width=44 * s)
d.ellipse([gx - r + 6 * s, gy - r + 6 * s, gx + r - 6 * s, gy + r - 6 * s], outline=gold, width=30 * s)
d.arc([gx - r + 50 * s, gy - r + 50 * s, gx + r - 50 * s, gy + r - 50 * s], 200, 260, fill=(255, 255, 255, 200), width=14 * s)
img = img.resize((N, N), Image.LANCZOS)
img.save(sys.argv[1])
