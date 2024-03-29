# Adalight RGB Clone for Zephyr

This project clones the serial/USB CDC protocol used by Adalight RGB LEDs using Zephyr RTOS, and allows the use of a Zephyr-supported LED strip (e.g. WS2812 controller based) with a supported microcontroller/board so that it can be used from a PC.

The intended usage for this project is with Prismatik: https://github.com/psieg/Lightpack

## Requirements

 * Zephyr-supported board, see: https://docs.zephyrproject.org/latest/boards/index.html
 * Zephyr-supported LED strip, see: 
 * Serial/UART/CDC interface to PC
 * SWD/JTAG Programmer (for initial bootloader programming)

Default supported boards are: `stm32_bluepill` and `bl653_dvk`, any other board will need configuration created for it to be able to build this firmware.

## Setup

Follow the Zephyr getting started guide to get the required tools to be able to configure and build the firmware: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

## Getting the code

To get the code, create a new directory where you would like to download the code, get the latest version using west:

    west init -m https://github.com/thedjnK/adalight_rgb.git --mr main

Then download all the required libraries and modules using:

    west update

## Configuring (for custom boards not supported in-tree)

todo

## Building

### MCUboot

MCUboot is used as the bootloader for this project to allow for updating the code on devices without needing an external SWD/JTAG programmer by holding a GPIO at a specific level when powering the board up. The board firmware can then be updated via serial port/USB CDC using mcumgr.

To build MCUboot:

todo

### Adalight RGB

todo

## Circuit

### stm32_bluepill

todo

## Zephyr version

This code targets Zephyr RTOS 3.5

## License

This code is released under the Apache 2.0 license
