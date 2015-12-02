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

volatile unsigned int counterEnd;
volatile unsigned long pulseClock = 0;
int output = SILENCE;

note_t note[MAX_NOTES];
unsigned long lastDebugPrint = 0;

int tuningSetting = 0;

byte buttonPins[4] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN};
byte buttonState[4] = {HIGH, HIGH, HIGH, HIGH};
byte buttonReleaseState[4] = {HIGH, HIGH, HIGH, HIGH};
byte led[4] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};
byte ledState[4] = {LOW, LOW, LOW, LOW};
uint32_t ledEventTimer[4] = {0L, 0L, 0L, 0L};

// logarithmic volume scale for decay and release envelope contours
byte logVolume[65] = {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, 33, 35, 37, 40, 43, 46, 49, 53, 56, 60, 64};

float noteTable[(MIDI_HIGH-MIDI_LOW)+1];

byte PITCH_POT;
byte DETUNE_POT;
byte WAVEFORM_SELECT_POT;
byte LFO_RATE_POT;
byte LFO_DEPTH_POT;
byte VOLUME_POT0;
byte VOLUME_POT1;
byte FILTER_CUTOFF_POT0;
byte FILTER_CUTOFF_POT1;

byte waveformPotTolerance = 2;
byte lfoRatePotTolerance = 0;
byte lfoDepthPotTolerance = 0;
byte pitchPotTolerance = 0;
byte detunePotTolerance = 0;

boolean running;
byte selectedOsc = 0;
byte selectedLFO = 0;

int filterCutoffReading[NUM_FILTERS];
int pitchReading;
byte pitch;
int detuneReading;
float detune;
int waveformReading[NUM_OSC];
byte waveform[NUM_OSC];
byte volume[NUM_OSC];

boolean lfoEnabled[NUM_LFO][NUM_OSC];
boolean lastLFOEnabled[NUM_LFO][NUM_OSC];
int lastLFORateReading[NUM_LFO][NUM_OSC];
int lastLFODepthReading[NUM_LFO][NUM_OSC];

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

#ifdef DEBUG_ENABLE
  debugprint("available SRAM=", getMemory());
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
    }
  }

#ifdef DEBUG_ENABLE
  doDebug();
