"""Render authored enclosure meshes only; no third-party device references."""
from pathlib import Path
import struct
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
ROOT=Path(__file__).resolve().parents[1]
def triangles(path):
    raw=path.read_bytes(); count=struct.unpack_from('<I',raw,80)[0]
    dtype=np.dtype([('normal','<f4',(3,)),('vertices','<f4',(3,3)),('attribute','<u2')])
    return np.frombuffer(raw, dtype=dtype,count=count,offset=84)['vertices'].copy()
fig=plt.figure(figsize=(14,9),facecolor='#101923')
ax=fig.add_subplot(111,projection='3d',computed_zorder=False)
ax.set_facecolor('#101923')
parts=[('01_', '#91acbc',0),('02_','#747bdb',90),('03_','#52d2d0',-75),('04_','#efbd68',0),('05_','#d085ce',110)]
for prefix,color,dz in parts:
    path=next((ROOT/'cad/stl').glob(prefix+'*.stl'))
    v=triangles(path);v[:,:,2]+=dz
    # plot width, depth, height; offsets are illustrative only
    v=v[:,:,[0,2,1]]
    mesh=Poly3DCollection(v,facecolors=color,edgecolors=color,linewidths=0,shade=True,zorder=2)
    ax.add_collection3d(mesh)
ax.set_xlim(-120,120);ax.set_ylim(-100,260);ax.set_zlim(-135,135)
ax.set_box_aspect((240,360,270));ax.view_init(elev=22,azim=-55);ax.set_axis_off()
fig.text(.055,.91,'RADAR + CAMERA',color='white',fontsize=27,weight='bold')
fig.text(.055,.855,'R02  /  ENCLOSURE PROTOTYPE',color='#52d2d0',fontsize=16,weight='bold')
fig.text(.055,.105,'Authored enclosure geometry only · Covers separated for visibility',color='#d4dee7',fontsize=13)
fig.text(.055,.065,'Physical fit, retention, weather, RF, thermal and load performance unvalidated.',color='#e6bb78',fontsize=12)
(ROOT/'preview').mkdir(exist_ok=True)
fig.savefig(ROOT/'preview/R02_enclosure_only.png',dpi=150,facecolor=fig.get_facecolor())
