#include <Wire.h>
#include "Adafruit_MPR121.h"

// Put n = number of capacitors here, n > 1.
#define NUMCAPS 3
// This code assumes that you are using a dielectric the same size of the capacitors,
// i.e. (len)/n (for linear sliders) or (degrees)/n (for rotary encoders).

// Set type of capacitive input device (0 = linear, 1 for rotary)
#define TYPE 1

Adafruit_MPR121 cap = Adafruit_MPR121();
// Stores min, max, & current capacitance.
int channelMinMax[NUMCAPS * 3];
double channelRatios[NUMCAPS];
double segmentLen;

void setup() {
  while (!Serial);        // needed to keep leonardo/micro from starting too fast!
  Serial.begin(9600);
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while(1);
  }
  Serial.println("MPR121 found.");
  for (int i = 0; i < NUMCAPS * 3; i+=3) {
    channelMinMax[i] = 9999; // Some large number.
    channelMinMax[i + 1] = 0;
    channelMinMax[i + 2] = 0;
  }
  
  // Set the segment lengths
  if (TYPE == 0) { // linear
    segmentLen = 100.0 / (NUMCAPS - 1);
  } else { // rotary
    segmentLen = 360.0 / NUMCAPS;
  }
  
  // Calibration time (2s) - move slider/knob as much as possible!
  unsigned long setupStart = millis();
  Serial.println("Starting calibration (2 seconds). Move the slider or knob as much as possible!");
  while(millis() - setupStart < 2000) {
    for (int i = 0; i < NUMCAPS; i++ ) {
      readAndCalibrate(i);
    }
  }
  Serial.println("Finished calibration.");
}

void loop() {
  // Reading & Calibration
  
  for (int i = 0; i < NUMCAPS; i++ ) {
    readAndCalibrate(i);
    

    channelRatios[i] = getRatio(i);
    
      // Debugging (Prints each channel's min, current, max)
//    Serial.print("Channel ");
//    Serial.print( i );
//    Serial.print(" : ");
//    Serial.print(channelMinMax[i * 3]);
//    Serial.print(" / ");
//    Serial.print(channelMinMax[i * 3 + 1]);
//    Serial.print(" / ");
//    Serial.print(channelMinMax[i * 3 + 2]);
//    Serial.print("  ");
//    Serial.println();

    // Debugging (Prints each channel's ratio)
    Serial.print("Ratio ");
    Serial.print( i );
    Serial.print(" : ");
    Serial.print(channelRatios[i]);
    Serial.println();
  }
  
  // Finding Location
  
  // Get two highest ratios.
  int largestIndex = -1;
  double largestValue = -1;
  int secondIndex = -1;
  double secondValue = -1;
  for (int i = 0; i < NUMCAPS; i++) {
    if (channelRatios[i] > secondValue) {
      if (channelRatios[i] > largestValue) {
        secondIndex = largestIndex;
        secondValue = largestValue;
        largestIndex = i;
        largestValue = channelRatios[i];
      } else {
        secondIndex = i;
        secondValue = channelRatios[i];
      }
    }
  }
  
  // Use locations to get index.
  double loc;
  if (TYPE == 0) { // linear
    if (largestIndex < secondIndex) { //adjust from above largest index.
      double center = segmentLen * (largestIndex + 0.5);
      loc = center - largestValue / (largestValue + secondValue) * (segmentLen / 2);
    } else { //adjust from below largest index.
      double center = segmentLen * (largestIndex - 0.5);
      loc = center + largestValue / (largestValue + secondValue) * (segmentLen / 2);
    }
  } else { // rotary
    if (largestIndex == 0 && secondIndex == NUMCAPS) {
      // Edge case, between capacitor 0 & last (closer to 0)
      double center = 0;
      loc = center + largestValue / (largestValue + secondValue) * (segmentLen / 2);
    } else if (secondIndex == 0 && largestIndex == NUMCAPS) {
      // Edge case, between capacitor 0 & last (closer to last)
      double center = 0;
      loc = center - largestValue / (largestValue + secondValue) * (segmentLen / 2);
    } else if (largestIndex < secondIndex) { //adjust from in front of largest index.
      double center = segmentLen * (largestIndex + 1);
      loc = center - largestValue / (largestValue + secondValue) * (segmentLen / 2);
    } else { //adjust from below largest index.
      double center = segmentLen * (largestIndex);
      loc = center + largestValue / (largestValue + secondValue) * (segmentLen / 2);
    }
  }
  
  Serial.print("Detected at location: ");
  Serial.print(loc);
  if (TYPE == 0) { // linear
    Serial.print(" out of 100");
  } else { // rotary
    Serial.print(" degrees");
  }
  Serial.println(); 
  
  // One reading every half second.
  delay(500);
}

void readAndCalibrate(int i) {
  int curData = cap.filteredData(i);
  if (curData != 0) {
    if (channelMinMax[i * 3] > curData) { // new Min
      channelMinMax[i * 3] = curData;
    } else if (channelMinMax[i * 3 + 2] < curData) { // new Max
      channelMinMax[i * 3 + 2] = curData;
    }
  }
  channelMinMax[i * 3 + 1] = curData;
}

double getRatio(int i) {
  // Ratio is (cur - min) / (max - min)
  double diffMaxMin = (channelMinMax[i * 3 + 2] - channelMinMax[i * 3]);
  if (diffMaxMin == 0) { // don't divide by zero.
    diffMaxMin + 1;
  }
  double ratio = double(channelMinMax[i * 3 + 1] - channelMinMax[i * 3]) / diffMaxMin;
  return 1 - ratio; // Invert, so higher number when blocked.
}
