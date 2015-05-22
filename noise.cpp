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

int noiseBuf[NOISE_BUF_LEN];
volatile unsigned int noiseBufIndex = 0;  // position that the synth ISR reads from
volatile byte noiseUpdateCount = 0;
unsigned int noiseBufUpdateIndex = 0;   // position for updating values

void initNoise() {
  for(int i=0;i<NOISE_BUF_LEN;i++) {
    noiseBuf[i] = (random() % 2048) - 1024;  // range [-1024, 1023]
  }
}

void updateNoise() {
  unsigned int r = random() % 2048;
  noiseBuf[noiseBufUpdateIndex] = r - 1024;
  noiseBufUpdateIndex = (noiseBufUpdateIndex + 1) % NOISE_BUF_LEN;
  noiseBufIndex = (r >> 1);  // move the read index to a random location
}
