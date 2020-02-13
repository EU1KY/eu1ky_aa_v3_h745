# EU1KY Antenna Analyzer V3

This is a port of [eu1ky_aa_v3](https://github.com/EU1KY/eu1ky_aa_v3) for STM32H745I DISCOVERY board

## Getting Started

### Prerequisites

The firmware can be built in both Windows and Linux.

You'll need the following prerequisites to be installed:

* [Python 3](https://python.org), in system path, including its Scripts subdirectory.
* intelhex Python module (can be installed with command line 'pip install intelhex').
* [Git SCM](https://git-scm.com/).
* (Windows only) [GNU make for Windows](https://github.com/mbuilov/gnumake-windows) Download appropriate prebuilt file, rename it to make.exe and put it somewhere in a system PATH.
* (Linux only) build-essential package (e.g. in Ubuntu, install it with 'sudo apt-get install build-essential')
* [GNU ARM embedded toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads). Path to its bin subdirectory must be in the system PATH.
* [STM32 CubeProg](https://www.st.com/en/development-tools/stm32cubeprog.html), in system PATH

It is recommended to use [Visual Studio Code](https://code.visualstudio.com/download) as a convenient code editor, workspace file for it is included.
"C/C++" and "VsCode Action Buttons" extensions are highly recommended for it.
Though, VSCode is not very convenient for in-system debugging (with Native Debug extension)
because it does not show hardawre registers from SVD file. I prefer debugging in Windows,
using [EmBitz 1.11](https://www.embitz.org/) and [EBLink](https://github.com/EmBitz/EBlink) as GDB server.
EmBitz project file is also included.
SVD (STM32H7 System View Description) files for STM32H7 can be found on the [ST Microelectronics site](https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32h7-series/stm32h745-755/stm32h745xi.html#resource).

### Build instructions

To clone repository, open command interpreter in some convenient directory and execute the following commands:

```
git clone https://github.com/EU1KY/eu1ky_aa_v3_h745
cd eu1ky_aa_v3_h745
```

To build the release target, just open a command line interpreter in the project's directory, and run

```
make -j4
```

If you want to build unoptimized code, run

```
make -j4 debug
```

The build should be finished with no errors and no warnings.

## Flashing the firmware

In Windows you can connect the onboard ST-Link to PC with Micro-USB cable and then drag and drop
appropriate built FW binary (out/Debug/fw_image.bin, or out/Release/fw_image.bin) to the drive that appears in the system.

The same method should probably work in Linux.

Another way is to use STM32 CubeProg by running

```
make flash
```

to flash Release firmware, or

```
make flashdbg
```

to flash Debug.

## Differences from STM32F7 discovery

Though the boards are very similar, there are some significant differences that do not allow to port the code as a simple branch of the original one.

* STM32H7 has two CPU cores in SoC. Currently the code has been ported to Cortex-M7 core only, but I am planning to utilize the second core, Cortex-M4,
for HTML5 based web interface running on the device.
* The STM32H745I Discovery board has on-board eMMC, so you don't need an SD card.
* I've added NanoVNA protocol support, so NanoVNA Saver software can be used as an external interface to the device. You can choose the protocol used
in the Configuration menu.
* I am planning to build a new, significantly more capable RF frontend. Its support will be implemented first for this Discovery board, and, probably,
will be ported back to F746 Discovery, but later.
* An image file named logo.png can be placed in /aa/ directory. The image must be a PNG with dimensions 480x272 pixels. If it is there, and strictly
satisfies the specified dimensions, it will be displayed when the device boots up.

# License

As usually, WTFPL.

---------------
73! Yury Kuchura EU1KY
