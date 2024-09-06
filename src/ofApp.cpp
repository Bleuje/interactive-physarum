// by Etienne Jacob, see license and crediting in main.cpp

#include "ofApp.h"

uint16_t floatAsUint16(float x)
{
    return uint16_t(std::round(std::max(std::min(x, 1.0f), 0.0f) * 65535.0f));
}

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

    setterShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_setter.glsl");
    setterShader.linkProgram();

    depositShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_deposit.glsl");
    depositShader.linkProgram();

    moveShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_move.glsl");
    moveShader.linkProgram();

    blurShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_blur.glsl");
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




// INTERACTION

void ofApp::actionChangeSigmaCount(int dir)
{
    sigmaCount = (sigmaCount + sigmaCountModulo + dir) % sigmaCountModulo;
    penMoveLatestTime = getTime();
    latestSigmaChangeTime = getTime();
}

void ofApp::actionChangeParams(int dir)
{
    pointsDataManager.changeParamIndex(dir);

    transitionTriggerTime = getTime();
}

void ofApp::actionSwapParams()
{
    pointsDataManager.swapUsedPoints();

    transitionTriggerTime = getTime();
}

void ofApp::actionRandomParams()
{
    pointsDataManager.useRandomIndices();

    transitionTriggerTime = getTime();
}

void ofApp::actionChangeColorMode()
{
    colorModeType = (colorModeType + 1) % NUMBER_OF_COLOR_MODES;
}

void ofApp::actionTriggerWave()
{
    waveXarray[currentWaveIndex] = curActionX;
    waveYarray[currentWaveIndex] = curActionY;
    waveTriggerTimes[currentWaveIndex] = getTime();
    waveSavedSigmas[currentWaveIndex] = currentActionAreaSizeSigma;

    currentWaveIndex = (currentWaveIndex + 1) % MAX_NUMBER_OF_WAVES;

    waveActionAreaSizeSigma = currentActionAreaSizeSigma;


}

void ofApp::actionChangeDisplayType()
{
    displayType = (displayType + 1) % 2;
}

void ofApp::actionChangeSelectionIndex(int dir)
{
    pointsDataManager.changeSelectionIndex(dir);
}

void ofApp::actionSpawnParticles(int spawnType)
{
    particlesSpawn = spawnType;
    if(spawnType == 2)
    {
        setRandomSpawn();
    }
}

void ofApp::buttonPressed(ofxGamepadButtonEvent& e)
{
	//cout << "BUTTON " << e.button << " PRESSED" << endl;
    int buttonId = e.button;
    if(buttonId == 0)
    {
        if(settingsChangeMode == 0)
            actionRandomParams();
        else
        {
            pointsDataManager.resetCurrentPoint();
        }
    }
    if(buttonId == 1)
    {
        if(settingsChangeMode == 0)
            actionSwapParams();
        else
        {
            pointsDataManager.resetAllPoints();
        }
    }
    if(buttonId == 2)
    {
        //actionTriggerWave();
        actionSpawnParticles(2);
    }
    if(buttonId == 3)
    {
        //actionChangeDisplayType();
        actionSpawnParticles(1);
    }
    if(buttonId == 4)
    {
        actionChangeSigmaCount(1);
    }
    if(buttonId == 5)
    {
        actionChangeSigmaCount(-1);
    }
    if(buttonId == 6)
    {
        settingsChangeMode = (settingsChangeMode + 1) % 2;
        latestPointSettingsActionTime = getTime();
    }
    if(buttonId == 7)
    {
        actionChangeColorMode();
    }
    if(buttonId == 10)
    {
        actionTriggerWave();
        //actionSpawnParticles(2);
    }
    if(buttonId == 9)
    {
        actionChangeDisplayType();
    }

    paramsUpdate();
}

