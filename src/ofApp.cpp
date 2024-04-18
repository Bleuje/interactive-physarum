// by Etienne Jacob, see license and crediting in main.cpp

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(FRAME_RATE);

    ofEnableAntiAliasing();

    myFont.load("fonts/Raleway-Regular.ttf",floor(15.0*ofGetHeight()/HEIGHT));
    myFontBold.load("fonts/Raleway-Bold.ttf",floor(15.0*ofGetHeight()/HEIGHT));

    counter.resize(WIDTH*HEIGHT);
    counterBuffer.allocate(counter, GL_DYNAMIC_DRAW);

    trailReadBuffer.allocate(WIDTH, HEIGHT, GL_RGBA32F);
    trailWriteBuffer.allocate(WIDTH, HEIGHT, GL_RGBA32F);
    fboDisplay.allocate(WIDTH, HEIGHT, GL_RGBA32F);

    setterShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_setter.glsl");
    setterShader.linkProgram();

    depositShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_deposit.glsl");
    depositShader.linkProgram();

    moveShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_move.glsl");
    moveShader.linkProgram();

    blurShader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_blur.glsl");
    blurShader.linkProgram();

    particles.resize(NUMBER_OF_PARTICLES);
    float marginx = 3;
    float marginy = 3;

    for(auto & p: particles){
        p.data.x = ofRandom(marginx,WIDTH-marginx);
        p.data.y = ofRandom(marginy,HEIGHT-marginy);
        p.data.z = ofRandom(1);
        p.data.w = ofRandom(0,TWO_PI);
        p.data2.x = 0;
        p.data2.y = 0;
        p.data2.z = 0;
        p.data2.w = 0;
    }
    particlesBuffer.allocate(particles,GL_DYNAMIC_DRAW);

    scalingFactor = 1.0;

    simulationParameters.resize(NUMBER_OF_USED_POINTS);
    simulationParametersBuffer.allocate(simulationParameters,GL_DYNAMIC_DRAW);
    simulationParametersBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

    trailReadBuffer.getTexture().bindAsImage(0,GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1,GL_WRITE_ONLY);
    particlesBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    counterBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    fboDisplay.getTexture().bindAsImage(4,GL_WRITE_ONLY);
    

    trailReadBuffer.begin();

    trailReadBuffer.end();

    selectedPoints = {0,1,2,4,5,6,7,11,13,14,15,19,21,27,30,31,34,37,40};


    for(int i=0;i<NUMBER_OF_USED_POINTS;i++)
    {
        setSimulationParamsToSomeDefault(i);
    }

    savedSimulationParameters.resize(ORIGINAL_CONFIGS_NUMBER);
    for(int i=0;i<ORIGINAL_CONFIGS_NUMBER;i++)
    {
        savedSimulationParameters[i] = simulationParameters[currentSelectedSet];
    }

    for(int i=0;i<MAX_NUMBER_OF_WAVES;i++)
    {
        waveXarray[i] = WIDTH/2;
        waveYarray[i] = HEIGHT/2;
        waveTriggerTimes[i] = -12345;
    }

    ////////////////////////////////////////
    // check if there is a gamepad connected
    numberOfGamepads = ofxGamepadHandler::get()->getNumPads();

	if(numberOfGamepads>0){
			ofxGamepad* pad = ofxGamepadHandler::get()->getGamepad(0);
			ofAddListener(pad->onAxisChanged, this, &ofApp::axisChanged);
			ofAddListener(pad->onButtonPressed, this, &ofApp::buttonPressed);
			ofAddListener(pad->onButtonReleased, this, &ofApp::buttonReleased);
	}
	std::cout << "Number of gamepads : " << numberOfGamepads << std::endl;
    ////////////////////////////////////////
}

float ofApp::getActionAreaSizeSigma()
{
    return ofMap(sigmaCount,0,sigmaCountModulo,0.15,maxActionSize);
}

float ofApp::getTime()
{
    return 1.0*ofGetFrameNum()/FRAME_RATE;
}

float ofApp::currentTransitionProgress()
{
    return ofMap(getTime() - transitionTriggerTime, 0, TRANSITION_TIME, 0., 1., true);
}

bool ofApp::activeTransition()
{
    return (getTime() - transitionTriggerTime) <= TRANSITION_TIME;
}

