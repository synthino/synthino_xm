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

// Interrupt service routine for LFO updates and updating noise source.
ISR(TCC1_OVF_vect) {
  byte last;

  if (noiseUpdateCount++ > 2) {
    noiseUpdateCount = 0;
    boolean shouldUpdate = false;
    for(byte i=0;i<MAX_NOTES;i++) {
      if ((note[i].midiVal != NOTE_OFF)&& (note[i].waveformBuf == NULL)) {
	shouldUpdate = true;
	break;
      }
    }
    if (shouldUpdate) {
      updateNoise();
    }
  }

  for(byte lfo=0;lfo<NUM_LFO;lfo++) {
    if (lfoEnabled[lfo]) {
      lfoPhase[lfo] += lfoPhaseInc[lfo];
      last = lfoPhaseFraction[lfo];
      lfoPhaseFraction[lfo] += lfoPhaseFractionInc[lfo];
      if (lfoPhaseFraction[lfo] < last) {
	// overflow in pseudo-floating point counter
	lfoPhase[lfo]++;
      }
      if (lfoPhase[lfo] >= N_WAVEFORM_SAMPLES) {
	lfoPhase[lfo] -= N_WAVEFORM_SAMPLES;
      }
    }
  }
}
