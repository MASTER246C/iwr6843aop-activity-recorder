"""Parametric concept / fit-check model. Units mm. Requires CadQuery >=2.5.
Coordinate: X width, Y up, Z rearward; front outside surface Z=0.
Editable parameters below. Dimensions of unmeasured interfaces are NOT production datums.
"""
import cadquery as cq, json, os
from pathlib import Path
OUT=Path(__file__).parent; (OUT/'stl').mkdir(exist_ok=True); (OUT/'step').mkdir(exist_ok=True)
W,H,D=170.,210.,125.
CFG=json.loads((OUT/'parameters.json').read_text()); RADAR_SIGN=-1 # Proper Y180 rotation of official STEP: TOP points toward enclosure front.
WALL=1.; FRONT=3.; PCB_FRONT=CFG['pcb_front_mm']; PCB_T=1.604137; RADAR_Y=-28.194886; CAM_Y=65.; RADAR_X=-20.11235104*RADAR_SIGN
P={}; colors={}
def box(w,h,d,x=0,y=0,z=0):return cq.Workplane('XY').box(w,h,d).translate((x,y,z+d/2))
def rr(w,h,d,r=5,x=0,y=0,z=0):return box(w,h,d,x,y,z).edges('|Z').fillet(r)
def cyl(r,d,x,y,z):return cq.Workplane('XY').circle(r).extrude(d).translate((x,y,z))
def ring(w,h,iw,ih,d,z,y=0,r=4,ir=2,x=0):return rr(w,h,d,r,x=x,y=y,z=z).cut(rr(iw,ih,d+2,ir,x=x,y=y,z=z-1))
def slot(w,length,d,x,y,z):return cq.Workplane('XY').center(x,y).slot2D(length,w,90).extrude(d).translate((0,0,z))
def put(name,obj,color):P[name]=obj;colors[name]=color
# Main 1 mm wall cup, strengthened front and removable rear rim.
body=rr(W,H,D,10).cut(rr(W-2*WALL,H-2*WALL,D+1,9,z=FRONT))
body=body.union(ring(W,H,W-10,H-10,7,118,r=10,ir=5))
body=body.cut(rr(146,94,14,6,y=-28,z=-5)).cut(rr(112,58,10,5,y=CAM_Y,z=-4))
# Replaceable bezel pins in local bosses; small printed retention pins enter from front.
front_holes=[(x,CAM_Y+y) for x in (-60,60) for y in (-31,31)]
for x,y in front_holes:
 body=body.union(cyl(4.8,8,x,y,0)).cut(cyl(1.65,9,x,y,-.5))
# Rear snap-rivet bosses: 5.2 mm through bore, no metal insert.
for x in (-74,74):
 for y in (-92,92):body=body.union(cyl(8.3,15,x,y,110)).cut(cyl(2.6,16,x,y,110))
# Tripod structural crossmember bonded into bottom and braced into shell.
body=body.union(box(168,13,76,y=-97.5,z=28))
for x in (-78,78):body=body.union(box(8,34,76,x=x,y=-86,z=28))
# Replaceable printed quarter-20 cassette, accessible internally. Thread axis is vertical Y.
# Build in XY then rotate cylindrical Z-axis to Y.
def yhole(rad,length,x,y,z):return cyl(rad,length,0,0,0).rotate((0,0,0),(1,0,0),-90).translate((x,y,z))
body=body.cut(yhole(6.2,21,0,-108,68))
# Captive replaceable printed cassette in keyed rectangular pocket; open internally.
body=body.cut(box(32.4,7.5,28.4,y=-94.75,z=53.8))
for x in (-23,23):body=body.cut(yhole(1.65,10,x,-99,68))
# Carrier supports use printed 3.4 mm retention pins; no added metal.
for x in (-67,67):
 for y in (-70,12):
  body=body.union(box(18,10,36.8,x=75 if x>0 else -75,y=y,z=3.2)).cut(cyl(1.7,8,x,y,34))
# Camera shelf rails y=34. Slotted shelf moves in Z; shim height at assembly to align glass.
for x in (-66,66):
 body=body.union(box(12,8,85,x=x,y=30,z=2))
 for z in (25,65):body=body.cut(yhole(1.65,9,x,26,z))
