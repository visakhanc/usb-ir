
USB-IR - IR receiver based on V-USB
===================================

![HID IR Remote](https://github.com/visakhanc/usb-ir/blob/master/pic/side-2.jpg)

USB-IR is an infrared remote control receiver with a USB communication enabled by v-usb usb stack. So, it can be programmed as an IR receiver for PC with specific functions assigned to various keys of a remote control.


HID Bootloader option
---------------------

Firmware updates can be done through USB itself! For this, HID bootloader should be initially flashed into the AVR through ISP pins. After this, to program firmware using bootloader, just plug in the USB while pressing and holding the button. It will get detected as 'HIDBoot' device. Now use the command line tool for HID bootloader to program hex files. like this:
bootloadHID.exe -r main.hex

Find the HID bootloader here: [HID bootloader](https://github.com/visakhanc/vusb_projects/tree/master/bootloadHID.2012-12-08)


Examples
--------

[HID IR Remote](https://github.com/visakhanc/usb-ir/blob/master/firmware/hid-ir-remote) : Demonstrates the usage of USB-IR as a multimedia remote control receiver for PC. Sony IR protocol (SIRC) is used here. Implemented functions are: Volume Up/Down, Next, Previous, Pause, Play, Stop, Mute.


