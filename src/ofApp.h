#pragma once

// by Etienne Jacob, see license and crediting in main.cpp

#include "ofMain.h"
#include "ofxGamepadHandler.h"
#include "pointsManagement.h"

#define WIDTH 1280
#define HEIGHT 736
#define FRAME_RATE 60
#define NUMBER_OF_COLOR_MODES 2
#define NUMBER_OF_PARTICLES (512*512*22)
#define MAX_NUMBER_OF_WAVES 5
#define TRANSITION_TIME 0.5

class ofApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();

    double scalingFactor;
    void setSimulationParamsToSomeDefault(int setIndex);

    template<class valueType>
    void updateParamTowardsMatrixValue(valueType& value, int matrixColumnIndex, int typeIndex);
    bool activeTransition();

    void setSimulationParams(int setIndex, int typeIndex);
    void paramsUpdate();

    void switchToOtherType(int typeIndex);
    std::array<int,NUMBER_OF_USED_POINTS> targetParamsIndex = {};
    int currentSelectedSet = 0;
    void drawPad(float col, float alpha);

    float actionAreaSizeSigma = 0.3;
    int sigmaCount = 2;
    int sigmaCountModulo = 6;
    float maxActionSize = 0.85;
    float getActionAreaSizeSigma();
    int displayType = 1;
    int colorModeType = 1;
    void drawCustomCircle(ofVec2f pos,float R,float r);

    float getTime();
    float currentTransitionProgress();

    void actionChangeSigmaCount(int dir);
    void actionChangeParams(int dir);
    void actionSwapParams();
    void actionRandomParams();
    void actionChangeColorMode();

    float curTranslationAxis1 = 0;
    float curTranslationAxis2 = 0;
    float curMoveBiasActionX = 0;
    float curMoveBiasActionY = 0;
    float curActionX = WIDTH/2;
    float curActionY = HEIGHT/2;
    float translationStep = 6.5;
    int currentWaveIndex = 0;
    float curL2 = 0;
    float curR2 = 0;
    std::array<float, MAX_NUMBER_OF_WAVES> waveXarray = {};
    std::array<float, MAX_NUMBER_OF_WAVES> waveYarray = {};
    std::array<float, MAX_NUMBER_OF_WAVES> waveTriggerTimes = {};
    float transitionTriggerTime = -12345;
    float waveActionAreaSizeSigma = 0.001;

    ofFbo trailReadBuffer,trailWriteBuffer,fboDisplay;
    ofShader setterShader,moveShader,depositShader,blurShader;
    ofVbo vbo;

    std::vector<uint32_t> counter;
    ofBufferObject counterBuffer;

    // int numFrames = 4000;

    struct Particle{
        glm::vec4 data;
        glm::vec4 data2;
    };

    vector<Particle> particles;
    ofBufferObject particlesBuffer;

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

    ofTrueTypeFont myFont, myFontBold;

    PointsDataManager pointsDataManager;

    std::vector<PointSettings> simulationParameters;
    ofBufferObject simulationParametersBuffer;
};
