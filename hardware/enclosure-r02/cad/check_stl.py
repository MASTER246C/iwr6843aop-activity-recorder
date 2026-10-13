from pathlib import Path
import struct,json,collections
R=Path(__file__).parent
out=[]
for f in (R/'stl').glob('*.stl'):
 b=f.read_bytes();n=struct.unpack_from('<I',b,80)[0];edges=collections.Counter();verts=set()
 for i in range(n):
  v=struct.unpack_from('<12fH',b,84+50*i);p=[tuple(round(c,5) for c in v[j:j+3]) for j in (3,6,9)]
  verts.update(p)
  for j in range(3):edges[tuple(sorted((p[j],p[(j+1)%3])))]+=1
 bad=sum(v!=2 for v in edges.values())
 out.append(dict(part=f.stem,triangles=n,vertices=len(verts),nonmanifold_edge_count=bad,closed_edge_manifold=bad==0))
(R/'stl_validation.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
assert all(p['closed_edge_manifold'] for p in out)
