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

#define LOAD 0
#define SAVE 1

boolean resetPressed = false;
unsigned long resetPressTime;
float tuningAdjustment = BASE_TUNING_ADJUSTMENT;
byte patch = UNSET;


void checkReset() {
  boolean currentlyPressed = (digitalRead(BUTTON4_PIN) == LOW);
  if (currentlyPressed != resetPressed) {
    if (currentlyPressed) {
      resetPressTime = millis();
      resetPressed = true;
    } else {
      resetPressed = false;
    }
  } else {
    if (currentlyPressed) {
      if ((millis() - resetPressTime) > RESET_PRESS_DURATION) {
	resetPressed = false;
	patch = UNSET;
	arpRunning = false;
	seqRunning = false;
	if (buttonPressed(BUTTON1)) {
	  for(int i=0;i<MAX_NOTES;i++) {
	    stopNote(i);
	  }
	  patch = selectPatch(SAVE);
	  savePatch(patch);
	  beep(81, 50);
	}
	reset(true);
      }
    }
  }
}

void reset(boolean softReset) {
  arpRunning = false;
  seqRunning = false;

  initNoise();

  for(int i=0;i<4;i++) {
    buttonState[i] = HIGH;
    buttonReleaseState[i] = HIGH;
    ledState[i] = LOW;
    ledEventTimer[i] = 0L;
  }

  for(int i=0;i<MAX_NOTES;i++) {
    stopNote(i);
  }

  for(int i=0;i<MAX_NOTES;i++) {
    note[i].midiVal = NOTE_OFF;
    note[i].midiChannel = 0;
    note[i].trigger = UNSET;
    note[i].frequency = 0;
    note[i].lastFrequency = 0;
    note[i].waveform = 0;
    note[i].waveformBuf = waveformBuffers[0];
    note[i].isSample = false;
    note[i].isPreview = false;
    note[i].phase = 0;
    note[i].phaseInc = 0;
    note[i].phaseFraction = 0;
    note[i].phaseFractionInc = 0;
    note[i].envelopePhase = OFF;
  }

  button[0].midiVal = 60;
  button[1].midiVal = 64;
  button[2].midiVal = 67;
  button[3].midiVal = 72;

  filterCutoff = 255; // set to highest value regardless of pot position.
  filterResonance = 255;
  setFilterFeedback();
  filterCutoffPotTolerance = POT_LOCK_TOLERANCE;
  filterResonancePotTolerance = POT_LOCK_TOLERANCE;

  if ((!softReset) && (buttonPressed(BUTTON1))) {
    if (buttonPressed(BUTTON2)) {
      // Factory reset procedure
      eepromClear();
      beep(50);
    } else {
      // Test procedure
      test();
    }
  } else {
    if (buttonPressed(BUTTON2)) {
      patch = selectPatch(LOAD);
      if (!patchValid(patch)) {
	beep(45, 100);
	delay(100);
	beep(45, 100);
      }
    }
  }

  readGlobalSettings();
  if ((buttonPressed(BUTTON4)) && (!softReset)) {
    adjustTuning();
  }

  // precompute frequencies. Indexed by MIDI note number.
  tuningAdjustment = BASE_TUNING_ADJUSTMENT + ((float)tuningSetting) / 256.0; // [-2, 2]
  for(int i=MIDI_LOW;i<=MIDI_HIGH;i++) {
    noteTable[i-MIDI_LOW] = 440.0 * (pow(2, ((i-69)+tuningAdjustment)/12.0));
  }

  mode = selectMode();

  if (mode == MODE_SYNTH) {
    ATTACK_TIME_POT = 0;
    DECAY_TIME_POT = 0;
    SUSTAIN_LEVEL_POT = 1;
    RELEASE_TIME_POT = 1;
    LFO_RATE_POT = 2;
    LFO_DEPTH_POT = 3;
    CHANNEL_SELECT_POT = 4;
    PITCH_POT = 5;
    DETUNE_POT = 5;
    WAVEFORM_SELECT_POT = 6;
    LFO_WAVEFORM_SELECT_POT = 6;
    FILTER_CUTOFF_POT = 7;
    FILTER_RESONANCE_POT = 7;
  }
  if (mode == MODE_ARPEGGIATOR) {
    ATTACK_TIME_POT = 0;
    DECAY_TIME_POT = 0;
    SUSTAIN_LEVEL_POT = 1;
    RELEASE_TIME_POT = 1;
    LFO_RATE_POT = 2;
    LFO_DEPTH_POT = 3;
    ARPEGGIATOR_ROOT_NOTE_POT = 4;
    ARPEGGIATOR_NOTE_LENGTH_POT = 4;
    BPM_POT = 5;
    WAVEFORM_SELECT_POT = 6;
    LFO_WAVEFORM_SELECT_POT = 6;
    FILTER_CUTOFF_POT = 7;
    FILTER_RESONANCE_POT = 7;
  }
  if (mode == MODE_GROOVEBOX) {
    ATTACK_TIME_POT = 0;
    RELEASE_TIME_POT = 1;
    TRACK_TRANSPOSE_POT = 2;
    TRACK_FADER_POT = 3;
    TRACK_SELECT_POT = 4;
    BPM_POT = 5;
    WAVEFORM_SELECT_POT = 6;
    FILTER_CUTOFF_POT = 7;
  }

  for(byte i=0;i<N_NOTE_BUTTONS;i++) {
    button[i].pitchReading = sampledAnalogRead(PITCH_POT);
  }  

  for(byte i=0;i<N_SETTINGS;i++) {
    settings[i].attackReading = sampledAnalogRead(ATTACK_TIME_POT);
    settings[i].decayReading = 0;
    settings[i].sustainReading = 1023;
    settings[i].releaseReading = sampledAnalogRead(RELEASE_TIME_POT);
    settings[i].pitchBend = 0;
    settings[i].detune = 0.0;
    settings[i].waveformReading = sampledAnalogRead(WAVEFORM_SELECT_POT);

    int setting = map(settings[i].attackReading, POT_MIN, 1023, 1, ATTACK_RANGE);
    if (settings[i].attackReading < POT_MIN) {
      setting = 0;
    }
    settings[i].attackVolLevelDuration = setting;

    setting = map(settings[i].decayReading, POT_MIN, 1023, 1, DECAY_RANGE);
    if (settings[i].decayReading < POT_MIN) {
      setting = 0;
    }
    settings[i].decayVolLevelDuration = setting;

    float fSetting = settings[i].sustainReading / 1000.0;
    if (fSetting > 1.0) {
      fSetting = 1.0;
    }
    settings[i].sustainVolLevel = fSetting;

    setting = map(settings[i].releaseReading, POT_MIN, 1023, 1, RELEASE_RANGE);
    if (settings[i].releaseReading < POT_MIN) {
      setting = 0;
    }
    settings[i].releaseVolLevelDuration = setting;

    settings[i].waveform = map(settings[i].waveformReading, 0, 1024, 0, N_TOTAL_WAVEFORMS);
  }

  lastLFOWaveformReading = sampledAnalogRead(LFO_WAVEFORM_SELECT_POT);
  lfoWaveform = map(lastLFOWaveformReading, 0, 1024, 0, N_LFO_WAVEFORMS);

  lastFilterCutoffReading = sampledAnalogRead(FILTER_CUTOFF_POT) >> 2; // range 0-255
  filterCutoffReading = lastFilterCutoffReading;
  lastFilterResonanceReading = sampledAnalogRead(FILTER_RESONANCE_POT) >> 2;

  if (mode == MODE_SYNTH) {
    selectedSettingsReading = sampledAnalogRead(CHANNEL_SELECT_POT);
    selectedSettings = map(selectedSettingsReading, 0, 1024, 0, N_MIDI_CHANNELS);
  }

  if (mode == MODE_ARPEGGIATOR) {
    initArpeggiator();
  }

  if (mode == MODE_GROOVEBOX) {
    initGroovebox();
    selectedSettingsReading = sampledAnalogRead(TRACK_SELECT_POT);
    selectedSettings = map(selectedSettingsReading, 0, 1024, 0, SEQ_NUM_TRACKS);
  }

  fnEnabled = false;

  if (patch != UNSET) {
    loadPatch(patch);
    setPotReadings();
  }


  writeGlobalSettings();
  delay(50);

#ifdef DEBUG_ENABLE
  debugprint("tuningAdjustment=", tuningAdjustment);
  debugprint(" A4=", noteTable[69-MIDI_LOW], 4);
  debugprintln(" Hz");
#endif

}

