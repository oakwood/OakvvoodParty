#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetVerticalSync(true);
    ofSetWindowShape(1280, 800);
    debugModeEnabled = false;
    ofEnableAlphaBlending();
    
    
    // Setup Kinect
    kinect.setRegistration(true);
    kinect.init(true);
    kinect.open();
    kinect.setLed(ofxKinect::LED_OFF);
    
   
    // Setup Patches
    preProcessing.setup(2, "Pre Processing");
    preProcessing.allocate(kinect.width, kinect.height);
    OFX_PATCHES_REGISTER_ALL_EFFECTS(preProcessing);
    preProcessing.loadSettings();
    
    postProcessing.setup(2, "Post Processing");
    postProcessing.allocate(kinect.width * 2, kinect.height * 2);
    OFX_PATCHES_REGISTER_ALL_EFFECTS(postProcessing);
    postProcessing.loadSettings();
    
    // Setup canvas and warp blend tool
    canvas.allocate(1280, 800);
    warpBlendTool.setup(&(canvas.getTexture()));
    camViewport.set(0, 0, canvas.getWidth(), canvas.getHeight());
    
    // Setup gui and load settings
    gui.setup("Settings", "settings.xml");
    gui.setPosition(250, 0);
        
    ofxIntSlider * kinectAngle = new ofxIntSlider();
    kinectAngle->setup("Kinect Angle", 0, -30, 30);
    kinectAngle->addListener(this, &ofApp::onKinectAngleChange);
    gui.add(kinectAngle);
        
    ofxIntSlider * gridDivisions = new ofxIntSlider();
    gridDivisions->setup("Grid Divisions", 10, 3, 20);
    gridDivisions->addListener(this, &ofApp::onGridChange);
    gui.add(gridDivisions);
        
    ofxIntSlider * gridThickness = new ofxIntSlider();
    gridThickness->setup("Grid Thickness", 1, 1, 10);
    gridThickness->addListener(this, &ofApp::onGridChange);
    gui.add(gridThickness);
    
    ofxFloatSlider * cameraScale = new ofxFloatSlider();
    cameraScale->setup("Camera Scale", 1, 0.5, 5);
    gui.add(cameraScale);
        
    ofxToggle * drawWireframe = new ofxToggle();
    drawWireframe->setup("Draw Wireframe", false);
    gui.add(drawWireframe);
    
    ofxToggle * editTextures = new ofxToggle();
    editTextures->setup("Edit Textures", false);
    gui.add(editTextures);
    
    ofxToggle * editWarpBlendTool = new ofxToggle();
    editWarpBlendTool->setup("Edit Warp Blend Tool", false);
    gui.add(editWarpBlendTool);
        
    gui.loadFromFile("settings.xml");
    
    // kickoff grid
    int i = 1;
    onGridChange(i);
    
}

