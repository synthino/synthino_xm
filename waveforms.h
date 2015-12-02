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

#define N_WAVEFORMS 12
#define N_TOTAL_WAVEFORMS 12

#define N_LFO_WAVEFORMS 5
#define N_WAVEFORM_SAMPLES 600


extern const int16_t *waveformBuffers[N_TOTAL_WAVEFORMS];
extern const int16_t *lfoWaveformBuffers[N_LFO_WAVEFORMS];

extern const int16_t sin_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t tri_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t saw_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t squ[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t piano_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t epiano_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t eorgan_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t cello_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t violin_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t oboe_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t flute_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t ebass_0001[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t c604_0027[N_WAVEFORM_SAMPLES] PROGMEM;
extern const int16_t akwf_1603[N_WAVEFORM_SAMPLES] PROGMEM;


#endif
