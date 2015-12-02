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

ISR(TCC0_OVF_vect) {
  byte last;

  // output to DAC
  DACB.CH0DATA = output;
  output = SILENCE;

  int mix;
  int sum = 0;

  note_t *n;
  for(byte i=0;i<MAX_NOTES;i++) {
    n = &note[i];
    if (n->midiVal != NOTE_OFF) {
      n->phase += n->phaseInc;
      last = n->phaseFraction;
      n->phaseFraction += n->phaseFractionInc;
      if (n->phaseFraction < last) {
	// overflow in pseudo-floating point counter
	n->phase++;
      }

      if (n->phase >= N_WAVEFORM_SAMPLES) {
	if (n->midiVal != NOTE_PENDING_OFF) {
	  n->phase -= N_WAVEFORM_SAMPLES;
	  // don't assign volume until we cross zero
	  n->volume = n->volumeNext;
	} else {
	  n->midiVal = NOTE_OFF;
	  continue;
	}
      }

      mix = pgm_read_word(n->waveformBuf + n->phase);

      mix = mix >> 1;
      mix = adjustAmplitude(mix, n->volume);

      // Filter:
      if (filterCutoff[i] != 255) {
	int tmp = (mix - buf0[i]) + (feedback[i] * (buf0[i] - buf1[i]) >> 8);
	buf0[i] += ((long)filterCutoff[i] * tmp) >> 8;
	buf1[i] += ((long)filterCutoff[i] * (buf0[i] - buf1[i])) >> 8;
	mix = buf1[i];
      }

      sum += mix;

    }
  }

  
  output = sum + SILENCE;

  // clip
  if (output < 0) {
    output = 0;
  } 
  else {
    if (output > 4095) {
      output = 4095;
    }
  }


#ifdef DEBUG_ENABLE
  counterEnd = TCC0.CNT;
#endif
}


/*
 * Adjust amplitude using only bitshifting.  Crude but cheap.
 */
