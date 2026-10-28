import bpy

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
mats = [s.material.name for s in obj.material_slots]
uv_layer = obj.data.uv_layers.active.data

# Let's inspect UVs with modulo 1.0
mat_uvs_mod = {}
for p in obj.data.polygons:
    mname = mats[p.material_index]
    if mname not in mat_uvs_mod:
        mat_uvs_mod[mname] = []
    for l_idx in p.loop_indices:
        uv = uv_layer[l_idx].uv
        u_mod = uv.x % 1.0
        v_mod = uv.y % 1.0
        mat_uvs_mod[mname].append((u_mod, v_mod))

for mname, uvs in mat_uvs_mod.items():
    us = [u[0] for u in uvs]
    vs = [u[1] for u in uvs]
    print(f"Mod 1.0 UV for '{mname}': U=[{min(us):.4f}, {max(us):.4f}], V=[{min(vs):.4f}, {max(vs):.4f}]")
