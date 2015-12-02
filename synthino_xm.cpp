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
#include "waveforms.h"

volatile unsigned int counterEnd;
volatile unsigned long pulseClock = 0;
int output = SILENCE;
byte mode = MODE_SYNTH;

note_t note[MAX_NOTES];
settings_t settings[4];
button_t button[N_NOTE_BUTTONS];
boolean debug = false;
unsigned long lastDebugPrint = 0;

int tuningSetting = 0;

byte buttonPins[4] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN};
byte buttonState[4] = {HIGH, HIGH, HIGH, HIGH};
byte buttonReleaseState[4] = {HIGH, HIGH, HIGH, HIGH};
boolean fnEnabled = false;
byte led[4] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};
byte ledState[4] = {LOW, LOW, LOW, LOW};
uint32_t ledEventTimer[4] = {0L, 0L, 0L, 0L};

// logarithmic volume scale for attack envelope contour
byte attackLogVolume[65] = {0, 4, 8, 11, 15, 18, 21, 24, 27, 29, 31, 33, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 58, 58, 58, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 64};

// maps volumes to indexes into attackLogVolume[]
byte inverseAttackLogVolume[65] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, 33, 35, 37, 39, 42, 45, 49, 53, 58, 63, 64};

// logarithmic volume scale for decay and release envelope contours
byte logVolume[65] = {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, 33, 35, 37, 40, 43, 46, 49, 53, 56, 60, 64};

// maps volumes to indexes into logVolume[]
byte inverseLogVolume[65] = {0, 5, 10, 14, 18, 21, 24, 26, 28, 30, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 56, 57, 57, 57, 58, 58, 58, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63, 64};


float noteTable[(MIDI_HIGH-MIDI_LOW)+1];

byte ATTACK_TIME_POT;
byte DECAY_TIME_POT;
byte SUSTAIN_LEVEL_POT;
byte RELEASE_TIME_POT;
byte LFO_RATE_POT;
byte LFO_DEPTH_POT;
byte CHANNEL_SELECT_POT;
byte BPM_POT;
byte ARPEGGIATOR_NOTE_LENGTH_POT;
byte ARPEGGIATOR_ROOT_NOTE_POT;
byte TRACK_SELECT_POT;
byte TRACK_FADER_POT;
byte TRACK_TRANSPOSE_POT;
byte PITCH_POT;
byte DETUNE_POT;
byte WAVEFORM_SELECT_POT;
byte LFO_WAVEFORM_SELECT_POT;
byte FILTER_CUTOFF_POT;
byte FILTER_RESONANCE_POT;

byte attackPotTolerance = 0;
byte releasePotTolerance = 0;
byte decayPotTolerance = 0;
byte sustainPotTolerance = 0;
byte waveformPotTolerance = 2;
byte lfoRatePotTolerance = 0;
byte lfoDepthPotTolerance = 0;
byte faderPotTolerance = 0;
byte filterCutoffPotTolerance = 0;
byte filterResonancePotTolerance = 0;
byte detunePotTolerance = 0;

byte selectedSettings = 0;
int selectedSettingsReading;
int trackSelectReading;

int filterCutoffReading;
int lastFilterCutoffReading;
int lastFilterResonanceReading = 255;

boolean lfoEnabled[NUM_LFO] = {false, false};
boolean lastLFOEnabled[NUM_LFO] = {false, false};
int lastLFORateReading[NUM_LFO] = {0, 0};
int lastLFODepthReading[NUM_LFO] = {0, 0};
int lastLFOWaveformReading;

int lastBPMReading;
int lastArpNoteLengthReading;
int lastArpRootNoteReading;
int lastTrackTransposeReading;

unsigned int bpm = DEFAULT_BPM;
boolean ppqToggle = true; // used to control execution of PPQ ISR

int main (void) {
  init();
  setup();
  while (1) {
    loop();
  }
}

