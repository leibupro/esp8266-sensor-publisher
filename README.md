# esp8266-sensor-publisher

Application assets (user application source, sdk headers,
sdk libraries, linker scripts) are in [`application`](application)
directory. Therefore, one can build a binary from within this
repository without having the sdk installed, only with the
`xtensa-lx106-elf-gcc` compiler tool chain. Example sdk projects
(e.g. [`hello_world`](hello_world), [`wifi_station`](wifi_station))
are only there, if one has the sdk installed additionally, e.g. under
`/opt/esp8266-rtos-sdk`. This enables to change the configuration (run
`make menuconfig` in the example project directory) and re-compile the
sdk libraries.

The flash routines for the application intentionally only flash the
application binary to the ROM address `0x10000`. This is in contrast
to the default sdk project builds which also re-flash the bootloader
every time.

[`grab_libraries`](application/grab_libraries) script is only for
copying all the needed sdk assets to the application directory.

Binary artefacts (i.e. `*.a`) from the sdk are managed through `git`
large file storage, LFS.


## Prerequisites

`esp8266` is a legacy machine which uses an exotic ISA, `xtensa`.
(Instead of `riscv32` as is on newer esp devices.) Therefore one has
to install the according compiler/linker toolchain and the sdk sources
for the `esp8266` device.

Assuming Arch Linux, these resources are available as `AUR` packages.
The `esptool` is available as official Arch package.

 * `xtensa-lx106-elf-gcc-bin`
 * `esp8266-rtos-sdk`
 * `esptool`


## SDK libraries

The only purpose of the [`hello_world`](hello_world) directory is to
build the static libraries of the `esp8266` sdk. When building the
application, one can then link against these libraries. It contains
the [`cmake_build`](hello_world/cmake_build) script in order to
initiate the `cmake` build procedure.

