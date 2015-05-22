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
#include "waveforms.h"

int readPots();
void tone(byte);

void test() {
  boolean on = false;
  int maxPotValue = 0;
  byte midiVal;
  
  while (true) {
    maxPotValue = readPots();
    // map to a note from 48 to 72
    midiVal = map(maxPotValue, 0, 1024, 48, 73);
  
    for(byte i=0;i<4;i++) {
      ledState[i] = false;
    }
    on = false;
    if (buttonPressed(BUTTON1)) {
      ledState[0] = HIGH;
      on = true;
    }
    if (buttonPressed(BUTTON2)) {
      ledState[1] = HIGH;
      on = true;
    }
    if (buttonPressed(BUTTON3)) {
      ledState[2] = HIGH;
      on = true;
    }
    if (buttonPressed(BUTTON4)) {
      ledState[3] = HIGH;
      on = true;
    }

    for(byte i=0;i<4;i++) {
      digitalWrite(led[i], ledState[i]);
    }

    if (on) {
      tone(midiVal);
    } else {
      stopNote(0);
    }
  }
}


void tone(byte midiVal) {
  note_t *n = &note[0];
  if (n->midiVal == NOTE_OFF) {
    initNote(0, midiVal);
    n->waveform = 1;
    n->waveformBuf = waveformBuffers[1];
    n->volume = MAX_NOTE_VOL;
    n->volumeNext = MAX_NOTE_VOL;
  }
  n->frequency = 440.0 * (pow(2, ((midiVal-69))/12.0));
  setPhaseIncrement(0);
}

int readPots() {
  int max = 0;
  int v;
  for(byte i=0;i<8;i++) {
    v = sampledAnalogRead(i);
    if (v > max) {
      max = v;
    }
  }
  return max;
}
