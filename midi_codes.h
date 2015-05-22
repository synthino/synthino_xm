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


#ifndef _MIDI_CODES_H_
#define _MIDI_CODES_H_

#define ATTACK_TIME          73
#define DECAY_TIME           75
#define SUSTAIN_VOLUME       79
#define RELEASE_TIME         72
#define SOUND_VARIATION      70   // waveform
#define VIBRATO_RATE         76   // pitch LFO rate
#define MODULATION_DEPTH     1    // pitch LFO depth
#define EFFECT_CONTROL_1     12   // LFO waveform
#define EFFECT_CONTROL_2     13   // filter LFO rate
#define EFFECTS_1_DEPTH      91   // filter LFO depth
#define TIMBRE               71   // filter resonance
#define BRIGHTNESS           74   // filter cutoff frequency
#define GENERAL_PURPOSE_1    16   // arpeggiator/groovebox tempo BPM (maps to 20-300) (alternative to clock)
#define GENERAL_PURPOSE_2    17   // arpeggiator note length
#define GENERAL_PURPOSE_3    18   // arpeggiator/groovebox transpose
#define GENERAL_PURPOSE_4    19   // detune channel
#define CHANNEL_VOLUME       7    // used in groovebox mode
#define CHANNEL_VOLUME_T1    20   // alternate for track 1 volume, regardless of channel
#define CHANNEL_VOLUME_T2    21   // alternate for track 2 volume, regardless of channel
#define CHANNEL_VOLUME_T3    22   // alternate for track 3 volume, regardless of channel
#define CHANNEL_VOLUME_T4    23   // alternate for track 4 volume, regardless of channel
#define ALL_NOTES_OFF        123


#endif
