/*
  Synthino polyphonic synthesizer
  Copyright (C) 2014-2015 Michael Krumpus

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef _USB_H_
#define _USB_H_

#define F_USB 48000000
#define ARCH ARCH_XMEGA
//#define USBDEBUG

#include <LUFA/Common/Common.h>
#include <LUFAConfig.h>
#include <synthinoxm_usb.h>
#include <Arduino.h>

#ifdef __cplusplus
extern "C"{
#endif

  extern USB_ClassInfo_MIDI_Device_t MIDI_Interface;
  void initUSB();

#ifdef __cplusplus
} // extern "C"
#endif

#endif
