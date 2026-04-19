# esp8266-sensor-publisher

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

