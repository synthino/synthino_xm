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

#include "usb.h"

USB_ClassInfo_MIDI_Device_t MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
				.DataINEndpoint           =
					{
						.Address          = MIDI_STREAM_IN_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint          =
					{
						.Address          = MIDI_STREAM_OUT_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
			},
	};


void initUSB() {
  // Start internal 32MHz oscillator
  OSC.CTRL |= OSC_RC32MEN_bm;
  while (!(OSC.STATUS & OSC_RC32MRDY_bm));

  uint16_t DFLLCompare = (F_USB / 1024);

  OSC.DFLLCTRL   |= OSC_RC32MCREF_USBSOF_gc;
  DFLLRC32M.COMP1 = (DFLLCompare & 0xFF);
  DFLLRC32M.COMP2 = (DFLLCompare >> 8);

  NVM.CMD        = NVM_CMD_READ_CALIB_ROW_gc;
  DFLLRC32M.CALA = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSCA));
  DFLLRC32M.CALB = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC));
  NVM.CMD        = NVM_CMD_NO_OPERATION_gc;

  DFLLRC32M.CTRL  = DFLL_ENABLE_bm;
  USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
#ifdef USBDEBUG
  digitalWrite(24, LOW);
  digitalWrite(25, HIGH);
  digitalWrite(20, HIGH);
  digitalWrite(21, LOW);
#endif
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
#ifdef USBDEBUG
  digitalWrite(24, HIGH);
  digitalWrite(25, LOW);
  digitalWrite(20, LOW);
  digitalWrite(21, LOW);
#endif
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&MIDI_Interface);

	if (!ConfigSuccess) {
	  // error: blink LED 4
	  for(int i=0;i<5;i++) {
	    digitalWrite(21, HIGH);
	    delay(50);
	    digitalWrite(21, LOW);
	    delay(50);
	  }
	}


}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&MIDI_Interface);
}
