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
#include "MIDI.h"
#include "USBMIDI.h"

boolean seqRunning;
boolean seqSynchStart = false;
boolean seqPreview = false;
byte metronomeMode = METRONOME_MODE_OFF;
byte metronomeNoteIndex = UNSET;
byte currentTrack = 0;
volatile boolean seqLock = false;
byte seqStartPulse = PPQ >> 2; // 1/16 notes
volatile byte seqIndex = SEQ_LENGTH-1;
byte seqLEDIndex = 0;
sequenceNote_t seq[SEQ_LENGTH][SEQ_NUM_TRACKS];
sequenceTrack_t track[SEQ_NUM_TRACKS];

void grooveboxNoteOn(byte channelNum, byte midiNote, byte velocity) {
  seqLock = true;
  // snap to nearest sequencer position
  byte seqSnapIndex;
  int noteStart = pulseClock;
  // noteStartOffset is the timing error. It is the difference between when the note
  // was pressed and the nearest sequencer note boundary.
  int noteStartOffset = (pulseClock % seqStartPulse);
  boolean snappedForward = false;
  if ((noteStartOffset > 0) && (noteStartOffset <= (seqStartPulse/2))) {
    // snap backward
    seqSnapIndex = seqIndex;
  } else {
    // snap forward
    seqSnapIndex = (seqIndex + 1) % SEQ_LENGTH;
    snappedForward = true;
  }
  if (seqSynchStart) {
    seqSnapIndex = 0;
  }

  sequenceNote_t *sn;
  note_t *n;
  byte currentNoteSeqIndex = track[currentTrack].noteSeqIndex;
  sn = &seq[currentNoteSeqIndex][currentTrack];
  n = &note[sn->noteIndex];
  if (n->trigger == currentTrack) {
    if ((n->envelopePhase != OFF) || (n->isSample)) {
      // A note on the current track is playing.
      if ((!seqPreview) && (seqRunning) && (n->envelopePhase != RELEASE)) {
	// If we are not previewing and sequence is running, and it's
	// not in RELEASE phase yet, adjust its duration and stop the note.
	if ((sn->duration == 0) && (currentNoteSeqIndex != seqSnapIndex)) {
	  // If the note being played has not been released yet, set a duration for it.
	  unsigned int newDuration;
	  if ((snappedForward) && (noteStartOffset > 0)) {
	    newDuration = (noteStart + (seqStartPulse-noteStartOffset)) - sn->startPulse;
	  } else {
	    newDuration = (noteStart - noteStartOffset) - sn->startPulse;
	  }
	  debugprint("setting duration of note ", currentNoteSeqIndex);
	  debugprintln(" to ", newDuration);
	  sn->duration = newDuration;
	}
      }
      stopNote(sn->noteIndex);
      note[sn->noteIndex].trigger = UNSET;
    }
  }

  byte ni = doNoteOn(selectedSettings+1, midiNote, velocity);
  if (channelNum == 10) {
    setDrumParameters(ni, midiNote, velocity);
  }
  note[ni].volumeScale = track[currentTrack].volumeScale;
  if (note[ni].volumeScale != 1.0) {
    if (note[ni].isSample) {
      note[ni].doScale = true;
    }
  }
  note[ni].isPreview = seqPreview;
  if ((!seqPreview) && (seqRunning || seqSynchStart)) {
    sn = &seq[seqSnapIndex][currentTrack];
    sn->noteIndex = ni;
    track[currentTrack].noteSeqIndex = seqSnapIndex;
    
    note[sn->noteIndex].trigger = currentTrack;
    sn->startPulse = noteStart;
    sn->midiVal = midiNote;
    if (channelNum == 10) {
      sn->midiVal = sn->midiVal | 0x80; // set high bit to indicate ch10
    }
    sn->transpose = 0;
    track[currentTrack].state = SEQ_RECORD;
    sn->velocity = velocity;
    sn->duration = 0;
    sn->waveform = note[ni].waveform;
  }
  if (seqSynchStart) seqSynchStart = false;
  seqLock = false;
}


