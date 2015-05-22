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

#include <avr/pgmspace.h>
#include "waveforms.h"

const int16_t *waveformBuffers[N_TOTAL_WAVEFORMS] = {
  sin_0001,
  tri_0001,
  saw_0001,
  squ,
  piano_0001,
  epiano_0001,
  eorgan_0001,
  cello_0001,
  violin_0001,
  oboe_0001,
  flute_0001,
  ebass_0001,
  NULL,         // noise
  kick,
  snare,
  hihat,
  tom,
  clap
};


const int16_t *lfoWaveformBuffers[N_LFO_WAVEFORMS] = {
  sin_0001,
  squ,
  saw_0001,
  c604_0027,
  akwf_1603
};

const int16_t sampleLength[N_SAMPLES] = {
  KICK_LENGTH,
  SNARE_LENGTH,
  HIHAT_LENGTH,
  TOM_LENGTH,
  CLAP_LENGTH
};

