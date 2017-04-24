#pragma once

#include "ofMain.h"
#include "ofxPatches.h"
#include "ofxKinect.h"
#include "ofxGui.h"
#include "ofxWarpBlendTool.h"

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    void onKinectAngleChange(int & value);
    void onGridChange(int & value);
    void exit();
    
    ofxPanel gui;
    ofEasyCam cam;
    ofRectangle camViewport;
    //ofCamera cam;
    ofMesh grid;
    ofFbo gridImage;
    ofFbo gridImageNormalized;
    ofxKinect kinect;
    ofxPatches::Manager preProcessing;
    ofxPatches::Manager postProcessing;
    ofFbo reducer;
    ofPixels pixels;
    ofFbo canvas;
    ofxWarpBlendTool::Controller warpBlendTool;
    
    
    int columns;
    int rows;
    
    int debugModeEnabled;
};
