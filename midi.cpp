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
#include "MIDI.h"
#include "USBMIDI.h"
#include "midi_codes.h"
#include "waveforms.h"

#define N_BPM_MEASUREMENTS 24
boolean midiClock = false;
unsigned long lastClockPulseMillis;
unsigned long lastClockPulse;
int bpmMeasurements[N_BPM_MEASUREMENTS];
int clockMeasurementCount = 0;

void midiInit() {
#ifdef MIDI_ENABLE
  // Initialize MIDI library.
  MIDI.begin(MIDI_CHANNEL_OMNI);    
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleSystemReset(handleSystemReset);
  MIDI.setHandleClock(handleClock);
#endif

#ifdef USBMIDI_ENABLE
  USBMIDI.setHandleNoteOn(handleNoteOn);
  USBMIDI.setHandleNoteOff(handleNoteOff);
  USBMIDI.setHandleProgramChange(handleProgramChange);
  USBMIDI.setHandleControlChange(handleControlChange);
  USBMIDI.setHandlePitchBend(handlePitchBend);
  USBMIDI.setHandleStart(handleStart);
  USBMIDI.setHandleContinue(handleContinue);
  USBMIDI.setHandleStop(handleStop);
  USBMIDI.setHandleSystemReset(handleSystemReset);
  USBMIDI.setHandleClock(handleClock);
  initUSB();
#endif
}

void updateMIDIClockInfo() {
  if ((millis() - lastClockPulseMillis) > 500) {
    midiClock = false;
    clockMeasurementCount = 0;
  }
}

void handleNoteOn(byte channelNum, byte midiNote, byte velocity) { 
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprint("ON ");
    debugprint("ch ", channelNum);
    debugprint(" : ", midiNote);
    debugprintln(" : ", velocity);
  }
#endif
  boolean isDrumChannel = (channelNum == 10); // remember this
  channelNum = channelNum % N_MIDI_CHANNELS;
  if (channelNum == 0) channelNum = N_MIDI_CHANNELS;
  if ((midiNote < MIDI_LOW) || (midiNote > MIDI_HIGH)) {
    return;
  }
  if (mode == MODE_SYNTH) {
    if ((isDrumChannel) && ((midiNote < 35) || (midiNote > 81))) {
      return;
    }
    if (velocity > 0) {
      byte noteIndex = doNoteOn(channelNum, midiNote, velocity);
      if (isDrumChannel) {
	// if the note came in on the drum channel (10), do special processing
	setDrumParameters(noteIndex, midiNote, velocity);
      }
    } else {
      // note off
      if (isDrumChannel) {
	doDrumNoteOff(midiNote);
      } else {
	doNoteOff(channelNum, midiNote);
      }
    }
    return;
  }
  if (mode == MODE_ARPEGGIATOR) {
    if (velocity > 0) {
      arpeggiatorNoteOn(midiNote);
    } else {
      arpeggiatorNoteOff(midiNote);
    }
    return;
  }

  if (mode == MODE_GROOVEBOX) {
    if ((isDrumChannel) && ((midiNote < 35) || (midiNote > 81))) {
      return;
    }
    if (isDrumChannel) {
      channelNum = 10;
    }
    if (velocity > 0) {
      grooveboxNoteOn(channelNum, midiNote, velocity);
    } else {
      grooveboxNoteOff(channelNum, midiNote, velocity);
    }
  }
}

void handleNoteOff(byte channelNum, byte midiNote, byte velocity) { 
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprint("OFF ");
    debugprint("ch ", channelNum);
    debugprint(" : ", midiNote);
    debugprintln(" : ", velocity);
  }
