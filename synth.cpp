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

void readSynthButtons() {
  readFnButton();
  int diff;

  for(byte i=0;i<N_NOTE_BUTTONS;i++) {
    if (buttonPressedNew(i)) {
      if (!fnEnabled) {
	button[i].pitchReading = sampledAnalogRead(PITCH_POT);
      }
      button[i].noteIndex = doNoteOn(selectedSettings+1, button[i].midiVal, MAX_VELOCITY);
      note[button[i].noteIndex].trigger = i;

    } else {
      byte noteIndex = button[i].noteIndex;
      if ((noteIndex != UNSET) && (note[noteIndex].midiVal > NOTE_PENDING_OFF) && (note[noteIndex].trigger == i)) {
	if (buttonHeld(i)) {
	  if (!fnEnabled) {
	    int currentPitchReading = sampledAnalogRead(PITCH_POT);
	    diff = abs(currentPitchReading - button[i].pitchReading);
	    if (diff > 5) {
	      // the pot has moved since the button was pressed
	      button[i].midiVal = map(currentPitchReading, 0, 1024, POT_MIDI_LOW, POT_MIDI_HIGH+1);
	      button[i].pitchReading = currentPitchReading;
	      if (button[i].midiVal < MIDI_LOW) {
		button[i].midiVal = MIDI_LOW;
	      }
	      if (button[i].midiVal > MIDI_HIGH) {
		button[i].midiVal = MIDI_HIGH;
	      }

	      button[i].pitchReading = currentPitchReading;
	      note[noteIndex].midiVal = button[i].midiVal;
	    }
	  }
	} else {
	  if (note[noteIndex].envelopePhase != RELEASE) {
	    // button released
	    note[noteIndex].trigger = UNSET;
	    button[i].noteIndex = UNSET;
	    doNoteOff(selectedSettings+1, button[i].midiVal);
	  }
	}
      }
    }
  }
}

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

// find a note in the array of MAX_NOTES available notes to use
byte findNoteIndex() {
  for(byte i=0;i<MAX_NOTES;i++) {
    // find a slot for this note
    if (note[i].midiVal <= NOTE_PENDING_OFF) {
      while ((note[i].midiVal != NOTE_OFF) && ((note[i].phaseInc > 0) || (note[i].phaseFractionInc > 0))) {
	// wait until note reaches NOTE_OFF state to avoid click
      }
      return i;
    }
  }

  // There are no notes that are not being used. Check to see if any
  // are in RELEASE envelope phase and use it. Find the oldest of the
  // notes in RELEASE phase.
  byte noteIndex = 255;
  unsigned long oldest = millis();
  for(byte i=0;i<MAX_NOTES;i++) {
    if ((note[i].envelopePhase == RELEASE) && (!note[i].isSample) && (note[i].startTime < oldest)) {
      noteIndex = i;
      oldest = note[i].startTime;
    }
  }
  if (noteIndex != 255) {
    stopNote(noteIndex);
    return noteIndex;
  }


  // All notes are being used. Choose the note that is "oldest".
  // Start by looking for non-sample notes.
  noteIndex = 255;
  for(byte i=0;i<MAX_NOTES;i++) {
    if (!note[i].isSample) {
      noteIndex = i;
      break;
    }
  }
  if (noteIndex != 255) {
    for(byte i=0;i<MAX_NOTES;i++) {
      if ((!note[i].isSample) && (note[i].startTime < note[noteIndex].startTime)) {
	// try to find an older nonsample note.
	noteIndex = i;
      }
    }
  } else {
    // All notes are samples. Now try to find the oldest.
    noteIndex = 0;
    for(byte i=1;i<MAX_NOTES;i++) {
      if (note[i].startTime < note[noteIndex].startTime) {
	noteIndex = i;
      }
    }
  }
  // At this point noteIndex is the oldest note, giving priority to samples.

  if (note[noteIndex].phaseInc > 0) {
    stopNote(noteIndex);
  }
  return noteIndex;
}

