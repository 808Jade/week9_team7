import bpy
import math
from mathutils import Matrix, Vector

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
mesh = obj.data

# 1. Rotate 180 deg around Z
rot_mat = Matrix.Rotation(math.pi, 4, 'Z')
mesh.transform(rot_mat)

# 2. Scale 0.0254
scale_mat = Matrix.Scale(0.0254, 4)
mesh.transform(scale_mat)

# 3. Grounding
min_z = min(v.co.z for v in mesh.vertices)
trans_mat = Matrix.Translation(Vector((0, 0, -min_z)))
mesh.transform(trans_mat)

mesh.update()

min_x = min(v.co.x for v in mesh.vertices)
max_x = max(v.co.x for v in mesh.vertices)
min_y = min(v.co.y for v in mesh.vertices)
max_y = max(v.co.y for v in mesh.vertices)
min_z = min(v.co.z for v in mesh.vertices)
max_z = max(v.co.z for v in mesh.vertices)

print(f"Final bounds in Blender (after mesh.transform):")
print(f"  X: [{min_x:.4f}, {max_x:.4f}] (width: {max_x - min_x:.4f}m)")
print(f"  Y: [{min_y:.4f}, {max_y:.4f}] (depth: {max_y - min_y:.4f}m)")
print(f"  Z: [{min_z:.4f}, {max_z:.4f}] (height: {max_z - min_z:.4f}m)")
