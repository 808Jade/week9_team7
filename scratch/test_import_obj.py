import bpy

bpy.ops.wm.read_factory_settings(use_empty=True)
obj_path = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\Portal_Chell.obj"
bpy.ops.wm.obj_import(filepath=obj_path)

print("Materials in imported obj:")
for mat in bpy.data.materials:
    print("Material:", mat.name)
    if mat.node_tree:
        for node in mat.node_tree.nodes:
            if node.type == 'TEX_IMAGE':
                print(f"  TEX_IMAGE: {node.image.name if node.image else 'None'}, path={node.image.filepath if node.image else ''}")
