import argparse
import os.path
import zipfile
import xml.etree.ElementTree as ET

objectmaps = {
    "PathScripts.PathAdaptive": "Path.Op.Adaptive",
    "PathScripts.PathCustom": "Path.Op.Custom",
    "PathScripts.PathDeburr": "Path.Op.Deburr",
    "PathScripts.PathDressupDogbone": "Path.Dressup.DogboneII",
    "PathScripts.PathDressupHoldingTags": "Path.Dressup.Tags",
    "PathScripts.PathDrilling": "Path.Op.Drilling",
    "PathScripts.PathEngrave": "Path.Op.Engrave",
    "PathScripts.PathHelix": "Path.Op.Helix",
    "PathScripts.PathIconViewProvider": "Path.Base.Gui.IconViewProvider",
    "PathScripts.PathJob": "Path.Main.Job",
    "PathScripts.PathMillFace": "Path.Op.MillFace",
    "PathScripts.PathPocketShape": "Path.Op.Pocket",
    "PathScripts.PathProbe": "Path.Op.Probe",
    "PathScripts.PathProfile": "Path.Op.Profile",
    "PathScripts.PathSetupSheet": "Path.Base.SetupSheet",
    "PathScripts.PathSlot": "Path.Op.Slot",
    "PathScripts.PathStock": "Path.Main.Stock",
    "PathScripts.PathSurface": "Path.Op.Surface",
    "PathScripts.PathThreadMilling": "Path.Op.ThreadMilling",
    "PathScripts.PathToolBit": "Path.Tool.Bit",
    "PathScripts.PathToolController": "Path.Tool.Controller",
    "PathScripts.PathVcarve": "Path.Op.Vcarve",
    "PathScripts.PathWaterline": "Path.Op.Waterline",
}

viewprovidermaps = {
    "PathScripts.PathAdaptiveGui": "Path.Op.Gui.Adaptive",
    "PathScripts.PathCustomGui": "Path.Op.Gui.Custom",
    "PathScripts.PathDeburrGui": "Path.Op.Gui.Deburr",
    "PathScripts.PathDressupTagGui": "Path.Dressup.Gui.Tags",
    "PathScripts.PathDrillingGui": "Path.Op.Gui.Drilling",
    "PathScripts.PathEngraveGui": "Path.Op.Gui.Engrave",
    "PathScripts.PathIconViewProvider": "Path.Base.Gui.IconViewProvider",
    "PathScripts.PathJobGui": "Path.Main.Gui.Job",
    "PathScripts.PathMillFaceGui": "Path.Op.Gui.MillFace",
    "PathScripts.PathOpGui": "Path.Op.Gui.Base",
    "PathScripts.PathPocketShapeGui": "Path.Op.Gui.Pocket",
    "PathScripts.PathProbeGui": "Path.Op.Gui.Probe",
    "PathScripts.PathProfileGui": "Path.Op.Gui.Profile",
    "PathScripts.PathSetupSheetGui": "Path.Base.Gui.SetupSheet",
    "PathScripts.PathSlotGui": "Path.Op.Gui.Slot",
    "PathScripts.PathSurfaceGui": "Path.Op.Gui.Surface",
    "PathScripts.PathToolBitGui": "Path.Tool.Gui.Bit",
    "PathScripts.PathToolControllerGui": "Path.Tool.Gui.Controller",
    "PathScripts.PathVcarveGui": "Path.Op.Gui.Vcarve",
    "PathScripts.PathWaterlineGui": "Path.Op.Gui.Waterline",
}
#simple copy
#propertybag


def is_valid_file(parser, arg):
    if not os.path.exists(arg):
        parser.error("The file %s does not exist!" % arg)
    else:
        return open(arg, "r")  # return an open file handle


parser = argparse.ArgumentParser(description="Fixes FreeCAD Path Jobs")
parser.add_argument(
    "-i",
    dest="filename",
    required=True,
    help="input FreeCAD project file",
    metavar="FILE",
    type=lambda x: is_valid_file(parser, x),
)
parser.add_argument(
    "-o",
    dest="outputfilename",
    required=True,
    help="output FreeCAD project file",
    metavar="FILE",
)
args = parser.parse_args()


def cleanupDoc(docxml, node):
    print(f"cleaning {node}")
    tree = ET.parse(docxml)
    root = tree.getroot()

    pathmaps = objectmaps if node == "ObjectData" else viewprovidermaps

    objects = root.find(node)
    for child in objects:
        res = child.findall("./Properties/Property[@name='Proxy']/Python")
        for r in res:
            try:
                modstring = r.attrib["module"]
            except:
                pass
                #print(ET.tostring(r))
            if modstring not in pathmaps:
                print(f"module {modstring} not substituted")
            else:
                #if node =="ViewProviderData" and modstring == "PathScripts.PathOpGui":
                #    opname = child.attrib['name']
                #    print(f"Operation node: {opname}")
                #    if opname == "Slot":
                #        #mapout = pathmaps[modstring]
                #        #r.attrib["module"] = f"{mapout}.Slot"
                #        r.attrib["encoded"] = "no"
                #        r.attrib["value"] = ""

                mapout = pathmaps[modstring]
                r.attrib["module"] = mapout
                # print(f"substituted {mapout} for {modstring}")
    newXML = ET.tostring(root)
    return newXML


with zipfile.ZipFile(args.filename.name, "r") as zin:
    with zipfile.ZipFile(args.outputfilename, "x") as zout:
        zout.comment = zin.comment
        for item in zin.infolist():
            if item.filename == "Document.xml":
                with zin.open(item.filename) as docxml:
                    newdata = cleanupDoc(docxml, "ObjectData")
                    zout.writestr(item.filename, newdata)
            elif item.filename == "GuiDocument.xml":
                with zin.open(item.filename) as docxml:
                    newdata = cleanupDoc(docxml, "ViewProviderData")
                    zout.writestr(item.filename, newdata)
            else:
                zout.writestr(item, zin.read(item.filename))
