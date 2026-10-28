import bpy

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
print("Raw loc:", obj.location)
print("Raw rot:", obj.rotation_euler)
print("Raw scale:", obj.scale)
print("Raw matrix_world:\n", obj.matrix_world)
bb = [list(v) for v in obj.bound_box]
print(f"Raw bound_box: X=[{min(v[0] for v in bb):.4f}, {max(v[0] for v in bb):.4f}], Y=[{min(v[1] for v in bb):.4f}, {max(v[1] for v in bb):.4f}], Z=[{min(v[2] for v in bb):.4f}, {max(v[2] for v in bb):.4f}]")
