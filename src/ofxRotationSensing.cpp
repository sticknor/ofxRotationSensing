//
//  ofxRotationSensing.cpp
//  ofxRotationSensing
//
//  Created by Sam Ticknor on 8/10/16.
//
//

#include "ofxRotationSensing.h"

float ofxRotationSensing::getRotationalVelocity(ofVec2f flowVector, int flowX, int flowY){
  
  // First, find the tangential unit vector of the point (flowX, flowY)
  // This math is simplified as:
  // x = cos(arctan2(y/x) + 90deg) = -y/r
  // y = sin(arctan2(y/x) + 90deg) = x/r
  
  // so find r
  float flowRadius;
  flowRadius = pow((pow(flowX-centerX, 2)+pow(flowY-centerY, 2)), .5);

  // etc..
  if (flowRadius != 0){
    double uvx = flowY/flowRadius;
    double uvy = -flowX/flowRadius;
    
    // This is
    // the tangential unit vector,
    // going clockwise,
    // at (flowX, flowY),
    // on the circle centered at (centerX, centerY)
    ofVec2f unitVector(uvx, uvy);
    
    // now use vector projection to apply the flow velocity to the unit vector
    float rotationalVelocity;
    float scalar = flowVector.dot(unitVector.getNormalized()); // just a precaution ;)
    if (scalar < 0){
      rotationalVelocity = -unitVector.scale(scalar).length();
    } else {
      rotationalVelocity = unitVector.scale(scalar).length();
    }
    
    return rotationalVelocity;
  }
  
  return 0.0;
}
