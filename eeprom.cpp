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

#include <avr/eeprom.h>
#include "synthino_xm.h"
#include "waveforms.h"

#define EEPROM_MAGIC_NUMBER 0xbad0
#define PATCH_VALID_MARK 0xe3ad
#define SEQUENCE_VALID_MARK 0xf7da
#define PATCH_BASE_ADDR 5
#define PATCH_SIZE 509
#define NUM_PATCHES 4

byte readByte(uint16_t);
uint16_t writeByte(uint16_t, byte);
unsigned int readWord(uint16_t);
uint16_t writeWord(uint16_t, unsigned int);
float readFloat(uint16_t);
uint16_t writeFloat(uint16_t, float);

byte ledToggle = LOW;

void readGlobalSettings() {
  if (eepromValid()) {
    mode = readByte(2);
    tuningSetting = readWord(3);
#ifdef DEBUG_ENABLE
    debugprintln("tuningSetting = ", tuningSetting);
#endif
  }
}

void writeGlobalSettings() {
  eepromSetValid();

  cli();
  writeByte(2, mode);
  writeWord(3, tuningSetting);
  sei();
}

boolean patchValid(byte i) {
  cli();
  uint16_t addr = PATCH_BASE_ADDR + (i * PATCH_SIZE);
  unsigned int valid = readWord(addr);
  sei();
  return (valid == PATCH_VALID_MARK);
}

void toggleLED(byte i) {
  if (ledToggle == HIGH) {
    ledToggle = LOW;
  } else {
    ledToggle = HIGH;
  }
  digitalWrite(led[i], ledToggle);
}

void savePatch(byte p) {
  debugprintln("saving patch ", p);
  cli();
  uint16_t addr = PATCH_BASE_ADDR + (p * PATCH_SIZE);
  uint16_t startAddr = addr;
  addr = writeWord(addr, PATCH_VALID_MARK);
  for(byte i=0;i<N_SETTINGS;i++) {
    toggleLED(p);
    addr = writeWord(addr, settings[i].attackVolLevelDuration);
    addr = writeWord(addr, settings[i].decayVolLevelDuration);
    uint16_t sustain = (settings[i].sustainVolLevel * 1000);
    addr = writeWord(addr, sustain);
    toggleLED(p);
    addr = writeWord(addr, settings[i].releaseVolLevelDuration);
    int16_t detune = (settings[i].detune * 1024);
    addr = writeWord(addr, detune);
    addr = writeByte(addr, settings[i].waveform);
  }

  toggleLED(p);
  addr = writeWord(addr, filterCutoff);
  addr = writeWord(addr, filterResonance);
  addr = writeByte(addr, lfoWaveform);

  if (mode == MODE_GROOVEBOX) {
    addr = writeWord(addr, SEQUENCE_VALID_MARK);
    for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
      toggleLED(p);
      uint16_t volume = track[t].volumeScale * 1023;
      addr = writeWord(addr, volume);
      for(byte s=0;s<SEQ_LENGTH;s++) {
	toggleLED(p);
	addr = writeByte(addr, seq[s][t].midiVal);
	addr = writeByte(addr, seq[s][t].velocity);
	addr = writeByte(addr, seq[s][t].waveform);
	addr = writeWord(addr, seq[s][t].duration);
	addr = writeWord(addr, seq[s][t].startPulse);
      }
    }
    
  }
  digitalWrite(led[p], LOW);
  debugprintln("size = ", addr-startAddr);
  debugprintln("end address = ", addr);
  sei();
}

