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

void initNote(byte i, byte midiNote) {
  note[i].midiChannel = 0;
  note[i].volume = 0;
  note[i].volumeNext = 0;
  note[i].volumeScale = 1.0;
  note[i].volumeScaled = false;
  note[i].doScale = false;
  note[i].lastFrequency = 0.0;
  note[i].phase = 0;
  note[i].phaseInc = 0;
  note[i].phaseFraction = 0;
  note[i].phaseFractionInc = 0;
  note[i].midiVal = midiNote;
  note[i].isSample = false;
  note[i].isPreview = false;
  note[i].waveform = UNSET;
  note[i].trigger = UNSET;
}


void startNote(byte i, byte midiNote) {
  initNote(i, midiNote);
  byte waveformNum = waveform[i];
  note[i].waveform = waveformNum;
  note[i].waveformBuf = waveformBuffers[waveformNum];
  note[i].volume = note[i].volumeNext;

  // Restart filter LFO
  lfoPhase[LFO_FILTER][i] = 0;
}


void stopNote(byte noteIndex) {
  note[noteIndex].midiVal = NOTE_PENDING_OFF;
  while ((note[noteIndex].midiVal != NOTE_OFF) && ((note[noteIndex].phaseInc > 0) || (note[noteIndex].phaseFractionInc > 0))) {
    // wait until note reaches NOTE_OFF state to avoid click
  }
}

void setPhaseIncrement(byte i) {
  // don't recompute phase if the frequency has not changed.
  if (note[i].frequency == note[i].lastFrequency) return;

  note[i].lastFrequency = note[i].frequency;
  float phaseIncFloat;
  phaseIncFloat = (note[i].frequency * N_WAVEFORM_SAMPLES) / OUTPUT_RATE;
  // convert the phase increment to an int
  note[i].phaseInc = (int)phaseIncFloat;
  // but also use a counter to keep track of the fractional part
  phaseIncFloat -= note[i].phaseInc;
  note[i].phaseFractionInc = (byte)(phaseIncFloat * 256.0);
}

