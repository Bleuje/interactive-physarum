// by Etienne Jacob, see license and crediting in main.cpp

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(FRAME_RATE);

    ofEnableAntiAliasing();

    u = float(ofGetHeight())/1080;

    myFont.load("fonts/Raleway-Regular.ttf",floor(22.0 * u));
    myFontBold.load("fonts/Raleway-Bold.ttf",floor(22.0 * u));

    gamepadControlsImage.load("images/xboxgamepadcontrols.png");
    informationImage.load("images/interactive-physarum-info.png");

    counter.resize(SIMULATION_WIDTH*SIMULATION_HEIGHT);
    counterBuffer.allocate(counter, GL_DYNAMIC_DRAW);

    trailReadBuffer.allocate(SIMULATION_WIDTH, SIMULATION_HEIGHT, GL_RG16F);
    trailWriteBuffer.allocate(SIMULATION_WIDTH, SIMULATION_HEIGHT, GL_RG16F);
    fboDisplay.allocate(SIMULATION_WIDTH, SIMULATION_HEIGHT, GL_RGBA8);

    setterShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_setter.glsl");
    setterShader.linkProgram();

    depositShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_deposit.glsl");
    depositShader.linkProgram();

    moveShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_move.glsl");
    moveShader.linkProgram();

    blurShader.setupShaderFromFile(GL_COMPUTE_SHADER,"shaders/computeshader_blur.glsl");
    blurShader.linkProgram();

    particles.resize(NUMBER_OF_PARTICLES * PARTICLE_PARAMETERS_COUNT);
    float marginx = 3;
    float marginy = 3;

    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        particles[PARTICLE_PARAMETERS_COUNT * i + 0] = floatAsUint16(ofRandom(marginx, SIMULATION_WIDTH - marginx) / SIMULATION_WIDTH);
        particles[PARTICLE_PARAMETERS_COUNT * i + 1] = floatAsUint16(ofRandom(marginy, SIMULATION_HEIGHT - marginy) / SIMULATION_HEIGHT);
        particles[PARTICLE_PARAMETERS_COUNT * i + 2] = floatAsUint16(ofRandom(1));
        particles[PARTICLE_PARAMETERS_COUNT * i + 3] = floatAsUint16(ofRandom(1));
        particles[PARTICLE_PARAMETERS_COUNT * i + 4] = 0;
        particles[PARTICLE_PARAMETERS_COUNT * i + 5] = 0;
    }
    particlesBuffer.allocate(particles,GL_DYNAMIC_DRAW);

    simulationParameters.resize(NUMBER_OF_USED_POINTS);
    simulationParametersBuffer.allocate(simulationParameters,GL_DYNAMIC_DRAW);
    simulationParametersBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

    trailReadBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);
    particlesBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    counterBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    fboDisplay.getTexture().bindAsImage(4,GL_WRITE_ONLY);

    for(int i=0;i<MAX_NUMBER_OF_WAVES;i++)
    {
        waveXarray[i] = SIMULATION_WIDTH/2;
        waveYarray[i] = SIMULATION_HEIGHT/2;
        waveTriggerTimes[i] = -12345;
        waveSavedSigmas[i] = 0.5;
    }

    for(int i=0;i<MAX_NUMBER_OF_RANDOM_SPAWN;i++)
    {
        randomSpawnXarray[i] = SIMULATION_WIDTH/2;
        randomSpawnYarray[i] = SIMULATION_HEIGHT/2;
    }

    ////////////////////////////////////////
    // check if there is a gamepad connected
    numberOfGamepads = ofxGamepadHandler::get()->getNumPads();

    if(numberOfGamepads>0){
        for(int i=0;i<numberOfGamepads;i++)
        {
            ofxGamepad* pad = ofxGamepadHandler::get()->getGamepad(i);
            ofAddListener(pad->onAxisChanged, this, &ofApp::axisChanged);
            ofAddListener(pad->onButtonPressed, this, &ofApp::buttonPressed);
            ofAddListener(pad->onButtonReleased, this, &ofApp::buttonReleased);
        }
    }
	std::cout << "Number of gamepads : " << numberOfGamepads << std::endl;
    ////////////////////////////////////////


    std::cout << "Number of points : " << pointsDataManager.getNumberOfPoints() << std::endl;

    paramsUpdate();
}