byte doNoteOn(byte channelNum, byte midiNote, byte velocity) {
  byte i = findNoteIndex();
  if (velocity > MAX_VELOCITY) {
    velocity = MAX_VELOCITY;
  }
  initNote(i, midiNote);
  note[i].startTime = millis();
  note[i].midiChannel = channelNum;
  byte waveformNum = settings[channelNum-1].waveform;
  note[i].waveform = waveformNum;
  note[i].waveformBuf = waveformBuffers[waveformNum];
  if (waveformNum >= N_WAVEFORMS) {
    note[i].isSample = true;
    note[i].sampleLength = sampleLength[waveformNum - N_WAVEFORMS];
  }
  note[i].envelopePhase = ATTACK;
  note[i].volIndexInc = 1;
  note[i].volLevelDuration = settings[channelNum-1].attackVolLevelDuration;
  note[i].volLevelRemaining = settings[channelNum-1].attackVolLevelDuration;

  note[i].targetVolIndex = map(velocity, 0, MAX_VELOCITY, 0, MAX_NOTE_VOL);

  if ((settings[channelNum-1].attackVolLevelDuration > 0) && (!note[i].isSample)) {
    note[i].volIndex = 0;
    note[i].targetVolIndex = inverseAttackLogVolume[logVolume[note[i].targetVolIndex]];
  } else {
    // optimize if there is no attack.  go directly to target volume
    note[i].volIndex = note[i].targetVolIndex;
    note[i].volume = logVolume[note[i].volIndex];
    note[i].volumeNext = note[i].volume;
  }

  // Restart filter LFO
  lfoPhase[LFO_FILTER] = 0;
  return i;
}

// Release all notes on the specified channel with the specified MIDI note value.
void doNoteOff(byte channelNum, byte midiNote) {
  for(byte i=0;i<MAX_NOTES;i++) {
    // find the note
    byte midiChannel = note[i].midiChannel & 0x7F;
    if ((note[i].midiVal == midiNote) && (midiChannel == channelNum)) {
      releaseNote(&note[i]);
    }
  }
}

void releaseNote(note_t *n) {
  if (n->isSample) {
    byte channel = n->midiChannel & 0x7F; // mask off high bit channel 10 indicator
    int releaseDur = settings[channel-1].releaseVolLevelDuration;
    unsigned int l = n->phase + map(releaseDur, 0, RELEASE_RANGE/2, 0, (n->sampleLength - n->phase));
    n->sampleLength = constrain(l, 0, n->sampleLength);
    n->envelopePhase = RELEASE;
    n->trigger = UNSET;
    return;
  }

  n->volIndexInc = -1;
  n->targetVolIndex = 0;
  n->volLevelDuration = settings[n->midiChannel-1].releaseVolLevelDuration;
  n->volLevelRemaining = settings[n->midiChannel-1].releaseVolLevelDuration;
  if (settings[n->midiChannel-1].releaseVolLevelDuration > 0) {
    if (n->envelopePhase == ATTACK) {
      // switch volumeIndex to logarithmic scale for RELEASE phase.
      n->volIndex = inverseLogVolume[n->volume];
      n->volume = logVolume[n->volIndex];
      n->volumeNext = n->volume;
    }
  }
  n->envelopePhase = RELEASE;
}

void stopNote(byte noteIndex) {
  note[noteIndex].midiVal = NOTE_PENDING_OFF;
  while ((note[noteIndex].midiVal != NOTE_OFF) && ((note[noteIndex].phaseInc > 0) || (note[noteIndex].phaseFractionInc > 0))) {
    // wait until note reaches NOTE_OFF state to avoid click
  }
  note[noteIndex].envelopePhase = OFF;
}

void setPhaseIncrement(byte i) {
  if (note[i].waveformBuf == NULL) {
    // For noise generation, we use the MIDI note value to set
    // note phaseInc to a number representing the probability of 
    // changing the noise output value on the next cycle.
    // Instead of a linear mapping of midiVal, use the nonlinear frequency scale.
    float lowFreq = noteTable[36-MIDI_LOW];
    float highFreq = noteTable[72-MIDI_LOW];
    float range = highFreq - lowFreq;
    float f = note[i].frequency;
    if (f < lowFreq) f = lowFreq;
    if (f > highFreq) f = highFreq;
    float diff = f - lowFreq;
    float amt = (float)(diff / range);
    int a = (int)(amt * 100.0);
    note[i].phaseInc = map(a, 0, 100, 25, 2047);
    return;
  }

  // don't recompute phase if the frequency has not changed.
  if (note[i].frequency == note[i].lastFrequency) return;

  if ((note[i].volumeScale != 1.0) && (!note[i].volumeScaled)) {
    // don't let the note be played until we have adjusted the
    // volume in processEnvelope. This avoids clicks.
    return;
  }

  note[i].lastFrequency = note[i].frequency;
  float phaseIncFloat;
  if (!note[i].isSample) {
    phaseIncFloat = (note[i].frequency * N_WAVEFORM_SAMPLES) / OUTPUT_RATE;
  } else {
    // for samples, we want the phase increment to be 1 for midiVal = 60 (middle C)
    phaseIncFloat = note[i].frequency / noteTable[60-MIDI_LOW];
  }
  // convert the phase increment to an int
  note[i].phaseInc = (int)phaseIncFloat;
  // but also use a counter to keep track of the fractional part
  phaseIncFloat -= note[i].phaseInc;
  note[i].phaseFractionInc = (byte)(phaseIncFloat * 256.0);
}

