import bpy
import os
import shutil
import math
from mathutils import Matrix, Vector

src_dir = r"C:\Users\JUNGLE\Desktop\chell-portal2"
glb_path = os.path.join(src_dir, "source", "chellPortal2.glb")
raw_tex_dir = os.path.join(src_dir, "textures")

out_dir = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell"
out_tex_dir = os.path.join(out_dir, "textures")
os.makedirs(out_tex_dir, exist_ok=True)

print("=== 1. COPYING TEXTURES ===")
for f in os.listdir(raw_tex_dir):
    src_f = os.path.join(raw_tex_dir, f)
    dst_f = os.path.join(out_tex_dir, f)
    if os.path.isfile(src_f):
        shutil.copy2(src_f, dst_f)
        print(f"Copied: {f}")

print("\n=== 2. LOADING GLB AND APPLYING TRANSFORMS ===")
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

mesh_obj = bpy.data.objects["chell_p2_reference"]
mesh = mesh_obj.data

# 1. Rotate 180 deg around Z so Chell faces +Y (Engine +X Forward)
rot_mat = Matrix.Rotation(math.pi, 4, 'Z')
mesh.transform(rot_mat)

# 2. Scale by 0.0254 (inch to meter)
scale_mat = Matrix.Scale(0.0254, 4)
mesh.transform(scale_mat)

# 3. Grounding: bottom of shoes at Z = 0
min_z = min(v.co.z for v in mesh.vertices)
trans_mat = Matrix.Translation(Vector((0, 0, -min_z)))
mesh.transform(trans_mat)

# The engine OBJ importer converts OBJ V coordinates with `1 - V`. Pre-compensate
# for that here so the cooked mesh uses the original GLB/Blender UVs. Keep values
# outside [0, 1]; the engine's OBJ material sampler uses wrap addressing.
for loop in mesh.uv_layers.active.data:
    loop.uv.y = 1.0 - loop.uv.y

mesh.update()

min_x = min(v.co.x for v in mesh.vertices); max_x = max(v.co.x for v in mesh.vertices)
min_y = min(v.co.y for v in mesh.vertices); max_y = max(v.co.y for v in mesh.vertices)
min_z = min(v.co.z for v in mesh.vertices); max_z = max(v.co.z for v in mesh.vertices)

print(f"Blender Bounds after transform:")
print(f"  X: [{min_x:.4f}, {max_x:.4f}] (width: {max_x - min_x:.4f}m)")
print(f"  Y: [{min_y:.4f}, {max_y:.4f}] (depth: {max_y - min_y:.4f}m)")
print(f"  Z: [{min_z:.4f}, {max_z:.4f}] (height: {max_z - min_z:.4f}m)")

print("\n=== 3. EXPORTING OBJ ===")
obj_out_path = os.path.join(out_dir, "Portal_Chell.obj")
bpy.ops.object.select_all(action='DESELECT')
mesh_obj.select_set(True)
bpy.context.view_layer.objects.active = mesh_obj

bpy.ops.wm.obj_export(
    filepath=obj_out_path,
    export_selected_objects=True,
    export_materials=True,
    forward_axis='NEGATIVE_Z',
    up_axis='Y'
)
print(f"Exported: {obj_out_path} ({os.path.getsize(obj_out_path)} bytes)")

print("\n=== 4. WRITING MTL FILE WITH ALPHA SUPPORT ===")
mtl_out_path = os.path.join(out_dir, "Portal_Chell.mtl")

mat_map = {
    "chell_torso_diffuse": {
        "Kd": "1.0 1.0 1.0",
        "map_Kd": "textures/chell_torso_diffuse_color_1.png",
        "map_bump": "textures/chell_torso_diffuse_normal_0.png",
    },
    "chell_legs_diffuse": {
        "Kd": "1.0 1.0 1.0",
        "map_Kd": "textures/chell_legs_diffuse_color_3.png",
        "map_bump": "textures/chell_legs_diffuse_normal_2.png",
    },
    "chell_head_diffuse": {
        "Kd": "1.0 1.0 1.0",
        "map_Kd": "textures/chell_head_diffuse_color_5.png",
        "map_bump": "textures/chell_head_diffuse_normal_4.png",
    },
    "chell_hair": {
        "Kd": "1.0 1.0 1.0",
        "d": "0.999",  # Trigger AlphaBlend PSO in HitoriEngine!
        "map_Kd": "textures/chell_hair_color-chell_hair_trans_6.png",
    },
    "chell_teeth": {
        "Kd": "1.0 1.0 1.0",
        "map_Kd": "textures/chell_teeth_color_7.png",
    },
    "eyeball_r": {
        "Kd": "1.0 1.0 1.0",
        "map_Kd": "textures/gman_eye_color_psd_a416d4da_albedo_8.png",
    }
}

with open(mtl_out_path, "w", encoding="utf-8") as f:
    f.write("# Chell Portal 2 Material\n")
    for name, info in mat_map.items():
        f.write(f"\nnewmtl {name}\n")
        f.write(f"Kd {info.get('Kd', '1.0 1.0 1.0')}\n")
        if "d" in info:
            f.write(f"d {info['d']}\n")
        if "map_Kd" in info:
            f.write(f"map_Kd {info['map_Kd']}\n")
        if "map_bump" in info:
            f.write(f"map_bump {info['map_bump']}\n")

print(f"Written MTL: {mtl_out_path}")
print("=== CONVERSION COMPLETE! ===")
