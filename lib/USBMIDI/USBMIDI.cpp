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


#include "USBMIDI.h"
#include <stdlib.h>

USBMIDI_Class USBMIDI;

USBMIDI_Class::USBMIDI_Class()
{ 
  mNoteOffCallback  		        = NULL;
  mNoteOnCallback		       	= NULL;
  mAfterTouchPolyCallback		= NULL;
  mControlChangeCallback		= NULL;
  mProgramChangeCallback		= NULL;
  mAfterTouchChannelCallback		= NULL;
  mPitchBendCallback		       	= NULL;
  mClockCallback			= NULL;
  mStartCallback			= NULL;
  mStopCallback				= NULL;
  mSystemResetCallback			= NULL;
}


USBMIDI_Class::~USBMIDI_Class()
{

}

bool USBMIDI_Class::read() {
  MIDI_EventPacket_t midiEvent;;
  boolean received = false;
  while (MIDI_Device_ReceiveEventPacket(&MIDI_Interface, &midiEvent)) {
    received = true;
    uint8_t type;
    if (midiEvent.Event == 0xF) {
      type = midiEvent.Data1;
    } else {
      type = midiEvent.Event << 4;
    }
    uint8_t channel = (midiEvent.Data1+1) & 0xF;
    launchCallback(type, channel, midiEvent.Data2, midiEvent.Data3);
  }

  MIDI_Device_USBTask(&MIDI_Interface);
  USB_USBTask();

  return received;
}



void USBMIDI_Class::setHandleNoteOff(void (*fptr)(byte channel, byte note, byte velocity))			{ mNoteOffCallback = fptr; }
void USBMIDI_Class::setHandleNoteOn(void (*fptr)(byte channel, byte note, byte velocity))			{ mNoteOnCallback = fptr; }
void USBMIDI_Class::setHandleAfterTouchPoly(void (*fptr)(byte channel, byte note, byte pressure))	{ mAfterTouchPolyCallback = fptr; }
void USBMIDI_Class::setHandleControlChange(void (*fptr)(byte channel, byte number, byte value))	{ mControlChangeCallback = fptr; }
void USBMIDI_Class::setHandleProgramChange(void (*fptr)(byte channel, byte number))				{ mProgramChangeCallback = fptr; }
void USBMIDI_Class::setHandleAfterTouchChannel(void (*fptr)(byte channel, byte pressure))			{ mAfterTouchChannelCallback = fptr; }
void USBMIDI_Class::setHandlePitchBend(void (*fptr)(byte channel, int bend))						{ mPitchBendCallback = fptr; }
void USBMIDI_Class::setHandleClock(void (*fptr)(void))												{ mClockCallback = fptr; }
void USBMIDI_Class::setHandleStart(void (*fptr)(void))												{ mStartCallback = fptr; }
void USBMIDI_Class::setHandleContinue(void (*fptr)(void))												{ mContinueCallback = fptr; }
void USBMIDI_Class::setHandleStop(void (*fptr)(void))												{ mStopCallback = fptr; }
void USBMIDI_Class::setHandleSystemReset(void (*fptr)(void))										{ mSystemResetCallback = fptr; }

void USBMIDI_Class::disconnectCallbackFromType(uint8_t type) {
	
  switch (type) {
  case MIDI_COMMAND_NOTE_OFF:
    mNoteOffCallback = NULL;
    break;
  case MIDI_COMMAND_NOTE_ON:
    mNoteOnCallback = NULL;
    break;
  case MIDI_COMMAND_CLOCK:
    mClockCallback = NULL;
    break;
  case MIDI_COMMAND_START:
    mStartCallback = NULL;
    break;
  case MIDI_COMMAND_CONTINUE:
    mContinueCallback = NULL;
    break;
  case MIDI_COMMAND_STOP:
    mStopCallback = NULL;
    break;
  case MIDI_COMMAND_RESET:
    mSystemResetCallback = NULL;
    break;


  case MIDI_COMMAND_NOTE_PRESSURE:
    mAfterTouchPolyCallback = NULL;
    break;
  case MIDI_COMMAND_CONTROL_CHANGE:
    mControlChangeCallback = NULL;
    break;
  case MIDI_COMMAND_PROGRAM_CHANGE:
    mProgramChangeCallback = NULL;
    break;
  case MIDI_COMMAND_CHANNEL_PRESSURE: 
    mAfterTouchChannelCallback = NULL;
    break;
  case MIDI_COMMAND_PITCH_WHEEL_CHANGE:
    mPitchBendCallback = NULL;
    break;


  default:
    break;
  }
	
}


void USBMIDI_Class::launchCallback(byte type, byte channel, byte data1, byte data2) {
	
  switch (type) {
    // Notes
  case MIDI_COMMAND_NOTE_OFF:
    if (mNoteOffCallback != NULL)	mNoteOffCallback(channel,data1,data2);
    break;
  case MIDI_COMMAND_NOTE_ON:
    if (mNoteOnCallback != NULL) mNoteOnCallback(channel,data1,data2);
    break;

  case MIDI_COMMAND_CLOCK:
    if (mClockCallback != NULL) mClockCallback();
    break;
  case MIDI_COMMAND_START:
    if (mStartCallback != NULL) mStartCallback();
    break;
  case MIDI_COMMAND_CONTINUE:
    if (mContinueCallback != NULL) mContinueCallback();
    break;
  case MIDI_COMMAND_STOP:
    if (mStopCallback != NULL) mStopCallback();
    break;
  case MIDI_COMMAND_RESET:
    if (mSystemResetCallback != NULL) mSystemResetCallback();
    break;
			
		  
    // Continuous controllers
  case MIDI_COMMAND_CONTROL_CHANGE:
    if (mControlChangeCallback != NULL) mControlChangeCallback(channel,data1,data2);  
    break;
  case MIDI_COMMAND_PITCH_WHEEL_CHANGE:
    if (mPitchBendCallback != NULL) mPitchBendCallback(channel,(int)((data1 & 0x7F) | ((data2 & 0x7F)<< 7)) - 8192);
    break;
  case MIDI_COMMAND_NOTE_PRESSURE:
    if (mAfterTouchPolyCallback != NULL) mAfterTouchPolyCallback(channel,data1,data2);
    break;
  case MIDI_COMMAND_CHANNEL_PRESSURE:
    if (mAfterTouchChannelCallback != NULL) mAfterTouchChannelCallback(channel,data1);
    break;
  case MIDI_COMMAND_PROGRAM_CHANGE:
    if (mProgramChangeCallback != NULL) mProgramChangeCallback(channel,data1);
    break;
  default:
    break;
  }
}



