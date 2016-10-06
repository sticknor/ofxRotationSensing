//
//  ofxRotationSensing.h
//  ofxRotationSensing
//
//  Created by Sam Ticknor on 8/10/16.
//
//

#pragma once

#include "ofMain.h"

class ofxRotationSensing : public ofBaseApp {
  
public:
  int centerX = 0;
  int centerY = 0;
    
  float getRotationalVelocity(ofVec2f flowVector, int x, int y);
};
