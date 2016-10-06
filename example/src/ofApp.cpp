#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  
  // setup camera
#ifndef USE_USB_CAMERA
  videoFeed.load(videoSource);
  videoFeed.play();
#endif
#ifdef USE_USB_CAMERA
  videoFeed.setDesiredFrameRate(60);
  videoFeed.initGrabber(640, 480);
#endif
  
  // setup gui
  gui.setup();
  
  // interaction constants
  gui.add(centerX.setup("Center X", videoFeed.getWidth()/2, 0, videoFeed.getWidth()));
  gui.add(centerY.setup("Center Y", videoFeed.getHeight()/2, 0, videoFeed.getHeight()));
  gui.add(samplingDensity.setup("Sampling density", 2, 1, 10));
  gui.add(flowThreshold.setup("Flow threshold", 15, 0, 30));
  gui.add(cwFlowConstant.setup("CW velocity", 0.7, 0.0, 2.0));
  gui.add(cwAcceleration.setup("CW de-acceleration", 0.15, 0.0, 2.0));
  gui.add(ccwFlowConstant.setup("CCW velocity", 0.5, 0.0, 2.0));
  gui.add(ccwAcceleration.setup("CCW de-acceleration", 0.18, 0.0, 2.0));
  
  // optical flow constants
  gui.add(pyramidScale.setup("Pyramid scale", 0.35, 0.0, .99));
  pyramidScale.addListener(this, &ofApp::setPyramidScale);
  
  gui.add(pyramidLevels.setup("Pyramid levels", 5, 1, 10));
  pyramidLevels.addListener(this, &ofApp::setPyramidLevels);
  
  gui.add(windowSize.setup("Window size", 10, 1, 10));
  windowSize.addListener(this, &ofApp::setWindowSize);
  
  gui.add(iterationsPerLevel.setup("Iterations per level", 1, 1, 10));
  iterationsPerLevel.addListener(this, &ofApp::setIterationsPerLevel);
  
  gui.add(expansionArea.setup("Expansion area", 3, 1, 10));
  expansionArea.addListener(this, &ofApp::setExpansionArea);
  
  gui.add(expansionSigma.setup("Expansion sigma", 2.25, 0.0, 10.0));
  expansionSigma.addListener(this, &ofApp::setExpansionSigma);
  
  gui.loadFromFile("settings.xml");
  
  // setup optical flow
  opticalFlow.setup(
                    2*interactionRadius,
                    2*interactionRadius,
                    pyramidScale,
                    pyramidLevels,
                    windowSize,
                    iterationsPerLevel,
                    expansionArea,
                    expansionSigma,
                    false,
                    false
                    );
}

//--------------------------------------------------------------
void ofApp::update() {
  videoFeed.update();
  if(videoFeed.isFrameNew()) {
    ofPixels feed = videoFeed.getPixels();
    
    // grab feed before crop
    videoFrameColorDisplayed.setFromPixels(feed);
    
    // crop feed
    feed.crop(centerX-interactionRadius,
              centerY-interactionRadius,
              2*interactionRadius,
              2*interactionRadius);
    
    // grab frame from cropped feed
    videoFrameColor.setFromPixels(feed);
    
    // set to gray
    videoFrameGrayscale = videoFrameColor;
    
    // handle background subtraction if enabled
    if (!backgroundLearned && backgroundSubtraction){
      videoBackground = videoFrameGrayscale; // Grab frame for background image
      backgroundLearned = true;
    } else if (backgroundSubtraction) {
      videoFrameGrayscale.absDiff(videoBackground);
    }
    
    // update optical flow
    opticalFlow.update(videoFrameGrayscale);
    
    double count = 0.0;
    double cwFlowCount= 0.0;
    double ccwFlowCount = 0.0;
    double cwFlowSum = 0.0;
    double ccwFlowSum = 0.0;
    
    for (int x = 0; x < interactionRadius; x += samplingDensity){
      for (int y = -interactionRadius; y < interactionRadius; y += samplingDensity){
        // compute pixel position in relation to center of interaction area
        int x0 = centerX + x;
        int y0 = centerY + y;
        
        // get flow
        ofVec2f flow = opticalFlow.getVelAtPixel(x0, y0);
        if (flow.length() > flowThreshold){
          // get velocity
          float rotationalVelocity = rotationCalculator.getRotationalVelocity(flow, x, y);
        
          if(rotationalVelocity < 0){
            cwFlowSum += abs(rotationalVelocity);
            cwFlowCount += 1.0;
          } else if (rotationalVelocity > 0){
            ccwFlowSum += abs(rotationalVelocity);
            ccwFlowCount += 1.0;
          }
          count += 1.0;
    
          // now average flow in both directions and translate to velocity
          if (count > 0.0){
            double flowCountRatio = (cwFlowCount / ccwFlowCount + 1);
            if (flowCountRatio >= .333) {
              cwVelocity = max(cwVelocity, cwFlowConstant * cwFlowSum / count);
            }
            if (flowCountRatio <= 3) {
              ccwVelocity = max(ccwVelocity, ccwFlowConstant * ccwFlowSum / count);
            }
          } // wont divide by 0
        } // checking first treshold
      } // y foor loop
    } // x for loop
  } // frame new
  
  // Update velocity
  cwVelocity = max(0.0, cwVelocity - cwAcceleration);
  ccwVelocity = max(0.0, ccwVelocity - ccwAcceleration);
  
  // Update total rotation
  cwRotation += cwVelocity;
  ccwRotation += ccwVelocity;
}