void grooveboxNoteOff(byte channelNum, byte midiNote, byte velocity) {
  if ((seqPreview) || (!seqRunning)) {
    if (channelNum == 10) {
      doDrumNoteOff(midiNote);
    } else {
      doNoteOff(selectedSettings+1, midiNote);
    }
    return;
  }

  seqLock = true;
  sequenceNote_t *sn;

  int noteEnd = pulseClock;
  sn = &seq[track[currentTrack].noteSeqIndex][currentTrack];
  note_t *n = &note[sn->noteIndex];
  byte midiVal = sn->midiVal & 0x7F;
  if ((midiVal == midiNote) && ((n->envelopePhase == OFF) || (n->envelopePhase == RELEASE)) && (!n->isSample)) {
    debugprint("WARNING: noteOff on track ", currentTrack);
    debugprintln(" : unexpected note envelope = ", n->envelopePhase);
  }

  if (midiVal == midiNote) {
    // Set the duration for the note we just turned off.
    unsigned int duration = noteEnd - sn->startPulse;
    if (duration == 0) {
      debugprintln("duration is 0, but setting to 1: ", midiNote);
      duration = 1; // rare, but avoid duration 0 to simplify logic
    }
    sn->duration = duration;
    if (n->trigger == currentTrack) {
      // It is possible that the note was a sample and already done/reused.
      releaseNote(&note[sn->noteIndex]);
    }
    track[currentTrack].state = SEQ_PLAY;
  } else {
    debugprint("no match on ", midiVal);
    debugprint(", trigger ", n->trigger);
    debugprint(", noteSeqIndex ", track[currentTrack].noteSeqIndex);
    debugprintln(", noteIndex ", sn->noteIndex);
  }
  seqLock = false;
}