void setup(void) {

#ifdef DEBUG_ENABLE
  Serial.begin(115200);
#endif

  hardwareInit();
  reset(false);
  midiInit();

#ifdef DEBUG_ENABLE
  debugprint("mode=", mode);
  debugprint(", available SRAM=", getMemory());
  debugprintln();
#endif
}




void loop(void) {
  int reading;
  int setting;
  float fSetting;
  int diff;

  checkReset();
  
  for(byte i=0;i<4;i++) {
    if (ledEventTimer[i] > 0) {
      if (millis() >= ledEventTimer[i]) {
	// Change the state of the LED
	ledState[i] = (ledState[i] == LOW) ? HIGH : LOW;
	ledEventTimer[i] = 0; // disable event timer
      }
    } else {
      // if an event timer is not being used, set to LOW unless it's the arp LED
      if (! (((mode == MODE_ARPEGGIATOR) && (i == arpLEDIndex)) || 
	     ((mode == MODE_GROOVEBOX) && (i == seqLEDIndex)))) {
	ledState[i] = LOW;
      }
    }
  }

  if ((fnEnabled) && (ledEventTimer[FN_LED] == 0)) {
    ledEventTimer[FN_LED] = millis() + LED_BLINK_MS;
  }


#ifdef DEBUG_ENABLE
  doDebug();
#endif
#ifdef MIDI_ENABLE
  while (MIDI.read());
#endif
#ifdef USBMIDI_ENABLE
  while (USBMIDI.read());
#endif

  updateMIDIClockInfo();

  if (mode == MODE_SYNTH) {
    reading = sampledAnalogRead(CHANNEL_SELECT_POT);
    setting = map(reading, 0, 1024, 0, N_MIDI_CHANNELS);
    diff = abs(selectedSettingsReading - reading);
    if ((diff > 5) && (selectedSettings != setting)) {
      selectedSettings = setting;
      selectedSettingsReading = reading;
      setPotReadings();
      for(byte i=0;i<N_NOTE_BUTTONS;i++) {
	byte noteIndex = button[i].noteIndex;
	if ((noteIndex != UNSET) && (note[noteIndex].trigger == i)) {
	  note[noteIndex].waveformBuf = waveformBuffers[settings[selectedSettings].waveform];
	  note[noteIndex].phase = 0;
	  note[noteIndex].phaseInc = 0;
	  note[noteIndex].lastFrequency = 0.0;
	  if (settings[selectedSettings].waveform >= N_WAVEFORMS) {
	    note[noteIndex].isSample = true;
	    note[noteIndex].sampleLength = sampleLength[settings[selectedSettings].waveform - N_WAVEFORMS];
	  } else {
	    note[noteIndex].isSample = false;
	  }
	  note[noteIndex].midiChannel = selectedSettings+1;
	}
      }
      ledState[selectedSettings] = HIGH;
      ledEventTimer[selectedSettings] = millis() + LED_BLINK_MS;
    }

    // Read detune
    if (fnEnabled) {
      reading = sampledAnalogRead(DETUNE_POT);
      diff = abs(reading - settings[selectedSettings].detuneReading);
      if (diff > detunePotTolerance) { 
	detunePotTolerance = 2;
	settings[selectedSettings].detuneReading = reading;
	if ((reading > 365) && (reading < 565)) {
	  fSetting = 0.0;
	} else {
	  if (reading <=365) {
	    // lower half of pot
	    setting = -(map(reading, 0, 365, 1024, 0));
	  } else {
	    setting = map(reading, 565, 1023, 0, 1024);
	  }
	  fSetting = setting / 1024.0; // range is [-1.0, 1.0]
	}
	settings[selectedSettings].detune = fSetting;
      }
    }


    readSynthButtons();
  }

  if (mode == MODE_ARPEGGIATOR) {
    if (!fnEnabled) {
      reading = sampledAnalogRead(ARPEGGIATOR_ROOT_NOTE_POT);
      diff = abs(reading - lastArpRootNoteReading);
      if (diff > 5) {
	lastArpRootNoteReading = reading;
	if (arpType != ARP_TYPE_MIDI) {
	  arpRootNote = map(reading, 0, 1023, MIDI_LOW, MIDI_HIGH-12);
	  setArpeggioNotes();
	} else {
	  transposeMIDIArpeggio(map(reading, 0, 1023, MIDI_LOW, MIDI_HIGH-12));
	}
      }
    } else {
      reading = sampledAnalogRead(ARPEGGIATOR_NOTE_LENGTH_POT);
      diff = abs(reading - lastArpNoteLengthReading);
      if (diff > 5) {
	arpNoteLength = map(reading, 0, 1024, 2, arpNoteStartPulse);
	lastArpNoteLengthReading = reading;
      }
    }
    
    reading = sampledAnalogRead(BPM_POT);
    diff = abs(reading - lastBPMReading);
    if ((diff > 5) && (!midiClock)) {
      lastBPMReading = reading;
      unsigned int newBPM = (unsigned int)map(reading, 0, 1023, MIN_BPM, MAX_BPM);
      setBPM(newBPM);
    }

    readArpButtons();
  }

  if (mode == MODE_GROOVEBOX) {
    reading = sampledAnalogRead(TRACK_SELECT_POT);
    diff = abs(selectedSettingsReading - reading);
    setting = map(reading, 0, 1024, 0, SEQ_NUM_TRACKS);
    if ((diff > 5) && (selectedSettings != setting)) {
      // turn off the note being recorded on the current track.
      sequenceNote_t *sn = &seq[track[currentTrack].noteSeqIndex][currentTrack];
      if ((note[sn->noteIndex].trigger == currentTrack) && (note[sn->noteIndex].envelopePhase != OFF)) {
	if (track[currentTrack].state == SEQ_RECORD) {
	  handleNoteOff(currentTrack+1, sn->midiVal, 0);
	}
	if (!seqRunning) {
	  stopNote(sn->noteIndex);
	}
      }
      selectedSettings = setting;
      selectedSettingsReading = reading;
      currentTrack = selectedSettings;
      setPotReadings();
      ledState[selectedSettings] = HIGH;
      ledEventTimer[selectedSettings] = millis() + LED_BLINK_MS;
    }

    reading = sampledAnalogRead(BPM_POT);
    diff = abs(reading - lastBPMReading);
    if ((diff > 5) && (!midiClock)) {
      unsigned int newBPM = map(reading, 0, 1023, MIN_BPM, MAX_BPM);
      setBPM(newBPM);
      lastBPMReading = reading;
    }

    reading = sampledAnalogRead(FILTER_CUTOFF_POT) >> 2; // range 0-255
    diff = abs(reading - lastFilterCutoffReading);
    if (diff > filterCutoffPotTolerance) {
      filterCutoffPotTolerance = 0;
      filterCutoffReading = reading;
      lastFilterCutoffReading = reading;
      if (!lfoEnabled[LFO_FILTER]) {
	// Only use the knob reading if the LFO is not modulating the filter.
	filterCutoff = filterCutoffReading;
	setFilterFeedback();
      }
    }

    reading = sampledAnalogRead(TRACK_FADER_POT);
    diff = abs(reading - settings[selectedSettings].volumeReading);
    if (reading < POT_MIN) {
      reading = 0;
    }
    if (diff > faderPotTolerance) {
      faderPotTolerance = 3;
      settings[selectedSettings].volumeReading = reading;
      track[currentTrack].volumeScale = (float)reading / 1023.0;
      for(byte i=0;i<MAX_NOTES;i++) {
	if ((note[i].trigger == currentTrack) || (note[i].isPreview) || (!seqRunning)) {
	  note[i].volumeScale = track[currentTrack].volumeScale;
	  note[i].doScale = true;
	}
      }
    }

    reading = sampledAnalogRead(TRACK_TRANSPOSE_POT);
    diff = reading - lastTrackTransposeReading;
    if ((abs(diff) > 3) && (seqRunning)) {
      seqLock = true;
      int transposeAmount = map(reading, 0, 1023, MIDI_LOW, MIDI_HIGH-12) - map(lastTrackTransposeReading, 0, 1023, MIDI_LOW, MIDI_HIGH-12);
      lastTrackTransposeReading = reading;
      for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
	for(byte i=0;i<SEQ_LENGTH;i++) {
	  if (seq[i][t].midiVal != UNSET) {
	    seq[i][t].transpose += transposeAmount;
	  }
	}
      }
      seqLock = false;
    }

    readGrooveboxButtons();
  }

  // Compare the waveform pot position to the position it had when the
  // current Channel was selected. Only change the channel waveform if it
  // is different.
  if (!fnEnabled) {
    reading = sampledAnalogRead(WAVEFORM_SELECT_POT);
    diff = abs(reading - settings[selectedSettings].waveformReading);
    setting = map(reading, 0, 1024, 0, N_TOTAL_WAVEFORMS);
    if ((diff > waveformPotTolerance) && (setting != settings[selectedSettings].waveform)) {
      // tolerance of 2 prevents too much toggling at waveform boundaries.
      waveformPotTolerance = 2;
      settings[selectedSettings].waveformReading = reading;
      // The waveform knob has moved since the channel was chosen.
      setWaveform(selectedSettings, setting);
    }
  } else {
    reading = sampledAnalogRead(LFO_WAVEFORM_SELECT_POT);
    diff = abs(reading - lastLFOWaveformReading);
    setting = map(reading, 0, 1024, 0, N_LFO_WAVEFORMS);
    if ((diff > 5) && (setting != lfoWaveform)) {
      lastLFOWaveformReading = reading;
      lfoWaveform = setting;
      //lfoWaveformBuf = lfoWaveformBuffers[lfoWaveform];
    }
  }


  // Read envelope settings
  if (!fnEnabled) {
    reading = sampledAnalogRead(ATTACK_TIME_POT);
    diff = abs(reading - settings[selectedSettings].attackReading);
    setting = map(reading, POT_MIN, 1023, 1, ATTACK_RANGE);
    if (reading < POT_MIN) {
      setting = 0;
    }
    if (diff > attackPotTolerance) { 
      attackPotTolerance = 0;
      settings[selectedSettings].attackVolLevelDuration = setting;
      settings[selectedSettings].attackReading = reading;
    }

    reading = sampledAnalogRead(RELEASE_TIME_POT);
    diff = abs(reading - settings[selectedSettings].releaseReading);
    setting = map(reading, POT_MIN, 1023, 1, RELEASE_RANGE);
    if (reading < POT_MIN) {
      setting = 0;
    }
    if (diff > releasePotTolerance) {
      releasePotTolerance = 0;
      settings[selectedSettings].releaseVolLevelDuration = setting;
      settings[selectedSettings].releaseReading = reading;
    }
  } else {
    reading = sampledAnalogRead(DECAY_TIME_POT);
    diff = abs(reading - settings[selectedSettings].decayReading);
    setting = map(reading, POT_MIN, 1023, 1, DECAY_RANGE);
    if (reading < POT_MIN) {
      setting = 0;
    }
    if (diff > decayPotTolerance) {
      decayPotTolerance = 0;
      settings[selectedSettings].decayVolLevelDuration = setting;
      settings[selectedSettings].decayReading = reading;
    }

    reading = sampledAnalogRead(SUSTAIN_LEVEL_POT);
    diff = abs(reading - settings[selectedSettings].sustainReading);
    fSetting = reading / 1000.0;
    if (fSetting > 1.0) {
      fSetting = 1.0;
    }
    if (diff > sustainPotTolerance) {
      sustainPotTolerance = 0;
      settings[selectedSettings].sustainVolLevel = fSetting;
      settings[selectedSettings].sustainReading = reading;
    }
  }

  if ((mode == MODE_SYNTH) || (mode == MODE_ARPEGGIATOR)) {
    // Read filter settings
    if (!fnEnabled) {
      reading = sampledAnalogRead(FILTER_CUTOFF_POT) >> 2; // range 0-255
      diff = abs(reading - lastFilterCutoffReading);
      if (diff > filterCutoffPotTolerance) {
	filterCutoffPotTolerance = 0;
	filterCutoffReading = reading;
	lastFilterCutoffReading = reading;
	if (!lfoEnabled[LFO_FILTER]) {
	  // Only use the knob reading if the LFO is not modulating the filter.
	  filterCutoff = filterCutoffReading;
	  setFilterFeedback();
	}
      }
    } else {
      reading = sampledAnalogRead(FILTER_RESONANCE_POT) >> 2;
      diff = abs(reading - lastFilterResonanceReading);
      if (diff > filterResonancePotTolerance) {
	filterResonancePotTolerance = 1;
	lastFilterResonanceReading = reading;
	filterResonance = reading;
	setFilterFeedback();
      }
    }

  
    // Read LFO settings
    byte lfoNum;
    if (!fnEnabled) {
      lfoNum = LFO_PITCH;
    } else {
      lfoNum = LFO_FILTER;
    }

    reading = sampledAnalogRead(LFO_RATE_POT);
    setting = map(reading, 5, 1023, 0, 1023);
    if (reading < 5) {
      setting = 0;
    }
    diff = abs(reading - lastLFORateReading[lfoNum]);
    if (diff > lfoRatePotTolerance) {
      lfoRatePotTolerance = 0;
      lastLFORateReading[lfoNum] = reading;
      if (reading > 0) {
	if (!lfoEnabled[lfoNum]) {
	  // reset the LFO phase
	  lfoPhase[lfoNum] = 0;
	}
	lfoEnabled[lfoNum] = true;
	if (reading < 512) {
	  lfoFrequency[lfoNum] = reading / 25.55;    // map to frequency in range [0.0, 20.0]
	} else {
	  lfoFrequency[lfoNum] = 20.0 + ((reading-512) / 1.2775);    // map to frequency in range [20.0, 420.0]
	}
      } else {
	lfoEnabled[lfoNum] = false;
      }
      if (lfoDepth[lfoNum] == 0.0) {
	// since the rate pot was moved, force reading of depth pot.
	lastLFODepthReading[lfoNum] = 0;
      }
    }

    reading = sampledAnalogRead(LFO_DEPTH_POT);
    setting = map(reading, 0, 1023, 20, 1023);
    diff = abs(reading - lastLFODepthReading[lfoNum]);
    if (diff > lfoDepthPotTolerance) {
      lfoDepthPotTolerance = 0;
      lfoDepth[lfoNum] = setting / 2048.0;  // 0.0-0.5
    }
  } // read pots if MODE_SYNTH or MODE_ARPEGGIATOR


  for(byte lfo=0;lfo<NUM_LFO;lfo++) {
    if (lfoEnabled[lfo]) {
      updateLFO(lfo);
    }
  }

  if (lfoEnabled[LFO_FILTER]) {
    filterCutoff = filterCutoffReading + ((int)(128.0 * (lfoShift[LFO_FILTER] * lfoDepth[LFO_FILTER])));
    filterCutoff = constrain(filterCutoff, 0, 255);
    setFilterFeedback();
  } else {
    if (lastLFOEnabled[LFO_FILTER]) {
      // The filter LFO has just been disabled, so revert back to the knob setting.
      filterCutoff = filterCutoffReading;
      setFilterFeedback();
    }
  }

  lastLFOEnabled[LFO_PITCH] = lfoEnabled[LFO_PITCH];
  lastLFOEnabled[LFO_FILTER] = lfoEnabled[LFO_FILTER];
  
  for(byte i=0;i<MAX_NOTES;i++) {
    if (note[i].midiVal > NOTE_PENDING_OFF) {
      if (mode == MODE_SYNTH) {
	byte midiChannel = note[i].midiChannel & 0x7F;
	ledState[midiChannel-1] = HIGH;
      }
      note[i].frequency = noteTable[note[i].midiVal-MIDI_LOW];

      processEnvelope(i);

      // Adjust each note frequency for detune.
      if (!note[i].isSample) {
	float detune = settings[(note[i].midiChannel & 0x7F)-1].detune;
	// detune is in range [-1.0, 1.0]
	if (detune != 0.0) {
	  // Adjust the frequency of the note by up to 1 semitone.
	  float f = note[i].frequency;
	  byte n2;
	  float delta; // amount of frequency shift
	  if (detune < 0.0) {
	    n2 = max(MIDI_LOW, (note[i].midiVal - 1));
	    delta = (f - noteTable[n2-MIDI_LOW]) * detune;
	  } else {
	    n2 = min(MIDI_HIGH, (note[i].midiVal + 1));
	    delta = (noteTable[n2-MIDI_LOW] - f) * detune;
	  }
	  note[i].frequency += delta;
	}
      }


      // Adjust each note frequency for pitch bend.
      if (!note[i].isSample) {
	int bend = settings[(note[i].midiChannel & 0x7F)-1].pitchBend;
	// bend is in range [-8192, 8191]
	if (bend != 0) {
	  // Adjust the frequency of the note. Pitch bend range is +/-2 semitones.
	  float f = note[i].frequency;
	  byte n2;
	  float delta; // amount of frequency shift per unit of bend
	  if (bend < 0) {
	    n2 = max(MIDI_LOW, (note[i].midiVal - 2));
	    delta = (f - noteTable[n2-MIDI_LOW]) / 8192.0;
	  } else {
	    n2 = min(MIDI_HIGH, (note[i].midiVal + 2));
	    delta = (noteTable[n2-MIDI_LOW] - f) / 8191.0;
	  }
	  note[i].frequency += (delta * bend);
	}
      }

      // Adjust each note frequency for the LFO effect.
      if (lfoEnabled[LFO_PITCH]) {
	if (!note[i].isSample) {
	  note[i].frequency += note[i].frequency * (lfoShift[LFO_PITCH] * lfoDepth[LFO_PITCH]);
	}
      }


      setPhaseIncrement(i);
    }
  }

  for(byte i=0;i<4;i++) {
    digitalWrite(led[i], ledState[i]);
  }

}

