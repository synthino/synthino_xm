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
  }

  running = false;
  filterCutoff[0] = 255; // set to highest value regardless of pot position.
  filterCutoff[1] = 255; // set to highest value regardless of pot position.
  filterResonance = 255;
  setFilterFeedback(0);
  setFilterFeedback(1);

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

  PITCH_POT = 0;
  DETUNE_POT = 0;
  WAVEFORM_SELECT_POT = 1;
  LFO_RATE_POT = 2;
  LFO_DEPTH_POT = 3;
  VOLUME_POT0 = 4;
  VOLUME_POT1 = 6;
  FILTER_CUTOFF_POT0 = 5;
  FILTER_CUTOFF_POT1 = 7;

  for(byte i=0;i<NUM_LFO;i++) {
    for(byte j=0;j<NUM_OSC;j++) {
      lfoEnabled[i][j] = false;
      lastLFOEnabled[i][j] = false;
      lastLFORateReading[i][j] = 0;
      lastLFODepthReading[i][j] = 0;
      lfoFrequency[i][j] = 0.0;
      lfoDepth[i][j] = 0.0;
      lfoPhase[i][j] = 0;
      lfoPhaseInc[i][j] = 0;
      lfoPhaseFraction[i][j] = 0;
      lfoPhaseFractionInc[i][j] = 0;
    }
  }  

  selectedOsc = 0;
  selectedLFO = LFO_PITCH;

  detune = 0.0;
  int reading = sampledAnalogRead(WAVEFORM_SELECT_POT);
  waveformReading[0] = reading;
  waveformReading[1] = reading;
  waveform[0] = map(reading, 0, 1024, 0, N_TOTAL_WAVEFORMS);
  waveform[1] = map(reading, 0, 1024, 0, N_TOTAL_WAVEFORMS);
  detuneReading = sampledAnalogRead(DETUNE_POT);
  pitchReading = sampledAnalogRead(PITCH_POT);
  pitch = map(reading, 0, 1024, MIDI_LOW, MIDI_HIGH+1);

  if (patch != UNSET) {
    loadPatch(patch);
    setOscPotReadings();
    setLFOPotReadings();
  }


  writeGlobalSettings();
  delay(50);

  for(byte i=0;i<4;i++) {
    digitalWrite(led[i], HIGH);
  }
  delay(200);
  for(byte i=0;i<4;i++) {
    digitalWrite(led[i], LOW);
  }


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