void doGroovebox() {
  if (!seqRunning) return;
  unsigned int duration;

  sequenceNote_t *sn;
  for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
    sn = &seq[track[t].noteSeqIndex][t];
    note_t *n = &note[sn->noteIndex];
    if (track[t].state == SEQ_PLAY) {
      // check to see if this note's duration is elapsed, but only
      // if it has not been released.
      if ((n->trigger == t) && (n->envelopePhase != RELEASE) && (n->envelopePhase != OFF)) {
	duration = pulseClock - sn->startPulse;
	if (duration >= sn->duration) {
	  // time to release this note
	  if (duration != sn->duration) {
	    debugprintln("track=", t);
	    debugprintln("noteSeqIndex=", track[t].noteSeqIndex);
	    debugprint("ERROR: releasing note with duration ", sn->duration);
	    debugprintln(" at computed duration ", duration);
	    debugprintln("env=", n->envelopePhase);
	    sn->duration = duration;
	  }
	  releaseNote(n);
	}
      }
    }
  }

  if ((pulseClock % seqStartPulse) == 0) {
    sequenceNote_t *sn;
    sequenceNote_t *nextsn;
    seqIndex = (seqIndex + 1) % SEQ_LENGTH;
    if (seqIndex < 10) {
      debugprint(" ", seqIndex);
    } else {
      debugprint("", seqIndex);
    }
    debugprint(":");
  
    seqLEDIndex = seqIndex / (SEQ_LENGTH / 4);
    digitalWrite(led[seqLEDIndex], HIGH);
    ledState[seqLEDIndex] = HIGH;

    // The play/record head has reached a note boundary.
    for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
      sn = &seq[track[t].noteSeqIndex][t];
      note_t *n = &note[sn->noteIndex];
      if (track[t].state == SEQ_PLAY) {
	// Check if it is time to start a new note at this position in the sequence.
	nextsn = &seq[seqIndex][t];
	if (nextsn->midiVal != UNSET) {
	  // There is a note here in the sequence to play
	  // See if there is a currently playing note and stop it.
	  if (n->trigger == t) {
	    if ((n->envelopePhase != RELEASE) && (n->envelopePhase != OFF)) {
	      // A currently playing note should have been released above
	      // because its duration should have elapsed.
	      debugprint(" adjusting note ", track[t].noteSeqIndex);
	      debugprint(" duration from  ", sn->duration);
	      debugprintln(" to ", pulseClock - sn->startPulse);
	      sn->duration = pulseClock - sn->startPulse;
	    }
	    // Stop the note
	    stopNote(sn->noteIndex);
	    n->trigger = UNSET;
	  }

	  if ((pulseClock > nextsn->startPulse) && ((pulseClock - nextsn->startPulse) < seqStartPulse)) {
	    // avoid double hit for short note that was already released but snapped to this position
	    debugprintln("avoiding double hit: ", pulseClock-nextsn->startPulse);
	  } else {
	    byte midiVal = (nextsn->midiVal & 0x7F);
	    boolean isDrumChannel = (nextsn->midiVal) & 0x80; // high bit indicates ch10
	    if (nextsn->waveform < N_WAVEFORMS) {
	      // transpose notes that are not samples
	      midiVal = constrain(nextsn->midiVal + nextsn->transpose, MIDI_LOW, MIDI_HIGH);
	    }
	    nextsn->noteIndex = doNoteOn(t+1, midiVal, nextsn->velocity);
	    if (isDrumChannel) {
	      setDrumParameters(nextsn->noteIndex, midiVal, nextsn->velocity);
	    }
	    note[nextsn->noteIndex].trigger = t;
	    nextsn->startPulse = pulseClock;
	    track[t].noteSeqIndex = seqIndex;
	    // Set the waveform to the one that was recorded.
	    // This only works if we don't call setPhaseIncrement() in doNoteOn()
	    if (!isDrumChannel) {
	      note[nextsn->noteIndex].waveformBuf = waveformBuffers[nextsn->waveform];
	    }
	    if (nextsn->waveform >= N_WAVEFORMS) {
	      note[nextsn->noteIndex].isSample = true;
	      if (!isDrumChannel) {
		// already taken care of in setDrumParameters().
		note[nextsn->noteIndex].sampleLength = sampleLength[nextsn->waveform - N_WAVEFORMS];
		note[nextsn->noteIndex].volIndex = note[nextsn->noteIndex].targetVolIndex;
		note[nextsn->noteIndex].volume = logVolume[note[nextsn->noteIndex].volIndex];
		note[nextsn->noteIndex].volumeNext = note[nextsn->noteIndex].volume;
	      }
	    } else {
	      note[nextsn->noteIndex].isSample = false;
	    }

	    note[nextsn->noteIndex].volumeScale = track[t].volumeScale;
	    if (note[nextsn->noteIndex].volumeScale != 1.0) {
	      if (note[nextsn->noteIndex].isSample) {
		note[nextsn->noteIndex].doScale = true;
	      }
	    }
	  }
	}
      } // track in SEQ_PLAY state


      if (track[t].state == SEQ_RECORD) {
	// a note is currently being held and recorded
	// check to see if there is a note here in the sequence 
	// and if so, overwrite it.
	if ((seq[seqIndex][t].midiVal != UNSET) && (seqIndex != track[t].noteSeqIndex)) {
	  // Only clear the note if it's not the one being recorded.
	  // I may be the one being recorded because the recording started before
	  // the pulseClock reached this point in the sequence and the new note 
	  // "snapped" to this location.
	  seq[seqIndex][t].midiVal = UNSET;
	  seq[seqIndex][t].transpose = 0;
	  // Note that this may not actually be the right thing to do because
	  // the currently recorded note might be released right after the play
	  // head passes this point in the sequence, and the release snaps back to this
	  // point. If that were the case, we'd want to keep the note that is already
	  // here.
	}
      }

      if (seq[seqIndex][t].midiVal != UNSET) {
	debugprint(" ", (seq[seqIndex][t].midiVal & 0x7F));
	if (seq[seqIndex][t].duration > 0) {
	  debugprint("[", seq[seqIndex][t].duration);
	} else {
	  debugprint("[UNKNOWN");
	}
	debugprint("]");
      } else {
	debugprint("   ");
      }
      if (t == (SEQ_NUM_TRACKS-1)) {
	debugprintln("");
      }
    } // for each track

    if (metronomeMode != METRONOME_MODE_OFF) {
      byte tickFrequency;
      if (metronomeMode == METRONOME_MODE_QUARTER) {
	tickFrequency = 4;
      }
      if (metronomeMode == METRONOME_MODE_SIXTEENTH) {
	tickFrequency = 1;
      }
      if ((seqIndex % tickFrequency) == 0) {
	if (seqIndex == 0) {
	  metronomeNoteIndex = metronomeTick(64, MAX_NOTE_VOL);
	} else {
	  metronomeNoteIndex = metronomeTick(60, 40);
	}
      }
    }
  }



  if (((pulseClock - (seqStartPulse/2)) % seqStartPulse) == 0) {
    digitalWrite(led[seqLEDIndex], LOW);
    ledState[seqLEDIndex] = LOW;
  }
}

