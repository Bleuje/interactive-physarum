// by Etienne Jacob, see license and crediting in main.cpp

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetFrameRate(GlobalSettings::FRAME_RATE);

    ofEnableAntiAliasing();

    u = float(ofGetHeight()) / 1080;

    myFont.load("fonts/Raleway-Regular.ttf", floor(22.0 * u));
    myFontBold.load("fonts/Raleway-Bold.ttf", floor(22.0 * u));

    gamepadControlsImage.load("images/xboxgamepadcontrols.png");
    informationImage.load("images/interactive-physarum-info.png");

    counter.resize(GlobalSettings::SIMULATION_WIDTH * GlobalSettings::SIMULATION_HEIGHT);
    counterBuffer.allocate(counter, GL_DYNAMIC_DRAW);

    trailReadBuffer.allocate(GlobalSettings::SIMULATION_WIDTH, GlobalSettings::SIMULATION_HEIGHT, GL_RG16F);
    trailWriteBuffer.allocate(GlobalSettings::SIMULATION_WIDTH, GlobalSettings::SIMULATION_HEIGHT, GL_RG16F);
    fboDisplay.allocate(GlobalSettings::SIMULATION_WIDTH, GlobalSettings::SIMULATION_HEIGHT, GL_RGBA8);

    setterShader.setupShaderFromFile(GL_COMPUTE_SHADER, "shaders/computeshader_setter.glsl");
    setterShader.linkProgram();

    depositShader.setupShaderFromFile(GL_COMPUTE_SHADER, "shaders/computeshader_deposit.glsl");
    depositShader.linkProgram();

    moveShader.setupShaderFromFile(GL_COMPUTE_SHADER, "shaders/computeshader_move.glsl");
    moveShader.linkProgram();

    diffusionShader.setupShaderFromFile(GL_COMPUTE_SHADER, "shaders/computeshader_diffusion.glsl");
    diffusionShader.linkProgram();

    particles.resize(GlobalSettings::NUMBER_OF_PARTICLES * GlobalSettings::PARTICLE_PARAMETERS_COUNT);

    auto floatAsUint16 = [](float x) -> uint16_t
    {
        return static_cast<uint16_t>(std::round(std::clamp(x, 0.0f, 1.0f) * 65535.0f));
    };

    for (size_t i = 0; i < GlobalSettings::NUMBER_OF_PARTICLES; i++)
    {
        particles[GlobalSettings::PARTICLE_PARAMETERS_COUNT * i + 0] = floatAsUint16(ofRandom(0, GlobalSettings::SIMULATION_WIDTH) / GlobalSettings::SIMULATION_WIDTH);
        particles[GlobalSettings::PARTICLE_PARAMETERS_COUNT * i + 1] = floatAsUint16(ofRandom(0, GlobalSettings::SIMULATION_HEIGHT) / GlobalSettings::SIMULATION_HEIGHT);
        particles[GlobalSettings::PARTICLE_PARAMETERS_COUNT * i + 2] = floatAsUint16(ofRandom(1));
        particles[GlobalSettings::PARTICLE_PARAMETERS_COUNT * i + 3] = floatAsUint16(ofRandom(1));
        particles[GlobalSettings::PARTICLE_PARAMETERS_COUNT * i + 4] = 0;
        particles[GlobalSettings::PARTICLE_PARAMETERS_COUNT * i + 5] = 0;
    }
    particlesBuffer.allocate(particles, GL_DYNAMIC_DRAW);

    simulationParameters.resize(NUMBER_OF_USED_POINTS);
    simulationParametersBuffer.allocate(simulationParameters, GL_DYNAMIC_DRAW);
    simulationParametersBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

    trailReadBuffer.getTexture().bindAsImage(0, GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1, GL_WRITE_ONLY);
    particlesBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    counterBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    fboDisplay.getTexture().bindAsImage(4, GL_WRITE_ONLY);

    for (int i = 0; i < GlobalSettings::MAX_NUMBER_OF_WAVES; i++)
    {
        waveXarray[i] = GlobalSettings::SIMULATION_WIDTH / 2;
        waveYarray[i] = GlobalSettings::SIMULATION_HEIGHT / 2;
        waveTriggerTimes[i] = -12345;
        waveSavedSigmas[i] = 0.5;
    }

    for (int i = 0; i < GlobalSettings::MAX_NUMBER_OF_RANDOM_SPAWN; i++)
    {
        randomSpawnXarray[i] = GlobalSettings::SIMULATION_WIDTH / 2;
        randomSpawnYarray[i] = GlobalSettings::SIMULATION_HEIGHT / 2;
    }

    ////////////////////////////////////////
    // check if there is a gamepad connected
    numberOfGamepads = ofxGamepadHandler::get()->getNumPads();

    for (int i = 0; i < numberOfGamepads; i++) {
        ofxGamepad* pad = ofxGamepadHandler::get()->getGamepad(i);
        int gamepadIndex = i;
    
        axisListeners.push_back(std::make_shared<ofEventListener>(
            pad->onAxisChanged.newListener([this, gamepadIndex](ofxGamepadAxisEvent &e) {
                this->axisChanged(e, gamepadIndex);
            })
        ));
    
        buttonPressedListeners.push_back(std::make_shared<ofEventListener>(
            pad->onButtonPressed.newListener([this, gamepadIndex](ofxGamepadButtonEvent &e) {
                this->buttonPressed(e, gamepadIndex);
            })
        ));
    
        buttonReleasedListeners.push_back(std::make_shared<ofEventListener>(
            pad->onButtonReleased.newListener([this, gamepadIndex](ofxGamepadButtonEvent &e) {
                this->buttonReleased(e, gamepadIndex);
            })
        ));
    }
    numberOfActiveGamepads = numberOfGamepads;
    std::cout << "Number of gamepads : " << numberOfGamepads << std::endl;
    ////////////////////////////////////////

    latestActivtyTimeArray[0] = -12345;
    latestActivtyTimeArray[1] = -12345.1;

    for (int i = 0; i < std::max(1, numberOfActiveGamepads); i++)
    {
        actionAreaSizeSigmaArray[i] = 0.3;
        sigmaCountArray[i] = 2;
        moveBiasActionXArray[i] = 0;
        moveBiasActionYArray[i] = 0;
        actionXArray[i] = GlobalSettings::SIMULATION_WIDTH / 2 + GlobalSettings::SIMULATION_WIDTH / 4 * ofRandom(-1, 1);
        actionYArray[i] = GlobalSettings::SIMULATION_HEIGHT / 2 + GlobalSettings::SIMULATION_HEIGHT / 4 * ofRandom(-1, 1);
        translationAxis1Array[i] = 0;
        translationAxis2Array[i] = 0;
        // latestActivtyTimeArray[i] = 0;
    }

    std::cout << "Number of points : " << pointsDataManager.getNumberOfPoints() << std::endl;

    paramsUpdate();

    pointsDataManager.reloadUsedPointsTargets();
}

