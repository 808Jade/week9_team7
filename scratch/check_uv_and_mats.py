import bpy
from mathutils import Vector

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
mats = [s.material.name for s in obj.material_slots]

print("=== CHECKING UV MAPS ===")
for uv_layer in obj.data.uv_layers:
    print(f"UV layer: {uv_layer.name}, active={uv_layer.active}")

print("\n=== POLYGON AND VERTEX STATS PER MATERIAL ===")
for idx, mname in enumerate(mats):
    polys = [p for p in obj.data.polygons if p.material_index == idx]
    verts = set(v_idx for p in polys for v_idx in p.vertices)
    centers = [p.center for p in polys]
    min_z = min(c.z for c in centers)
    max_z = max(c.z for c in centers)
    min_y = min(c.y for c in centers)
    max_y = max(c.y for c in centers)
    print(f"Material [{idx}] '{mname}': {len(polys)} polys, {len(verts)} verts | Z:[{min_z:.2f}, {max_z:.2f}] | Y:[{min_y:.2f}, {max_y:.2f}]")
