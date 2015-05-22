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

#include "synthino_xm.h"

boolean arpRunning;
byte arpTimeDivision; // 1/16 notes
int arpNoteStartPulse;
byte arpLEDIndex;
byte arpRootNote;   // the root note of the arpeggio
byte arpNoteLength; // expressed in pulses, max is arpNoteStartPulse
volatile byte arpPosition;   // current position in the arpeggio
volatile byte arpNoteIndex;  // index in note[] of note being played.
unsigned long arpReleaseTime;
byte arpLength;     // number of total notes in the arpeggio
byte arpPattern; // up, down, updown, random
byte arpType;
int arpDirection; // used for UPDOWN pattern
byte arpIntervals[ARP_TYPE_MAX][8]; // no values for ARP_TYPE_ MIDI
byte arpLengths[ARP_TYPE_MAX];      // no values for ARP_TYPE_ MIDI
byte arpeggio[ARP_MAX_NOTES];
unsigned long arpeggioNoteTime[ARP_MAX_NOTES];
byte midiArpeggioNotes[ARP_MAX_NOTES];
unsigned long midiArpeggioNoteTime[ARP_MAX_NOTES];
boolean arpMIDILatch; // Should MIDI invoked arpeggio stay on when keys released


void doArpeggiator() {
  if (pulseClock == arpReleaseTime) {
    // Time to release the currently playing note.
    if (arpNoteIndex != UNSET) {
      releaseNote(&note[arpNoteIndex]);
    }
    digitalWrite(led[arpLEDIndex], LOW);
    ledState[arpLEDIndex] = LOW;
    arpNoteIndex = UNSET;
  }

  if ((arpLength > 0) && (pulseClock % arpNoteStartPulse) == 0) {
    // Time to start the next note.
    byte midiNote = arpeggio[arpPosition];
    if (arpNoteIndex != UNSET) {
      debugprint("**** stuck!");
      debugprint(" rel= ", arpReleaseTime);
      debugprintln(" pc= ", pulseClock);
      releaseNote(&note[arpNoteIndex]);
    }
    arpNoteIndex = doNoteOn(1, midiNote, MAX_VELOCITY);
    arpReleaseTime = pulseClock + arpNoteLength;

    arpLEDIndex = arpPosition % N_LEDS;

    // Change arpPosition according to arpeggiator pattern
    if (arpPattern == ARP_PATTERN_RANDOM) {
      arpPosition = (byte)random(0, arpLength);
    }
    if (arpPattern == ARP_PATTERN_UP) {
      arpPosition = (arpPosition + 1) % arpLength;
    }
    if (arpPattern == ARP_PATTERN_DOWN) {
      if (arpPosition == 0) {
	arpPosition = arpLength - 1;
      } else {
	arpPosition--;
      }
    }
    if (arpPattern == ARP_PATTERN_UPDOWN) {
      if (arpDirection == 1) {
	// going up
	arpPosition++;
	if (arpPosition >= arpLength) {
	  arpPosition = arpLength-1;
	  arpDirection = -1;
	}
	if (arpPosition > arpLength) {
	  debugprint("", arpPosition);
	  debugprintln(" > ", arpLength);
	}
      } else {
	if (arpPosition == 0) {
	  arpDirection = 1;
	} else {
	  arpPosition--;
	}
      }
    }
    digitalWrite(led[arpLEDIndex], HIGH);
    ledState[arpLEDIndex] = HIGH;
  }
}

void arpeggiatorNoteOn(byte midiNote) {
  if (arpType != ARP_TYPE_MIDI) {
    // set the root note for the arpeggiator.
    arpRootNote = midiNote;
  } else {
    for(byte i=0;i<ARP_MAX_NOTES;i++) {
      if (midiArpeggioNotes[i] == midiNote) {
	midiArpeggioNotes[i] = 0;
	midiArpeggioNoteTime[i] = 0;
      }
    }
    // find a non-zero slot in midiArpeggioNotes for this note.
    for(byte i=0;i<ARP_MAX_NOTES;i++) {
      if (midiArpeggioNotes[i] == 0) {
	midiArpeggioNotes[i] = midiNote;
	midiArpeggioNoteTime[i] = millis();
	break;
      }
    }
  }
  setArpeggioNotes();
  if (!arpRunning) resetArpeggiator();
  arpRunning = true; // auto-start
}

