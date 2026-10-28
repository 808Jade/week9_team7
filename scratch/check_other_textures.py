import bpy
import numpy as np

for name in ["chell_head_diffuse_color_5.png", "chell_legs_diffuse_color_3.png", "chell_hair_color-chell_hair_trans_6.png"]:
    img = bpy.data.images.load(r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\textures\\" + name)
    w, h = img.size[0], img.size[1]
    pixels = np.array(img.pixels[:]).reshape((h, w, 4))
    print(f"\n--- {name} ({w}x{h}) ---")
    print("Min RGB:", np.min(pixels[:, :, :3], axis=(0,1)))
    print("Max RGB:", np.max(pixels[:, :, :3], axis=(0,1)))
    print("Mean RGB:", np.mean(pixels[:, :, :3], axis=(0,1)))
    print("Min Alpha:", np.min(pixels[:, :, 3]))
    print("Max Alpha:", np.max(pixels[:, :, 3]))
    bpy.data.images.remove(img)