void loadPatch(byte p) {
  debugprintln("loading patch ", p);
  if (!patchValid(p)) {
    return;
  }

  cli();
  uint16_t addr = PATCH_BASE_ADDR + (p * PATCH_SIZE);
  addr += 2;
  for(byte i=0;i<N_SETTINGS;i++) {
    toggleLED(p);
    settings[i].attackVolLevelDuration = readWord(addr);
    addr += sizeof(uint16_t);
    settings[i].decayVolLevelDuration = readWord(addr);
    addr += sizeof(uint16_t);
    uint16_t sustain = readWord(addr);
    settings[i].sustainVolLevel = sustain / 1000.0;
    addr += sizeof(uint16_t);
    toggleLED(p);
    settings[i].releaseVolLevelDuration = readWord(addr);
    addr += sizeof(uint16_t);
    int16_t detune = readWord(addr);
    settings[i].detune = (float)(detune / 1024.0);
    addr += sizeof(uint16_t);
    settings[i].waveform = readByte(addr);
    addr += sizeof(uint8_t);
  }

  toggleLED(p);
  filterCutoff = readWord(addr);
  addr += sizeof(uint16_t);
  filterResonance = readWord(addr);
  addr += sizeof(uint16_t);
  lfoWaveform = readByte(addr);
  lfoWaveformBuf = lfoWaveformBuffers[lfoWaveform];
  addr += sizeof(uint8_t);

  if (mode == MODE_GROOVEBOX) {
    uint16_t sequenceValidMark = readWord(addr);
    addr += sizeof(uint16_t);
    if (sequenceValidMark == SEQUENCE_VALID_MARK) {
      debugprintln("loading sequence");
      for(byte t=0;t<SEQ_NUM_TRACKS;t++) {
	toggleLED(p);
	uint16_t volume = readWord(addr);
	addr += sizeof(uint16_t);
	track[t].volumeScale = (float)volume / 1023.0;
	for(byte s=0;s<SEQ_LENGTH;s++) {
	  toggleLED(p);
	  seq[s][t].midiVal = readByte(addr);
	  addr += sizeof(uint8_t);
	  seq[s][t].velocity = readByte(addr);
	  addr += sizeof(uint8_t);
	  seq[s][t].waveform = readByte(addr);
	  addr += sizeof(uint8_t);
	  seq[s][t].duration = readWord(addr);
	  addr += sizeof(uint16_t);
	  seq[s][t].startPulse = readWord(addr);
	  addr += sizeof(uint16_t);
	}
      }
    } else {
      debugprintln("valid sequence not found");
    }
  }
  digitalWrite(led[p], LOW);

  sei();
}



void eepromClear() {
  cli();
  for(int i=0;i<2048;i++) {
    if ((i % 8) == 0) {
      toggleLED(0);
    }
    writeByte(i, 0);
  }
  digitalWrite(led[0], LOW);
  sei();
}

boolean eepromValid() {
  // determine if the EEPROM has ever been written by this firmware
  // so we can determine if the values can be trusted
  unsigned int magic = readWord(0);
#ifdef DEBUG_ENABLE
  if (magic == EEPROM_MAGIC_NUMBER) {
    debugprintln("EEPROM valid");
  } else {
    debugprintln("EEPROM NOT valid: ", (int)magic, 16);
  }
#endif
  return (magic == EEPROM_MAGIC_NUMBER);
}
  
void eepromSetValid() {
  cli();
  writeWord(0, EEPROM_MAGIC_NUMBER);
  sei();
}



uint16_t writeByte(uint16_t addr, byte val) {
  eeprom_write_byte((uint8_t *)addr, val);
  return addr + sizeof(uint8_t);
}

byte readByte(uint16_t addr) {
  return eeprom_read_byte((uint8_t *)addr);
}

uint16_t writeWord(uint16_t addr, uint16_t val) {
  eeprom_write_word((uint16_t *)addr, val);
  return addr + sizeof(uint16_t);
}

unsigned int readWord(uint16_t addr) {
  return eeprom_read_word((uint16_t *)addr);
}

uint16_t writeFloat(uint16_t addr, float val) {
  eeprom_write_float((float *)addr, val);
  return addr + sizeof(float);
}

float readFloat(uint16_t addr) {
  return eeprom_read_float((float *)addr);
}
