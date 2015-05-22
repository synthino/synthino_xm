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

boolean buttonPressed(byte button) {
  return buttonPressed(button, true);
}

// Return true if the button is pressed.
boolean buttonPressed(byte button, boolean updateButtonState) {
  if (digitalRead(buttonPins[button]) == LOW) {
    // the button is currently pressed
    if ((buttonState[button] == HIGH) && (updateButtonState)) {
      // if the button was not pressed before, update the state.
      buttonState[button] = LOW;
    }
    return true;
  } else {
    // The button is currently not pressed
    if ((buttonState[button] == LOW) && (updateButtonState)) {
      // if the button was pressed before, update the state.
      buttonState[button] = HIGH;
    }
    return false;
  }
}

// Return true if the button is pressed and it is a new press (not held)
boolean buttonPressedNew(byte button) {
  if (digitalRead(buttonPins[button]) == LOW) {
    // The button is currently pressed
    if (buttonState[button] == HIGH) {
      // This is a new press.
      buttonState[button] = LOW;
      return true;
    }
    // This is not a new press.
    return false; 
  } 
  else {
    // The button is currently not pressed
    if (buttonState[button] == LOW) {
      buttonState[button] = HIGH;
    }
    return false;
  }
}

// Return true if the button is pressed and it was pressed the last time the state was changed.
boolean buttonHeld(byte button) {
  if (digitalRead(buttonPins[button]) == LOW) {
    // the button is currently pressed
    if (buttonState[button] == HIGH) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

// Return true if the button is not pressed and it has been released since last check.
boolean buttonReleased(byte button) {
  if (digitalRead(buttonPins[button]) == HIGH) {
    // The button is currently not pressed
    if (buttonReleaseState[button] == LOW) {
      // This is a new release.
      buttonReleaseState[button] = HIGH;
      return true;
    }
    // This is not a new release
    return false; 
  } 
  else {
    // The button is currently pressed
    if (buttonReleaseState[button] == HIGH) {
      buttonReleaseState[button] = LOW;
    }
    return false;
  }
}

int sampledAnalogRead(int pin) {
  int sum = analogRead(pin);
  sum += analogRead(pin);
  return (sum >> 1);
}


