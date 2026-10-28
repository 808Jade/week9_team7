obj_path = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\Portal_Chell.obj"

uvs = []
with open(obj_path, "r", encoding="utf-8") as f:
    for line in f:
        if line.startswith("vt "):
            parts = line.strip().split()
            uvs.append((float(parts[1]), float(parts[2])))

if uvs:
    us, vs = zip(*uvs)
    print(f"Preserved {len(uvs)} original OBJ UVs: U=[{min(us):.4f}, {max(us):.4f}], V=[{min(vs):.4f}, {max(vs):.4f}]")
