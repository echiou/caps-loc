#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "Adafruit_MPR121.h"

// Put n = number of capacitors here, n > 1.
#define NUMCAPS 3
// This code assumes that you are using a dielectric the same size of the capacitors,

// Put m = number of neopixels here.
#define NUMNEOPIXELS 41
#define NEOPIXELPIN 6

Adafruit_MPR121 cap = Adafruit_MPR121();
// Stores min, max, & current capacitance.
int channelMinMax[NUMCAPS * 3];
double channelRatios[NUMCAPS];
double segmentLen;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMNEOPIXELS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  while (!Serial); // needed to keep leonardo/micro from starting too fast!
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
  segmentLen = 100.0 / (NUMCAPS - 1);
  
  // Calibration time (2s)
  unsigned long setupStart = millis();
  Serial.println("Starting calibration (2 seconds). Move the slider or knob as much as possible!");
  while(millis() - setupStart < 2000) {
    for (int i = 0; i < NUMCAPS; i++ ) {
      readAndCalibrate(i);
    }
  }
  Serial.println("Finished calibration.");
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Reading & Calibration
  
  for (int i = 0; i < NUMCAPS; i++ ) {
    readAndCalibrate(i);
    
    channelRatios[i] = getRatio(i);
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
  if (largestIndex < secondIndex) { //adjust from above largest index.
    double center = segmentLen * (largestIndex + 0.5);
    loc = center - (largestValue - secondValue) / largestValue * (segmentLen / 2);
  } else { //adjust from below largest index.
    double center = segmentLen * (largestIndex - 0.5);
    loc = center + (largestValue - secondValue) / largestValue * (segmentLen / 2);
  }
  
//  Serial.print("Detected at location: ");
//  Serial.print(loc);
//  Serial.print(" out of 100");
//  Serial.println();
  
  singlePositionWhite(loc, 3);
  strip.show();
  
  // One reading every 0.1 second.
  delay(50);
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

// Parameter 1 pos = position of slider (double out of 100).
// Parameter 2 numPixels = number of pixels on either side to turn on (faded).
// Parameter 3 color = color of pixel.
void singlePositionWhite(double pos, int numPixels) {
  int posScaled = (int) (pos / 100 * NUMNEOPIXELS + 0.5);
 
  // Debugging (lights on) 
//  Serial.print("Lighting up pixels ");
//  Serial.print(posScaled - numPixels);
//  Serial.print(" through ");
//  Serial.print(posScaled + numPixels);
//  Serial.println();
  
  for (int i=0; i < NUMNEOPIXELS; i++) {
    if (i >= posScaled - numPixels && i <= posScaled + numPixels){
      int distance = i - posScaled;
      double ratio = (double)((numPixels - abs(distance)) + 1) / (numPixels + 1);
      int brightness = (int)(pow(ratio, 2) * 255);
      strip.setPixelColor(i, strip.Color(brightness, brightness, brightness));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
}