void processEnvelope(byte i) {
  if ((note[i].isSample) || (note[i].envelopePhase == SUSTAIN)) {
    if ((note[i].doScale) && (note[i].volumeScale != 1.0)) {
      note[i].volumeNext = (byte)(logVolume[note[i].volIndex] * note[i].volumeScale);
      if (note[i].isSample) {
	note[i].volume = note[i].volumeNext;
      }
      note[i].doScale = false;
      note[i].volumeScaled = true;
    }
    return;
  }
  if ((note[i].volLevelRemaining <= 0) || (note[i].volLevelRemaining > 1000)) {
    // It is time to change the volume to the next level
    // The check for a value above 1000 is a guard against extreme cases where
    // the output ISR is so busy it is starving this code from checking the
    // zero threshold and the value wraps below -32767 and becomes positive again.
    if (note[i].volIndex == note[i].targetVolIndex) {
      // this was the last volume level to use for this phase.
      // switch to new phase of envelope.
      byte phase = note[i].envelopePhase;
      if (phase == ATTACK) {
	byte midiChannel = note[i].midiChannel & 0x7F;
	// switch volumeIndex to logarithmic scale for non-ATTACK phase.

	byte v = note[i].volume;
	if ((note[i].volumeScale != 1.0) && (note[i].volumeScaled)) {
	  // the volume has already been scaled, so unscale it before 
	  // determining the index into the release log contour.
	  if (note[i].volumeScale > 0) {
	    v = (byte)(v / note[i].volumeScale);
	  } else {
	    v = 0;
	  }
	  if (v > 64) {
	    // should not happen
	    v = 64;
	  }
	}
	note[i].volIndex = inverseLogVolume[v];

	if ((note[i].volumeScale != 1.0) && (!note[i].volumeScaled)) {
	  // adjust volume in case where there was no attack.
	  note[i].volume = (byte)(note[i].volume * note[i].volumeScale);
	  note[i].volumeScaled = true;
	  note[i].volumeNext = note[i].volume;
	}
	

	// optimize for case where there is no decay
	if (settings[midiChannel-1].sustainVolLevel == 1.0) {
	  note[i].envelopePhase = SUSTAIN;
	  return;
	}
        note[i].envelopePhase = DECAY;
        // decay to the sustain volume which is a percentage of the original velocity
        note[i].targetVolIndex = (byte)(settings[midiChannel-1].sustainVolLevel * note[i].targetVolIndex);

	if (note[i].targetVolIndex > note[i].volIndex) {
	  debugprint("ERROR: ", note[i].targetVolIndex);
	  debugprintln(" > ", note[i].volIndex);
	  note[i].volIndex = note[i].targetVolIndex;
	}
        note[i].volIndexInc = -1; // decreasing volume
        note[i].volLevelDuration = settings[midiChannel-1].decayVolLevelDuration;
        note[i].volLevelRemaining = settings[midiChannel-1].decayVolLevelDuration;
      } else {
        if (phase == DECAY) {
          note[i].envelopePhase = SUSTAIN;
        } else {
          if (phase == RELEASE) {
            // done with this note.
            note[i].midiVal = NOTE_PENDING_OFF;
	    note[i].envelopePhase = OFF;
	    note[i].trigger = UNSET;
          }
        }
      }
    } else {
      if ((note[i].envelopePhase == RELEASE) && (note[i].volLevelDuration == 0)) {
	note[i].volIndex = 0;
	note[i].volumeNext = 0;
      } else {
	// not yet at the target volume.
	// Take one step closer by changing the volume.
	note[i].volIndex += note[i].volIndexInc;
	note[i].volLevelRemaining = note[i].volLevelDuration;
	byte v;
	if (note[i].envelopePhase == ATTACK) {
	  v = attackLogVolume[note[i].volIndex];
	} else {
	  v = logVolume[note[i].volIndex];
	}
	if (note[i].volumeScale != 1.0) {
	  // adjust volume
	  v = (byte)(v * note[i].volumeScale);
	  note[i].volumeScaled = true;
	}
	note[i].volumeNext = v;
      }
    }
  }
}