//--------------------------------------------------------------
void ofApp::draw() {
  ofSetColor(255);
  ofBackground(0);
  
  // draw video feed
  videoFrameColorDisplayed.draw(0, 0, videoFrameColorDisplayed.getWidth(), videoFrameColorDisplayed.getHeight());
  
  // draw crosshairs and bounding box
  // cross hairs
  ofDrawLine(0, centerY, videoFeed.getWidth(), centerY);
  ofDrawLine(centerX, 0, centerX, videoFeed.getHeight());
  // bouding box
  // left line
  ofDrawLine(centerX, centerY-interactionRadius, centerX, centerY+interactionRadius);
  // right line
  ofDrawLine(centerX+interactionRadius, centerY-interactionRadius, centerX+interactionRadius, centerY+interactionRadius);
  // top line
  ofDrawLine(centerX, centerY-interactionRadius, centerX+interactionRadius, centerY-interactionRadius);
  // bottom line
  ofDrawLine(centerX, centerY+interactionRadius, centerX+interactionRadius, centerY+interactionRadius);
  
  // Draw outer circle
  ofPushMatrix();
  ofTranslate(centerX+1.5*interactionRadius, centerY);
  ofSetColor(255, 255, 255);
  ofFill();
  ofRotate(-ccwRotation);
  ofDrawEllipse(0, -60, 5, 5);
  ofSetColor(100, 255, 100);
  ofFill();
  ofDrawEllipse(0, 0, 120, 120);
  ofPopMatrix();
  
  // Draw inner circle
  ofPushMatrix();
  ofTranslate(centerX+1.5*interactionRadius, centerY);
  ofSetColor(0, 0, 255);
  ofFill();
  ofRotate(cwRotation);
  ofDrawEllipse(0, -40, 5, 5);
  ofSetColor(255, 100, 100);
  ofFill();
  ofDrawEllipse(0, 0, 80, 80);
  ofPopMatrix();
  
  ofSetColor(255);
  
  // Guis
  gui.draw();
}

//--------------------------------------------------------------
void ofApp::saveSettings(){
  gui.saveToFile("settings.xml");
}

//--------------------------------------------------------------
//* STREAM *//
double ofApp::getCwRotation() {
  return (int)cwRotation % 360;
}

double ofApp::getCcwRotation() {
  return (int)ccwRotation % 360;
}

double ofApp::getCwVelocity() {
  return cwVelocity;
}

double ofApp::getCcwVelocity() {
  return ccwVelocity;
}

//--------------------------------------------------------------
//* VIDEO *//
void ofApp::grabNewBackground() {
  backgroundLearned = false;
}

//--------------------------------------------------------------
//*  OPTICAL FLOW SETTERS *//
void ofApp::setPyramidScale(float &scale){
  opticalFlow.setPyramidScale((double)scale);
}

void ofApp::setPyramidLevels(int &levels){
  opticalFlow.setPyramidLevels(levels);
}

void ofApp::setWindowSize(int &size){
  opticalFlow.setWindowSize(size);
}

void ofApp::setIterationsPerLevel(int &iterations){
  opticalFlow.setIterationsPerLevel(iterations);
}

void ofApp::setExpansionArea(int &area){
  opticalFlow.setExpansionArea(area);
}

void ofApp::setExpansionSigma(float &sigma){
  opticalFlow.setExpansionSigma((double)sigma);
}

//--------------------------------------------------------------
