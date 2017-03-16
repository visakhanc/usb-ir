
USB-IR - V-USB based IR receiver
================================

![HID IR Remote](usb-ir/pic/side-2.jpg)

USB-IR is an infrared remote control receiver with a USB communication enabled by v-usb usb stack. So, it can be programmed as an IR receiver for PC with specific functions assigned to various keys of a remote control.


HID Bootloader option
---------------------

Firmware updates can be done through USB itself! For this, HID bootloader should be flashed into the AVR through ISP pins. 
To enter bootloader mode, press and hold the button while plugging the USB; it will get detected as 'HIDBoot' device.  After this, the command line tool for HID bootloader can be used to load hex files. Like this:
bootloadHID.exe -r main.hex


Examples
--------

[HID IR Remote](https://github.com/visakhanc/usb-ir/firmware/hid-ir-remote) : Demonstrates the usage of USB-IR as a multimedia remote control receiver for PC. Sony IR protocol (SIRC) is used here. Implemented functions are: Volume Up/Down, Next, Previous, Pause, Play, Stop, Mute.


