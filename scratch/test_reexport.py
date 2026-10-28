import bpy
import os

bpy.ops.wm.read_factory_settings(use_empty=True)
obj_path = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\Portal_Chell.obj"
bpy.ops.wm.obj_import(filepath=obj_path)

out_obj = r"C:\Users\JUNGLE\Desktop\week9_team7\scratch\reexport_from_obj.obj"
bpy.ops.wm.obj_export(filepath=out_obj, export_materials=True, path_mode='AUTO')

mtl_path = r"C:\Users\JUNGLE\Desktop\week9_team7\scratch\reexport_from_obj.mtl"
if os.path.exists(mtl_path):
    with open(mtl_path, "r", encoding="utf-8") as f:
        print("Re-exported MTL contents:\n", f.read())
else:
    print("No MTL file created!")