void ofApp::axisChanged(ofxGamepadAxisEvent& e)
{
	//cout << "AXIS " << e.axis << " VALUE " << ofToString(e.value) << endl;

    int axisType = e.axis;
    float value = e.value;
    if(axisType==6 && e.value>0.5)
    {
        if(settingsChangeMode == 0)
            actionChangeParams(1);
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex,1);
            latestPointSettingsActionTime = getTime();
        }
    }
    if(axisType==6 && e.value<-0.5)
    {
        if(settingsChangeMode == 0)
            actionChangeParams(-1);
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex,-1);
            latestPointSettingsActionTime = getTime();
        }
    }
    if(axisType==7 && e.value>0.5)
    {
        if(settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(-1);
        else
        {
            settingsChangeIndex = (settingsChangeIndex + 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
    }
    if(axisType==7 && e.value<-0.5)
    {
        if(settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(1);
        else
        {
            settingsChangeIndex = (settingsChangeIndex - 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
    }
    if(axisType==0 || axisType==1)
    {
        if(axisType==0) curTranslationAxis1 = 0;
        if(axisType==1) curTranslationAxis2 = 0;
        if(abs(e.value)>0.09)
        {
            if(axisType==0) curTranslationAxis1 = e.value;
            if(axisType==1) curTranslationAxis2 = e.value;

            penMoveLatestTime = getTime();
        }
    }
    
    if(axisType==3 || axisType==4)
    {
        if(axisType==3) curMoveBiasActionX = 0;
        if(axisType==4) curMoveBiasActionY = 0;

        if(abs(e.value)>0.09)
        {
            if(axisType==3) curMoveBiasActionX = e.value;
            if(axisType==4) curMoveBiasActionY = e.value;

            penMoveLatestTime = getTime();
        }
    }
    

    
    if(axisType==2)
    {
        curL2 = e.value;
    }
    if(axisType==5)
    {
        curR2 = e.value;
    }
    

    paramsUpdate();
}

void ofApp::buttonReleased(ofxGamepadButtonEvent& e)
{
	//cout << "BUTTON " << e.button << " RELEASED" << endl;
}

void ofApp::keyPressed(int key){
    switch(key)
    {
        case OF_KEY_RIGHT:
            if(settingsChangeMode == 0)
                actionChangeParams(1);
            else
            {
                pointsDataManager.changeValue(settingsChangeIndex,1);
                latestPointSettingsActionTime = getTime();
            }
            break;
        case OF_KEY_LEFT:
            if(settingsChangeMode == 0)
                actionChangeParams(-1);
            else
            {
                pointsDataManager.changeValue(settingsChangeIndex,-1);
                latestPointSettingsActionTime = getTime();
            }
            break;
        case OF_KEY_UP:
            if(settingsChangeMode == 0)
                pointsDataManager.changeSelectionIndex(1);
            else
            {
                settingsChangeIndex = (settingsChangeIndex - 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
                latestPointSettingsActionTime = getTime();
            }
            break;
        case OF_KEY_DOWN:
            if(settingsChangeMode == 0)
                pointsDataManager.changeSelectionIndex(-1);
            else
            {
                settingsChangeIndex = (settingsChangeIndex + 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
                latestPointSettingsActionTime = getTime();
            }
            break;
        case ' ':
            actionRandomParams();
            break;
        case 'r':
            actionRandomParams();
            break;
        case 's':
            actionSwapParams();
            break;
        case 'd':
            actionSpawnParticles(2);
            break;
        case 'f':
            actionSpawnParticles(1);
            break;
        case 'x':
            actionChangeSigmaCount(-1);
            break;
        case 'c':
            actionChangeSigmaCount(1);
            break;
        case 'p':
            actionChangeDisplayType();
            break;
        case 'w':
            actionTriggerWave();
            break;
        case 'h':
            actionChangeDisplayType();
            break;
        case 'a':
            if(settingsChangeMode == 0)
            {
                actionChangeColorMode();
            }
            else
            {
                pointsDataManager.resetCurrentPoint();
            }
            break;
        case 'b':
            if(settingsChangeMode == 0)
            {
                // nothing
            }
            else
            {
                pointsDataManager.resetAllPoints();
            }
            break;
        case '5':
            settingsChangeMode = (settingsChangeMode + 1) % 2;
            latestPointSettingsActionTime = getTime();
            break;
    }

    paramsUpdate();
}

void ofApp::mousePressed(int x, int y, int button){
    switch(button)
    {
        case 0:
            actionSpawnParticles(2);
            break;
        case 2:
            actionSpawnParticles(1);
            break;
        case 1:
            actionTriggerWave();
            break;
    }
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

void ofApp::drawTextBox(const std::string& stringToShow, ofTrueTypeFont* pFont, float col, float alpha)
{
    ofPushMatrix();
    ofSetColor(col,150);
    ofTranslate(-10*u,-32*u);
    ofDrawRectangle(0,0,20*u + pFont->stringWidth(stringToShow),41*u);
    ofPopMatrix();

    ofSetColor(255-col);
    ofPushMatrix();
    pFont->drawString(stringToShow,0,0);
    ofPopMatrix();
}

std::string ofApp::roundedString(float value)
{
    std::stringstream stream;
    // Set fixed-point notation and round to three decimal places
    stream << std::fixed << std::setprecision(DIGITS_PRECISION) << value;
    std::string result = stream.str();
    return result;
}

void ofApp::drawCustomCircle(ofVec2f pos,float R,float r)
{
	int mCircle = 14;
    float r2 = ofMap(R,0,700,0.5*r,1.5*r)*1.0;

    R *= u;

    float alphaFactor = 1.0;
    if(ACTIVATE_PEN_FADE) 
    {
        alphaFactor = ofMap(getTime() - penMoveLatestTime, 0, PEN_FADE_DURATION, 1, 0, true);
    }

    for(int i=0;i<mCircle;i++)
    {
        float rot = 0.3*sin(PI*getTime()*0.18) + ofMap(i,0,mCircle,0,TWO_PI);

        ofPushMatrix();
        ofTranslate(pos.x, pos.y);
        ofRotateRad(rot);
        ofTranslate(R, 0);
        ofScale(u);

        ofFill();
        ofSetRectMode(OF_RECTMODE_CENTER);
        ofSetColor(255,120*alphaFactor);
        ofDrawRectangle(ofVec2f(0,0),r2+3,r2*6+3);
        ofSetColor(0,190*alphaFactor);
        ofDrawRectangle(ofVec2f(0,0),r2,r2*6);
        ofSetRectMode(OF_RECTMODE_CORNER);
        ofPopMatrix();
    }
}

// draw pad with arrows when explaining controls
void ofApp::drawPad(float col, float alpha)
{
    ofPushMatrix();

    ofScale(u);
    
    for(int i=0;i<4;i++)
    {
        ofPushMatrix();
        ofRotateRad(HALF_PI*i);
        float rectangleHeight = 25;
        ofSetColor(255-col,255*pow(alpha/255.0,2.0));
        ofDrawRectangle(0,-rectangleHeight/2,30,rectangleHeight);
        ofSetColor(col,255*pow(alpha/255.0,2.0));
        ofBeginShape();
        ofVertex(16,rectangleHeight/3);
        ofVertex(25,0);
        ofVertex(16,-rectangleHeight/3);
        ofEndShape(true);
        ofPopMatrix();
    }
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