#endif

  readButtons();

  // Read pitch
  if (running && (selectedOsc == 0)) {
    reading = sampledAnalogRead(PITCH_POT);
    diff = abs(reading - pitchReading);
    if (diff > pitchPotTolerance) {
      pitchReading = reading;
      pitch = map(reading, 0, 1024, MIDI_LOW, MIDI_HIGH+1);
      note[0].midiVal = pitch;
      note[1].midiVal = pitch;
    }
  }


  // Read detune
  if (selectedOsc == 1) {
    reading = sampledAnalogRead(DETUNE_POT);
    diff = abs(reading - detuneReading);
    if (diff > detunePotTolerance) { 
      detunePotTolerance = 2;
      detuneReading = reading;
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
      detune = fSetting;
    }
  }


  // Set volume of each oscillator
  reading = sampledAnalogRead(VOLUME_POT0);
  volume[0] = map(reading, 0, 1023, 0, MAX_NOTE_VOL);
  reading = sampledAnalogRead(VOLUME_POT1);
  volume[1] = map(reading, 0, 1023, 0, MAX_NOTE_VOL);


  // Compare the waveform pot position to the position it had when the
  // current Channel was selected. Only change the channel waveform if it
  // is different.
  reading = sampledAnalogRead(WAVEFORM_SELECT_POT);
  diff = abs(reading - waveformReading[selectedOsc]);
  setting = map(reading, 0, 1024, 0, N_TOTAL_WAVEFORMS);
  if ((diff > waveformPotTolerance) && (setting != waveform[selectedOsc])) {
    // tolerance of 2 prevents too much toggling at waveform boundaries.
    waveformPotTolerance = 2;
    waveformReading[selectedOsc] = reading;
    setWaveform(selectedOsc, setting);
  }


  // Read filter settings
  filterCutoffReading[0] = sampledAnalogRead(FILTER_CUTOFF_POT0) >> 2; // range 0-255
  if (!lfoEnabled[LFO_FILTER][0]) {
    // Only use the knob reading if the LFO is not modulating the filter.
    filterCutoff[0] = filterCutoffReading[0];
    setFilterFeedback(0);
  }
  filterCutoffReading[1] = sampledAnalogRead(FILTER_CUTOFF_POT1) >> 2; // range 0-255
  if (!lfoEnabled[LFO_FILTER][1]) {
    // Only use the knob reading if the LFO is not modulating the filter.
    filterCutoff[1] = filterCutoffReading[1];
    setFilterFeedback(1);
  }


  // Read LFO settings
  reading = sampledAnalogRead(LFO_RATE_POT);
  setting = map(reading, 5, 1023, 0, 1023);
  if (reading < 5) {
    setting = 0;
  }
  diff = abs(reading - lastLFORateReading[selectedLFO][selectedOsc]);
  if (diff > lfoRatePotTolerance) {
    lfoRatePotTolerance = 0;
    lastLFORateReading[selectedLFO][selectedOsc] = reading;
    if (reading > 0) {
      if (!lfoEnabled[selectedLFO][selectedOsc]) {
	// reset the LFO phase
	lfoPhase[selectedLFO][selectedOsc] = 0;
      }
      lfoEnabled[selectedLFO][selectedOsc] = true;
      if (reading < 512) {
	lfoFrequency[selectedLFO][selectedOsc] = reading / 102.2;    // map to frequency in range [0.0, 5.0]
      } else {
	lfoFrequency[selectedLFO][selectedOsc] = 5.0 + ((reading-512) / 1.2937);    // map to frequency in range [5.0, 400.0]
      }
    } else {
      lfoEnabled[selectedLFO][selectedOsc] = false;
    }
    if (lfoDepth[selectedLFO][selectedOsc] == 0.0) {
      // since the rate pot was moved, force reading of depth pot.
      lastLFODepthReading[selectedLFO][selectedOsc] = 0;
    }
  }
  
  reading = sampledAnalogRead(LFO_DEPTH_POT);
  setting = map(reading, 0, 1023, 20, 1023);
  diff = abs(reading - lastLFODepthReading[selectedLFO][selectedOsc]);
  if (diff > lfoDepthPotTolerance) {
    lfoDepthPotTolerance = 0;
    lfoDepth[selectedLFO][selectedOsc] = setting / 1023.0;  // 0.0-1.0
  }


  for(byte lfo=0;lfo<NUM_LFO;lfo++) {
    for(byte osc=0;osc<NUM_OSC;osc++) {
      if (lfoEnabled[lfo][osc]) {
	updateLFO(lfo, osc);
      }
    }
  }


  for(byte osc=0;osc<NUM_OSC;osc++) {
    lastLFOEnabled[LFO_PITCH][osc] = lfoEnabled[LFO_PITCH][osc];
    lastLFOEnabled[LFO_FILTER][osc] = lfoEnabled[LFO_FILTER][osc];
    lastLFOEnabled[LFO_AMP][osc] = lfoEnabled[LFO_AMP][osc];
  }


  for(byte i=0;i<NUM_OSC;i++) {
    if (note[i].midiVal > NOTE_PENDING_OFF) {
      note[i].frequency = noteTable[note[i].midiVal-MIDI_LOW];

      // Detune oscillator 1 relative to oscillator 2
      if (i == 1) {
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


      // Adjust each oscillator frequency if pitch LFO enabled.
      if (lfoEnabled[LFO_PITCH][i]) {
	note[i].frequency += note[i].frequency * (lfoShift[LFO_PITCH][i] * (0.5 * lfoDepth[LFO_PITCH][i]));
      }

      // Adjust each oscillator filter if filter LFO enabled.
      if (lfoEnabled[LFO_FILTER][i]) {
	filterCutoff[i] = filterCutoffReading[i] + ((int)(64.0 * (lfoShift[LFO_FILTER][i] * lfoDepth[LFO_FILTER][i])));
	filterCutoff[i] = constrain(filterCutoff[i], 0, 255);
	setFilterFeedback(i);
      } else {
	if (lastLFOEnabled[LFO_FILTER][i]) {
	  // The filter LFO has just been disabled, so revert back to the knob setting.
	  filterCutoff[i] = filterCutoffReading[i];
	  setFilterFeedback(i);
	}
      }

      if (lfoEnabled[LFO_AMP][i]) {
	int v = volume[i] + ((int)(MAX_NOTE_VOL * (lfoShift[LFO_AMP][i] * lfoDepth[LFO_AMP][i])));
	if (v < 0) {
	  v = 0;
	}
	if (v > MAX_NOTE_VOL) {
	  v = MAX_NOTE_VOL;
	}
	note[i].volumeNext = logVolume[v];
      } else {
	note[i].volumeNext = logVolume[volume[i]];
      }

      setPhaseIncrement(i);
    }
  }

  for(byte i=0;i<4;i++) {
    digitalWrite(led[i], ledState[i]);
  }

}