void ofApp::paramsUpdate()
{
    pointsDataManager.updateCurrentValuesFromTransitionProgress(currentTransitionProgress());

    for(int k=0;k<NUMBER_OF_USED_POINTS;k++)
    {
        simulationParameters[k] = pointsDataManager.getPointsParamsFromArray(pointsDataManager.currentPointValues[k]);
    }

    simulationParametersBuffer.updateData(simulationParameters);
}

//--------------------------------------------------------------
void ofApp::update(){
    float time = getTime();

    paramsUpdate();
    updateActionAreaSizeSigma();

    if((getTime() - latestPointSettingsActionTime) >= SETTINGS_DISAPPEAR_DURATION)
    {
        settingsChangeMode = 0;
    }
    if(settingsChangeMode == 1)
    {
        settingsChangeIndex = std::min(SETTINGS_SIZE-1,settingsChangeIndex);
    }
    if(settingsChangeMode == 2)
    {
        settingsChangeIndex = std::min(ACTION_VALUES_SIZE-1,settingsChangeIndex);
    }

    if(numberOfGamepads == 0)
    {
        curActionX = ofMap(ofGetMouseX(), 0, ofGetWidth(), 0, SIMULATION_WIDTH, true);
        curActionY = ofMap(ofGetMouseY(), 0, ofGetHeight(), 0, SIMULATION_HEIGHT, true);
    }
    else
    {
        curActionX += curTranslationAxis1*translationStep;
        curActionY += curTranslationAxis2*translationStep;

        if(LOOP_PEN_POSITION)
        {
            curActionX = fmod(curActionX + SIMULATION_WIDTH, SIMULATION_WIDTH);
            curActionY = fmod(curActionY + SIMULATION_HEIGHT, SIMULATION_HEIGHT);
        }
        else
        {
            curActionX = ofClamp(curActionX, 0, SIMULATION_WIDTH);
            curActionY = ofClamp(curActionY, 0, SIMULATION_HEIGHT);
        }
    }

    if(numberOfGamepads == 0)
    {
        curL2 = -1; // L2 for no "inertia" effect, when using keyboard only
    }

    setterShader.begin();
    setterShader.setUniform1i("width",trailReadBuffer.getWidth());
    setterShader.setUniform1i("height",trailReadBuffer.getHeight());
    setterShader.setUniform1i("value",0);
    setterShader.dispatchCompute(SIMULATION_WIDTH / 32, SIMULATION_HEIGHT / 32, 1);
    setterShader.end();


    trailReadBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);

    moveShader.begin();
    moveShader.setUniform1i("width",trailReadBuffer.getWidth());
    moveShader.setUniform1i("height",trailReadBuffer.getHeight());
    moveShader.setUniform1f("time",time);

    moveShader.setUniform1f("actionAreaSizeSigma",currentActionAreaSizeSigma);

    moveShader.setUniform1f("actionX",curActionX);
    moveShader.setUniform1f("actionY",curActionY);

    moveShader.setUniform1f("moveBiasActionX",curMoveBiasActionX);
    moveShader.setUniform1f("moveBiasActionY",curMoveBiasActionY);

    moveShader.setUniform1fv("waveXarray", waveXarray.data(), waveXarray.size());
    moveShader.setUniform1fv("waveYarray", waveYarray.data(), waveYarray.size());
    moveShader.setUniform1fv("waveTriggerTimes", waveTriggerTimes.data(), waveTriggerTimes.size());
    moveShader.setUniform1fv("waveSavedSigmas", waveSavedSigmas.data(), waveSavedSigmas.size());

    moveShader.setUniform1f("mouseXchange",1.0*ofGetMouseX()/ofGetWidth());
    moveShader.setUniform1f("L2Action",ofMap(curL2,-1,1,0,1.0,true));

    moveShader.setUniform1i("spawnParticles", int(particlesSpawn));
    moveShader.setUniform1f("spawnFraction",SPAWN_FRACTION);
    moveShader.setUniform1i("randomSpawnNumber",randomSpawnNumber);
    moveShader.setUniform1fv("randomSpawnXarray", randomSpawnXarray.data(), randomSpawnXarray.size());
    moveShader.setUniform1fv("randomSpawnYarray", randomSpawnYarray.data(), randomSpawnYarray.size());

    moveShader.setUniform1f("pixelScaleFactor",PIXEL_SCALE_FACTOR);

    moveShader.dispatchCompute(particles.size() / (128 * PARTICLE_PARAMETERS_COUNT), 1, 1);
    moveShader.end();



    depositShader.begin();
    depositShader.setUniform1i("width",trailReadBuffer.getWidth());
    depositShader.setUniform1i("height",trailReadBuffer.getHeight());
    depositShader.setUniform1f("depositFactor",DEPOSIT_FACTOR);
    depositShader.setUniform1i("colorModeType",colorModeType);
    depositShader.setUniform1i("numberOfColorModes",NUMBER_OF_COLOR_MODES);
    depositShader.dispatchCompute(SIMULATION_WIDTH / 32, SIMULATION_HEIGHT / 32, 1);
    depositShader.end();

    trailReadBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);

    blurShader.begin();
    blurShader.setUniform1i("width",trailReadBuffer.getWidth());
    blurShader.setUniform1i("height",trailReadBuffer.getHeight());
    blurShader.setUniform1f("PI",PI);
    blurShader.setUniform1f("decayFactor",DECAY_FACTOR);
    blurShader.setUniform1f("time",time);
    blurShader.dispatchCompute(trailReadBuffer.getWidth()/32,trailReadBuffer.getHeight()/32,1);
    blurShader.end();


    if(particlesSpawn) particlesSpawn = 0;

    std::stringstream strm;
    strm << "fps: " << ofGetFrameRate();
    ofSetWindowTitle(strm.str());
}

