import bpy

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
mats = [s.material.name for s in obj.material_slots]
uv_layer = obj.data.uv_layers.active.data

mat_uvs = {}
for p in obj.data.polygons:
    mname = mats[p.material_index]
    if mname not in mat_uvs:
        mat_uvs[mname] = []
    for l_idx in p.loop_indices:
        uv = uv_layer[l_idx].uv
        mat_uvs[mname].append((uv.x, uv.y))

for mname, uvs in mat_uvs.items():
    us = [u[0] for u in uvs]
    vs = [u[1] for u in uvs]
    print(f"GLB Material '{mname}': U=[{min(us):.3f}, {max(us):.3f}], V=[{min(vs):.3f}, {max(vs):.3f}]")
