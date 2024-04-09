// by Etienne Jacob, see license and crediting in main.cpp

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);

    ofEnableAntiAliasing();

    myFont.load("fonts/Raleway-Regular.ttf",floor(15.0*ofGetHeight()/HEIGHT));
    myFontBold.load("fonts/Raleway-Bold.ttf",floor(15.0*ofGetHeight()/HEIGHT));

    counter.resize(WIDTH*HEIGHT);
    counterBuffer.allocate(counter, GL_DYNAMIC_DRAW);

    fbo.allocate(WIDTH, HEIGHT, GL_RGBA32F);
    fbo2.allocate(WIDTH, HEIGHT, GL_RGBA32F);
    fboDisplay.allocate(WIDTH, HEIGHT, GL_RGBA32F);

    settershader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_setter.glsl");
    settershader.linkProgram();

    depositshader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_deposit.glsl");
    depositshader.linkProgram();

    moveshader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_move.glsl");
    moveshader.linkProgram();

    computefragshader.setupShaderFromFile(GL_COMPUTE_SHADER,"computeshader_blur.glsl");
    computefragshader.linkProgram();

    particles.resize(512*512*14);
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

    simulationParameters.resize(NUMBER_OF_PARAM_SETS);
    scalingCounts.resize(NUMBER_OF_PARAM_SETS);
    for(int i=0;i<NUMBER_OF_PARAM_SETS;i++) scalingCounts[i] = 0;
    simulationParametersBuffer.allocate(simulationParameters,GL_DYNAMIC_DRAW);
    simulationParametersBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

    fbo.getTexture().bindAsImage(0,GL_READ_ONLY);
    fbo2.getTexture().bindAsImage(1,GL_WRITE_ONLY);
    particlesBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    counterBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    fboDisplay.getTexture().bindAsImage(4,GL_WRITE_ONLY);
    

    fbo.begin();

    fbo.end();

    selectedSets = {0,1,2,4,5,6,7,11,13,14,15,19,21,27,30,31,34,37,40};


    for(int i=0;i<NUMBER_OF_PARAM_SETS;i++)
    {
        setSimulationParamsToSomeDefault(i);
    }

    savedSimulationParameters.resize(ORIGINAL_CONFIGS_NUMBER);
    savedScalingCounts.resize(ORIGINAL_CONFIGS_NUMBER);
    for(int i=0;i<ORIGINAL_CONFIGS_NUMBER;i++)
    {
        savedSimulationParameters[i] = simulationParameters[currentSelectedSet];
        savedScalingCounts[i] = scalingCounts[currentSelectedSet];
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

//--------------------------------------------------------------
void ofApp::update(){
    float t = 1.0*ofGetFrameNum()/700;


    float time = ofGetFrameNum()*0.1 + timeOffset;

    curActionX += curTranslationAxis1*translationStep;
    curActionY += curTranslationAxis2*translationStep;
/*
    curActionX = fmod(curActionX + WIDTH, WIDTH);
    curActionY = fmod(curActionY + HEIGHT, HEIGHT);
*/
    curActionX = ofClamp(curActionX, 0, WIDTH);
    curActionY = ofClamp(curActionY, 0, HEIGHT);

    fbo2.begin();
    fbo.draw(0,0);
    fbo2.end();

    settershader.begin();
    settershader.setUniform1i("width",fbo.getWidth());
    settershader.setUniform1i("height",fbo.getHeight());
    settershader.setUniform1i("value",0);
    settershader.dispatchCompute(WIDTH / 32, HEIGHT / 32, 1);
    settershader.end();


    moveshader.begin();
    moveshader.setUniform1i("width",fbo.getWidth());
    moveshader.setUniform1i("height",fbo.getHeight());
    moveshader.setUniform1f("time",time);

    moveshader.setUniform1f("actionAreaSizeSigma",getActionAreaSizeSigma());

    /*
    moveshader.setUniform1f("actionX",ofGetMouseX());
    moveshader.setUniform1f("actionY",ofGetMouseY());
    */
    moveshader.setUniform1f("actionX",curActionX);
    moveshader.setUniform1f("actionY",curActionY);
/*
    moveshader.setUniform1f("sensorBiasActionX",curSensorBiasActionX);
    moveshader.setUniform1f("sensorBiasActionY",curSensorBiasActionY);
*/
    moveshader.setUniform1f("moveBiasActionX",curMoveBiasActionX);
    moveshader.setUniform1f("moveBiasActionY",curMoveBiasActionY);

    moveshader.dispatchCompute(particles.size()/128,1,1);
    moveshader.end();



    depositshader.begin();
    depositshader.setUniform1i("width",fbo.getWidth());
    depositshader.setUniform1i("height",fbo.getHeight());
    depositshader.setUniform1f("depositFactor",0.003);
    depositshader.dispatchCompute(WIDTH / 32, HEIGHT / 32, 1);
    depositshader.end();

    fbo.begin();
    fbo2.draw(0,0);
    fbo.end();


    computefragshader.begin();
    computefragshader.setUniform1i("width",fbo.getWidth());
    computefragshader.setUniform1i("height",fbo.getHeight());
    computefragshader.setUniform1f("PI",PI);
    computefragshader.setUniform1f("decayFactor",0.75);
    computefragshader.setUniform1f("time",time);
    computefragshader.dispatchCompute(fbo.getWidth()/32,fbo.getHeight()/32,1);
    computefragshader.end();


    fbo.begin();
    fbo2.draw(0,0);
    fbo.end();

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
}

void ofApp::actionRandomParams()
{
    int sz = selectedSets.size();

    targetParamsIndex[0] = floor(ofRandom(sz));
    targetParamsIndex[1] = floor(ofRandom(sz));
}

void ofApp::buttonPressed(ofxGamepadButtonEvent& e)
{
	//cout << "BUTTON " << e.button << " PRESSED" << endl;
    int buttonId = e.button;
    if(buttonId == 0)
    {
        actionSwapParams();
    }
    if(buttonId == 1)
    {
        actionRandomParams();
    }
    if(buttonId == 2)
    {
        // "X button"
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
        //actionChangeColors();
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

    setSimulationParams(0,selectedSets[targetParamsIndex[0]]);
    setSimulationParams(1,selectedSets[targetParamsIndex[1]]); 
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
        currentSelectedSet = (currentSelectedSet - 1 + NUMBER_OF_PARAM_SETS) % NUMBER_OF_PARAM_SETS;
    }
    if(axisType==7 && e.value<-0.5)
    {
        currentSelectedSet = (currentSelectedSet + 1 + NUMBER_OF_PARAM_SETS) % NUMBER_OF_PARAM_SETS;
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
        //std::cout << "(" << curActionX << "," << curActionY << ")" << std::endl;
    }
    
    if(axisType==3 || axisType==4)
    {
        if(axisType==3) curSensorBiasActionX = 0;
        if(axisType==4) curSensorBiasActionY = 0;

        if(axisType==3) curMoveBiasActionX = 0;
        if(axisType==4) curMoveBiasActionY = 0;

        if(abs(e.value)>0.09)
        {
            if(axisType==3) curSensorBiasActionX = e.value;
            if(axisType==4) curSensorBiasActionY = e.value;

            if(axisType==3) curMoveBiasActionX = e.value;
            if(axisType==4) curMoveBiasActionY = e.value;
        }
    }
    

    /*
    if(axisType==2)
    {
        curL2 = e.value;
    }
    if(axisType==5)
    {
        curR2 = e.value;
    }
    */

    //std::cout << "Current parameters choice index : " << getSetName(targetParamsIndex[currentSelectedSet]) << std::endl;

    setSimulationParams(currentSelectedSet,selectedSets[targetParamsIndex[currentSelectedSet]]);  
}

void ofApp::buttonReleased(ofxGamepadButtonEvent& e)
{
	//cout << "BUTTON " << e.button << " RELEASED" << endl;
}

void ofApp::drawCustomCircle(ofVec2f pos,float R,float r)
{
	int mCircle = 34;
    float r2 = ofMap(R,0,700,0.5*r,1.5*r)*0.6;

    float time = ofGetFrameNum()*0.1 + timeOffset;

    for(int i=0;i<mCircle;i++)
    {
        float rot = 0.3*sin(PI*time*0.03);
        float theta1 = ofMap(i,0,mCircle,0,TWO_PI) + rot;
        float theta2 = ofMap(i+0.5,0,mCircle,0,TWO_PI) + rot;

        float x1 = R*cos(theta1) + pos.x;
        float y1 = R*sin(theta1) + pos.y;

        float x2 = R*cos(theta2) + pos.x;
        float y2 = R*sin(theta2) + pos.y;

        //ofDrawLine(ofVec2f(x1,y1),ofVec2f(x2,y2));
        ofFill();
        ofDrawCircle(ofVec2f(x1,y1),r2);
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
        
        float time = ofGetFrameNum()*0.1 + timeOffset;

        float R = getActionAreaSizeSigma()*600*(1.0 + 0.08*sin(0.4f*time));

        float cx = ofMap(curActionX,0,WIDTH,0,ofGetWidth());
        float cy = ofMap(curActionY,0,HEIGHT,0,ofGetHeight());

        ofSetCircleResolution(100);

        ofSetColor(220);
        ofNoFill();
        ofSetLineWidth(9);
        drawCustomCircle(ofVec2f(cx,cy),R,9);

        ofSetColor(25);
        ofNoFill();
        ofSetLineWidth(9);
        drawCustomCircle(ofVec2f(cx,cy),R,6);
        
        ofPopMatrix();
    }

    ofFill();

    ofPushMatrix();

    float col = 0;
    float u = ofGetHeight()/HEIGHT;

    for(int setIndex=0;setIndex<NUMBER_OF_PARAM_SETS;setIndex++)
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
        std::string setString = prefix + getSetName(targetParamsIndex[setIndex]) + (setIndex==currentSelectedSet ? " <" : "");

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


void ofApp::printCurrentScalingFactor()
{
    std::cout << "Scale factor = " << simulationParameters[currentSelectedSet].defaultScalingFactor * pow(1.05, simulationParameters[currentSelectedSet].scalingFactorCount) << std::endl;
}

std::string ofApp::getSetName(int targetParamsIndex_)
{
    std::string ret = "params ";
    ret.push_back(char('A' + targetParamsIndex_));
    return ret;
}

void ofApp::actionChangeParams(int dir)
{
    int sz = selectedSets.size();
    targetParamsIndex[currentSelectedSet] = (targetParamsIndex[currentSelectedSet] + dir + sz)%sz;
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
        case OF_KEY_RIGHT:
            scalingCounts[currentSelectedSet]++;
            break;
        case OF_KEY_LEFT:
            scalingCounts[currentSelectedSet]--;
            break;
        case ' ':
            currentSelectedSet = (currentSelectedSet + 1) % NUMBER_OF_PARAM_SETS;
            break;
    }

    //std::cout << "Current parameters choice index : " << getSetName(targetParamsIndex[currentSelectedSet]) << std::endl;

    setSimulationParams(currentSelectedSet,selectedSets[targetParamsIndex[currentSelectedSet]]);  
}

void ofApp::switchToOtherType(int typeIndex)
{
    savedScalingCounts[targetParamsIndex[currentSelectedSet]] = scalingCounts[currentSelectedSet];
    targetParamsIndex[currentSelectedSet] = typeIndex;
    scalingCounts[currentSelectedSet] = savedScalingCounts[targetParamsIndex[currentSelectedSet]];
}

void ofApp::setSimulationParams(int setIndex, int typeIndex)
{
    simulationParameters[setIndex].typeIndex = typeIndex;

    simulationParameters[setIndex].defaultScalingFactor = ParametersMatrix[typeIndex][PARAMS_DIMENSION-1];
    simulationParameters[setIndex].scalingFactorCount = scalingCounts[setIndex];

    simulationParameters[setIndex].SensorDistance0 = ParametersMatrix[typeIndex][0];
    simulationParameters[setIndex].SD_exponent = ParametersMatrix[typeIndex][1];
    simulationParameters[setIndex].SD_amplitude = ParametersMatrix[typeIndex][2];

    simulationParameters[setIndex].SensorAngle0 = ParametersMatrix[typeIndex][3];
    simulationParameters[setIndex].SA_exponent = ParametersMatrix[typeIndex][4];
    simulationParameters[setIndex].SA_amplitude = ParametersMatrix[typeIndex][5];

    simulationParameters[setIndex].RotationAngle0 = ParametersMatrix[typeIndex][6];
    simulationParameters[setIndex].RA_exponent = ParametersMatrix[typeIndex][7];
    simulationParameters[setIndex].RA_amplitude = ParametersMatrix[typeIndex][8];

    simulationParameters[setIndex].JumpDistance0 = ParametersMatrix[typeIndex][9];
    simulationParameters[setIndex].JD_exponent = ParametersMatrix[typeIndex][10];
    simulationParameters[setIndex].JD_amplitude = ParametersMatrix[typeIndex][11];

    simulationParameters[setIndex].SensorBias1 = ParametersMatrix[typeIndex][12];
    simulationParameters[setIndex].SensorBias2 = ParametersMatrix[typeIndex][13];

    simulationParametersBuffer.updateData(simulationParameters);

    //printCurrentScalingFactor();
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

    simulationParameters[i].JumpDistance0 = 0.75;
    simulationParameters[i].JD_exponent = 2.0;
    simulationParameters[i].JD_amplitude = 0.;

    simulationParameters[i].SensorBias1 = 0.;
    simulationParameters[i].SensorBias2 = 0.;

    simulationParametersBuffer.updateData(simulationParameters);
}

// draw pad with arrows when explaining controls
void ofApp::drawPad(float col, float alpha)
{
    ofPushMatrix();
    //ofTranslate(x,y);
    
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
    //printCurrentScalingFactor();
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