# Removable rear cover with split cable access panel; no permanently open drain.
service_holes=[(x,y) for x in(-75,75) for y in(-76,20)]
for x,y in service_holes:body=body.union(cyl(6,9,x,y,0)).cut(cyl(2.6,11,x,y,-1))
put('01_main_shell',body,(.17,.25,.28))
cover=rr(W,H,3,10,z=126).union(ring(159.4,199.4,155.4,195.4,4,122,r=5,ir=3))
for x in (-74,74):
 for y in (-92,92):cover=cover.cut(cyl(2.6,10,x,y,121))
for x in (-74,74):
 for y in (-92,92):cover=cover.cut(cyl(8.6,5,x,y,121))
# Edge-open rear notch passes attached USB-A connectors before split seal panels install.
cover=cover.cut(box(88,42,12,y=-85,z=120))
for x in (-47,47):
 for y in (-69,-93):cover=cover.cut(cyl(1.65,6,x,y,125))
for suffix,cy in [('upper',-69),('lower',-93)]:
 panel=box(100,24,3,y=cy,z=129)
 if suffix=='lower':panel=panel.union(box(88,5,3,y=-102.5,z=126))
 for x in (-30,0,30):panel=panel.cut(cyl(3,5,x,-81,128))
 for x in (-47,47):panel=panel.cut(cyl(1.7,5,x,cy,128))
 put('16_split_cable_panel_'+suffix+'_D6',panel,(.35,.43,.46))
RAW_X=26.58904654*RADAR_SIGN;RAW_Y=-24.
cover=cover.cut(rr(50,36,12,4,x=RAW_X,y=RAW_Y,z=121))
raw_holes=[(RAW_X+x,RAW_Y+y) for x in(-30,30) for y in(-23,23)]
for x,y in raw_holes:cover=cover.union(cyl(5.5,6,x,y,123)).cut(cyl(2.6,12,x,y,122))
raw_blank=rr(72,58,3,6,x=RAW_X,y=RAW_Y,z=130)
raw_gasket=ring(70,56,52,38,1,129,x=RAW_X,y=RAW_Y,r=5,ir=4)
for x,y in raw_holes:
 raw_blank=raw_blank.cut(cyl(2.6,5,x,y,129));raw_gasket=raw_gasket.cut(cyl(2.7,3,x,y,128))
put('22_RAW_upgrade_access_blank',raw_blank,(.25,.36,.4))
put('G04_RAW_access_gasket_TEMPLATE',raw_gasket,(.07,.08,.09))
put('02_rear_cover',cover,(.21,.3,.33))
gasket=ring(169,209,160.5,200.5,1,125,r=9.5,ir=5)
for x in (-74,74):
 for y in (-92,92):gasket=gasket.cut(cyl(2.7,3,x,y,124))
put('G01_rear_gasket_TEMPLATE',gasket,(.07,.08,.09))
# Full lower-front service cover: remove to reach S1/S2, reset and USB without removing PCB.
service=rr(162,108,3,8,y=-28,z=-4).cut(rr(52,52,12,3,x=RADAR_X,y=RADAR_Y,z=-5))
# Raised ledge carries a caulk/soft gasket and membrane INSIDE the shell, close to AOP.
MEMBRANE_INNER=CFG['radar_membrane_inner_z_mm']
service=service.union(ring(64,64,52,52,MEMBRANE_INNER-.8+1,-1,x=RADAR_X,y=RADAR_Y,r=4,ir=3))
service_gasket=ring(160,106,146,94,1,-1,y=-28,r=7,ir=6)
for x,y in service_holes:
 service=service.cut(cyl(2.6,5,x,y,-5));service_gasket=service_gasket.cut(cyl(2.7,3,x,y,-2))