void adjustTuning() {
  beep(50);
  delay(25);
  beep(50);
  delay(200);

  // start playing an A4
  initNote(0, 69);
  note[0].frequency = 440.0;
  setPhaseIncrement(0);
  note[0].volume = MAX_NOTE_VOL;
  note[0].volumeNext = MAX_NOTE_VOL;
  while (buttonPressed(BUTTON4)) {
    tuningSetting = map(sampledAnalogRead(7), 0, 1023, -512, 511);
    tuningAdjustment = (float)(BASE_TUNING_ADJUSTMENT + ((float)tuningSetting) / 256.0); // [-2, 2]
    note[0].frequency = 440.0 * (pow(2, (tuningAdjustment)/12.0));
#ifdef DEBUG_ENABLE
    delay(100);
    debugprint("tuningAdjustment = ", tuningAdjustment, 4);
    debugprintln("   f = ", note[0].frequency, 4);
#endif
    setPhaseIncrement(0);
    delay(10);
  }
  note[0].midiVal = NOTE_PENDING_OFF;
  writeGlobalSettings();
}

byte selectPatch(byte mode) {
  byte selected = 0;
  
  if (mode == SAVE) {
    beep(50);
  }
  // user is selecting a patch to load or save
  byte b = (mode == SAVE) ? BUTTON1 : BUTTON2;
  while (buttonPressed(b)) {
    selected = map(sampledAnalogRead(4), 0, 1024, 0, 4);
    for(byte i=0;i<4;i++) {
      if (i == selected) {
	digitalWrite(led[i], HIGH);
      } else {
	digitalWrite(led[i], LOW);
      }
    }
  }
  delay(20);
  for(byte i=0;i<4;i++) {
    digitalWrite(led[i], LOW);
  }
  return selected;
}

