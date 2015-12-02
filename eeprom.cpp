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

#define EEPROM_MAGIC_NUMBER 0xbadd
#define PATCH_VALID_MARK 0xe3ab
#define PATCH_BASE_ADDR 5
#define PATCH_SIZE 63
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
    tuningSetting = readWord(3);
#ifdef DEBUG_ENABLE
    debugprintln("tuningSetting = ", tuningSetting);
#endif
  }
}

void writeGlobalSettings() {
  eepromSetValid();

  cli();
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

  addr = writeByte(addr, pitch);

  addr = writeFloat(addr, detune);

  addr = writeByte(addr, waveform[0]);
  addr = writeByte(addr, waveform[1]);

  for(byte i=0;i<NUM_LFO;i++) {
    for(byte j=0;j<NUM_OSC;j++) {
      addr = writeByte(addr, lfoEnabled[i][j]);
      addr = writeFloat(addr, lfoFrequency[i][j]);
      addr = writeFloat(addr, lfoDepth[i][j]);
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

  pitch = readByte(addr);
  addr += sizeof(uint8_t);

  detune = readFloat(addr);
  addr += sizeof(float);

  waveform[0] = readByte(addr);
  addr += sizeof(uint8_t);
  waveform[1] = readByte(addr);
  addr += sizeof(uint8_t);

  for(byte i=0;i<NUM_LFO;i++) {
    for(byte j=0;j<NUM_OSC;j++) {
      lfoEnabled[i][j] = readByte(addr);
      addr += sizeof(uint8_t);
      lfoFrequency[i][j] = readFloat(addr);
      addr += sizeof(float);
      lfoDepth[i][j] = readFloat(addr);
      addr += sizeof(float);
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