//--------------------------------------------------------------
void ofApp::update(){
    float time = getTime();

    if(activeTransition())
    {
        paramsUpdate();
    }

    curActionX += curTranslationAxis1*translationStep;
    curActionY += curTranslationAxis2*translationStep;
/*
    curActionX = fmod(curActionX + WIDTH, WIDTH);
    curActionY = fmod(curActionY + HEIGHT, HEIGHT);
*/
    curActionX = ofClamp(curActionX, 0, WIDTH);
    curActionY = ofClamp(curActionY, 0, HEIGHT);

    trailWriteBuffer.begin();
    trailReadBuffer.draw(0,0);
    trailWriteBuffer.end();

    setterShader.begin();
    setterShader.setUniform1i("width",trailReadBuffer.getWidth());
    setterShader.setUniform1i("height",trailReadBuffer.getHeight());
    setterShader.setUniform1i("value",0);
    setterShader.dispatchCompute(WIDTH / 32, HEIGHT / 32, 1);
    setterShader.end();


    moveShader.begin();
    moveShader.setUniform1i("width",trailReadBuffer.getWidth());
    moveShader.setUniform1i("height",trailReadBuffer.getHeight());
    moveShader.setUniform1f("time",time);

    moveShader.setUniform1f("actionAreaSizeSigma",getActionAreaSizeSigma());
    moveShader.setUniform1f("waveActionAreaSizeSigma",waveActionAreaSizeSigma);

    /*
    moveShader.setUniform1f("actionX",ofGetMouseX());
    moveShader.setUniform1f("actionY",ofGetMouseY());
    */
    moveShader.setUniform1f("actionX",curActionX);
    moveShader.setUniform1f("actionY",curActionY);

    moveShader.setUniform1f("moveBiasActionX",curMoveBiasActionX);
    moveShader.setUniform1f("moveBiasActionY",curMoveBiasActionY);

    moveShader.setUniform1fv("waveXarray", waveXarray.data(), waveXarray.size());
    moveShader.setUniform1fv("waveYarray", waveYarray.data(), waveYarray.size());
    moveShader.setUniform1fv("waveTriggerTimes", waveTriggerTimes.data(), waveTriggerTimes.size());

    moveShader.setUniform1f("mouseXchange",1.0*ofGetMouseX()/ofGetWidth());
    moveShader.setUniform1f("L2Action",ofMap(curL2,-1,1,0,1.0,true));

    moveShader.dispatchCompute(particles.size()/128,1,1);
    moveShader.end();



    depositShader.begin();
    depositShader.setUniform1i("width",trailReadBuffer.getWidth());
    depositShader.setUniform1i("height",trailReadBuffer.getHeight());
    depositShader.setUniform1f("depositFactor",0.003);
    depositShader.setUniform1i("colorModeType",colorModeType);
    depositShader.setUniform1i("numberOfColorModes",NUMBER_OF_COLOR_MODES);
    depositShader.dispatchCompute(WIDTH / 32, HEIGHT / 32, 1);
    depositShader.end();

    trailReadBuffer.begin();
    trailWriteBuffer.draw(0,0);
    trailReadBuffer.end();


    blurShader.begin();
    blurShader.setUniform1i("width",trailReadBuffer.getWidth());
    blurShader.setUniform1i("height",trailReadBuffer.getHeight());
    blurShader.setUniform1f("PI",PI);
    blurShader.setUniform1f("decayFactor",0.75);
    blurShader.setUniform1f("time",time);
    blurShader.dispatchCompute(trailReadBuffer.getWidth()/32,trailReadBuffer.getHeight()/32,1);
    blurShader.end();


    trailReadBuffer.begin();
    trailWriteBuffer.draw(0,0);
    trailReadBuffer.end();

    std::stringstream strm;
    strm << "fps: " << ofGetFrameRate();
    ofSetWindowTitle(strm.str());

}

void ofApp::actionChangeSigmaCount(int dir)
{
    sigmaCount = (sigmaCount + sigmaCountModulo + dir) % sigmaCountModulo;
}

void ofApp::actionSwapParams()
{
    int targetParamsIndexAux = targetParamsIndex[0];
    targetParamsIndex[0] = targetParamsIndex[1];
    targetParamsIndex[1] = targetParamsIndexAux;

    transitionTriggerTime = getTime();
}

void ofApp::actionRandomParams()
{
    int sz = selectedPoints.size();

    targetParamsIndex[0] = floor(ofRandom(sz));
    targetParamsIndex[1] = floor(ofRandom(sz));

    transitionTriggerTime = getTime();
}

void ofApp::actionChangeColorMode()
{
    colorModeType = (colorModeType + 1) % NUMBER_OF_COLOR_MODES;
}