void ofApp::paramsUpdate()
{
    pointsDataManager.updateCurrentValuesFromTransitionProgress(currentTransitionProgress());

    for (int k = 0; k < NUMBER_OF_USED_POINTS; k++)
    {
        simulationParameters[k] = pointsDataManager.getPointsParamsFromArray(pointsDataManager.currentPointValues[k]);
    }

    simulationParametersBuffer.updateData(simulationParameters);
}

//--------------------------------------------------------------
void ofApp::update()
{
    float time = getTime();

    paramsUpdate();

    if ((getTime() - latestPointSettingsActionTime) >= GlobalSettings::SETTINGS_DISAPPEAR_DURATION)
    {
        settingsChangeMode = 0;
    }

    inactivityElapsedTime[0] = getTime() - latestActivtyTimeArray[0];
    inactivityElapsedTime[1] = getTime() - latestActivtyTimeArray[1];
    int isActive1 = inactivityElapsedTime[0] <= GlobalSettings::MAX_GAMEPAD_INACTIVIY;
    int isActive2 = inactivityElapsedTime[1] <= GlobalSettings::MAX_GAMEPAD_INACTIVIY;

    int numberOfTrulyActiveGamepads = isActive1 + isActive2;
    int bestGamepadIndex = inactivityElapsedTime[0] < inactivityElapsedTime[1] ? 0 : 1;

    if (numberOfTrulyActiveGamepads == 2 && numberOfActiveGamepads == 1)
    {
        if (bestGamepadIndex == 0)
            actionSwapParams();
        numberOfActiveGamepads = 2;
    }

    if (numberOfTrulyActiveGamepads == 1 && previousNumberOfTrulyActiveGamepads == 0)
    {
        numberOfActiveGamepads = 1;
        singleActiveGamepadIndex = bestGamepadIndex;
    }
    else if (numberOfTrulyActiveGamepads <= 1 && numberOfActiveGamepads == 2)
    {
        numberOfActiveGamepads = 1;
        if (bestGamepadIndex == 1)
            actionSwapParams();
        singleActiveGamepadIndex = bestGamepadIndex;
    }
    else if (numberOfActiveGamepads == 0)
    {
        singleActiveGamepadIndex = 0;
    }
    previousNumberOfTrulyActiveGamepads = numberOfTrulyActiveGamepads;

    if (numberOfActiveGamepads == 0)
    {
        actionXArray[singleActiveGamepadIndex] = ofMap(ofGetMouseX(), 0, ofGetWidth(), 0, GlobalSettings::SIMULATION_WIDTH, true);
        actionYArray[singleActiveGamepadIndex] = ofMap(ofGetMouseY(), 0, ofGetHeight(), 0, GlobalSettings::SIMULATION_HEIGHT, true);
        updateActionAreaSizeSigma(singleActiveGamepadIndex);
    }
    else
    {
        for (int i = 0; i < numberOfActiveGamepads; i++)
        {
            int index;
            if (numberOfActiveGamepads == 1)
                index = singleActiveGamepadIndex;
            else
                index = i;

            updateActionAreaSizeSigma(index);

            actionXArray[index] += translationAxis1Array[index] * translationStep;
            actionYArray[index] += translationAxis2Array[index] * translationStep;

            if (GlobalSettings::LOOP_PEN_POSITION)
            {
                actionXArray[index] = fmod(actionXArray[index] + GlobalSettings::SIMULATION_WIDTH, GlobalSettings::SIMULATION_WIDTH);
                actionYArray[index] = fmod(actionYArray[index] + GlobalSettings::SIMULATION_HEIGHT, GlobalSettings::SIMULATION_HEIGHT);
            }
            else
            {
                actionXArray[index] = ofClamp(actionXArray[index], 0, GlobalSettings::SIMULATION_WIDTH);
                actionYArray[index] = ofClamp(actionYArray[index], 0, GlobalSettings::SIMULATION_HEIGHT);
            }
        }
    }

    if (numberOfGamepads == 0)
    {
        curL2Array[0] = -1; // L2 for no "inertia" effect, when using keyboard only
        curL2Array[1] = -1;
        curR2Array[0] = -1;
        curR2Array[1] = -1;
    }
    else if (numberOfGamepads <= 1)
    {
        curL2Array[1 - singleActiveGamepadIndex] = -1;
        curR2Array[1 - singleActiveGamepadIndex] = -1;
    }

    setterShader.begin();
    setterShader.setUniform1i("width", trailReadBuffer.getWidth());
    setterShader.setUniform1i("height", trailReadBuffer.getHeight());
    setterShader.setUniform1i("value", 0);
    setterShader.dispatchCompute(GlobalSettings::SIMULATION_WIDTH / GlobalSettings::WORK_GROUP_SIZE, GlobalSettings::SIMULATION_HEIGHT / GlobalSettings::WORK_GROUP_SIZE, 1);
    setterShader.end();

    trailReadBuffer.getTexture().bindAsImage(0, GL_READ_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(1, GL_WRITE_ONLY);

    moveShader.begin();
    moveShader.setUniform1i("width", trailReadBuffer.getWidth());
    moveShader.setUniform1i("height", trailReadBuffer.getHeight());
    moveShader.setUniform1f("time", time);

    moveShader.setUniform1i("numberOfActiveGamepads", numberOfActiveGamepads);
    moveShader.setUniform1i("singleActiveGamepadIndex", singleActiveGamepadIndex);

    moveShader.setUniform1fv("actionAreaSizeSigmaArray", actionAreaSizeSigmaArray.data(), actionAreaSizeSigmaArray.size());

    moveShader.setUniform1fv("actionXArray", actionXArray.data(), actionXArray.size());
    moveShader.setUniform1fv("actionYArray", actionYArray.data(), actionYArray.size());

    moveShader.setUniform1fv("moveBiasActionXArray", moveBiasActionXArray.data(), moveBiasActionXArray.size());
    moveShader.setUniform1fv("moveBiasActionYArray", moveBiasActionYArray.data(), moveBiasActionYArray.size());

    moveShader.setUniform1fv("waveXarray", waveXarray.data(), waveXarray.size());
    moveShader.setUniform1fv("waveYarray", waveYarray.data(), waveYarray.size());
    moveShader.setUniform1fv("waveTriggerTimes", waveTriggerTimes.data(), waveTriggerTimes.size());
    moveShader.setUniform1fv("waveSavedSigmas", waveSavedSigmas.data(), waveSavedSigmas.size());

    moveShader.setUniform1f("mouseXchange", 1.0 * ofGetMouseX() / ofGetWidth());
    moveShader.setUniform1fv("L2ActionArray", curL2Array.data(), curL2Array.size());

    moveShader.setUniform1i("spawnParticles", int(particlesSpawn));
    moveShader.setUniform1f("spawnFraction", GlobalSettings::SPAWN_FRACTION);
    moveShader.setUniform1i("randomSpawnNumber", randomSpawnNumber);
    moveShader.setUniform1fv("randomSpawnXarray", randomSpawnXarray.data(), randomSpawnXarray.size());
    moveShader.setUniform1fv("randomSpawnYarray", randomSpawnYarray.data(), randomSpawnYarray.size());
    moveShader.setUniform1i("spawnGamepadIndex", spawnGamepadIndex);

    moveShader.setUniform1f("pixelScaleFactor", GlobalSettings::PIXEL_SCALE_FACTOR);

    moveShader.dispatchCompute(particles.size() / (GlobalSettings::PARTICLE_WORK_GROUPS * GlobalSettings::PARTICLE_PARAMETERS_COUNT), 1, 1);
    moveShader.end();

    depositShader.begin();
    depositShader.setUniform1i("width", trailReadBuffer.getWidth());
    depositShader.setUniform1i("height", trailReadBuffer.getHeight());
    depositShader.setUniform1f("depositFactor", GlobalSettings::DEPOSIT_FACTOR);
    depositShader.setUniform1i("colorModeType", colorModeType);
    depositShader.setUniform1i("numberOfColorModes", GlobalSettings::NUMBER_OF_COLOR_MODES);
    depositShader.dispatchCompute(GlobalSettings::SIMULATION_WIDTH / GlobalSettings::WORK_GROUP_SIZE, GlobalSettings::SIMULATION_HEIGHT / GlobalSettings::WORK_GROUP_SIZE, 1);
    depositShader.end();

    trailReadBuffer.getTexture().bindAsImage(1, GL_WRITE_ONLY);
    trailWriteBuffer.getTexture().bindAsImage(0, GL_READ_ONLY);

    diffusionShader.begin();
    diffusionShader.setUniform1i("width", trailReadBuffer.getWidth());
    diffusionShader.setUniform1i("height", trailReadBuffer.getHeight());
    diffusionShader.setUniform1f("decayFactor", GlobalSettings::DECAY_FACTOR);
    diffusionShader.dispatchCompute(trailReadBuffer.getWidth() / GlobalSettings::WORK_GROUP_SIZE, trailReadBuffer.getHeight() / GlobalSettings::WORK_GROUP_SIZE, 1);
    diffusionShader.end();

    if (particlesSpawn)
        particlesSpawn = 0;

    std::stringstream strm;
    strm << "fps: " << ofGetFrameRate();
    ofSetWindowTitle(strm.str());
}

// DRAW

void ofApp::draw()
{
    u = float(ofGetHeight()) / 1080;

    float R2action = ofMap(curR2Array[0] + curR2Array[1] + 2.0, 0, 1.3, 0, 1, true);
    if (numberOfGamepads == 0)
        R2action = 0;

    ofPushMatrix();

    ofPushMatrix();
    ofScale(1.0 * ofGetWidth() / fboDisplay.getWidth(), 1.0 * ofGetHeight() / fboDisplay.getHeight());
    fboDisplay.draw(0, 0);
    ofPopMatrix();

    if (numberOfActiveGamepads == 2)
        displayType = 1;

    // draw circle
    if (displayType == 1)
    {
        for (int i = 0; i < std::max(1, numberOfActiveGamepads); i++)
        {
            int index;
            if (numberOfActiveGamepads <= 1)
                index = singleActiveGamepadIndex;
            else
                index = i;

            ofPushMatrix();

            float time2 = getTime() * 6;

            float R = actionAreaSizeSigmaArray[index] * 600 * (1.0 + 0.08 * sin(0.4f * time2));

            float cx = ofMap(actionXArray[index], 0, GlobalSettings::SIMULATION_WIDTH, 0, ofGetWidth());
            float cy = ofMap(actionYArray[index], 0, GlobalSettings::SIMULATION_HEIGHT, 0, ofGetHeight());

            ofSetCircleResolution(100);

            int colorIndex = (numberOfActiveGamepads <= 1) ? 0 : (i + 1);

            drawCustomCircle(ofVec2f(cx, cy), R, 9, colorIndex);

            ofPopMatrix();
        }
    }

    ofFill();

    ofPushMatrix();

    float col = 0;

    for (int setIndex = 0; setIndex < NUMBER_OF_USED_POINTS; setIndex++)
    {
        ofPushMatrix();

        ofPushMatrix();
        ofTranslate(53 * u, 65 * u);
        ofScale(1.3);
        drawPad(100, 255);
        ofScale(0.92);
        drawPad(255, 255);
        ofPopMatrix();

        ofTranslate(116 * u, 50 * u + 50 * setIndex * u);

        if (numberOfActiveGamepads <= 1)
        {
            std::string prefix = setIndex == 0 ? "pinceau : " : "fond : ";
            std::string setString = prefix + pointsDataManager.getPointName(setIndex) + (setIndex == pointsDataManager.getSelectionIndex() ? " <" : "");

            ofTrueTypeFont *pBoldOrNotFont = setIndex == pointsDataManager.getSelectionIndex() ? &myFontBold : &myFont;
            drawTextBox(setString, pBoldOrNotFont, col, 255);
        }
        else
        {
            ofPushMatrix();
            float p = inactivityElapsedTime[setIndex] / GlobalSettings::MAX_GAMEPAD_INACTIVIY;
            drawPie(-2 * u, -10 * u, 16 * u, 1 - p, getPlayerColor(setIndex + 1));
            ofPushMatrix();
            ofTranslate(35 * u, 0);
            std::string prefix = setIndex == 0 ? "player 1: " : "player 2: ";
            std::string setString = prefix + pointsDataManager.getPointName(setIndex) + (true ? " <" : "");

            ofTrueTypeFont *pBoldOrNotFont = &myFontBold;
            drawTextBox(setString, pBoldOrNotFont, col, 255, setIndex + 1);
            ofPopMatrix();
            ofPopMatrix();
        }

        ofPopMatrix();
    }

    if (settingsChangeMode == 1)
    {
        ofPushMatrix();
        ofTranslate(50 * u, 180 * u);

        std::string pointName = "Réglages du " + pointsDataManager.getPointName(pointsDataManager.getSelectionIndex()) + " :";
        drawTextBox(pointName, &myFont, col, 255);

        ofScale(0.8);

        ofTranslate(0, 25 * u);

        for (int i = 0; i < GlobalSettings::SETTINGS_SIZE; i++)
        {
            ofTranslate(0, 44 * u);

            ofTrueTypeFont *pBoldOrNotFont = i == settingsChangeIndex ? &myFontBold : &myFont;

            std::string settingValueString = pointsDataManager.getSettingName(i) + " : " + roundedString(pointsDataManager.getValue(i)) + (i == settingsChangeIndex ? " <" : "");
            ;

            drawTextBox(settingValueString, pBoldOrNotFont, col, 110);
        }

        ofTranslate(0, 80 * u);
        std::string pressA = "Appuyer sur A pour reset les réglages du " + pointsDataManager.getPointName(pointsDataManager.getSelectionIndex()) + "";
        drawTextBox(pressA, &myFontBold, col, 110);

        ofTranslate(0, 44 * u);
        std::string pressB = "Appuyer sur B pour reset les réglages des tous les points";
        drawTextBox(pressB, &myFontBold, col, 110);

        ofPopMatrix();
    }

    ofPushMatrix();
    ofTranslate(u * 9, ofGetHeight() - 9 * u);
    ofScale(0.7);
    std::string creditString = "Inspiration et paramètres de \"36 Points\" par Sage Jenson";

    ofPushMatrix();
    ofSetColor(col, 150);
    ofTranslate(-10 * u, -32 * u);
    ofDrawRectangle(0, 0, 20 * u + myFont.stringWidth(creditString), 41 * u);
    ofPopMatrix();

    ofSetColor(255 - col);
    ofPushMatrix();
    myFont.drawString(creditString, 0, 0);
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
    float infoProgress = 1 - pow(1 - R2action, 2.0);

    ofPushMatrix();
    ofTranslate(-25 * u - 1500 * u * (1 - infoProgress), 230 * u);
    ofScale(1.5 * u);
    gamepadControlsImage.draw(0, 0);
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(1270 * u + 1000 * u * (1 - infoProgress), 230 * u);
    ofScale(0.7 * u);
    informationImage.draw(0, 0);
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(1250 * u, 25 * u - 50 * u * infoProgress);
    ofScale(0.7 * u);
    std::string pressB = "Gâchette de droite pour afficher informations et commandes.";
    drawTextBox(pressB, &myFontBold, col, 110);
    ofPopMatrix();

    ofPopMatrix();
}

// OTHER

void ofApp::updateActionAreaSizeSigma(int gamepadIndex)
{
    float target = ofMap(sigmaCountArray[gamepadIndex], 0, sigmaCountModulo, 0.15, maxActionSize);
    float lerper = pow(ofMap(getTime() - latestSigmaChangeTime, 0, GlobalSettings::ACTION_SIGMA_CHANGE_DURATION, 0, 1, true), 1.7);
    actionAreaSizeSigmaArray[gamepadIndex] = ofLerp(actionAreaSizeSigmaArray[gamepadIndex], target, lerper);
}

void ofApp::setRandomSpawn()
{
    // randomSpawnNumber = floor(ofRandom(MAX_NUMBER_OF_RANDOM_SPAWN/2,MAX_NUMBER_OF_RANDOM_SPAWN));
    randomSpawnNumber = GlobalSettings::MAX_NUMBER_OF_RANDOM_SPAWN;

    for (int i = 0; i < randomSpawnNumber; i++)
    {
        float theta = ofRandom(0, TWO_PI);
        float r = pow(ofRandom(0, 1), 0.5);
        float x = r * cos(theta);
        float y = r * sin(theta);
        randomSpawnXarray[i] = x;
        randomSpawnYarray[i] = y;
    }
}

float ofApp::getTime()
{
    return 1.0 * ofGetFrameNum() / GlobalSettings::FRAME_RATE;
}

float ofApp::currentTransitionProgress()
{
    return ofMap(getTime() - transitionTriggerTime, 0, GlobalSettings::TRANSITION_DURATION, 0., 1., true);
}

bool ofApp::activeTransition()
{
    return (getTime() - transitionTriggerTime) <= GlobalSettings::TRANSITION_DURATION;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{
}
