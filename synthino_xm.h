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
#define MIDI_ENABLE
#define USBMIDI_ENABLE

#define MODE_SYNTH 1
#define MODE_ARPEGGIATOR 2
#define MODE_GROOVEBOX 3

#define BUTTON1_PIN 12
#define BUTTON2_PIN 13
#define BUTTON3_PIN 15
#define BUTTON4_PIN 14
#define LED1_PIN 24
#define LED2_PIN 25
#define LED3_PIN 20
#define LED4_PIN 21

#define BUTTON1 0
#define BUTTON2 1
#define BUTTON3 2
#define BUTTON4 3
#define FN_BUTTON BUTTON4
#define N_LEDS 4
#define PREVIEW_LED 2
#define FN_LED 3
#define LED_BLINK_MS 200

#define SILENCE 2047
#define MIDI_LOW 21
#define MIDI_HIGH 108
#define POT_MIDI_LOW 24
#define POT_MIDI_HIGH 72
#define N_MIDI_CHANNELS 4
#define N_SETTINGS 4
#define NOTE_OFF 0
#define NOTE_PENDING_OFF 1
#define MAX_NOTES 5
#define MAX_NOTE_VOL 64
#define N_NOTE_BUTTONS 3
#define LFO_PITCH 0
#define LFO_FILTER 1
#define NUM_LFO 2
#define UNSET 255
#define MAX_VELOCITY 50
#define OUTPUT_RATE 25000
#define LFO_CLOCK_RATE 1000
#define RESET_PRESS_DURATION 2000
#define NOISE_BUF_LEN 1024

#define ARP_MAX_NOTES 16
#define DEFAULT_BPM 120
#define MIN_BPM 20
#define MAX_BPM 300
#define PPQ 96
#define ARP_PATTERN_UP 0
#define ARP_PATTERN_DOWN 1
#define ARP_PATTERN_UPDOWN 2
#define ARP_PATTERN_RANDOM 3
#define ARP_PATTERN_MAX 3

#define ARP_TYPE_MAJOR_CHORD 0
#define ARP_TYPE_MINOR_CHORD 1
#define ARP_TYPE_M7_CHORD 2
#define ARP_TYPE_m7_CHORD 3
#define ARP_TYPE_MIDI 4
#define ARP_TYPE_MAX 4
#define ARP_TIME_DIVISION_MAX 3

#define SEQ_LENGTH 16
#define SEQ_NUM_TRACKS 4
#define SEQ_PLAY 0
#define SEQ_RECORD 1
#define METRONOME_WAVEFORM 15
#define METRONOME_MODE_OFF 0
#define METRONOME_MODE_QUARTER 1
#define METRONOME_MODE_SIXTEENTH 2

#define BASE_TUNING_ADJUSTMENT -0.50

// constants for envelope phase
#define ATTACK 0
#define SUSTAIN 1
#define DECAY 2
#define RELEASE 3
#define OFF 4

#define POT_LOCK_TOLERANCE 5
#define POT_MIN 2
#define ATTACK_RANGE 500
#define DECAY_RANGE 500
#define RELEASE_RANGE 500

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

typedef struct {
  byte waveform;
  int attackVolLevelDuration;
  int decayVolLevelDuration;
  float sustainVolLevel;
  int releaseVolLevelDuration;
  int pitchBend;
  float detune;
  int waveformReading;
  int attackReading;
  int decayReading;
  int sustainReading;
  int releaseReading;
  int volumeReading;
  int detuneReading;
} settings_t;

typedef struct {
  int pitchReading;
  byte midiVal; // MIDI note assigned to button
  byte noteIndex;
} button_t;

typedef struct {
  byte midiVal;
  int transpose;
  byte velocity;
  byte noteIndex; // index into notes array
  byte waveform;
  unsigned int startPulse;
  unsigned int duration;
} sequenceNote_t;

typedef struct {
  byte noteSeqIndex; // sequence index of note currently playing or recording
  byte state;
  float volumeScale; // scaling for track fader. value is 0.0-1.0
} sequenceTrack_t;
  
extern float noteTable[];
extern byte ATTACK_TIME_POT;
extern byte DECAY_TIME_POT;
extern byte SUSTAIN_LEVEL_POT;
extern byte RELEASE_TIME_POT;
extern byte LFO_RATE_POT;
extern byte LFO_DEPTH_POT;
extern byte CHANNEL_SELECT_POT;
extern byte BPM_POT;
extern byte ARPEGGIATOR_NOTE_LENGTH_POT;
extern byte ARPEGGIATOR_ROOT_NOTE_POT;
extern byte TRACK_SELECT_POT;
extern byte TRACK_FADER_POT;
extern byte TRACK_TRANSPOSE_POT;
extern byte PITCH_POT;
extern byte DETUNE_POT;
extern byte WAVEFORM_SELECT_POT;
extern byte LFO_WAVEFORM_SELECT_POT;
extern byte FILTER_CUTOFF_POT;
extern byte FILTER_RESONANCE_POT;