put('21_front_board_service_panel',service,(.27,.38,.42))
put('G03_service_panel_gasket_TEMPLATE',service_gasket,(.07,.08,.09))
put('03_radar_window_0p30mm_PLA',rr(64,64,.3,4,x=RADAR_X,y=RADAR_Y,z=MEMBRANE_INNER-.3),(.76,.82,.8))
put('G02_radar_gasket_TEMPLATE',ring(64,64,52,52,.5,MEMBRANE_INNER-.8,x=RADAR_X,y=RADAR_Y,r=4,ir=3),(.08,.09,.1))
# Membrane bonds only around its perimeter; no chip contact or rigid retainer over taller switches.
# Camera OPEN bezel: no additional glazing; user requested its own glass faces opening.
cb=ring(130,76,90,38,3,-3,y=CAM_Y,r=7,ir=4)
for x,y in front_holes:cb=cb.cut(cyl(1.7,5,x,y,-4))
cb=cb.cut(ring(98,46,92,40,.9,-3.01,y=CAM_Y,r=6,ir=4))
put('05_camera_open_bezel_FIT_CHECK',cb,(.12,.16,.17))
# No camera gasket supplied as sealing datum is unknown; bezel intentionally replaceable.
# Slotted camera shelf. Housing cradle and printed shims provide retention.
shelf=box(146,4,80,y=36,z=8)
for x in (-66,66):
 for z in (28,65):
  cut=cq.Workplane('XY').slot2D(20,3.5).extrude(8).rotate((0,0,0),(1,0,0),90).rotate((0,0,0),(0,1,0),90).translate((x,41,z))
  # simpler rounded slot: transformed vertical slot in XZ plane
  cut=cq.Workplane('XZ').center(x,z).slot2D(20,3.5,90).extrude(10,both=True).translate((0,36,0))
  shelf=shelf.cut(cut)
shelf=shelf.cut(cq.Workplane('XZ').center(0,42).slot2D(46,6.8,90).extrude(10,both=True).translate((0,36,0)))
for x in (-55,55):shelf=shelf.cut(yhole(1.7,8,x,32,21))
put('06_camera_adjustable_shelf',shelf,(.6,.63,.64))
# Adjustable padded camera cradle: opening exceeds published overall width/height.
cradle=rr(118,58,10,5,y=67,z=16).cut(rr(100,46,12,3,y=65,z=15))
for x in (-55,55):
 cradle=cradle.cut(yhole(1.65,12,x,32,21))
 cradle=cradle.union(box(8,8,58,x=x,y=65,z=16))
cradle=cradle.union(box(118,8,5,y=65,z=74)).cut(box(16,10,7,y=65,z=73))
put('14_camera_cradle_FIT_CHECK',cradle,(.45,.51,.53))
# Radar generic carrier. This is not a board-specific hole pattern.
carrier=rr(144,100,3,4,y=-28,z=40).cut(rr(128,82,5,3,y=-28,z=39))
for x in(-67,67):
 for y in(-70,12):carrier=carrier.cut(cyl(1.7,5,x,y,39))
for x,y in((4.224344*RADAR_SIGN,-38.038656),(40.048052*RADAR_SIGN,-38.018811)):
 carrier=carrier.union(box(8,y+75,3,x=x,y=(y-75)/2,z=40)).cut(cyl(1.7,5,x,y,39))
for x in(-60,60):
 for y in(-46,16):
  carrier=carrier.union(box(10,10,3,x=x,y=y,z=40)).cut(cyl(1.7,5,x,y,39))
put('07_radar_universal_carrier',carrier,(.58,.61,.62))
# Electronics tray at rear; strap slots allow adhesive-free GPS/electronics fixtures.
tray=rr(132,80,3,4,y=-13,z=97)
for x in (-54,-27,0,27,54):
 for y in (-41,15):tray=tray.cut(slot(3.5,15,5,x,y,96))
for x in (-60,60):
 for y in (-46,16):tray=tray.cut(cyl(1.7,5,x,y,96))
# Tray uses four modeled 54 mm printed posts with matching holes and printed pins.
tray=tray.cut(rr(50,36,5,4,x=RAW_X,y=RAW_Y,z=96))
put('08_electronics_tray_OPTIONAL',tray,(.52,.57,.59))
cap=box(58,3,36,y=-89.5,z=50)
for x in (-23,23):cap=cap.cut(yhole(1.7,5,x,-92,68))
put('09_cassette_retaining_cap',cap,(.4,.46,.48))
# Real helical thread cutter: pitch 25.4/20 = 1.27 mm, nominal major 6.35 mm.
# Radial clearance must be calibrated using the three coupons before printing cassette.
def female_thread(blank,depth,clearance=None):
 if clearance is None:clearance=CFG['thread_radial_clearance_mm']
 pitch=1.27; minor=2.525+clearance; major=3.175+clearance
 blank=blank.cut(cyl(minor,depth+2,0,0,-1))
 path=cq.Wire.makeHelix(pitch,depth+2,minor).translate((0,0,-1))
 profile=cq.Workplane('XZ').polyline([(minor-.08,-1-pitch*.35),(major,-1),(minor-.08,-1+pitch*.35)]).close()
 tooth=profile.sweep(path,isFrenet=True)
 return blank.cut(tooth)