void setWaveform(byte oscIndex, byte waveformIndex) {
  waveform[oscIndex] = waveformIndex;
  note[selectedOsc].waveformBuf = waveformBuffers[waveformIndex];
  note[selectedOsc].phase = 0;
  note[selectedOsc].phaseInc = 0;
  note[selectedOsc].lastFrequency = 0.0;
}


// only call this when selectedOsc changed
void setOscPotReadings() {
  if (selectedOsc == 0) {
    pitchReading = sampledAnalogRead(PITCH_POT);
    pitchPotTolerance = POT_LOCK_TOLERANCE;
  } else {
    detuneReading = sampledAnalogRead(DETUNE_POT);
    detunePotTolerance = POT_LOCK_TOLERANCE * 4; // lock hard to avoid accidental detuning
  }

  waveformReading[selectedOsc] = sampledAnalogRead(WAVEFORM_SELECT_POT);
  waveformPotTolerance = POT_LOCK_TOLERANCE;
}

// call this when selectedOsc or selectedLFO changed
void setLFOPotReadings() {
  lastLFORateReading[selectedLFO][selectedOsc] = sampledAnalogRead(LFO_RATE_POT);
  lastLFODepthReading[selectedLFO][selectedOsc] = sampledAnalogRead(LFO_DEPTH_POT);
  lfoRatePotTolerance = POT_LOCK_TOLERANCE;
  lfoDepthPotTolerance = POT_LOCK_TOLERANCE;
}


void readButtons() {
  if (buttonPressedNew(START_BUTTON)) {
    running = !running;
    if (running) {
      startNote(0, pitch);
      startNote(1, pitch);
      ledState[RUNNING_LED] = HIGH;
    } else {
      stopNote(0);
      stopNote(1);
      ledState[RUNNING_LED] = LOW;
    }
  }

  if (buttonPressedNew(OSC_BUTTON)) {
    selectedOsc++;
    if (selectedOsc == NUM_OSC) {
      selectedOsc = 0;
    }
    if (selectedOsc == 1) {
      ledState[OSC_LED] = HIGH;
    } else {
      ledState[OSC_LED] = LOW;
    }
    setOscPotReadings();
    setLFOPotReadings();
  }

  if (buttonPressedNew(FILTER_LFO_BUTTON)) {
    if (selectedLFO != LFO_FILTER) {
      selectedLFO = LFO_FILTER;
      ledState[FILTER_LFO_LED] = HIGH;
      ledState[AMP_LFO_LED] = LOW;
    } else {
      selectedLFO = LFO_PITCH;
      ledState[FILTER_LFO_LED] = LOW;
      ledState[AMP_LFO_LED] = LOW;
    }
    setLFOPotReadings();
  }

  if (buttonPressedNew(AMP_LFO_BUTTON)) {
    if (selectedLFO != LFO_AMP) {
      selectedLFO = LFO_AMP;
      ledState[FILTER_LFO_LED] = LOW;
      ledState[AMP_LFO_LED] = HIGH;
    } else {
      selectedLFO = LFO_PITCH;
      ledState[FILTER_LFO_LED] = LOW;
      ledState[AMP_LFO_LED] = LOW;
    }
    setLFOPotReadings();
  }

}


