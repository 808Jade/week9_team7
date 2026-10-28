import bpy
import numpy as np

img = bpy.data.images.load(r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\textures\chell_torso_diffuse_color_1.png")
w, h = img.size[0], img.size[1]
print(f"Torso image: {w}x{h}")

# Sample pixels
pixels = np.array(img.pixels[:])
pixels = pixels.reshape((h, w, 4))

print("Center pixel RGBA:", pixels[h//2, w//2])
print("Bottom left RGBA:", pixels[10, 10])
print("Top right RGBA:", pixels[h-10, w-10])

# Check min and max RGB
print("Min RGB:", np.min(pixels[:, :, :3], axis=(0,1)))
print("Max RGB:", np.max(pixels[:, :, :3], axis=(0,1)))
print("Mean RGB:", np.mean(pixels[:, :, :3], axis=(0,1)))

bpy.data.images.remove(img)