cassette_blank=box(32,28,7,z=6.3).union(cyl(6,6.3,0,0,0))
cassette=female_thread(cassette_blank,13.3).rotate((0,0,0),(1,0,0),-90).translate((0,-104.8,68))
put('11_printed_quarter20_thread_cassette',cassette,(.76,.5,.16))
for i,c in enumerate((.15,.25,.35)):
 coupon=female_thread(box(22,22,8),8,c).translate((110+i*28,-60,0))
 put('C0'+str(i+1)+'_thread_coupon_radial_'+str(c).replace('.','p'),coupon,(.8,.6,.24))
# Reusable split retention pins: tapered friction shanks, head, and slot for flex.
def pin(d,length):
 shaft=cq.Workplane('XY').circle(d/2).extrude(length)
 shaft=shaft.union(cyl(d*.8,2,0,0,-2))
 shaft=shaft.cut(box(.65,d+1,length*.72,z=length*.28))
 return shaft
put('12_front_and_mount_pin_D3p4_L9',pin(3.4,9).translate((105,20,0)),(.8,.62,.24))
rear_pin=cyl(2.5,19.2,0,0,0).union(cyl(4.3,2,0,0,-2))
rear_pin=rear_pin.union(cq.Workplane('XY').workplane(offset=19.2).circle(3.1).workplane(offset=2.8).circle(2.15).loft())
rear_pin=rear_pin.cut(box(1.4,8,15,z=8))
put('13_rear_barbed_snap_rivet_L22',rear_pin.translate((120,20,0)),(.8,.62,.24))
# Camera secured by cradle, rear stop, front bezel shoulder and fitted shims.
# Verify all contact lands on housing, not glass, and fit pads after measurement.
plug=cyl(2.95,3,0,0,0).union(cyl(4.5,2,0,0,3))
put('15_rear_port_blank_SEALANT_REQUIRED',plug.translate((135,20,0)),(.4,.46,.48))
# Actual Rev G ODB mounting holes, not inferred from a different bracket drawing.
# Board normalized datum translated(-57.633955,-40.635725); diameter2.54, pitch35.8237.
for i,(x,y) in enumerate(((4.224344*RADAR_SIGN,-38.038656),(40.048052*RADAR_SIGN,-38.018811))):
 front=PCB_FRONT+PCB_T
 post=cq.Workplane('XY').workplane(offset=front).circle(2.3).workplane(offset=40-front).circle(4).loft().translate((x,y,0))
 post=post.cut(cyl(1.15,8,x,y,front-.01)).cut(cyl(1.65,8,x,y,32.01))
 put('17_PCB_mount_post_'+str(i+1)+'_RevG',post,(.7,.55,.25))
small_pin=pin(2.4,8)
put('20_PCB_pin_D2p4_L8',small_pin.translate((110,80,0)),(.7,.55,.25))
# Camera shims in 1, 2 and 3 mm thickness; place after measuring actual housing.
for i,t in enumerate((1,2,3)):
 put('18_camera_shim_'+str(t)+'mm',box(20,10,t,x=110+i*24,y=55,z=0),(.7,.55,.25))
for i,(x,y) in enumerate([(-60,-46),(60,-46),(-60,16),(60,16)]):
 post=cyl(4,54,x,y,43).cut(cyl(1.65,9,x,y,42)).cut(cyl(1.65,9,x,y,89))
 put('19_tray_post_'+str(i+1)+'_54mm',post,(.6,.63,.64))
def snap_rivet(shoulder):
 r=cyl(2.5,shoulder,0,0,0).union(cyl(4.3,2,0,0,-2))
 r=r.union(cq.Workplane('XY').workplane(offset=shoulder).circle(3.1).workplane(offset=2.8).circle(2.15).loft())
 return r.cut(box(1.4,8,shoulder+3-5,z=5))
