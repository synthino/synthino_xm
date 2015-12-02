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

// LFO parameters
float lfoFrequency[NUM_LFO][NUM_OSC] = {0.0, 0.0, 0.0};
float lfoDepth[NUM_LFO][NUM_OSC] = {0.0, 0.0, 0.0};
float lfoShift[NUM_LFO][NUM_OSC];
byte lfoWaveform = 0;
const int16_t *lfoWaveformBuf = lfoWaveformBuffers[0];
volatile unsigned int lfoPhase[NUM_LFO][NUM_OSC] = {0, 0, 0};
volatile unsigned int lfoPhaseInc[NUM_LFO][NUM_OSC] = {0, 0, 0};
volatile byte lfoPhaseFraction[NUM_LFO][NUM_OSC] = {0, 0, 0};
volatile byte lfoPhaseFractionInc[NUM_LFO][NUM_OSC] = {0, 0, 0};

void updateLFO(byte lfoNum, byte osc) {
  float lfoPhaseIncFloat = (lfoFrequency[lfoNum][osc] * N_WAVEFORM_SAMPLES) / LFO_CLOCK_RATE;
  lfoPhaseInc[lfoNum][osc] = (int)lfoPhaseIncFloat;
  lfoPhaseIncFloat -= lfoPhaseInc[lfoNum][osc];
  lfoPhaseFractionInc[lfoNum][osc] = (int)(lfoPhaseIncFloat * 256.0);
  // lfoShift is how much to change the modulated parameter.  Range is [-1.0:1.0]
  lfoShift[lfoNum][osc] = (((int)pgm_read_word(lfoWaveformBuf + lfoPhase[lfoNum][osc]))/1024.0);

}

