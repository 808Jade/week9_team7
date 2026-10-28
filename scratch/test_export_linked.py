import bpy
import os

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb")

tex_dir = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\textures"
for img in bpy.data.images:
    for f in os.listdir(tex_dir):
        if img.name in f:
            img.filepath = os.path.join(tex_dir, f)
            print(f"Set {img.name} -> {img.filepath}")

out_obj = r"C:\Users\JUNGLE\Desktop\week9_team7\scratch\test_export_linked.obj"
bpy.ops.wm.obj_export(filepath=out_obj, export_materials=True, path_mode='RELATIVE')

mtl_path = r"C:\Users\JUNGLE\Desktop\week9_team7\scratch\test_export_linked.mtl"
if os.path.exists(mtl_path):
    with open(mtl_path, "r", encoding="utf-8") as f:
        print("MTL contents:\n", f.read())