#endif
  boolean isDrumChannel = (channelNum == 10); // remember this
  channelNum = channelNum % N_MIDI_CHANNELS;
  if (channelNum == 0) channelNum = N_MIDI_CHANNELS;
  if ((midiNote < MIDI_LOW) || (midiNote > MIDI_HIGH)) {
    return;
  }
  if (mode == MODE_SYNTH) {
    if (isDrumChannel) {
      doDrumNoteOff(midiNote);
    } else {
      doNoteOff(channelNum, midiNote);
    }
    return;
  }
  if (mode == MODE_ARPEGGIATOR) {
    arpeggiatorNoteOff(midiNote);
    return;
  }
  if (mode == MODE_GROOVEBOX) {
    if (isDrumChannel) {
      channelNum = 10;
    }
    if ((isDrumChannel) && ((midiNote < 35) || (midiNote > 81))) {
      return;
    }
    grooveboxNoteOff(channelNum, midiNote, velocity);
  }
}

void doDrumNoteOff(byte midiNote) {
  for(byte i=0;i<MAX_NOTES;i++) {
    // find the note
    if ((note[i].origMidiVal == midiNote) && (note[i].midiVal > NOTE_PENDING_OFF) && (note[i].midiChannel & 0x80)) {
      releaseNote(&note[i]);
    }
  }
}

void setDrumParameters(byte i, byte midiNote, byte velocity) {
  note[i].midiChannel |= 0x80; // set high bit on channel to indicate it was a channel 10 drum
  // Override what was done in doNoteOn()
  note[i].volIndex = note[i].targetVolIndex;
  note[i].volume = logVolume[note[i].volIndex];
  note[i].volumeNext = note[i].volume;

  byte newMidiNote; // actual pitch to play
  byte waveform; // actual waveform to use
  float length = 0.0; // as percentage of original sample length
  
  switch (midiNote) {
  case 35: //  Bass Drum 2
    waveform = COWBELL_1_WAVEFORM;
    newMidiNote = 57;
    break;
  case 36: //  Bass Drum 1
    waveform = COWBELL_1_WAVEFORM;
    newMidiNote = 60;
    break;
  case 37: //  Side Stick/Rimshot
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 70;
    length = 0.66;
    break;
  case 38: //  Snare Drum 1
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 60;
    break;
  case 39: //  Hand Clap
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 60;
    break;
  case 40: //  Snare Drum 2
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 64;
    break;
  case 41: //  Low Tom 2
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 48;
    break;
  case 42: //  Closed Hi-hat
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 48;
    length = 0.33;
    break;
  case 43: //  Low Tom 1
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 55;
    break;
  case 44: //  Pedal Hi-hat
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 48;
    length = 0.66;
    break;
  case 45: //  Mid Tom 2
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 46: //  Open Hi-hat
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 47: //  Mid Tom 1
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 64;
    break;
  case 48: //  High Tom 2
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 72;
    break;
  case 49: //  Crash Cymbal 1
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 48;
    break;
  case 50: //  High Tom 1
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 75;
    break;
  case 51: //  Ride Cymbal 1
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 52: //  Chinese Cymbal
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 53: //  Ride Bell
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 54: //  Tambourine
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 55: //  Splash Cymbal
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 55;
    break;
  case 56: //  Cowbell
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 55;
    break;
  case 57: //  Crash Cymbal 2
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 57;
    break;
  case 58: //  Vibra Slap
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 55;
    break;
  case 59: //  Ride Cymbal 2
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 60: //  High Bongo
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 67;
    break;
  case 61: //  Low Bongo
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 55;
    length = 0.33;
    break;
  case 62: //  Mute High Conga
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 65;
    length = 0.33;
    break;
  case 63: //  Open High Conga
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 65;
    break;
  case 64: //  Low Conga
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 55;
    break;
  case 65: //  High Timbale
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 66: //  Low Timbale
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 55;
    break;
  case 67: //  High Agogo
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 65;
    break;
  case 68: //  Low Agogo
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 57;
    break;
  case 69: //  Cabasa
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 72;
    break;
  case 70: //  Maracas
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 70;
    break;
  case 71: //  Short Whistle
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 72;
    length = 0.33;
    break;
  case 72: //  Long Whistle
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 72;
    break;
  case 73: //  Short Guiro
    waveform = COWBELL_2_WAVEFORM;
    length = 0.33;
    newMidiNote = 60;
    break;
  case 74: //  Long Guiro
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 60;
    break;
  case 75: //  Claves
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 84;
    break;
  case 76: //  High Wood Block
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 96;
    break;
  case 77: //  Low Wood Block
    waveform = COWBELL_2_WAVEFORM;
    newMidiNote = 84;
    break;
  case 78: //  Mute Cuica
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    length = 0.33;
    break;
  case 79: //  Open Cuica
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 60;
    break;
  case 80: //  Mute Triangle
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 72;
    length = 0.33;
    break;
  case 81: //  Open Triangle
    waveform = COWBELL_3_WAVEFORM;
    newMidiNote = 72;
    break;
  }

  note[i].waveform = waveform;
  note[i].waveformBuf = waveformBuffers[waveform];
  note[i].origMidiVal = midiNote;
  note[i].midiVal = newMidiNote;
  unsigned int origLength = sampleLength[waveform - N_WAVEFORMS];
  if (length == 0.0) {
    // unmodified length
    note[i].sampleLength = origLength;
  } else {
    note[i].sampleLength = (int)((origLength * length));
  }
  note[i].isSample = true;
}