int adjustAmplitude(int v, byte level) {
  int tmp, tmp2;
  switch(level) {
  case 0: 
    return 0;
  case 1: // v - 63/64
    return (v >> 6);
  case 2: // v - 62/64
    return (v >> 5);
  case 3: // v - 61/64
    tmp = (v >> 4);
    return tmp - (tmp >> 2);
  case 4: // v - 60/64
    return (v >> 4);
  case 5: // v - 59/64
    tmp = (v >> 4);
    return tmp + (tmp >> 2);
  case 6: // v - 58/64
    tmp = (v >> 3);
    return tmp - (tmp >> 2);
  case 7: // v - 57/64
    tmp = (v >> 3);
    return tmp - (tmp >> 3);
  case 8: // v - 56/64
    return (v >> 3);
  case 9: // v - 55/64
    tmp = (v >> 3);
    return tmp + (tmp >> 3);
  case 10: // v - 54/64
    tmp = (v >> 3);
    return tmp + (tmp >> 2);
  case 11: // v - 53/64 
    tmp = (v >> 3);
    tmp2 = (tmp >> 2);
    return tmp + tmp2 + (tmp2 >> 1);
  case 12: // v - 52/64 
    tmp = (v >> 2);
    return tmp - (tmp >> 2);
  case 13: // v - 51/64
    tmp = (v >> 2);
    tmp2 = (tmp >> 3);
    return (tmp - tmp2) - (tmp2 >> 1);
  case 14: // v - 50/64 
    tmp = (v >> 2);
    return tmp - (tmp >> 3);
  case 15: // v - 49/64 
    tmp = (v >> 2);
    return tmp - (tmp >> 4);
  case 16: // v - 48/64 
    return (v >> 2);
  case 17: // v - 47/64 
    tmp = (v >> 2);
    return tmp + (tmp >> 4);
  case 18: // v - 46/64 
    tmp = (v >> 2);
    return tmp + (tmp >> 3);
  case 19: // v - 45/64
    tmp = (v >> 2);
    tmp2 = (tmp >> 2);
    return (tmp + tmp2) - (tmp2 >> 2);
  case 20: // v - 44/64
    tmp = (v >> 2);
    return tmp + (tmp >> 2);
  case 21: // v - 43/64
    tmp = (v >> 2);
    tmp2 = (tmp >> 2);
    return tmp + tmp2 + (tmp2 >> 2);
  case 22: // v - 42/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return (tmp - tmp2) - (tmp2 >> 2);
  case 23: // v = 41/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return (tmp - tmp2) - (tmp2 >> 3);
  case 24: // v = 40/64
    tmp = (v >> 1);
    return tmp - (tmp >> 2);
  case 25: // v = 39/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return (tmp - tmp2) + (tmp2 >> 3);
  case 26: // v - 38/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 3);
    return (tmp - tmp2) - (tmp2 >> 1);
  case 27: // v - 37/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 3);
    return (tmp - tmp2) - (tmp2 >> 2);
  case 28: // v - 36/64
    tmp = (v >> 1);
    return tmp - (tmp >> 3);
  case 29: // v - 35/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 4);
    return (tmp - tmp2) - (tmp2 >> 1);
  case 30: // v - 34/64
    tmp = (v >> 1);
    return tmp - (tmp >> 4);
  case 31: // v - 33/64
    tmp = (v >> 1);
    return tmp - (tmp >> 5);
  case 32: // v - 32/64
    return v >> 1;
  case 33: // v - 31/64
    tmp = (v >> 1);
    return tmp + (tmp >> 5);
  case 34: // v - 30/64
    tmp = (v >> 1);
    return tmp + (tmp >> 4);
  case 35: // v - 29/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 4);
    return tmp + tmp2 + (tmp2 >> 1);
  case 36: // v - 28/64
    tmp = (v >> 1);
    return tmp + (tmp >> 3);
  case 37: // v - 27/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 3);
    return tmp + tmp2 + (tmp2 >> 2);
  case 38: // v - 26/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 3);
    return tmp + tmp2 + (tmp2 >> 1);
  case 39: // v - 25/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 3);
    return tmp + tmp2 + (tmp2 >> 1) + (tmp2 >> 2);
  case 40: // v - 24/64
    tmp = (v >> 1);
    return tmp + (tmp >> 2);
  case 41: // v - 23/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return tmp + tmp2 + (tmp2 >> 3);
  case 42: // v - 22/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return tmp + tmp2 + (tmp2 >> 2);
  case 43: // v - 21/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return tmp + tmp2 + (tmp2 >> 2) + (tmp2 >> 3);
  case 44: // v - 20/64
    tmp = (v >> 1);
    tmp2 = (tmp >> 2);
    return tmp + tmp2 + (tmp2 >> 1);
  case 45: // v - 19/64
    tmp = (v >> 2);
    tmp2 = (tmp >> 3);
    return (v - tmp) - tmp2 - (tmp2 >> 1);
  case 46: // v - 18/64
    tmp = (v >> 2);
    return (v - tmp) - (tmp >> 3);
  case 47: // v - 17/64
    tmp = (v >> 2);
    return (v - tmp) - (tmp >> 4);
  case 48: // v - 16/64
    return v - (v >> 2);
  case 49: // v - 15/64
    tmp = (v >> 2);
    return (v - tmp) + (tmp >> 4);
  case 50: // v - 14/64
    tmp = (v >> 2);
    return (v - tmp) + (tmp >> 3);
  case 51: // v - 13/64
    tmp = (v >> 3);
    return (v - tmp) - (tmp >> 1) - (tmp >> 3);
  case 52: // v - 12/64
    tmp = (v >> 3);
    return (v - tmp) - (tmp >> 1);
  case 53: // v - 11/64
    tmp = (v >> 3);
    tmp2 = (tmp >> 2);
    return (v - tmp) - tmp2 - (tmp2 >> 1);
  case 54: // v - 10/64
    tmp = (v >> 3);
    return (v - tmp) - (tmp >> 2);
  case 55: // v - 9/64
    tmp = (v >> 3);
    return (v - tmp) - (tmp >> 3);
  case 56: // v - 8/64 = 1/8
    return v - (v >> 3);
  case 57: // v - 7/64
    tmp = (v >> 4);
    return (v - tmp) - (tmp >> 2) - (tmp >> 1);
  case 58: // v - 6/64
    tmp = (v >> 4);
    return (v - tmp) - (tmp >> 1);
  case 59: // v - 5/64
    tmp = (v >> 4);
    return (v - tmp) - (tmp >> 2);
  case 60: // v - 4/64 = 1/16
    return v - (v >> 4); 
  case 61: // v - 3/64
    tmp = (v >> 5);
    return (v - tmp) - (tmp >> 1);
  case 62: // v - 2/64 = 1/32
    return v - (v >> 5);
  case 63: // v - 1/64
    return v - (v >> 6); 
  case 64:
    return v;
  }
  return 0;
}
