# Synthino XM Polyphonic MIDI Synthesizer #

The source tree includes

* Synthino XM source code
* Arduino API and hardware configuration for ATxmega128a4u microcontroller
* LUFA USB driver with configuration for Synthino XM
* ported Arduino MIDI library
* simple USBMIDI library API that uses the LUFA driver

Building the source code requires the AVR GCC toolchain:

* Windows: [Atmel AVR Toolchain](http://www.atmel.com/tools/atmelavrtoolchainforwindows.aspx)
* Mac OS X: [CrossPack for AVR Development](http://www.obdev.at/products/crosspack/index.html)

To build:

* 'make'

To upload to the Synthino XM device, hold down button 3 while powering device, then:

* 'make flash'

Utility required to upload synthino_xm.hex to the Synthino XM device:

* Windows: [Atmel Flip](http://www.atmel.com/tools/flip.aspx)
* Mac OS X: [dfu-programmer](http://synthino.com/downloads/DFUProgrammer.dmg)


If you already have dfu-programmer installed (for example, via macports), then instead of the 'make flash' command above, you should run these two commands after holding down button 3 and powering on the device:

* 'dfu-programmer atxmega128a4u erase'
* 'dfu-programmer atxmega128a4u flash synthino_xm.hex


For more information on Synthino XM, visit [synthino.com](http://synthino.com)