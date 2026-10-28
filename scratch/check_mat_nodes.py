import bpy

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb")

for mat in bpy.data.materials:
    print("Material:", mat.name)
    if mat.use_nodes:
        for node in mat.node_tree.nodes:
            print("  Node:", node.type, node.name)
            if node.type == 'TEX_IMAGE' and node.image:
                print("    Image:", node.image.name, "filepath:", node.image.filepath, "packed:", node.image.packed_file is not None)

