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

void hardwareInit() {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);

  analogReference(AREF_EXTERNAL_B);

  // Configure DAC
  DACB.CTRLA = DAC_CH0EN_bm | DAC_ENABLE_bm;
  DACB.CTRLB = DAC_CHSEL_SINGLE_gc;
  DACB.CTRLC = DAC_REFSEL_INT1V_gc;

  // Bring the DAC up to the middle of the voltage scale
  // gradually to avoid a pop.
  for(int i=0;i<=SILENCE;i++) {
    DACB.CH0DATA = i;
    delayMicroseconds(10);
  }

  // Configure timers

  // Audio timer
  TCC0.CTRLA = TC_CLKSEL_DIV1_gc ; // 1:1 with main clock
  TCC0.CTRLB = 0;
  TCC0.CTRLC = 0;
  TCC0.CTRLD = 0;
  TCC0.CTRLE = 0;
  TCC0.PER = (int)(F_CPU / OUTPUT_RATE);  // 32MHz / 25KHz = 1280 cycles
  TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;

  // LFO update timer
  TCC1.CTRLA = TC_CLKSEL_DIV1_gc ; // 1:1 with main clock
  TCC1.CTRLB = 0;
  TCC1.CTRLC = 0;
  TCC1.CTRLD = 0;
  TCC1.CTRLE = 0;
  TCC1.PER = (int)(F_CPU / 1000); // once per millisecond, 1kHz
  TCC1.INTCTRLA = TC_OVFINTLVL_LO_gc;

}
