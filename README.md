## KNOT - USB MIDI Host

Knot is the bridge between old-school MIDI gear like synthesizers, groove boxes, guitar pedals and neat USB MIDI devices. Leave out the computer from the equation and control instruments seamlessly with Grid - or any other - USB controller.

![knot, the standalone usb midi host](https://storage.googleapis.com/intechstudio-storage/knot_usb_midi_host-1.png)

You can find more info on the [website](https://intech.studio/shop/knot) or in the [Knot - Starter's Guide](https://docs.intech.studio/guides/knot/knot-start).

## Electrical components

| Component                 | Part number          | Preview | Links        |
|---------------------------|----------------------|---------|--------------|
| KNOT board            | PCBA-KNOT       | <img src="../preview/Preview/Electrical/PCBA-KNOT-3D_top.png?raw=true" width="150"> <img src="../preview/Preview/Electrical/PCBA-KNOT-3D_bottom.png?raw=true" width="150"> | [[Schematic]](../preview/Preview/Electrical/PCBA-KNOT_Schematic.pdf) [[STEP]](../preview/Preview/Electrical/PCBA-KNOT-3D.step?raw=true) [[Gerber]](../preview/Preview/Electrical/mfg/PCBA-KNOT-gerber.zip?raw=true)  [[PCB3D]](../preview/Preview/Electrical/PCBA-KNOT-blender_export.pcb3d?raw=true)  |

## Mechanical components

| Component                 | Part number          | Type       | Preview | Links        |
|---------------------------|----------------------|------------|---------|--------------|
| Aluminum housing          | KNOT-HOUSING         | Fabricated | <img src="../preview/Preview/Mechanical/KNOT-HOUSING.png?raw=true" width="100"> | [[step]](../preview/Preview/Mechanical/KNOT-HOUSING.step) [[stl]](../preview/Preview/Mechanical/KNOT-HOUSING.stl) [[pdf]](../preview/Preview/Mechanical/KNOT-HOUSING_Drawing.pdf) |
| Aluminum front panel      | KNOT-FRONT           | Fabricated | <img src="../preview/Preview/Mechanical/KNOT-FRONT.png?raw=true" width="100"> | [[step]](../preview/Preview/Mechanical/KNOT-FRONT.step) [[stl]](../preview/Preview/Mechanical/KNOT-FRONT.stl) [[pdf]](../preview/Preview/Mechanical/KNOT-FRONT_Drawing.pdf) |
| Aluminum back panel       | KNOT-BACK            | Fabricated | <img src="../preview/Preview/Mechanical/KNOT-BACK.png?raw=true" width="100"> | [[step]](../preview/Preview/Mechanical/KNOT-BACK.step) [[stl]](../preview/Preview/Mechanical/KNOT-BACK.stl) [[pdf]](../preview/Preview/Mechanical/KNOT-BACK_Drawing.pdf) |

### Nightly Firmware version

The nightly firmware is an early release version of the stable release. As it is an early version, some issues could still persist. In the case of issues, please denote the usage of a nightly firmware in the issue report.

[Nightly Release](https://github.com/intechstudio/knot/raw/preview/Preview/Firmware/knot_esp32_nightly.uf2)

### Updating the firmware

- Hold the mode button while plugging the device using the USB-C port of the Knot.
- The device is identified as a USB mass storage named KNOT-S3 (3 green LEDs).
- The file "INFO_UF2.txt" only contains information for the bootloader version, not the firmware itself.
- You can check the current firmware version, opening the file "CURRENT.UF2" with a text or hexadecimal application.
- The first lines will contain the firmware date text. (e.g "May 29 2024" for the firmware "v1.0.5").
- Download the file "knot_release.zip" from the latest release [Stable Release](https://github.com/intechstudio/knot/releases) and extract it.
- Do not rename the ".uf2" file and copy it to the KNOT-S3 mass storage.
- The device will update and reboot alone.
- 3 blinking white LEDs means that the update is finished and that the device is waiting to a USB MIDI device.

### Knot features

- Standalone USB MIDI Host, works with Grid and any MIDI class compliant USB controller
- TRS MIDI input port
- TRS MIDI output port
- USB-C port for powering, updating the device
- DC in port for powering (6V or 15V with automatic polarity detection)
- Hardware A/B switch for changing the TRS wiring mode
- Mode button for updating the firmware
- 3 indicator LEDs, which will be utilized for various feedback

### USB MIDI Device compatibility with Knot as a host

*If you own a device you couldn't find in the list below, help us out by submitting a test of your own to support@intech.studio!*

| Manufacturer | Device name | Compatibility with Knot | Tester | Note |
| ---- | ---- | ---- | ---- | --- |
|    Intech Studio  |   Grid         | Tested - OK | Intech | |
|    Alesis  | Qmini  |  Tested - Not working | Community | |
|    AKAI  | MPK Mini MK2  |  Tested - OK | Community | |
|    AKAI  | MPK Mini Plus  |  Tested - OK | Community | |
|    Arturia  | Beatstep (non-Pro)  |  Tested - OK | Community | |
|    Arturia  | Keystep  |  Tested - OK | Community | |
|    Arturia  | MiniLab MK2   |  Tested - OK | Community | |
|    DoReMIDI  | USB-C MIDI adapter  |  Tested - OK | Community | |
|    Elektron  | Digitakt |  Tested - Not working | Community | |
|    Korg  | Electribe ES2  |  Tested - OK | Intech | |
|    Korg  | Microkey Air 37  |  Tested - OK | Community | |
|    Korg  | Nanokey 2  |  Tested - OK | Community | |
|    Korg  | Nanopad 2  |  Tested - Not working | Community | |
|    Livid Instruments  | DS-1  |  Tested - OK | Community | |
|    Midi Fighter  | Midi Fighter Twister  |  Tested - OK | Community | |
|    M-AUDIO  |   MIDISPORT UNO         | Tested - OK | Intech | |
|    M-AUDIO | Keystation MK3 (all key sizes) | Tested - Not working | Intech | |
|    Novation  | Circuit   | Tested - OK | Community | |
|    Novation   | Launch Control XL2  | Tested - OK | Community | |
|    Novation   | Launchpad MK3 Mini  | Tested - OK | Intech | |
|    Novation  | Launchpad MK2     | Tested - OK | Intech | |
|    Novation  | Launchkey 25   | Tested - Only Power | Intech | |
|    Roland  | MC-101   | Tested - OK | Community | |
|    Sonicware  | ELZ_1  |  Tested - OK | Community | |
|    Teenage Engineering  | EP-133 |  Tested - OK | Community | *Knot supplies ample power to the device* |
|    Teenage Engineering  | OP-1  |  Tested - No MIDI | Community | |
|    Teenage Engineering  | OP-1 field |  Tested - No MIDI | Community | |
|    Teenage Engineering  | OP-Z |  Tested - OK | Community | |




### 5-pin DIN/TRS MIDI Device compatibility

*If you own a device you couldn't find in the list below, help us out by submitting a test of your own to support@intech.studio!*

| Manufacturer | Device name | MIDI connector |Compatibility | Tester | Note |
| ---- | ---- | ---- | ---- | ---- | --- |
| 1010music |   Blackbox | TRS type-B       | Tested - OK |  Community | |
| 1010music |   Bluebox | TRS type-B       | Tested - OK |  Community | |
| Arturia  | Beatstep (non-Pro) | TS "type-C" |  Tested - !!! | Community | *Needs 3.5mm TS to TRS adapter* |
|Behringer |   TD-3-AM | 5-pin DIN       | Tested - OK |  Intech | *With adapter* |
|Behringer |   U-PHORIA UMC404HD | 5-pin DIN     | Tested - OK | Intech | *With adapter* |
|   Dreadbox  | Nymphes  | TRS type-B  | Tested - OK | Community | |
|    Elektron  | Digitakt | 5-pin DIN | Tested - OK | Community | *With adapter* |
|   Korg  | Electribe ES2  | TRS type-B  | Tested - OK | Intech | |
|   Korg   | Volca Series (Tested on Volca FM)  |  5-pin DIN | Tested - OK | Intech | *With adapter* |
| Mutable Instruments |   Yarns | 5-pin DIN     | Tested - OK | Intech | *With adapter* |
|    Polyend  | Tracker  | TRS type-B | Tested - OK | Intech | |
|    Teenage Engineering  | EP-133 | TRS type-A | Tested - OK | Community |  |
|    Teenage Engineering  | OP-Z | TRS type-A | Tested - OK | Community | *With Line Module* |
|    Woovebox  | Woovebox | TRS type-A | Tested - OK | Community | |

<!---
### Community contributors

A big thank you to our community contributors for testing these devices:
- Michal
- Tibi


|    Dirtywave  | M8 Tracker  | TRS type-A | Untested | - |
|   Audiothingies   | Micromonsta 2  | TRS type-A | Untested | - |
|    Bastl Instruments  | Softpop SP2  | 5-pin DIN | Untested | - |
|    Elektron  | Octatrack  | 5-pin DIN | Untested | - |
|    Elektron  | Syntax  | 5-pin DIN | Untested | - |
|    Elektron  | Digitakt  | 5-pin DIN | Untested | - |
|    Elektron  | Digitone  | 5-pin DIN | Untested | - |
|    Elektron  | Analog Rytm  | 5-pin DIN | Untested | - |
|    Elektron  | Analog Four  | 5-pin DIN | Untested | - |
|    Elektron  | Analog Keys  | 5-pin DIN | Untested | - |
|    Elektron  | Model:Samples  | TRS type-A | Untested | - |
|    Elektron  | Model:Cycles  | TRS type-A | Untested | - |
|    Arturia  | Microfreak  | TRS type-A  | Untested | - |
--->
