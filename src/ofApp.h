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
#define TRANSITION_DURATION 0.5
#define PEN_FADE_DURATION 1.0
#define SPAWN_FRACTION 0.1
#define ACTIVATE_PEN_FADE false
#define SETTINGS_SIZE 15
#define SETTINGS_DISAPPEAR_DURATION 10
#define ACTION_SIGMA_CHANGE_DURATION 0.26
#define DIGITS_PRECISION 3

class ofApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();

    PointsDataManager pointsDataManager; // loading initial stuff with PointsDataManager::PointsDataManager()
    void paramsUpdate();

    void drawCustomCircle(ofVec2f pos,float R,float r);
    void drawPad(float col, float alpha);
    void drawTextBox(float u, const std::string& stringToShow, ofTrueTypeFont* pFont, float col, float alpha);
    std::string roundedString(float value);

    float getTime();
    float currentTransitionProgress();
    bool activeTransition();

    void actionChangeSigmaCount(int dir);
    void actionChangeParams(int dir);
    void actionSwapParams();
    void actionRandomParams();
    void actionChangeColorMode();
    void actionTriggerWave();
    void actionChangeDisplayType();
    void actionChangeSelectionIndex(int dir);
    void actionSpawnParticles(int spawnType);


    float actionAreaSizeSigma = 0.3;
    int sigmaCount = 2;
    int sigmaCountModulo = 6;
    float maxActionSize = 0.85;
    float currentActionAreaSizeSigma = 0.5;
    float latestSigmaChangeTime = -12345;
    void updateActionAreaSizeSigma();

    int displayType = 1;
    int colorModeType = 1;
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
    float penMoveLatestTime = -12345;
    int particlesSpawn = 0;
    int settingsChangeMode = 0;
    int settingsChangeIndex = 0;
    float latestPointSettingsActionTime = -12345;

    ofFbo trailReadBuffer,trailWriteBuffer,fboDisplay;
    ofShader setterShader,moveShader,depositShader,blurShader;
    ofVbo vbo;

    std::vector<uint32_t> counter;
    ofBufferObject counterBuffer;
    std::vector<PointSettings> simulationParameters;
    ofBufferObject simulationParametersBuffer;
    struct Particle{
        glm::vec4 data;
        glm::vec4 data2;
    };
    std::vector<Particle> particles;
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

    // int numFrames = 4000;
};
