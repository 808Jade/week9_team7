import bpy
import math

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
bpy.context.view_layer.objects.active = obj
obj.select_set(True)

bb0 = [list(v) for v in obj.bound_box]
print(f"Before rot: Y=[{min(v[1] for v in bb0):.4f}, {max(v[1] for v in bb0):.4f}]")

obj.rotation_euler = (0, 0, math.pi)
bpy.ops.object.transform_apply(rotation=True)

bb1 = [list(v) for v in obj.bound_box]
print(f"After rot: Y=[{min(v[1] for v in bb1):.4f}, {max(v[1] for v in bb1):.4f}]")
