#pragma once

// by Etienne Jacob, see license and crediting in main.cpp

#include "ofMain.h"
#include "ofxGamepadHandler.h"
#include "parametersChoices.h"

#define WIDTH 1024
#define HEIGHT 576
#define NUMBER_OF_PARAM_SETS 2
#define ORIGINAL_CONFIGS_NUMBER 36

class ofApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();

    std::array<float, 30> simulationParams;
    double scalingFactor;
    void setSimulationParamsToSomeDefault(int setIndex);
    void setSimulationParams(int setIndex, int typeIndex);
    void switchToOtherType(int typeIndex);
    std::array<int,NUMBER_OF_PARAM_SETS> targetParamsIndex = {};
    int currentSelectedSet = 0;
    std::string getSetName(int targetParamsIndex);

    float actionAreaSizeSigma = 0.3;
    int sigmaCount = 2;
    int sigmaCountModulo = 6;
    float maxActionSize = 0.85;
    float getActionAreaSizeSigma();
    int displayType = 1;
    void drawCustomCircle(ofVec2f pos,float R,float r);

    void actionChangeSigmaCount(int dir);
    void actionChangeParams(int dir);
    void actionSwapParams();
    void actionRandomParams();

    float curTranslationAxis1 = 0;
    float curTranslationAxis2 = 0;
    float curActionX = WIDTH/2;
    float curActionY = HEIGHT/2;
    float translationStep = 6.5;

    ofFbo fbo,fbo2,fboDisplay;
    ofShader settershader,moveshader,depositshader,computefragshader;
    ofVbo vbo;

    std::vector<uint32_t> counter;
    ofBufferObject counterBuffer;

    int numFrames = 4000;

    struct Particle{
        glm::vec4 data;
        glm::vec4 data2;
    };

    vector<Particle> particles;
    ofBufferObject particlesBuffer;

    float timeOffset = ofRandom(10000);

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    // gamepad events
    int numberOfGamepads;
    void axisChanged(ofxGamepadAxisEvent &e);
    void buttonPressed(ofxGamepadButtonEvent &e);
    void buttonReleased(ofxGamepadButtonEvent &e);

    void printCurrentScalingFactor();

    ofTrueTypeFont myFont, myFontBold;

    struct ParametersSet{
        int typeIndex;

        float defaultScalingFactor;
        int scalingFactorCount;

        float SensorDistance0;
        float SD_exponent;
        float SD_amplitude;

        float SensorAngle0;
        float SA_exponent;
        float SA_amplitude;

        float RotationAngle0;
        float RA_exponent;
        float RA_amplitude;

        float JumpDistance0;
        float JD_exponent;
        float JD_amplitude;

        float SensorBias1;
        float SensorBias2;
    };

    std::vector<ParametersSet> simulationParameters;
    std::vector<int> scalingCounts;
    ofBufferObject simulationParametersBuffer;
    std::vector<ParametersSet> savedSimulationParameters;
    std::vector<int> savedScalingCounts;

    std::vector<int> selectedSets;
};