front_rivet=snap_rivet(13.2);raw_rivet=snap_rivet(10.2)
put('23_front_service_snap_rivet_L16',front_rivet.translate((135,80,0)),(.7,.55,.25))
put('24_RAW_access_snap_rivet_L13',raw_rivet.translate((150,80,0)),(.7,.55,.25))
# SIMPLIFIED R02: five functional parts; one common printed thumb screw and two PCB pins.
# Detailed intermediate fixtures above are consolidated here; only the following parts export.
old=P.copy();oldcolors=colors.copy();P={};colors={}
main=old['01_main_shell']
# Fill the replaceable-cassette construction and make the tripod thread integral in thick base.
main=main.union(box(168,13,76,y=-97.5,z=28)).union(yhole(6.2,1.1,0,-105,68))
threadblank=box(20,20,15,z=0)
quarter_cutter=threadblank.cut(female_thread(threadblank,15)).rotate((0,0,0),(1,0,0),-90).translate((0,-105.1,68))
main=main.cut(quarter_cutter)
# Integrate camera bezel/shelf/front cradle, carrier, PCB posts and electronics tray supports.
main=main.union(old['05_camera_open_bezel_FIT_CHECK']).union(old['06_camera_adjustable_shelf'])
cradle_fixed=old['14_camera_cradle_FIT_CHECK'].cut(box(150,30,10,y=65,z=74))
for x in(-55,55):cradle_fixed=cradle_fixed.union(box(12,12,58,x=x,y=65,z=16))
main=main.union(cradle_fixed).union(old['07_radar_universal_carrier']).union(old['08_electronics_tray_OPTIONAL'])
for x,y in((4.224344*RADAR_SIGN,-38.038656),(40.048052*RADAR_SIGN,-38.018811)):
 front=PCB_FRONT+PCB_T
 post=cq.Workplane('XY').workplane(offset=front).circle(2.3).workplane(offset=43-front).circle(4).loft().translate((x,y,0)).cut(cyl(1.15,8,x,y,front-.01))
 main=main.union(post)
for x in(-60,60):
 for y in(-46,16):main=main.union(cyl(4,57,x,y,43))
for x,y in front_holes:main=main.union(cyl(1.8,11,x,y,-3))
# Common coarse printed M6-ish hand screw. Nominal major6, pitch2, calibrated .25 radial clearance.
def coarse_cutter(depth=20):
 minor=2.6;major=3.25;pitch=2.
 path=cq.Wire.makeHelix(pitch,depth+2,minor).translate((0,0,-1))
 profile=cq.Workplane('XZ').polyline([(minor-.08,-1-.6),(major,-1),(minor-.08,-1+.6)]).close()
 return cyl(minor,depth+2,0,0,-1).union(profile.sweep(path,isFrenet=True))
cc=coarse_cutter()
for x in(-74,74):
 for y in(-92,92):
  main=main.union(cyl(2.7,15,x,y,110)).cut(cc.rotate((0,0,0),(1,0,0),180).translate((x,y,129)))
for x,y in service_holes:
 main=main.union(cyl(2.7,9,x,y,0)).cut(cc.translate((x,y,-4)))
for x in(-55,55):main=main.cut(cc.rotate((0,0,0),(1,0,0),180).translate((x,65,79)))
# Clearance beyond the two lower front bosses prevents screw-tip/support interference.
for x in(-75,75):main=main.cut(cyl(3.35,4,x,-76,9))
put('01_integrated_main_shell',main,(.3,.41,.45))
# Entire front wall is replaceable. The .30 mm window is an integral homogeneous membrane.
front_wall=old['21_front_board_service_panel']
front_wall=front_wall.union(ring(64,64,52,52,.5,MEMBRANE_INNER-.8,x=RADAR_X,y=RADAR_Y,r=4,ir=3)).union(old['03_radar_window_0p30mm_PLA'])
for x,y in service_holes:front_wall=front_wall.cut(cyl(3.35,5,x,y,-5))
put('03_front_service_wall_integral_0p30_window',front_wall,(.35,.46,.49))
# Closed rear cover is the replaceable interface for a future RAW module; no extra blank today.
rear=rr(W,H,3,10,z=126).union(ring(159.4,199.4,155.4,195.4,4,122,r=5,ir=3))
for x in(-74,74):
 for y in(-92,92):rear=rear.cut(cyl(8.6,5,x,y,121)).cut(cyl(3.35,10,x,y,121))
