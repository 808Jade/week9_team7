import bpy
import os

obj_path = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\Portal_Chell.obj"
mtl_path = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\Portal_Chell.mtl"

print("=== CHECKING MTL FILE ===")
with open(mtl_path, "r", encoding="utf-8") as f:
    print(f.read())

print("\n=== CHECKING OBJ USEMTL OCCURRENCES ===")
usemtl_counts = {}
with open(obj_path, "r", encoding="utf-8") as f:
    for line in f:
        if line.startswith("usemtl "):
            m = line.strip().split()[1]
            usemtl_counts[m] = usemtl_counts.get(m, 0) + 1

print("usemtl counts in OBJ:", usemtl_counts)
