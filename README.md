UnoCart
=======
The UnoCart is an SD-card multicart for the Atari 8-bit, supporting ROMs,CARs,XEXs and some ATR files.
It is the little brother of the [Ultimate Cart](https://github.com/robinhedwards/UltimateCart/).

The multicart is based on a STM32F4 microcontroller running at 168MHz. This is (just) fast enough to respond to requests
from the Atari's 6502 bus clocked at approximately 2 MHz. Files are loaded from SD card into the STM32F4's SRAM allowing
emulation of Atari cartridges up to 128k in size.

![Image](images/menu_small.jpg?raw=true)

When the Atari first boots, the cartridge displays a list of all the files on the SD card. When a ROM or CAR file is
selected, the cartridge will then emulate the selected cartridge type. The UnoCart supports standard 8k and 16k ROMS,
XEGS cartridges up to 128k in size, AtariMax 1mbit, Bounty Bob, OSS cartridges and many more.

XEX files (Atari executables) are also supported using a XEX loader built into the cartridge.

The cartridge can also emulate a disk drive. It does this by installing a Soft OS into the Atari which then redirects
D1: to the ATR file selected from the menu. Access for D2: upwards continue to be directed to the SIO port as normal.
Many games will not work, due to the soft OS technique used. However it is possible to boot to a DOS 2.5 ATR file,
do some programming in BASIC and save your program back to the ATR file. ATR files up to 16Meg in size are supported.

Hardware
--------
The UnoCart was first designed using a STM32F407 DISCOVERY board. An article describing how to build an UnoCart using
this board will be published in a future issue of [Excel Magazine](http://excel-retro-mag.co.uk).

I have also designed a custom PCB for the UnoCart pictured here. If you are interested in an assembled board, please
contact me on the AtariAge forums (my username is Electrotrains).

![Bottom/Back of PCB when inserted in Atari](images/board_front.jpg?raw=true)
![Top/Front of PCB when inserted in Atari](images/board_back.jpg?raw=true)

Firmware
--------
The UnoCart firmware is open source under a GPL license and is hosted here.

