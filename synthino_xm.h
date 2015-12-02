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


#ifndef _SYNTHINOXM_H_
#define _SYNTHINOXM_H_

#include <util/delay.h>
#include <Arduino.h>

//#define DEBUG_ENABLE

#define BUTTON1_PIN 12
#define BUTTON2_PIN 13
#define BUTTON3_PIN 15
#define BUTTON4_PIN 14
#define LED1_PIN 24
#define LED2_PIN 25
#define LED3_PIN 20
#define LED4_PIN 21

#define N_BUTTONS 4
#define N_LEDS 4
#define BUTTON1 0
#define BUTTON2 1
#define BUTTON3 2
#define BUTTON4 3
#define START_BUTTON BUTTON1
#define OSC_BUTTON BUTTON2
#define FILTER_LFO_BUTTON BUTTON3
#define AMP_LFO_BUTTON BUTTON4
#define RUNNING_LED 0
#define OSC_LED 1
#define FILTER_LFO_LED 2
#define AMP_LFO_LED 3
#define LED_BLINK_MS 200

#define SILENCE 2047
#define MIDI_LOW 21
#define MIDI_HIGH 60
#define NOTE_OFF 0
#define NOTE_PENDING_OFF 1
#define MAX_NOTES 2
#define MAX_NOTE_VOL 64
#define LFO_PITCH 0
#define LFO_FILTER 1
#define LFO_AMP 2
#define NUM_OSC 2
#define NUM_LFO 3
#define NUM_FILTERS 2
#define UNSET 255
#define OUTPUT_RATE 25000
#define LFO_CLOCK_RATE 1000
#define RESET_PRESS_DURATION 2000

#define BASE_TUNING_ADJUSTMENT -0.50

#define POT_LOCK_TOLERANCE 5
#define POT_MIN 2

typedef struct {
  volatile byte midiVal;
  byte origMidiVal; // used for channel 10 drums
  volatile float frequency;
  float lastFrequency;
  byte waveform;
  const int16_t *waveformBuf;
  boolean isSample;
  boolean isPreview;
  unsigned int sampleLength;
  int volLevelDuration;
  volatile int volLevelRemaining;
  byte envelopePhase;
  byte volIndex;
  byte targetVolIndex;
  int volIndexInc;
  byte midiChannel;
  byte trigger;
  unsigned long startTime;
  byte volume;
  byte volumeNext;
  float volumeScale;
  volatile boolean volumeScaled;
  boolean doScale;
  volatile unsigned int phase;
  volatile unsigned int phaseInc;
  volatile byte phaseFraction;
  volatile byte phaseFractionInc;
  volatile int lastOutput;
} note_t;

extern float noteTable[];
extern byte PITCH_POT;
extern byte DETUNE_POT;
extern byte WAVEFORM_SELECT_POT;
extern byte LFO_RATE_POT;
extern byte LFO_DEPTH_POT;
extern byte VOLUME_POT0;
extern byte VOLUME_POT1;
extern byte FILTER_CUTOFF_POT0;
extern byte FILTER_CUTOFF_POT1;

extern volatile unsigned int counterEnd;
extern int output;
extern note_t note[];
extern byte selectedOsc;
extern byte selectedLFO;
extern boolean running;
extern int tuningSetting;
extern byte logVolume[];
extern byte led[];
extern byte ledState[];
extern uint32_t ledEventTimer[];
extern byte buttonPins[];
extern byte buttonState[];
extern byte buttonReleaseState[];
extern unsigned long lastDebugPrint;

extern int pitchReading;
extern byte pitch;
extern int detuneReading;
extern float detune;
extern int waveformReading[];
extern byte waveform[];
extern byte volume[];
extern int lastLFORateReading[NUM_LFO][NUM_OSC];
extern int lastLFODepthReading[NUM_LFO][NUM_OSC];

extern byte pitchPotTolerance;
extern byte detunePotTolerance;
extern byte waveformPotTolerance;
extern byte lfoRatePotTolerance;
extern byte lfoDepthPotTolerance;


extern int filterCutoff[];
extern int filterCutoffReading[];
extern int filterResonance;
extern long feedback[];
extern int buf0[];
extern int buf1[];

extern float lfoFrequency[NUM_LFO][NUM_OSC];
extern float lfoDepth[NUM_LFO][NUM_OSC];
extern float lfoShift[NUM_LFO][NUM_OSC];
extern byte lfoWaveform;
extern volatile unsigned int lfoPhase[NUM_LFO][NUM_OSC];
extern volatile unsigned int lfoPhaseInc[NUM_LFO][NUM_OSC];
extern volatile byte lfoPhaseFraction[NUM_LFO][NUM_OSC];
extern volatile byte lfoPhaseFractionInc[NUM_LFO][NUM_OSC];
extern const int16_t *lfoWaveformBuf;
extern boolean lfoEnabled[NUM_LFO][NUM_OSC];
extern boolean lastLFOEnabled[NUM_LFO][NUM_OSC];


// Atmel toolchain has this macro named PROTECTED_WRITE
#ifndef _PROTECTED_WRITE
#define _PROTECTED_WRITE(reg, value) PROTECTED_WRITE(reg, value)
#endif

// Function Prototypes
void hardwareInit();
void reset(boolean);
void test();
void adjustTuning();
void checkReset();
void startNote(byte, byte);
void stopNote(byte);
void eepromClear();
void readGlobalSettings();
void writeGlobalSettings();
void loadPatch(byte);
void savePatch(byte);
boolean patchValid(byte);
boolean eepromValid();
void eepromSetValid();
void beep(int);
void beep(byte, int);
byte selectPatch(byte);
void readButtons();

void setWaveform(byte, byte);
void setOscPotReadings();
void setLFOPotReadings();
void setPhaseIncrement(byte);
int adjustAmplitude(int, byte);
void initNote(byte, byte);
void setFilterFeedback(byte);

void updateLFO(byte, byte);

int sampledAnalogRead(int);
boolean buttonPressed(byte);
boolean buttonPressed(byte, boolean);
boolean buttonPressedNew(byte);
boolean buttonHeld(byte);
boolean buttonReleased(byte);
int getMemory();
void debugprint(const char *, int);
void debugprintln(const char *, int);
void debugprint(const char *, int, int);
void debugprintln(const char *, int, int);
void debugprint(const char *, float, int);
void debugprintln(const char *, float, int);
void debugprint(const char *);
void debugprintln(const char *);
void debugprintln();

#ifdef DEBUG_ENABLE
void doDebug();
#endif




#endif