// DRAW

void ofApp::draw(){
    u = float(ofGetHeight())/1080;

    float R2action = ofMap(curR2,-1,0.3,0,1,true);
    if(numberOfGamepads==0) R2action = 0;

    ofPushMatrix();

    ofPushMatrix();
    ofScale(1.0*ofGetWidth()/fboDisplay.getWidth(),1.0*ofGetHeight()/fboDisplay.getHeight());
    fboDisplay.draw(0,0);
    ofPopMatrix();

    // draw circle
    if(displayType==1)
    {
        ofPushMatrix();
        
        float time2 = getTime()*6;

        float R = currentActionAreaSizeSigma*600*(1.0 + 0.08*sin(0.4f*time2));

        float cx = ofMap(curActionX,0,SIMULATION_WIDTH,0,ofGetWidth());
        float cy = ofMap(curActionY,0,SIMULATION_HEIGHT,0,ofGetHeight());

        ofSetCircleResolution(100);

        drawCustomCircle(ofVec2f(cx,cy),R,9);
        
        ofPopMatrix();
    }

    ofFill();

    ofPushMatrix();

    float col = 0;

    for(int setIndex=0;setIndex<NUMBER_OF_USED_POINTS;setIndex++)
    {
        ofPushMatrix();

        ofPushMatrix();
        ofTranslate(53*u,65*u);
        ofScale(1.3);
        drawPad(100,255);
        ofScale(0.92);
        drawPad(255,255);
        ofPopMatrix();

        ofTranslate(116*u,50*u + 50*setIndex*u);
        std::string prefix = setIndex==0 ? "pen: " : "background: ";
        std::string setString = prefix + pointsDataManager.getPointName(setIndex)
        + (setIndex==pointsDataManager.getSelectionIndex() ? " <" : "");

        ofTrueTypeFont * pBoldOrNotFont = setIndex==pointsDataManager.getSelectionIndex() ? &myFontBold : &myFont;
        drawTextBox(setString, pBoldOrNotFont, col, 255);

        ofPopMatrix();
    }

    if(settingsChangeMode == 1)
    {
        ofPushMatrix();
        ofTranslate(50*u,180*u);

        std::string pointName = pointsDataManager.getPointName(pointsDataManager.getSelectionIndex()) + " settings tuning:";
        drawTextBox(pointName, &myFont, col, 255);


        ofScale(0.8);

        ofTranslate(0,25*u);

        for(int i=0;i<SETTINGS_SIZE;i++)
        {
            ofTranslate(0,44*u);

            ofTrueTypeFont * pBoldOrNotFont = i==settingsChangeIndex ? &myFontBold : &myFont;

            std::string settingValueString = pointsDataManager.getSettingName(i) + " : "
                + roundedString(pointsDataManager.getValue(i))
                + (i==settingsChangeIndex ? " <" : "");;

            drawTextBox(settingValueString, pBoldOrNotFont, col, 110);
        }


        ofTranslate(0,80*u);
        std::string pressA = "Press A to reset " + pointsDataManager.getPointName(pointsDataManager.getSelectionIndex()) + " settings";
        drawTextBox(pressA, &myFontBold, col, 110);


        ofTranslate(0,44*u);
        std::string pressB = "Press B to reset settings of all points";
        drawTextBox(pressB, &myFontBold, col, 110);

        ofPopMatrix();
    }

    if(settingsChangeMode == 2)
    {
        ofPushMatrix();
        ofTranslate(50*u,180*u);

        std::string pointName = "Global advanced settings tuning:";
        drawTextBox(pointName, &myFont, col, 255);


        ofScale(0.8);

        ofTranslate(0,25*u);

        for(int i=0;i<ACTION_VALUES_SIZE;i++)
        {
            ofTranslate(0,44*u);

            ofTrueTypeFont * pBoldOrNotFont = i==settingsChangeIndex ? &myFontBold : &myFont;

            std::string settingValueString = "Value " + std::to_string(i+1) + " : "
                + std::to_string(actionValuesArray[i])
                + (i==settingsChangeIndex ? " <" : "");;

            drawTextBox(settingValueString, pBoldOrNotFont, col, 110);
        }


        ofTranslate(0,80*u);
        std::string pressA = "Press A to reset selected setting";
        drawTextBox(pressA, &myFontBold, col, 110);


        ofTranslate(0,44*u);
        std::string pressB = "Press B to reset all settings";
        drawTextBox(pressB, &myFontBold, col, 110);

        ofPopMatrix();
    }

    ofPushMatrix();
    ofTranslate(u*9,ofGetHeight() - 9*u);
    ofScale(0.7);
    std::string creditString = "Inspiration and parameters from mxsage's \"36 points\"";

    ofPushMatrix();
    ofSetColor(col,150);
    ofTranslate(-10*u,-32*u);
    ofDrawRectangle(0,0,20*u + myFont.stringWidth(creditString),41*u);
    ofPopMatrix();

    ofSetColor(255-col);
    ofPushMatrix();
    myFont.drawString(creditString,0,0);
    ofPopMatrix();

    ofPopMatrix();

    ofPopMatrix();


/*
    // Saving frames
    if(ofGetFrameNum()<numFrames){
        std::ostringstream str;
        int num = ofGetFrameNum();
        std::cout << "Saving frame " << num << "\n" << std::flush;
        str << std::setw(4) << std::setfill('0') << num;
        ofSaveScreen("frames/fr"+str.str()+".png");
    }
*/
    float infoProgress = 1-pow(1-R2action,2.0);

    ofPushMatrix();
    ofTranslate(-25*u - 1500*u*(1-infoProgress),230*u);
    ofScale(1.5*u);
    gamepadControlsImage.draw(0, 0);
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(1270*u + 1000*u*(1-infoProgress),230*u);
    ofScale(0.7*u);
    informationImage.draw(0, 0);
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(1300*u,25*u - 50*u*infoProgress);
    ofScale(0.7*u);
    std::string pressB = "Right trigger for controls and information";
    drawTextBox(pressB, &myFontBold, col, 110);
    ofPopMatrix();


    ofPopMatrix();
}