void ofApp::buttonPressed(ofxGamepadButtonEvent& e)
{
	//cout << "BUTTON " << e.button << " PRESSED" << endl;
    int buttonId = e.button;
    if(buttonId == 0)
    {
        actionRandomParams();
    }
    if(buttonId == 1)
    {
        actionSwapParams();
    }
    if(buttonId == 2)
    {
        waveXarray[currentWaveIndex] = curActionX;
        waveYarray[currentWaveIndex] = curActionY;
        waveTriggerTimes[currentWaveIndex] = getTime();

        currentWaveIndex = (currentWaveIndex + 1) % MAX_NUMBER_OF_WAVES;

        waveActionAreaSizeSigma = getActionAreaSizeSigma();
    }
    if(buttonId == 3)
    {
        displayType = (displayType + 1) % 2;
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
        //actionChange2D3D();
    }
    if(buttonId == 7)
    {
        actionChangeColorMode();
    }
    if(buttonId == 10)
    {
        //actionResetScales();
        //actionResetRotations();
    }
    if(buttonId == 9)
    {
        //actionResetTranslations();
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
        actionChangeParams(1);
    }
    if(axisType==6 && e.value<-0.5)
    {
        actionChangeParams(-1);
    }
    if(axisType==7 && e.value>0.5)
    {
        currentSelectedSet = (currentSelectedSet - 1 + NUMBER_OF_USED_POINTS) % NUMBER_OF_USED_POINTS;
    }
    if(axisType==7 && e.value<-0.5)
    {
        currentSelectedSet = (currentSelectedSet + 1 + NUMBER_OF_USED_POINTS) % NUMBER_OF_USED_POINTS;
    }
    if(axisType==0 || axisType==1)
    {
        if(axisType==0) curTranslationAxis1 = 0;
        if(axisType==1) curTranslationAxis2 = 0;
        if(abs(e.value)>0.09)
        {
            if(axisType==0) curTranslationAxis1 = e.value;
            if(axisType==1) curTranslationAxis2 = e.value;
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
    

    setSimulationParams(currentSelectedSet,selectedPoints[targetParamsIndex[currentSelectedSet]]);  
}

void ofApp::buttonReleased(ofxGamepadButtonEvent& e)
{
	//cout << "BUTTON " << e.button << " RELEASED" << endl;
}

void ofApp::drawCustomCircle(ofVec2f pos,float R,float r)
{
	int mCircle = 14;
    float r2 = ofMap(R,0,700,0.5*r,1.5*r)*1.0;

    float time2 = getTime()*6;

    for(int i=0;i<mCircle;i++)
    {
        float rot = 0.3*sin(PI*time2*0.03) + ofMap(i,0,mCircle,0,TWO_PI);

        ofPushMatrix();
        ofTranslate(pos.x, pos.y);
        ofRotateRad(rot);
        ofTranslate(R, 0);

        ofFill();
        ofSetRectMode(OF_RECTMODE_CENTER);
        ofSetColor(255,120);
        ofDrawRectangle(ofVec2f(0,0),r2+3,r2*6+3);
        ofSetColor(0,190);
        ofDrawRectangle(ofVec2f(0,0),r2,r2*6);
        ofSetRectMode(OF_RECTMODE_CORNER);
        ofPopMatrix();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
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

        float R = getActionAreaSizeSigma()*600*(1.0 + 0.08*sin(0.4f*time2));

        float cx = ofMap(curActionX,0,WIDTH,0,ofGetWidth());
        float cy = ofMap(curActionY,0,HEIGHT,0,ofGetHeight());

        ofSetCircleResolution(100);

        drawCustomCircle(ofVec2f(cx,cy),R,9);
        
        ofPopMatrix();
    }

    ofFill();

    ofPushMatrix();

    float col = 0;
    float u = ofGetHeight()/HEIGHT;

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
        std::string setString = prefix + getPointName(targetParamsIndex[setIndex]) + (setIndex==currentSelectedSet ? " <" : "");

        ofTrueTypeFont * pBoldOrNotFont = setIndex==currentSelectedSet ? &myFontBold : &myFont;

        ofPushMatrix();
        ofSetColor(col);
        ofTranslate(-10*u,-32*u);
        ofDrawRectangle(0,0,20*u + pBoldOrNotFont->stringWidth(setString),41*u);
        ofPopMatrix();

        ofSetColor(255-col);
        ofPushMatrix();
        pBoldOrNotFont->drawString(setString,0,0);
        ofPopMatrix();
        ofPopMatrix();
    }

    ofPushMatrix();
    ofTranslate(WIDTH*u*0.7,23*u);
    ofScale(0.7);
    std::string creditString = "Inspiration and parameters from mxsage's \"36 points\", different implementation";

    ofPushMatrix();
    ofSetColor(col);
    ofTranslate(-10*u,-32*u);
    ofDrawRectangle(0,0,20*u+myFont.stringWidth(creditString),41*u);
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

    ofPopMatrix();
}

std::string ofApp::getPointName(int targetParamsIndex_)
{
    std::string ret = "params ";
    ret.push_back(char('A' + targetParamsIndex_));
    return ret;
}

void ofApp::actionChangeParams(int dir)
{
    int sz = selectedPoints.size();
    targetParamsIndex[currentSelectedSet] = (targetParamsIndex[currentSelectedSet] + dir + sz)%sz;

    transitionTriggerTime = getTime();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch(key)
    {
        case OF_KEY_DOWN:
            actionChangeParams(1);
            break;
        case OF_KEY_UP:
            actionChangeParams(-1);
            break;
        case ' ':
            currentSelectedSet = (currentSelectedSet + 1) % NUMBER_OF_USED_POINTS;
            break;
    }

    setSimulationParams(currentSelectedSet,selectedPoints[targetParamsIndex[currentSelectedSet]]);  
}

void ofApp::switchToOtherType(int typeIndex)
{
    savedSimulationParameters[targetParamsIndex[currentSelectedSet]] = simulationParameters[currentSelectedSet];
    targetParamsIndex[currentSelectedSet] = typeIndex;
    simulationParameters[currentSelectedSet] = savedSimulationParameters[targetParamsIndex[currentSelectedSet]];
}

template<class valueType>
void ofApp::updateParamTowardsMatrixValue(valueType& value, int matrixColumnIndex, int typeIndex)
{
    float lerper = pow(currentTransitionProgress(),1.5);
    value = ofLerp(value,ParametersMatrix[typeIndex][matrixColumnIndex],lerper);
}

void ofApp::paramsUpdate()
{
    setSimulationParams(0,selectedPoints[targetParamsIndex[0]]);
    setSimulationParams(1,selectedPoints[targetParamsIndex[1]]); 
}

void ofApp::setSimulationParams(int setIndex, int typeIndex)
{
    simulationParameters[setIndex].typeIndex = typeIndex;
    simulationParameters[setIndex].scalingFactorCount = 0;

    updateParamTowardsMatrixValue(simulationParameters[setIndex].defaultScalingFactor, PARAMS_DIMENSION-1, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SensorDistance0, 0, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SD_exponent, 1, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SD_amplitude, 2, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SensorAngle0, 3, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SA_exponent, 4, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SA_amplitude, 5, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].RotationAngle0, 6, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].RA_exponent, 7, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].RA_amplitude, 8, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].MoveDistance0, 9, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].MD_exponent, 10, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].MD_amplitude, 11, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SensorBias1, 12, typeIndex);
    updateParamTowardsMatrixValue(simulationParameters[setIndex].SensorBias2, 13, typeIndex);

    simulationParametersBuffer.updateData(simulationParameters);
}

