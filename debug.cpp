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

void debugprint(const char *msg) {
  Serial.print(msg);
}

void debugprintln(const char *msg) {
  Serial.println(msg);
}

void debugprint(const char *msg, int val) {
  Serial.print(msg);
  Serial.print(val);
}

void debugprint(const char *msg, int val, int base) {
  Serial.print(msg);
  Serial.print(val, base);
}

void debugprintln(const char *msg, int val) {
  debugprint(msg, val);
  Serial.println();
}

void debugprintln(const char *msg, int val, int base) {
  debugprint(msg, val, base);
  Serial.println();
}

void debugprint(const char *msg, float val, int dec) {
  Serial.print(msg);
  Serial.print(val, dec);
}

void debugprintln(const char *msg, float val, int dec) {
  debugprint(msg, val, dec);
  Serial.println();
}

void debugprintln() {
  Serial.println();
}

#ifdef DEBUG_ENABLE
void doDebug() {
  if (debug) {
    if ((millis() - lastDebugPrint) >= 1000) {
      lastDebugPrint = millis();

      // Print the number of instruction cycles remaining at the end of the ISR.
      // The more work you try to do in the ISR, the lower this number will become.
      // If the number of cycles remaining reaches 0, then the ISR will take up
      // all the CPU time and the code in v will not run.

      /*
      Serial.print(" arpPosition: ");
      Serial.print(arpPosition);
      Serial.print(" arpLength: ");
      Serial.print(arpLength);
      */


      /*
      for(byte i=0;i<MAX_NOTES;i++) {
        Serial.print("midiVal[");
        Serial.print(i);
        Serial.print("]=");
        Serial.print(note[i].midiVal);
        Serial.print("  ");

        Serial.print("f[");
        Serial.print(i);
        Serial.print("]=");
        Serial.print(note[i].frequency);

        Serial.print(" vol[");
        Serial.print(i);
        Serial.print("]=");
        Serial.print(note[i].volume);
        Serial.print(" | ");
      }
      */


      if ((TCC0.PER - counterEnd) < 150) {
	Serial.print(" cycles = ");
	Serial.print(TCC0.PER - counterEnd);
	Serial.println("");
      }


      unsigned int remaining = TCC0.PER - counterEnd;
      if (remaining < 50) {
        Serial.print("WARNING: only ");
	Serial.print(remaining);
	Serial.println(" cycles remaining in ISR. Reduce sample rate or reduce the amount of code in the ISR.");
      }
    }
  } // if (debug)
}

#endif

int getMemory() {
  int size = 6000;
  byte *buf;
  while ((size > 0) && ((buf = (byte *) malloc(--size)) == NULL));
  free(buf);
  return size;
} 

