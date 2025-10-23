import cadquery as cq

myfile = "knot.step"
#result = cq.importers.importStep(myfile)

pcb_width = 56
pcb_height = 60
pcb_zdrop = 3
pcb_thickness = 1
pcb_bottom_clearance = 3
pcb_side_clearance = 2

box_width = 63
box_height= 63
box_bottom_thickness = pcb_zdrop + pcb_thickness + pcb_bottom_clearance + 1

pcb_top_clearance = 7
box_top_thickness = -pcb_zdrop + pcb_top_clearance + 1

top = cq.Workplane("XY").rect(box_width,box_height).extrude(box_top_thickness)\
    .faces("<Z").rect(pcb_width-4, pcb_height).cutBlind(box_top_thickness-1)\
    .faces("<Z").rarray(pcb_width-pcb_side_clearance,0,2,1).rect((pcb_side_clearance),pcb_height).extrude(-pcb_zdrop)

zref = -box_bottom_thickness

base = cq.Workplane("XY").rect(box_width,box_height).extrude(-box_bottom_thickness)\
    .faces(">Z").rect(pcb_width, pcb_height).cutBlind(-pcb_zdrop-pcb_thickness)\
    .faces(">Z").rect(pcb_width-pcb_side_clearance*2, pcb_height).cutBlind(-pcb_zdrop-pcb_thickness-pcb_bottom_clearance)
    
usb_zpos = zref + 7.65
usb_xpos = 0

absw_zpos = zref + 7.3
absw_xpos = -21
absw_radius = 4.95 / 2

boot_zpos = zref + 6.45
boot_xpos = -12.5
boot_radius = 2.8 / 2

led_zpos = zref + 7.95
led_xpos_1 = 12.5
led_xpos_2 = led_xpos_1 + 5
led_xpos_3 = led_xpos_1 + 10
led_radius = 3.1 / 2


slot_half = 5

# Features on the front
base = base.faces("<Y").workplane().center(usb_xpos, usb_zpos).rect(13.35,6).center(-usb_xpos, -usb_zpos).cutBlind(-10)\
    .faces("<Y").workplane().center(absw_xpos, absw_zpos+slot_half-absw_radius).slot2D(length=slot_half*2,diameter=absw_radius*2,angle=90).center(-absw_xpos, -absw_zpos-slot_half+absw_radius).cutBlind(-10)\
    .faces("<Y").workplane().center(boot_xpos, boot_zpos+slot_half-boot_radius).slot2D(length=slot_half*2,diameter=boot_radius*2,angle=90).center(-boot_xpos, -boot_zpos-slot_half+boot_radius).cutBlind(-10)\
    .faces("<Y").workplane().center(led_xpos_2, led_zpos+slot_half-led_radius).rarray(led_xpos_2-led_xpos_1,0,3,1).slot2D(length=slot_half*2,diameter=led_radius*2,angle=90).center(-led_xpos_2, -led_zpos-slot_half+led_radius).cutBlind(-10)
    
top = top.faces("<Y").workplane().center(usb_xpos, usb_zpos).rect(13.35,6).center(-usb_xpos, -usb_zpos).cutBlind(-10)\
    .faces("<Y").workplane().center(absw_xpos, absw_zpos-slot_half+absw_radius).slot2D(length=slot_half*2,diameter=absw_radius*2,angle=90).center(-absw_xpos, -absw_zpos+slot_half-absw_radius).cutBlind(-10)\
    .faces("<Y").workplane().center(boot_xpos, boot_zpos-slot_half+boot_radius).slot2D(length=slot_half*2,diameter=boot_radius*2,angle=90).center(-boot_xpos, -boot_zpos+slot_half-boot_radius).cutBlind(-10)\
    .faces("<Y").workplane().center(led_xpos_2, led_zpos-slot_half+led_radius).rarray(led_xpos_2-led_xpos_1,0,3,1).slot2D(length=slot_half*2,diameter=led_radius*2,angle=90).center(-led_xpos_2, -led_zpos+slot_half-led_radius).cutBlind(-10)
    

usbc_zpos = zref + 5.77
usbc_xpos = 5.5
usbc_width = 9.3
usbc_height = 3.6


dc_zpos = zref + 5.05
dc_xpos = 19.3
dc_radius = 6.2/2

trs_zpos = zref + 6.7
trs_xpos_1 = -7.5
trs_xpos_2 = -7.5-13
trs_radius = 5.2 / 2

base = base.faces(">Y").workplane().center(usbc_xpos, usbc_zpos).rect(usbc_width, usbc_height).center(-usbc_xpos, -usbc_zpos).cutBlind(-10)\
    .faces(">Y").workplane().center(dc_xpos, dc_zpos+slot_half-dc_radius).slot2D(length=slot_half*2,diameter=dc_radius*2,angle=90).center(-dc_xpos, -dc_zpos-slot_half+dc_radius).cutBlind(-10)\
    .faces(">Y").workplane().center(trs_xpos_1, trs_zpos+slot_half-trs_radius).slot2D(length=slot_half*2,diameter=trs_radius*2,angle=90).center(-trs_xpos_1, -trs_zpos-slot_half+trs_radius).cutBlind(-10)\
    .faces(">Y").workplane().center(trs_xpos_2, trs_zpos+slot_half-trs_radius).slot2D(length=slot_half*2,diameter=trs_radius*2,angle=90).center(-trs_xpos_2, -trs_zpos-slot_half+trs_radius).cutBlind(-10)

top = top.faces(">Y").workplane().center(usbc_xpos, usbc_zpos).rect(usbc_width, usbc_height).center(-usbc_xpos, -usbc_zpos).cutBlind(-10)\
    .faces(">Y").workplane().center(dc_xpos, dc_zpos-slot_half+dc_radius).slot2D(length=slot_half*2,diameter=dc_radius*2,angle=90).center(-dc_xpos, -dc_zpos+slot_half-dc_radius).cutBlind(-10)\
    .faces(">Y").workplane().center(trs_xpos_1, trs_zpos-slot_half+trs_radius).slot2D(length=slot_half*2,diameter=trs_radius*2,angle=90).center(-trs_xpos_1, -trs_zpos+slot_half-trs_radius).cutBlind(-10)\
    .faces(">Y").workplane().center(trs_xpos_2, trs_zpos-slot_half+trs_radius).slot2D(length=slot_half*2,diameter=trs_radius*2,angle=90).center(-trs_xpos_2, -trs_zpos+slot_half-trs_radius).cutBlind(-10)
    
#top = top.faces("<Y").cutBlind(50)
#base = base.faces("<Y").cutBlind(50)
