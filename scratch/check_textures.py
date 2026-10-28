import bpy
import os

tex_dir = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\textures"

for f in os.listdir(tex_dir):
    p = os.path.join(tex_dir, f)
    img = bpy.data.images.load(p)
    w, h = img.size[0], img.size[1]
    ch = img.channels
    has_alpha = img.is_float
    pixels = list(img.pixels[:40]) # first 10 pixels (RGBA)
    print(f"File: {f}")
    print(f"  Dimensions: {w}x{h}, Channels: {ch}")
    print(f"  First 4 pixels (RGBA): {pixels[:16]}")
    # Sample alpha values across image
    # Sample every 1000th pixel alpha
    alpha_samples = [img.pixels[i*4 + 3] for i in range(0, min(10000, w*h), 50)]
    min_a = min(alpha_samples)
    max_a = max(alpha_samples)
    avg_a = sum(alpha_samples) / len(alpha_samples)
    print(f"  Alpha sample stats: min={min_a:.2f}, max={max_a:.2f}, avg={avg_a:.2f}")
    bpy.data.images.remove(img)
