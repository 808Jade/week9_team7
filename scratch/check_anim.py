import bpy

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
print("Animation data:", obj.animation_data)
print("Shape keys:", obj.data.shape_keys)
print("Modifiers:", [m.name for m in obj.modifiers])
