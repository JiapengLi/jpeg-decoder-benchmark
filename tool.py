import fire
import os
from PIL import Image, ImageDraw
import numpy as np

def gen(W=None, H=None, num=1):
    for _ in range(num):
        _gen(W, H)

def _gen(W=None, H=None):
    # Canvas size
    if W is None:
        W = int.from_bytes(os.urandom(8), 'big') % 320
    if H is None:
        H = int.from_bytes(os.urandom(8), 'big') % 240

    W = 2 if W < 2 else W
    H = 2 if H < 2 else H

    img = Image.new("RGB", (W, H), (0, 0, 0))  # background (black, but will be fully covered)
    draw = ImageDraw.Draw(img)

    # Define 4 colors: R, G, B, W
    colors = [(255,0,0), (0,255,0), (0,0,255), (255,255,255)]

    # Compute half dimensions
    half_W, half_H = W // 2, H // 2

    # Define 4 quadrants (equal rectangles)
    rects = [
        (0, 0, half_W, half_H),           # Top-left
        (half_W, 0, W, half_H),           # Top-right
        (0, half_H, half_W, H),           # Bottom-left
        (half_W, half_H, W, H)            # Bottom-right
    ]

    # Draw the 4 rectangles
    for rect, color in zip(rects, colors):
        draw.rectangle(rect, fill=color)

    # Save the image
    tab = ['4:0:0', '4:4:4', '4:2:2', '4:2:0']

    for s in tab:
        filename = f'rgbw_{W}x{H}_{s.replace(":", "")}.jpg'
        if s == '4:0:0':
            img.convert('L').save(filename, "JPEG", quality=95, optimize=True)
        else:
            img.save(filename, "JPEG", quality=95, subsampling=s, optimize=True)


    print(f"Image saved as {filename}")

def rgen(path, width, height, subsampling=0, quality= 95, seed= None):
    """
    Generate a random RGB JPEG image and save it.

    Args:
        path (str): Output file path, e.g. "random.jpg".
        width (int): Image width in pixels.
        height (int): Image height in pixels.
        quality (int): JPEG quality (1–100, default=95).
        seed (int | None): Optional random seed for reproducibility.
    """
    rng = np.random.default_rng(seed)
    arr = rng.integers(0, 256, size=(height, width, 3), dtype=np.uint8)
    img = Image.fromarray(arr, mode="RGB")
    img.save(path, "JPEG", quality=quality, subsampling=subsampling, optimize=True)


if __name__ == "__main__":
    fire.Fire({
        "gen": gen,
        "rgen": rgen,
    })