// OTHER

void ofApp::updateActionAreaSizeSigma()
{
    float target = ofMap(sigmaCount,0,sigmaCountModulo,0.15,maxActionSize);
    float lerper = pow(ofMap(getTime() - latestSigmaChangeTime, 0, ACTION_SIGMA_CHANGE_DURATION, 0, 1, true),1.7);
    currentActionAreaSizeSigma = ofLerp(currentActionAreaSizeSigma, target, lerper);
}

void ofApp::setRandomSpawn()
{
    //randomSpawnNumber = floor(ofRandom(MAX_NUMBER_OF_RANDOM_SPAWN/2,MAX_NUMBER_OF_RANDOM_SPAWN));
    randomSpawnNumber = MAX_NUMBER_OF_RANDOM_SPAWN;

    for(int i=0;i<randomSpawnNumber;i++)
    {
        float theta = ofRandom(0,TWO_PI);
        float r = pow(ofRandom(0,1),0.5);
        float x = r*cos(theta);
        float y = r*sin(theta);
        randomSpawnXarray[i] = x;
        randomSpawnYarray[i] = y;
    }
}

float ofApp::getTime()
{
    return 1.0*ofGetFrameNum()/FRAME_RATE;
}

float ofApp::currentTransitionProgress()
{
    return ofMap(getTime() - transitionTriggerTime, 0, TRANSITION_DURATION, 0., 1., true);
}

bool ofApp::activeTransition()
{
    return (getTime() - transitionTriggerTime) <= TRANSITION_DURATION;
}



//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