rear=rear.cut(box(88,42,12,y=-85,z=120))
upper=box(108,24,3,y=-69,z=129)
lower=box(108,24,3,y=-93,z=129).union(box(88,5,3,y=-102.5,z=126))
for x in(-20,20):
 upper=upper.cut(cyl(3,5,x,-81,128));lower=lower.cut(cyl(3,5,x,-81,128))
rear=rear.union(upper)
for x in(-47,47):
 rear=rear.union(cyl(6,13,x,-93,116)).cut(cc.rotate((0,0,0),(1,0,0),180).translate((x,-93,132)))
 lower=lower.cut(cyl(3.35,5,x,-93,128))
put('02_removable_rear_cover',rear,(.28,.39,.43))
put('05_split_cable_clamp_strip',lower,(.28,.39,.43))
# A single removable camera keeper permits insertion from the open rear.
keeper=box(124,14,5,y=65,z=74).cut(box(16,8,7,y=60,z=73))
for x in(-55,55):keeper=keeper.cut(cyl(3.35,7,x,65,73))
put('04_camera_keeper_bar',keeper,(.5,.57,.59))
# One screw design, 12 installed:4front+4rear+2camera+2cable strip.
pitch=2.;path=cq.Wire.makeHelix(pitch,16,2.35)
profile=cq.Workplane('XZ').polyline([(2.25,-.6),(3,0),(2.25,.6)]).close()
screw=cyl(2.35,16,0,0,0).union(profile.sweep(path,isFrenet=True)).union(cyl(6,3,0,0,-3))
screw=screw.intersect(box(20,20,19,z=-3))
for angle in range(0,360,60):
 cutter=cyl(.7,4,5.7,0,-3.5).rotate((0,0,0),(0,0,1),angle);screw=screw.cut(cutter)
put('06_common_thumb_screw_D6_pitch2_L16',screw.translate((110,0,0)),(.7,.52,.2))
put('07_PCB_retaining_pin_D2p4_L8',pin(2.4,8).translate((130,0,0)),(.7,.52,.2))
# Optional test coupons are separate from installed part count.
for n,obj in old.items():
 if n.startswith('C0'):put(n,obj,oldcolors[n])
coarse_coupon=box(24,24,12).cut(coarse_cutter(12)).translate((110,40,0))
put('C04_common_screw_fit_coupon',coarse_coupon,(.7,.52,.2))
assembly=cq.Assembly(name='Radar_Camera_Prototype_R02_SIMPLIFIED')
validation=[]
for name,p in P.items():
 sh=p.val();valid=sh.isValid();solids=len(sh.Solids());bb=sh.BoundingBox()
 assert valid and solids==1,(name,valid,solids)
 cq.exporters.export(p,str(OUT/'stl'/f'{name}.stl'),tolerance=.06,angularTolerance=.12)
 cq.exporters.export(p,str(OUT/'step'/f'{name}.step'))
 if not name.startswith(('C0','06_','07_')):assembly.add(p,name=name,color=cq.Color(*colors[name]))
 validation.append(dict(part=name,valid=valid,solids=solids,volume_mm3=round(sh.Volume(),3),bbox_mm=[round(bb.xlen,3),round(bb.ylen,3),round(bb.zlen,3)]))
for x,y in service_holes:assembly.add(screw.translate((x,y,-4)),name=f'front_screw_{x}_{y}',color=cq.Color(.7,.52,.2))
for x in(-74,74):
 for y in(-92,92):assembly.add(screw.rotate((0,0,0),(1,0,0),180).translate((x,y,129)),name=f'rear_screw_{x}_{y}',color=cq.Color(.7,.52,.2))
for x in(-55,55):assembly.add(screw.rotate((0,0,0),(1,0,0),180).translate((x,65,79)),name=f'camera_screw_{x}',color=cq.Color(.7,.52,.2))
for x in(-47,47):assembly.add(screw.rotate((0,0,0),(1,0,0),180).translate((x,-93,132)),name=f'cable_screw_{x}',color=cq.Color(.7,.52,.2))
assembly.save(str(OUT/'assembly.step'))
(OUT/'validation.json').write_text(json.dumps(validation,indent=2));(OUT/'colors.json').write_text(json.dumps(colors))
print(json.dumps(validation,indent=2))
