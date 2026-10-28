import bpy
from mathutils import Vector

glb_path = r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

obj = bpy.data.objects["chell_p2_reference"]
mats = [s.material.name for s in obj.material_slots]

hair_mat_idx = mats.index("chell_hair")
head_mat_idx = mats.index("chell_head_diffuse")

hair_polys = [p for p in obj.data.polygons if p.material_index == hair_mat_idx]
head_polys = [p for p in obj.data.polygons if p.material_index == head_mat_idx]

print(f"Hair polys count: {len(hair_polys)}")
print(f"Head polys count: {len(head_polys)}")

# Find bounds of hair vs head
hair_centers = [p.center for p in hair_polys]
head_centers = [p.center for p in head_polys]

def get_bounds(pts):
    return (
        Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts))),
        Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    )

hair_min, hair_max = get_bounds(hair_centers)
head_min, head_max = get_bounds(head_centers)

print(f"Hair center bounds: X=[{hair_min.x:.2f}, {hair_max.x:.2f}], Y=[{hair_min.y:.2f}, {hair_max.y:.2f}], Z=[{hair_min.z:.2f}, {hair_max.z:.2f}]")
print(f"Head center bounds: X=[{head_min.x:.2f}, {head_max.x:.2f}], Y=[{head_min.y:.2f}, {head_max.y:.2f}], Z=[{head_min.z:.2f}, {head_max.z:.2f}]")

# Check if hair polygons are in front of head polygons (in raw space, -Y is forward)
hair_front = min(hair_min.y, min(v.co.y for p in hair_polys for v in [obj.data.vertices[i] for i in p.vertices]))
head_front = min(head_min.y, min(v.co.y for p in head_polys for v in [obj.data.vertices[i] for i in p.vertices]))
print(f"Most forward Y (min Y): Hair={hair_front:.2f}, Head={head_front:.2f}")

# Also check materials shader setup in raw glb
hair_mat = bpy.data.materials["chell_hair"]
print("Hair material blend_method:", getattr(hair_mat, "blend_method", None))
print("Hair material shadow_method:", getattr(hair_mat, "shadow_method", None))
if hair_mat.node_tree:
    for node in hair_mat.node_tree.nodes:
        print(f"  Hair Node: {node.name} ({node.type})")
        for inp in node.inputs:
            if inp.is_linked:
                for link in inp.links:
                    print(f"    Input '{inp.name}' linked from {link.from_node.name}.{link.from_socket.name}")
