import bpy
import os

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb")
out_obj = r"C:\Users\JUNGLE\Desktop\week9_team7\scratch\test_export_copy.obj"
bpy.ops.wm.obj_export(filepath=out_obj, export_materials=True, path_mode='COPY')

print("Files in scratch with test_export_copy:", [f for f in os.listdir(r"C:\Users\JUNGLE\Desktop\week9_team7\scratch") if "test_export_copy" in f])
mtl_path = r"C:\Users\JUNGLE\Desktop\week9_team7\scratch\test_export_copy.mtl"
if os.path.exists(mtl_path):
    with open(mtl_path, "r", encoding="utf-8") as f:
        print("MTL contents:\n", f.read()[:500])
