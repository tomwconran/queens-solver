import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageGrab


def load_image(args: list[str]) -> Image.Image:
    if len(args) > 1:
        return Image.open(args[1])
    img = ImageGrab.grabclipboard()
    if not isinstance(img, Image.Image):
        sys.exit(
            "no image on the clipboard: take a screenshot with "
            "Cmd+Ctrl+Shift+4, or pass an image path"
        )
    return img


img_array = np.asarray(load_image(sys.argv).convert("RGB")).astype(int)
light_mask = img_array.sum(axis=2) > 300

H, W = light_mask.shape

light_rows = np.nonzero(light_mask.sum(axis=1) > W / 2)[0]
light_columns = np.nonzero(light_mask.sum(axis=0) > H / 2)[0]
top, bottom, left, right = (
    light_rows[0],
    light_rows[-1],
    light_columns[0],
    light_columns[-1],
)

grid_lines = light_mask[top : bottom + 1, left : right + 1].mean(axis=0) < 0.5
n = (np.diff(grid_lines.astype(int)) == 1).sum() + 1

cell_w = (right - left + 1) / n
cell_h = (bottom - top + 1) / n

colours = np.zeros((n, n, 3))

for r in range(n):
    for c in range(n):
        centre_x = left + (c + 0.5) * cell_w
        centre_y = top + (r + 0.5) * cell_h
        colours[r, c] = img_array[
            int(centre_y - cell_h / 5) : int(centre_y + cell_h / 5),
            int(centre_x - cell_w / 5) : int(centre_x + cell_w / 5),
        ].mean(axis=(0, 1))

THRESHOLD = 10

region_colours = []
regions = np.zeros((n, n), dtype=int)

for r in range(n):
    for c in range(n):
        colour = colours[r, c]
        if region_colours:
            distances = np.linalg.norm(np.array(region_colours) - colour, axis=1)
            nearest = int(np.argmin(distances))
            if distances[nearest] < THRESHOLD:
                regions[r, c] = nearest
                continue
        region_colours.append(colour)
        regions[r, c] = len(region_colours) - 1
if len(region_colours) != n:
    print(f"error: found {len(region_colours)} regions, expected {n}")
    sys.exit(1)


rows = ["".join(chr(ord("A") + regions[r, c]) for c in range(n)) for r in range(n)]
puzzle_text = f"{n}\n" + "\n".join(rows) + "\n"

solver = Path(__file__).parent.parent / "build" / "queens"
subprocess.run(
    [solver, "/dev/stdin", "hybrid"], input=puzzle_text, text=True, check=True
)