void handleProgramChange(byte channelNum, byte number) { 
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprint("PC ");
    debugprint("ch ", channelNum);
    debugprintln(" : ", number);
  }
#endif
  channelNum = channelNum % N_MIDI_CHANNELS;
  if (channelNum == 0) channelNum = N_MIDI_CHANNELS;
  if (number < N_TOTAL_WAVEFORMS) {
    setWaveform(channelNum-1, number);
  }
}


void handleControlChange(byte channelNum, byte number, byte value) { 
  int setting;
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprint("CC ");
    debugprint("ch ", channelNum);
    debugprint(" : ", number);
    debugprintln(" : ", value);
  }
#endif
  channelNum = channelNum % N_MIDI_CHANNELS;
  if (channelNum == 0) channelNum = N_MIDI_CHANNELS;
  if (number == ALL_NOTES_OFF) {
    for(byte i=0;i<MAX_NOTES;i++) {
      byte midiChannel = note[i].midiChannel & 0x7F;
      if ((midiChannel == channelNum) && (note[i].midiVal > NOTE_PENDING_OFF)) {
	stopNote(i);
      }
    }
    return;
  }
  if (number == ATTACK_TIME) {
    if ((channelNum-1) == selectedSettings) {
      attackPotTolerance = POT_LOCK_TOLERANCE;
    }
    setting = map(value, 0, 127, 0, ATTACK_RANGE);
    settings[channelNum-1].attackVolLevelDuration = setting;
    return;
  }
  if (number == DECAY_TIME) {
    if ((channelNum-1) == selectedSettings) {
      decayPotTolerance = POT_LOCK_TOLERANCE;
    }
    setting = map(value, 0, 127, 0, DECAY_RANGE);
    settings[channelNum-1].decayVolLevelDuration = setting;
    return;
  }
  if (number == SUSTAIN_VOLUME) {
    if ((channelNum-1) == selectedSettings) {
      sustainPotTolerance = POT_LOCK_TOLERANCE;
    }
    float fSetting = map(value, 0, 127, 0, 1000) / 1000.0;
    settings[channelNum-1].sustainVolLevel = fSetting;
    return;
  }
  if (number == RELEASE_TIME) {
    if ((channelNum-1) == selectedSettings) {
      releasePotTolerance = POT_LOCK_TOLERANCE;
    }
    setting = map(value, 0, 127, 0, RELEASE_RANGE);
    settings[channelNum-1].releaseVolLevelDuration = setting;
    return;
  }
  if ((number == VIBRATO_RATE) || (number == EFFECT_CONTROL_2)) {
    byte lfoNum;
    lfoRatePotTolerance = POT_LOCK_TOLERANCE;
    if (number == VIBRATO_RATE) {
      lfoNum = LFO_PITCH;
    } else {
      lfoNum = LFO_FILTER;
    }
    if (value > 0) {
      if (!lfoEnabled[lfoNum]) {
	// reset the LFO phase
	lfoPhase[lfoNum] = 0;
      }
      lfoEnabled[lfoNum] = true;
      if (value < 64) {
	lfoFrequency[lfoNum] = value / 3.15; // map to frequency in range [0.0, 20.0]
      } else {
	lfoFrequency[lfoNum] = 20.0 + ((value-64) / 0.1575); // map to frequency in range [20.0 420.0]
      }
    } else {
      lfoEnabled[lfoNum] = false;
    }
    return;
  }

  if ((number == MODULATION_DEPTH) || (number == EFFECTS_1_DEPTH)) {
    byte lfoNum;
    lfoDepthPotTolerance = POT_LOCK_TOLERANCE;
    if (number == MODULATION_DEPTH) {
      lfoNum = LFO_PITCH;
    } else {
      lfoNum = LFO_FILTER;
    }
    setting = map(value, 0, 127, 0, 1023);
    lfoDepth[lfoNum] = setting / 2048.0;  // 0.0-0.5
    return;
  }

  if (number == SOUND_VARIATION) {
    waveformPotTolerance = POT_LOCK_TOLERANCE;
    byte waveform = map(value, 0, 128, 0, N_TOTAL_WAVEFORMS);
    if (waveform != settings[channelNum-1].waveform) {
      setWaveform(channelNum-1, waveform);
    }
    return;
  }

  if (number == EFFECT_CONTROL_1) {
    // LFO waveform
    setting = map(value, 0, 128, 0, N_LFO_WAVEFORMS);
    lfoWaveformBuf = lfoWaveformBuffers[setting];
    return;
  }

  if (number == BRIGHTNESS) {
    setting = map(value, 0, 127, 0, 255);
    filterCutoffPotTolerance = POT_LOCK_TOLERANCE;
    filterCutoffReading = setting;
    if (!lfoEnabled[LFO_FILTER]) {
      filterCutoff = filterCutoffReading;
      setFilterFeedback();
    }
    return;
  }

  if (number == TIMBRE) {
    setting = map(value, 0, 127, 0, 255);
    filterResonancePotTolerance = POT_LOCK_TOLERANCE;
    filterResonance = setting;
    setFilterFeedback();
    return;
  }

  if (number == GENERAL_PURPOSE_1) {
    // tempo BPM for arpeggiator/groovebox.
    // this is an alternative to using the MIDI clock signal
    if ((mode == MODE_ARPEGGIATOR) || (mode == MODE_GROOVEBOX)) {
      unsigned int usetting = map(value, 0, 127, MIN_BPM, MAX_BPM);
      setBPM(usetting);
    }
    return;
  }

  if (number == GENERAL_PURPOSE_2) {
    // arpeggiator note length
    if (mode == MODE_ARPEGGIATOR) {
      arpNoteLength = map(value, 0, 128, 2, arpNoteStartPulse);
    }
    return;
  }

  if (number == GENERAL_PURPOSE_3) {
    // arpegiator/groovebox transpose
    if (mode == MODE_ARPEGGIATOR) {
      setting = map(value, 0, 127, MIDI_LOW, MIDI_HIGH-12);
      if (arpType != ARP_TYPE_MIDI) {
	arpRootNote = setting;
	setArpeggioNotes();
      } else {
	transposeMIDIArpeggio(setting);
      }
    }
    if (mode == MODE_GROOVEBOX) {
      // transpose track
      setting = value - 64;
      if (seqRunning) {
	seqLock = true;
	for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
	  for(byte i=0;i<SEQ_LENGTH;i++) {
	    if (seq[i][t].midiVal != UNSET) {
	      seq[i][t].transpose += setting;
	    }
	  }
	}
	seqLock = false;
      }
    }
    return;
  }

  if (number == GENERAL_PURPOSE_4) {
    float fSetting;
    if ((value > 52) && (value < 76)) {
      fSetting = 0.0;
    } else {
      if (value <= 52) {
	// lower half of pot
	setting = -(map(value, 0, 52, 1024, 0));
      } else {
	setting = map(value, 76, 127, 0, 1024);
      }
      fSetting = setting / 1024.0; // range is [-1.0, 1.0]
    }
    settings[channelNum-1].detune = fSetting;
    return;
  }

  // Volume control. Since the channel number may be changed, check these last.
  if (number == CHANNEL_VOLUME_T1) {
    number = CHANNEL_VOLUME;
    channelNum = 1;
  }
  if (number == CHANNEL_VOLUME_T2) {
    number = CHANNEL_VOLUME;
    channelNum = 2;
  }
  if (number == CHANNEL_VOLUME_T3) {
    number = CHANNEL_VOLUME;
    channelNum = 3;
  }
  if (number == CHANNEL_VOLUME_T4) {
    number = CHANNEL_VOLUME;
    channelNum = 4;
  }
  if (number == CHANNEL_VOLUME) {
    if (mode == MODE_GROOVEBOX) {
      faderPotTolerance = POT_LOCK_TOLERANCE;
      track[channelNum-1].volumeScale = (float)value / 127.0;
      for(byte i=0;i<MAX_NOTES;i++) {
	if ((note[i].trigger == (channelNum-1)) || (note[i].isPreview) || (!seqRunning)) {
	  note[i].volumeScale = track[channelNum-1].volumeScale;
	  note[i].doScale = true;
	}
      }
    }
  }
}