void arpeggiatorNoteOff(byte midiNote) {
  for(byte i=0;i<ARP_MAX_NOTES;i++) {
    if (midiArpeggioNotes[i] == midiNote) {
      midiArpeggioNotes[i] = 0;
      midiArpeggioNoteTime[i] = 0;
    }
  }
  if (!arpMIDILatch) {
    setArpeggioNotes();
  }
}

void transposeMIDIArpeggio(byte newRoot) {
  if (arpRunning) {
    // transpose the notes in the arpeggio
    arpRunning = false; // disable arpeggiator while we change the notes and length;
    for(byte i=arpLength-1;i>0;i--) {
      // adjust each note, preserving the intervals between notes
      arpeggio[i] = newRoot + (arpeggio[i] - arpeggio[0]);
      if (arpeggio[i] > MIDI_HIGH) {
	arpeggio[i] = MIDI_HIGH;
      }
    }
    arpeggio[0] = newRoot;
    arpRunning = true;
  }
}

void initArpeggiator() {
  selectedSettings = 0;
  arpRunning = false;
  arpTimeDivision = 2; // 1/16 notes
  arpNoteStartPulse = PPQ >> arpTimeDivision;
  arpRootNote = 60;
  arpNoteLength = arpNoteStartPulse / 2;
  arpPosition = 0;
  arpNoteIndex = UNSET;
  arpPattern = ARP_PATTERN_UP;
  arpType = ARP_TYPE_MAJOR_CHORD;
  arpDirection = 1;
  arpMIDILatch = true; // Should MIDI invoked arpeggio stay on when keys released

  arpIntervals[ARP_TYPE_MAJOR_CHORD][0] = 0;
  arpIntervals[ARP_TYPE_MAJOR_CHORD][1] = 4;
  arpIntervals[ARP_TYPE_MAJOR_CHORD][2] = 7;
  arpIntervals[ARP_TYPE_MAJOR_CHORD][3] = 12;
  arpLengths[ARP_TYPE_MAJOR_CHORD] = 4;

  arpIntervals[ARP_TYPE_MINOR_CHORD][0] = 0;
  arpIntervals[ARP_TYPE_MINOR_CHORD][1] = 3;
  arpIntervals[ARP_TYPE_MINOR_CHORD][2] = 7;
  arpIntervals[ARP_TYPE_MINOR_CHORD][3] = 12;
  arpLengths[ARP_TYPE_MINOR_CHORD] = 4;

  arpIntervals[ARP_TYPE_M7_CHORD][0] = 0;
  arpIntervals[ARP_TYPE_M7_CHORD][1] = 4;
  arpIntervals[ARP_TYPE_M7_CHORD][2] = 7;
  arpIntervals[ARP_TYPE_M7_CHORD][3] = 10;
  arpLengths[ARP_TYPE_M7_CHORD] = 4;

  arpIntervals[ARP_TYPE_m7_CHORD][0] = 0;
  arpIntervals[ARP_TYPE_m7_CHORD][1] = 3;
  arpIntervals[ARP_TYPE_m7_CHORD][2] = 7;
  arpIntervals[ARP_TYPE_m7_CHORD][3] = 10;
  arpLengths[ARP_TYPE_m7_CHORD] = 4;


  lastBPMReading = sampledAnalogRead(BPM_POT);
  unsigned int newBPM = (unsigned int)map(lastBPMReading, 0, 1023, MIN_BPM, MAX_BPM);
  setBPM(newBPM);
  lastArpNoteLengthReading = sampledAnalogRead(ARPEGGIATOR_NOTE_LENGTH_POT);
  lastArpRootNoteReading = sampledAnalogRead(ARPEGGIATOR_ROOT_NOTE_POT);
  arpLength = arpLengths[arpType];
  setArpeggioNotes();
}

