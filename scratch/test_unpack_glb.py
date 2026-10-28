import bpy

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=r"C:\Users\JUNGLE\Desktop\chell-portal2\source\chellPortal2.glb")

res = bpy.ops.file.unpack_all(method='USE_LOCAL')
print("Unpack result:", res)
