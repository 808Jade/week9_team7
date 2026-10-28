import bpy
import math

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
mats = [s.material.name for s in obj.material_slots]
head_idx = mats.index("chell_head_diffuse")
uv_layer = obj.data.uv_layers.active.data

# Find face polygon near nose / mouth
# Nose tip is typically max Y in raw coordinates (closest to front, min Y raw)
# and around Z = 69-70
nose_polys = [p for p in obj.data.polygons if p.material_index == head_idx and p.center.y < -6.3 and 68.5 < p.center.z < 70.0]
print(f"Nose polygons found: {len(nose_polys)}")

for p in nose_polys[:3]:
    for l_idx in p.loop_indices:
        uv = uv_layer[l_idx].uv
        u_norm = uv.x - math.floor(uv.x)
        v_norm = uv.y - math.floor(uv.y)
        print(f"Nose vert UV raw: ({uv.x:.3f}, {uv.y:.3f}) -> norm: ({u_norm:.3f}, {v_norm:.3f})")

# In typical head diffuse texture (2048x2048):
# Where is the nose in chell_head_diffuse_color_5.png?
# Usually face is in the center or left. Let's inspect the image!