void setWaveform(byte settingsIndex, byte waveformIndex) {
  settings[settingsIndex].waveform = waveformIndex;
  if ((mode != MODE_GROOVEBOX) || (seqPreview) || (!seqRunning)) {
    for(byte i=0;i<MAX_NOTES;i++) {
      boolean changeWaveform = (note[i].midiChannel == (settingsIndex+1));
      if ((seqPreview) && (!note[i].isPreview)) {
	// if in preview mode, only change preview notes
	changeWaveform = false;
      }
      if (changeWaveform) {
	note[i].waveformBuf = waveformBuffers[settings[settingsIndex].waveform];
	note[i].phase = 0;
	note[i].phaseInc = 0;
	note[i].lastFrequency = 0.0;
	if (settings[settingsIndex].waveform >= N_WAVEFORMS) {
	  note[i].isSample = true;
	  note[i].sampleLength = sampleLength[settings[settingsIndex].waveform - N_WAVEFORMS];
	} else {
	  note[i].isSample = false;
	}
      }
    }
  }
}

void setPotReadings() {
  byte lfoNum;
  if (!fnEnabled) {
    settings[selectedSettings].attackReading = sampledAnalogRead(ATTACK_TIME_POT);
    settings[selectedSettings].releaseReading = sampledAnalogRead(RELEASE_TIME_POT);
    settings[selectedSettings].waveformReading = sampledAnalogRead(WAVEFORM_SELECT_POT);
    lastFilterCutoffReading = sampledAnalogRead(FILTER_CUTOFF_POT) >> 2; // range 0-255
    attackPotTolerance = POT_LOCK_TOLERANCE;
    releasePotTolerance = POT_LOCK_TOLERANCE;
    waveformPotTolerance = POT_LOCK_TOLERANCE;
    filterCutoffPotTolerance = POT_LOCK_TOLERANCE;
    lfoNum = LFO_PITCH;
    if (mode == MODE_ARPEGGIATOR) {
      lastArpRootNoteReading = sampledAnalogRead(ARPEGGIATOR_ROOT_NOTE_POT);
    }

    if (mode == MODE_GROOVEBOX) {
      lastBPMReading = sampledAnalogRead(BPM_POT);
      lastTrackTransposeReading = sampledAnalogRead(TRACK_TRANSPOSE_POT);
      settings[selectedSettings].volumeReading = sampledAnalogRead(TRACK_FADER_POT);
      faderPotTolerance = POT_LOCK_TOLERANCE;
    }

  } else {
    settings[selectedSettings].decayReading = sampledAnalogRead(DECAY_TIME_POT);
    settings[selectedSettings].sustainReading = sampledAnalogRead(SUSTAIN_LEVEL_POT);
    decayPotTolerance = POT_LOCK_TOLERANCE;
    sustainPotTolerance = POT_LOCK_TOLERANCE;

    settings[selectedSettings].detuneReading = sampledAnalogRead(DETUNE_POT);
    detunePotTolerance = POT_LOCK_TOLERANCE * 8; // lock hard to avoid accidental detuning

    lastLFOWaveformReading = sampledAnalogRead(LFO_WAVEFORM_SELECT_POT);
    lastFilterResonanceReading = sampledAnalogRead(FILTER_RESONANCE_POT) >> 2;
    filterResonancePotTolerance = POT_LOCK_TOLERANCE;
    lfoNum = LFO_FILTER;
    if (mode == MODE_ARPEGGIATOR) {
      lastArpNoteLengthReading = sampledAnalogRead(ARPEGGIATOR_NOTE_LENGTH_POT);
    }
  }

  if ((mode == MODE_SYNTH) || (mode == MODE_ARPEGGIATOR)) {
    lastLFORateReading[lfoNum] = sampledAnalogRead(LFO_RATE_POT);
    lastLFODepthReading[lfoNum] = sampledAnalogRead(LFO_DEPTH_POT);
    lfoRatePotTolerance = POT_LOCK_TOLERANCE;
    lfoDepthPotTolerance = POT_LOCK_TOLERANCE;
  }

}

void readFnButton() {
  if (buttonPressedNew(FN_BUTTON)) {
    fnEnabled = !fnEnabled;
    if (fnEnabled) {
      ledState[FN_LED] = HIGH;
      ledEventTimer[FN_LED] = millis() + LED_BLINK_MS;
    }
    setPotReadings();
  }
}


void setBPM(unsigned int newBPM) {
  bpm = newBPM;
  TCE0.PER = (F_CPU / 8) / (((bpm<<1) * PPQ) / 60);
}


// Interrupt service routine for 96ppq clock
ISR(TCE0_OVF_vect) {
  ppqToggle = !ppqToggle;
  // only run every other counter overflow to accomodate low BPM
  // values with the 16bit timer
  if (!ppqToggle) return;

  if (mode == MODE_GROOVEBOX) {
    if (seqLock) {
      // if the sequence is being manipulated in any way, do not advance the pulseClock
      // or manipulate the data structures.
      return;
    } else {
      doGroovebox();
    }
  }

  if (mode == MODE_ARPEGGIATOR) {
    if (!arpRunning) {
      // if the arpeggiator is being manipulated in any way, do not advance the pulseClock
      // or manipulate the data structures.
      return;
    } else {
      doArpeggiator();
    }
  }

  pulseClock++;
}