extern volatile unsigned int counterEnd;
extern int output;
extern volatile unsigned long pulseClock;
extern byte mode;
extern note_t note[];
extern settings_t settings[];
extern byte selectedSettings;
extern button_t button[];
extern int tuningSetting;
extern byte logVolume[];
extern byte attackLogVolume[];
extern byte inverseAttackLogVolume[];
extern byte inverseLogVolume[];
extern byte led[];
extern byte ledState[];
extern uint32_t ledEventTimer[];
extern byte buttonPins[];
extern byte buttonState[];
extern byte buttonReleaseState[];
extern boolean fnEnabled;
extern boolean debug;
extern unsigned long lastDebugPrint;

extern int selectedSettingsReading;

extern int filterCutoffReading;
extern int lastFilterCutoffReading;
extern int lastFilterResonanceReading;

extern int lastLFORateReading[];
extern int lastLFODepthReading[];
extern int lastLFOWaveformReading;

extern int lastBPMReading;
extern int lastArpNoteLengthReading;
extern int lastArpRootNoteReading;
extern int lastTrackTransposeReading;

extern byte attackPotTolerance;
extern byte releasePotTolerance;
extern byte decayPotTolerance;
extern byte sustainPotTolerance;
extern byte waveformPotTolerance;
extern byte lfoRatePotTolerance;
extern byte lfoDepthPotTolerance;
extern byte faderPotTolerance;
extern byte filterCutoffPotTolerance;
extern byte filterResonancePotTolerance;

extern int filterCutoff;
extern int filterResonance;
extern long feedback;
extern int buf0;
extern int buf1;
extern int noiseBuf[];
extern volatile unsigned int noiseBufIndex;
extern volatile byte noiseUpdateCount;
extern unsigned int noiseBufUpdateIndex;

extern float lfoFrequency[];
extern float lfoDepth[];
extern float lfoShift[];
extern byte lfoWaveform;
extern volatile unsigned int lfoPhase[];
extern volatile unsigned int lfoPhaseInc[];
extern volatile byte lfoPhaseFraction[];
extern volatile byte lfoPhaseFractionInc[];
extern const int16_t *lfoWaveformBuf;
extern boolean lfoEnabled[];

extern unsigned int bpm;
extern boolean arpRunning;
extern byte arpTimeDivision;
extern int arpNoteStartPulse;
extern byte arpLEDIndex;
extern byte arpRootNote;
extern byte arpNoteLength;
extern volatile byte arpPosition;
extern volatile byte arpNoteIndex;
extern unsigned long arpReleaseTime;
extern byte arpLength;     // number of total notes in the arpeggio
extern byte arpPattern;
extern byte arpType;
extern int arpDirection;
extern byte arpIntervals[ARP_TYPE_MAX][8];
extern byte arpLengths[ARP_TYPE_MAX];
extern byte arpeggio[ARP_MAX_NOTES];
extern byte midiArpeggioNotes[ARP_MAX_NOTES];
extern boolean arpMIDILatch;

extern boolean seqRunning;
extern boolean seqSynchStart;
extern boolean seqPreview;
extern byte metronomeMode;
extern byte metronomeNoteIndex;
extern byte currentTrack;
extern volatile boolean seqLock;
extern byte seqStartPulse;
extern volatile byte seqIndex;
extern byte seqLEDIndex;
extern sequenceNote_t seq[SEQ_LENGTH][SEQ_NUM_TRACKS];
extern sequenceTrack_t track[SEQ_NUM_TRACKS];

extern boolean midiClock;

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
void midiInit();
void handleNoteOn(byte, byte, byte);
void handleNoteOff(byte, byte, byte);
void handleProgramChange(byte, byte);
void handleControlChange(byte, byte, byte);
void handlePitchBend(byte, int);
void handleStart();
void handleContinue();
void handleStop();
void handleSystemReset();
void handleClock();
void updateMIDIClockInfo();
byte doNoteOn(byte, byte, byte);
void doNoteOff(byte, byte);
void releaseNote(note_t *);
void stopNote(byte);
void doDrumNoteOff(byte);
void setDrumParameters(byte, byte, byte);
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
byte selectMode();
byte selectPatch(byte);
void readFnButton();
void readSynthButtons();
void readArpButtons();
void readGrooveboxButtons();

void doGroovebox();
void grooveboxNoteOn(byte, byte, byte);
void grooveboxNoteOff(byte, byte, byte);
void initGroovebox();
void resetGroovebox();
void stopSequencer();

void doArpeggiator();
void arpeggiatorNoteOn(byte);
void arpeggiatorNoteOff(byte);
void transposeMIDIArpeggio(byte);
void initArpeggiator();
void resetArpeggiator();
void setArpeggioNotes();
void setMIDIArpeggioNotes();

void setBPM(unsigned int);
void setWaveform(byte, byte);
void setPotReadings();
void setPhaseIncrement(byte);
void processEnvelope(byte);
int adjustAmplitude(int, byte);
byte metronomeTick(byte, byte);
void initNote(byte, byte);
byte findNoteIndex();
void setFilterFeedback();

void updateLFO(byte);
void updateNoise();
void initNoise();

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