void initGroovebox() {
  seqPreview = false;
  metronomeMode = METRONOME_MODE_OFF;
  lastBPMReading = sampledAnalogRead(BPM_POT);
  unsigned int newBPM = (unsigned int)map(lastBPMReading, 0, 1023, MIN_BPM, MAX_BPM);
  setBPM(newBPM);
  for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
    settings[t].volumeReading = sampledAnalogRead(TRACK_FADER_POT);
    settings[t].sustainVolLevel = 1.0;

    track[t].noteSeqIndex = 0;
    track[t].state = SEQ_PLAY;
    track[t].volumeScale = 1.0;

    faderPotTolerance = POT_LOCK_TOLERANCE;

    for(byte s=0;s<SEQ_LENGTH;s++) {
      seq[s][t].midiVal = UNSET;
      seq[s][t].transpose = 0;
      seq[s][t].velocity = 0;
      seq[s][t].duration = 0;
      seq[s][t].noteIndex = 0;
      seq[s][t].startPulse = 0;
    }
  }
}

void resetGroovebox() {
  pulseClock = 0;
  seqIndex = SEQ_LENGTH-1;
  for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
    track[t].state = SEQ_PLAY;
  }
}

void readGrooveboxButtons() {
  if (buttonPressedNew(BUTTON1) && (!buttonPressed(BUTTON4))) {
    // start/stop live sequencer groovebox
    if (!seqRunning) {
      resetGroovebox();
      seqSynchStart = true;
      while ((buttonHeld(BUTTON1)) && (seqSynchStart)) {
	// wait until the button is released or until a MIDI event triggers the start
	if (buttonPressed(BUTTON4)) {
	  // save procedure
	  return;
	}
#ifdef MIDI_ENABLE
	MIDI.read();
#endif
#ifdef USBMIDI_ENABLE
	USBMIDI.read();
#endif
      }
    }
    seqSynchStart = false;
    seqRunning = !seqRunning;
    if (!seqRunning) {
      stopSequencer();
    }
  }

  if (buttonPressedNew(BUTTON2)) {
    // clear the sequence notes on this track.
    for(byte i=0;i<SEQ_LENGTH;i++) {
      seq[i][currentTrack].midiVal = UNSET;
      seq[i][currentTrack].transpose = 0;
    }
  }

  if ((buttonPressedNew(BUTTON3)) && (seqRunning)) {
    seqPreview = !seqPreview;
  }
  if (seqPreview) {
    ledState[PREVIEW_LED] = HIGH;
  }

  if (buttonPressedNew(BUTTON4)) {
    metronomeMode = (metronomeMode + 1) % 3;
    if ((metronomeMode == METRONOME_MODE_OFF) && (metronomeNoteIndex != UNSET)) {
      stopNote(metronomeNoteIndex);
      metronomeNoteIndex = UNSET;
    }
  }


}

void stopSequencer() {
  // if any notes are on stop them
  for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
    sequenceNote_t *sn = &seq[track[t].noteSeqIndex][t];
    if ((note[sn->noteIndex].envelopePhase != OFF)) {
      stopNote(sn->noteIndex);
      note[sn->noteIndex].trigger = UNSET;
    }
  }

  if (metronomeNoteIndex != UNSET) {
    stopNote(metronomeNoteIndex);
    metronomeNoteIndex = UNSET;
  }

  digitalWrite(led[seqLEDIndex], LOW);
  ledState[seqLEDIndex] = LOW;
  ledState[PREVIEW_LED] = LOW;
  seqPreview = false;
}

byte metronomeTick(byte midiNote, byte volume) {
  byte i = findNoteIndex();
  initNote(i, midiNote);
  note[i].startTime = millis();
  note[i].waveform = METRONOME_WAVEFORM;
  note[i].waveformBuf = waveformBuffers[METRONOME_WAVEFORM];
  note[i].isSample = true;
  note[i].sampleLength = sampleLength[METRONOME_WAVEFORM - N_WAVEFORMS];
  note[i].envelopePhase = ATTACK;
  note[i].volIndex = volume;
  note[i].volume = volume;
  note[i].volumeNext = volume;
  setPhaseIncrement(i);
  return i;
}

