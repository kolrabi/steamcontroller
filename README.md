# Steam Controller Library

This is a small C++ library for Windows, Linux and Mac OS systems that allows accessing the Steam Controller without steam. It exposes all button and axis data as well as acceleration, angular velocity and spatial orientation. 

It is an adaption of the earlier https://github.com/kolrabi/steamcontroller library, with a few significant changes:

- Port to C++, restructure API, remove C# wrapper
- Replace the backend by a minified & fixed version of https://github.com/signal11/hidapi
- Implement non-blocking API where possible

The changes were made mostly to be able to use this library in production for my game [abstractanks](https://ltjax.itch.io/abstractanks).

# Install (conan)

First you are going to have to my repository as a conan remote:
```
conan remote add ltjax https://api.bintray.com/conan/ltjax/conan 
```

Then you can add the dependency via
```
steam_controller/1.0@ltjax/testing
```

# How to use

There is a small demo program using this in the ```demo/``` folder that should be enough to get you started.

# Pitfalls

- You will need access to the hidraw devices. That means you will either have to change permissions on them or run as root. This dark udev magic should do the trick:

        SUBSYSTEM=="hidraw", ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1042", GROUP="plugdev", MODE="0660"
        SUBSYSTEM=="hidraw", ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1102", GROUP="plugdev", MODE="0660"
        SUBSYSTEM=="hidraw", ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1142", GROUP="plugdev", MODE="0660"

    You need to be member of the specified group of course.

- Don't expect this library to work properly while steam is running. It enables mouse/keyboard emulation when it is in the background.

- After exiting steam the controller can be in a state that makes it undetectable for the library. Simply unplugging and replugging should help in that case.

- If you against all warnings decide to activate the wireless dongle bootloader, only Steam can get you out of this.


