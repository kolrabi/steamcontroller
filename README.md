# Steam Controller Library

This is a little C library for Linux based operating systems that allows accessing the Steam Wireless Controller as a gamepad. It exposes all button and axis data as well as acceleration, angular velocity and spatial orientation. 

It is also possible to configure certain controller features and use its haptic feedback capabilities.

## Get started

Get the source and build it. The project can be built with CMake, but I guess you could just compile the .c file into a library.

### Pitfalls

- You will need access to the hidraw devices. That means you will either have to change permissions on them or run as root. This dark udev magic should do the trick:

        SUBSYSTEM=="usb", ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1042", GROUP="plugdev", MODE="0660"
        SUBSYSTEM=="usb", ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1102", GROUP="plugdev", MODE="0660"
        SUBSYSTEM=="usb", ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1142", GROUP="plugdev", MODE="0660"

    You need to be member of the specified group of course.

- Don't expect this library to work properly while steam is running. It enables mouse/keyboard emulation when it is in the background.

- After exiting steam the controller can be in a state that makes it undetectable for the library. Simply unplugging and replugging should help in that case.

- If you against all warnings decide to activate the wireless dongle bootloader, only Steam can get you out of this.

## TODO

(In no particular order)

- Figure out how to access the device without running as root or changing permissions. Steam games are able to do it too.
- Figure out how HID works under Windows and create a port for that as well.
- Research multiple controllers per dongle.
- Figure out scale and units for angular velocity.
- Understand pairing better, especially the meaning of `0x3c`.
- Add some example code for using the controller.
- Figure out the unknown feature ids.
- Confirm that the orientation vector is indeed part of a quaternion.
- Maybe completing the bootloader stuff. 
- Documentation.

## License

**The MIT License (MIT)**

*Copyright (c) 2016 Björn Paetzel* 

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