byte selectMode() {
  byte ledState = HIGH;
  while (true) {
    for(byte i=0;i<4;i++) {
      digitalWrite(led[i], ledState);
    }
    for(byte i=0;i<10;i++) {
      if (buttonPressedNew(BUTTON1)) {
	for(byte j=0;j<4;j++) {
	  digitalWrite(led[j], LOW);
	}
	digitalWrite(led[0], HIGH);
	while (buttonPressed(BUTTON1)) ;
	return MODE_SYNTH;
      }
      if (buttonPressedNew(BUTTON2)) {
	for(byte j=0;j<4;j++) {
	  digitalWrite(led[j], LOW);
	}
	digitalWrite(led[1], HIGH);
	while (buttonPressed(BUTTON2)) ;
	return MODE_ARPEGGIATOR;
      }
      if (buttonPressedNew(BUTTON3)) {
	for(byte j=0;j<4;j++) {
	  digitalWrite(led[j], LOW);
	}
	digitalWrite(led[2], HIGH);
	while (buttonPressed(BUTTON3)) ;
	return MODE_GROOVEBOX;
      }
      delay(40);
    }    
    if (ledState == HIGH) {
      ledState = LOW;
    } else {
      ledState = HIGH;
    }
  }
}

void beep(int duration) {
  beep(81, duration);
}

void beep(byte midiVal, int duration) {
  note_t *beepNote = &note[0];
  initNote(0, midiVal);
  beepNote->waveform = 1;
  beepNote->waveformBuf = waveformBuffers[1];
  beepNote->frequency = 440.0 * (pow(2, ((midiVal-69))/12.0));
  beepNote->volume = MAX_NOTE_VOL;
  beepNote->volumeNext = MAX_NOTE_VOL;
  setPhaseIncrement(0);
  delay(duration);
  beepNote->midiVal = NOTE_PENDING_OFF;
}