//--------------------------------------------------------------
void ofApp::update(){
    // Provide the kinect data to pre processing
    kinect.update();
    if(kinect.isFrameNew()){
        preProcessing.setTexture(kinect.getTexture(), 0);
        preProcessing.setTexture(kinect.getDepthTexture(), 1);
    }
    preProcessing.update();
    
    // Pipe the pre processing image into the post processing
    postProcessing.setTexture(preProcessing.getTexture(), 1);
    postProcessing.update();
    
    // Draw the post processing image into a normalized fbo, that
    // can be used for binding with the mesh
    gridImageNormalized.begin();
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(postProcessing.getWidth(), postProcessing.getHeight());
    ofScale(-1, -1);
    ofEnableAlphaBlending();
    ofClear(0,0);
    postProcessing.draw(0, 0);
    ofPopStyle();
    ofPopMatrix();
    gridImageNormalized.end();
    
    // Draw the pre processing image into a small fbo, so we can use that
    // data to move the vertices of the mesh
    reducer.begin();
    ofPushMatrix();
    ofTranslate(reducer.getWidth(), reducer.getHeight());
    ofScale(-1, -1);
    preProcessing.getTexture().draw(0, 0, reducer.getWidth(), reducer.getHeight());
    ofPopMatrix();
    reducer.end();
    reducer.getTexture().readToPixels(pixels);
    for(int i = 0; i < grid.getVertices().size(); i++) {
        grid.getVertices()[i].z = (pixels.getData()[i * 4] / 255.0) * 200;
    }
    
    // Now we have everything we need to draw the scene....
    
    // Draw scene inside canvas
    canvas.begin();
    ofClear(0,0,0,0);
    
    // Draw the mesh (wireframe or binded texture), inside
    // the camera
    cam.begin(camViewport);
    ofPushMatrix();
    float cameraScale =gui.getFloatSlider("Camera Scale");
    ofScale(cameraScale,cameraScale,cameraScale);
    if(gui.getToggle("Draw Wireframe")){
        grid.drawWireframe();
    }
    else {
        ofPushMatrix();
        ofPushStyle();
        ofSetColor(255);
        ofEnableDepthTest();
        ofEnableAlphaBlending();
        gridImageNormalized.getTexture().bind();
        grid.draw();
        gridImageNormalized.getTexture().unbind();
        ofDisableDepthTest();
        ofPopStyle();
        ofPopMatrix();
    }
    ofPopMatrix();
    cam.end();
    canvas.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    // Draw the warp tool
    warpBlendTool.draw();

    // Draw the debug information
    if(debugModeEnabled) {
        ofSetBackgroundColor(100);
        ofShowCursor();
        gui.draw();
        if(gui.getToggle("Edit Textures")){
            preProcessing.drawGUI();
            postProcessing.drawGUI();
        }
        else if(gui.getToggle("Edit Warp Blend Tool")){
            warpBlendTool.drawGui();
        }
    }
    else {
        ofSetBackgroundColor(0);
        ofHideCursor();
    }
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 'd' || key == 'D'){
        debugModeEnabled = !debugModeEnabled;
    }
    else if(key == 'f' || key == 'F'){
        ofToggleFullscreen();
    }
}
//--------------------------------------------------------------
void ofApp::onKinectAngleChange(int & value){
    kinect.setCameraTiltAngle(value);
}
//--------------------------------------------------------------
void ofApp::onGridChange(int & value){
    int gridDivisions = gui.getIntSlider("Grid Divisions");
    int gridThickness = gui.getIntSlider("Grid Thickness");
    
    columns = kinect.width / gridDivisions;
    rows = kinect.height / gridDivisions;
    grid = ofMesh::plane(kinect.width, kinect.height, columns, rows);
    
    gridImage.allocate(kinect.width * 2, kinect.height * 2);
    gridImage.begin();
    ofPushStyle();
    
    ofClear(0,0);
    ofSetColor(255);
    ofSetLineWidth(gridThickness);
    float xGap = gridImage.getWidth() / ((float)columns - 1.0);
    float yGap = gridImage.getHeight() / ((float)rows - 1.0);
    for(int x = 1; x < columns - 1; x++){
        float xPos = (float) x * xGap;
        ofDrawLine(xPos, 0, xPos, xPos + gridImage.getWidth());
    }
    for(int y = 1; y < rows - 1; y++){
        float yPos = (float) y * yGap;
        ofDrawLine(0, yPos, yPos + gridImage.getWidth(), yPos);
    }
    ofPopStyle();
    gridImage.end();
    
    ofDisableArbTex();
    gridImageNormalized.allocate(kinect.width * 2, kinect.height * 2);
    ofEnableArbTex();
    
    postProcessing.begin(0);
    gridImage.draw(0, 0);
    postProcessing.end(0);
    
    // Setup Pixel data
    reducer.allocate(columns, rows);
    pixels.allocate(columns, rows, OF_PIXELS_RGBA);
}
//--------------------------------------------------------------
void ofApp::exit() {
    //kinect.setCameraTiltAngle(0);
    kinect.close();
}

