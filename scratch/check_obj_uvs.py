import numpy as np

obj_path = r"C:\Users\JUNGLE\Desktop\week9_team7\Assets\Models\PortalChell\Portal_Chell.obj"

uvs = []
current_mtl = None
mtl_uvs = {}

with open(obj_path, "r", encoding="utf-8") as f:
    for line in f:
        if line.startswith("vt "):
            parts = line.strip().split()
            u = float(parts[1])
            v = float(parts[2])
            uvs.append((u, v))
        elif line.startswith("usemtl "):
            current_mtl = line.strip().split()[1]
            if current_mtl not in mtl_uvs:
                mtl_uvs[current_mtl] = []
        elif line.startswith("f "):
            if current_mtl and uvs:
                # get UV index from face tokens
                tokens = line.strip().split()[1:]
                for t in tokens:
                    sub = t.split("/")
                    if len(sub) > 1 and sub[1]:
                        uv_idx = int(sub[1]) - 1
                        mtl_uvs[current_mtl].append(uvs[uv_idx])

print(f"Total UVs: {len(uvs)}")
for mtl, m_uv in mtl_uvs.items():
    if m_uv:
        us = [p[0] for p in m_uv]
        vs = [p[1] for p in m_uv]
        print(f"Material '{mtl}': {len(m_uv)} UV refs | U: [{min(us):.3f}, {max(us):.3f}] | V: [{min(vs):.3f}, {max(vs):.3f}]")