void handlePitchBend(byte channelNum, int bend) {
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprint("PB ");
    debugprint("ch ", channelNum);
    debugprintln(" : ", bend);
  }
#endif
  channelNum = channelNum % N_MIDI_CHANNELS;
  if (channelNum == 0) channelNum = N_MIDI_CHANNELS;
  settings[channelNum-1].pitchBend = bend;
}

void handleStart() {
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprintln("START ");
  }
#endif
  // start arpeggiator / groovebox
  if (mode == MODE_GROOVEBOX) {
    if (!seqRunning) {
      resetGroovebox();
      seqRunning = true;
    }
  }
  if (mode == MODE_ARPEGGIATOR) {
    if (!arpRunning) {
      resetArpeggiator();
      arpRunning = true;
    }
  }
}

void handleContinue() {
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprintln("CONTINUE ");
  }
#endif
  handleStart();
}


void handleStop() {
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprintln("STOP ");
  }
#endif
  // stop arpeggiator / groovebox
  if (mode == MODE_GROOVEBOX) {
    if (seqRunning) {
      seqRunning = false;
      stopSequencer();
    }
  }
  if (mode == MODE_ARPEGGIATOR) {
    if (arpRunning) {
      arpRunning = false;
      resetArpeggiator();
    }
  }
}

void handleSystemReset() {
#ifdef DEBUG_ENABLE
  if (debug) {
    debugprintln("RESET ");
  }
#endif
  reset(true);
}

void handleClock() {
  midiClock = true;
  lastClockPulseMillis = millis();
  unsigned long now = micros();
  unsigned long pulseDuration;
  if (clockMeasurementCount > 0) {
    pulseDuration = now - lastClockPulse;
    byte index = (clockMeasurementCount-1) % N_BPM_MEASUREMENTS;
    int computedBPM = 60000000 / (pulseDuration * 24);
    bpmMeasurements[index] = computedBPM;
  }

  int sum = 0;
  byte numMeasurements = clockMeasurementCount;
  if (numMeasurements > N_BPM_MEASUREMENTS) {
    numMeasurements = N_BPM_MEASUREMENTS;
  }
  for(byte i=0;i<numMeasurements;i++) {
    sum += bpmMeasurements[i];
  }
  unsigned int newbpm = sum / numMeasurements;

  if ((newbpm != bpm) && (newbpm >= MIN_BPM) && (newbpm <= MAX_BPM)) {
    setBPM(newbpm);
  }
  lastClockPulse = now;
  clockMeasurementCount++;
}

