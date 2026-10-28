import bpy
from PIL import Image
import os

tex_dir = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\textures"

for fname in os.listdir(tex_dir):
    fpath = os.path.join(tex_dir, fname)
    if fname.endswith(".png"):
        im = Image.open(fpath)
        print(f"File: {fname}, size: {im.size}, mode: {im.mode}")
        # Get thumbnail / average color
        im_thumb = im.resize((16, 16))
        # check alpha if RGBA
        if im.mode == "RGBA":
            extrema = im.getextrema()
            print(f"  Extrema: R={extrema[0]}, G={extrema[1]}, B={extrema[2]}, A={extrema[3]}")