void ofApp::setSimulationParamsToSomeDefault(int i)
{
    simulationParameters[i].typeIndex = 1;

    simulationParameters[i].defaultScalingFactor = 1.0;
    simulationParameters[i].scalingFactorCount = 0;

    simulationParameters[i].SensorDistance0 = 7;
    simulationParameters[i].SD_exponent = 2.0;
    simulationParameters[i].SD_amplitude = 0.;

    simulationParameters[i].SensorAngle0 = TWO_PI/27;
    simulationParameters[i].SA_exponent = 2.0;
    simulationParameters[i].SA_amplitude = 0.;

    simulationParameters[i].RotationAngle0 = TWO_PI/15;
    simulationParameters[i].RA_exponent = 2.0;
    simulationParameters[i].RA_amplitude = 0.;

    simulationParameters[i].MoveDistance0 = 0.75;
    simulationParameters[i].MD_exponent = 2.0;
    simulationParameters[i].MD_amplitude = 0.;

    simulationParameters[i].SensorBias1 = 0.;
    simulationParameters[i].SensorBias2 = 0.;

    simulationParametersBuffer.updateData(simulationParameters);
}

// draw pad with arrows when explaining controls
void ofApp::drawPad(float col, float alpha)
{
    ofPushMatrix();
    
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
void ofApp::mousePressed(int x, int y, int button){

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