void resetArpeggiator() {
  boolean oldArpRunning = arpRunning;
  arpRunning = false; // disable arpeggiator while we change the notes and length;
  for(int i=0;i<MAX_NOTES;i++) {
    stopNote(i);
  }
  arpNoteIndex = UNSET;
  digitalWrite(led[arpLEDIndex], LOW);
  ledState[arpLEDIndex] = LOW;
  arpPosition = 0;
  pulseClock = 0;
  arpReleaseTime = 0;
  arpNoteStartPulse = PPQ >> arpTimeDivision;
  if (arpNoteLength >= arpNoteStartPulse) {
    arpNoteLength = arpNoteStartPulse-1;
  }
  arpRunning = oldArpRunning;
}

// set the notes in the arpeggio based on the arpRootNote and arpType
void setArpeggioNotes() {
  if (arpType == ARP_TYPE_MIDI) {
    setMIDIArpeggioNotes();
    return;
  }
  boolean oldArpRunning = arpRunning;
  arpRunning = false; // disable arpeggiator while we change the notes and length;
  arpLength = arpLengths[arpType];
  for(byte i=0;i<arpLength;i++) {
    arpeggio[i] = arpRootNote + arpIntervals[arpType][i];
  }

  arpRunning = oldArpRunning;
}

// set the arpeggio notes based on which MIDI notes are currently pressed
void setMIDIArpeggioNotes() {
  byte count = 0;
  boolean oldArpRunning = arpRunning;
  arpRunning = false; // disable arpeggiator while we change the notes and length;

  for(byte i=0;i<ARP_MAX_NOTES;i++) {
    if (midiArpeggioNotes[i] != 0) {
      arpeggio[count] = midiArpeggioNotes[i];
      arpeggioNoteTime[count] = midiArpeggioNoteTime[i];
      count++;
    }
  }
  // arpeggio now contains all the pressed MIDI notes in unsorted order
  // sort them using simple bubble sort
  byte tmpNote;
  unsigned long tmpTime;
  for(byte i=0;i<(count-1);i++) {
    for(byte j=0;j<count-i-1;j++) {
      if (arpeggioNoteTime[j] > arpeggioNoteTime[j+1]) {
	tmpNote = arpeggio[j];
	tmpTime = arpeggioNoteTime[j];

	arpeggio[j] = arpeggio[j+1];
	arpeggioNoteTime[j] = arpeggioNoteTime[j+1];

	arpeggio[j+1] = tmpNote;
	arpeggioNoteTime[j+1] = tmpTime;
      }
    }
  }
  arpLength = count;
  if (arpPosition >= arpLength) {
    if (arpLength > 0) {
      arpPosition = arpLength-1;
    } else {
      arpPosition = 0;
    }
  }
  debugprintln("arpLength = ", arpLength);
  arpRunning = oldArpRunning;
}


void readArpButtons() {
  readFnButton();
  if (buttonPressedNew(BUTTON1)) {
    // start/stop arpeggiator
    if (!arpRunning) resetArpeggiator();
    arpRunning = !arpRunning;
    if (!arpRunning) resetArpeggiator();
  }

  if (buttonPressedNew(BUTTON2)) {
    // select arpeggio pattern
    if (arpPattern == ARP_PATTERN_MAX) {
      arpPattern = 0;
    } else {
      arpPattern++;
    }
  }

  if (buttonPressedNew(BUTTON3)) {
    if (!fnEnabled) {
      // select arpeggio type
      if (arpType == ARP_TYPE_MAX) {
	arpType = 0;
      } else {
	arpType++;
      }
      if (arpType == ARP_TYPE_MIDI) {
	// clear the notes
	for(byte i=0;i<ARP_MAX_NOTES;i++) {
	  midiArpeggioNotes[i] = 0;
	  midiArpeggioNoteTime[i] = 0;
	}
      }
      setArpeggioNotes();
    } else {
      // select time division
      if (arpTimeDivision == ARP_TIME_DIVISION_MAX) {
	arpTimeDivision = 0;
      } else {
	arpTimeDivision++;
      }
      resetArpeggiator();
    }
  }
}
