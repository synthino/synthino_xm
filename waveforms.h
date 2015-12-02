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

#ifndef waveforms_h
#define waveforms_h

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "Arduino.h"

#define N_WAVEFORMS 0
#define N_SAMPLES 3
#define N_TOTAL_WAVEFORMS 3
#define COWBELL_1_WAVEFORM 0
#define COWBELL_2_WAVEFORM 1
#define COWBELL_3_WAVEFORM 2

#define N_LFO_WAVEFORMS 0
#define N_WAVEFORM_SAMPLES 600

// 25KHz samples
#define COWBELL_1_LENGTH 8555
#define COWBELL_2_LENGTH 9490
#define COWBELL_3_LENGTH 9987

extern const int16_t *waveformBuffers[N_TOTAL_WAVEFORMS];
extern const int16_t *lfoWaveformBuffers[N_LFO_WAVEFORMS];
extern const int16_t sampleLength[N_SAMPLES];

extern const int16_t cowbell_1[COWBELL_1_LENGTH] PROGMEM;
extern const int16_t cowbell_2[COWBELL_2_LENGTH] PROGMEM;
extern const int16_t cowbell_3[COWBELL_3_LENGTH] PROGMEM;


#endif
