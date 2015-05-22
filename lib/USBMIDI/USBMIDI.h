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


#ifndef LIB_USBMIDI_H_
#define LIB_USBMIDI_H_

#include <inttypes.h> 
#include "usb.h"

// define MIDI messages not defined in the LUFA library
#define MIDI_COMMAND_CLOCK    0xF8
#define MIDI_COMMAND_START    0xFA
#define MIDI_COMMAND_CONTINUE 0xFB
#define MIDI_COMMAND_STOP     0xFC
#define MIDI_COMMAND_RESET    0xFF

class USBMIDI_Class {
	
	
public:
	// Constructor and Destructor
	USBMIDI_Class();
	~USBMIDI_Class();
	
public:
	
	bool read();
	void setHandleNoteOff(void (*fptr)(byte channel, byte note, byte velocity));
	void setHandleNoteOn(void (*fptr)(byte channel, byte note, byte velocity));
	void setHandleAfterTouchPoly(void (*fptr)(byte channel, byte note, byte pressure));
	void setHandleControlChange(void (*fptr)(byte channel, byte number, byte value));
	void setHandleProgramChange(void (*fptr)(byte channel, byte number));
	void setHandleAfterTouchChannel(void (*fptr)(byte channel, byte pressure));
	void setHandlePitchBend(void (*fptr)(byte channel, int bend));
	void setHandleClock(void (*fptr)(void));
	void setHandleStart(void (*fptr)(void));
	void setHandleContinue(void (*fptr)(void));
	void setHandleStop(void (*fptr)(void));
	void setHandleSystemReset(void (*fptr)(void));
	
	void disconnectCallbackFromType(uint8_t type);
	
	
private:
	
	void launchCallback(byte type, byte channel, byte data1, byte data2);
	
	void (*mNoteOffCallback)(byte channel, byte note, byte velocity);
	void (*mNoteOnCallback)(byte channel, byte note, byte velocity);
	void (*mAfterTouchPolyCallback)(byte channel, byte note, byte velocity);
	void (*mControlChangeCallback)(byte channel, byte, byte);
	void (*mProgramChangeCallback)(byte channel, byte);
	void (*mAfterTouchChannelCallback)(byte channel, byte);
	void (*mPitchBendCallback)(byte channel, int);
	void (*mClockCallback)(void);
	void (*mStartCallback)(void);
	void (*mContinueCallback)(void);
	void (*mStopCallback)(void);
	void (*mSystemResetCallback)(void);
	
};

extern USBMIDI_Class USBMIDI;

#endif // LIB_USBMIDI_H_
