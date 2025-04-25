#pragma once

// by Etienne Jacob, see license and crediting in main.cpp

#include "ofMain.h"
#include "ofxGamepadHandler.h"
#include "utils.h"
#include "points_management.h"

#define SIMULATION_WIDTH 1280
#define SIMULATION_HEIGHT 736
#define NUMBER_OF_PARTICLES (512 * 512 * 22)
#define PARTICLE_PARAMETERS_COUNT 6
#define DECAY_FACTOR 0.75
#define DEPOSIT_FACTOR 0.003
#define PIXEL_SCALE_FACTOR 250.0

#define FRAME_RATE 60
#define NUMBER_OF_COLOR_MODES 10
#define MAX_NUMBER_OF_WAVES 5
#define TRANSITION_DURATION 0.5
#define PEN_FADE_DURATION 1.0
#define SPAWN_FRACTION 0.1
#define ACTIVATE_PEN_FADE false
#define LOOP_PEN_POSITION false
#define SETTINGS_SIZE 15
#define SETTINGS_DISAPPEAR_DURATION 10
#define ACTION_SIGMA_CHANGE_DURATION 0.26
#define DIGITS_PRECISION 3
#define MAX_NUMBER_OF_RANDOM_SPAWN 5
#define MAX_GAMEPAD_INACTIVIY 10

class ofApp : public ofBaseApp
{

public:
    void setup();
    void update();
    void draw();

    PointsDataManager pointsDataManager; // loading initial stuff with PointsDataManager::PointsDataManager()
    void paramsUpdate();

    void drawCustomCircle(ofVec2f pos, float R, float r, int colorIndex = 0);
    void drawPad(float col, float alpha);
    void drawTextBox(const std::string &stringToShow, ofTrueTypeFont *pFont, float col, float alpha, int contourType = 0);
    void drawPie(float x,float y,float radius,float p,ofColor color);
    ofColor getPlayerColor(int playerIndex);
    std::string roundedString(float value);
    float u = 1; // variable for screen resolution adaptation

    float getTime();
    float currentTransitionProgress();
    bool activeTransition();

    void actionChangeSigmaCount(int dir, int gamepadIndex);
    void actionChangeParams(int dir);
    void actionSwapParams();
    void actionRandomParams();
    void actionChangeColorMode();
    void actionTriggerWave();
    void actionChangeDisplayType();
    void actionChangeSelectionIndex(int dir);
    void actionSpawnParticles(int spawnType);

    int sigmaCountModulo = 6;
    float maxActionSize = 0.85;
    
    float latestSigmaChangeTime = -12345;
    void updateActionAreaSizeSigma(int gamepadIndex);

    int displayType = 1;
    int colorModeType = 3;
    std::array<float, 2> translationAxis1Array;
    std::array<float, 2> translationAxis2Array;
    std::array<float, 2> actionAreaSizeSigmaArray;
    std::array<int, 2> sigmaCountArray;
    std::array<float, 2> moveBiasActionXArray;
    std::array<float, 2> moveBiasActionYArray;
    std::array<float, 2> actionXArray;
    std::array<float, 2> actionYArray;
    float translationStep = 6.5;
    int currentWaveIndex = 0;
    std::array<float, 2> curL2Array;
    std::array<float, 2> curR2Array;
    std::array<float, MAX_NUMBER_OF_WAVES> waveXarray = {};
    std::array<float, MAX_NUMBER_OF_WAVES> waveYarray = {};
    std::array<float, MAX_NUMBER_OF_WAVES> waveTriggerTimes = {};
    std::array<float, MAX_NUMBER_OF_WAVES> waveSavedSigmas = {};
    float transitionTriggerTime = -12345;
    float waveActionAreaSizeSigma = 0.001;
    float penMoveLatestTime = -12345;
    int particlesSpawn = 0;
    int settingsChangeMode = 0;
    int settingsChangeIndex = 0;
    float latestPointSettingsActionTime = -12345;
    std::array<float, MAX_NUMBER_OF_RANDOM_SPAWN> randomSpawnXarray = {};
    std::array<float, MAX_NUMBER_OF_RANDOM_SPAWN> randomSpawnYarray = {};
    int randomSpawnNumber = 0;
    void setRandomSpawn();
    int numberOfActiveGamepads = 2;
    int singleActiveGamepadIndex = 0;
    int spawnGamepadIndex = 0;
    std::array<float, 2> inactivityElapsedTime;

    ofFbo trailReadBuffer, trailWriteBuffer, fboDisplay;
    ofShader setterShader, moveShader, depositShader, blurShader;

    std::vector<uint32_t> counter;
    ofBufferObject counterBuffer;
    std::vector<PointSettings> simulationParameters;
    ofBufferObject simulationParametersBuffer;
    struct Particle
    {
        glm::vec4 data;
        glm::vec4 data2;
    };
    std::vector<uint16_t> particles;
    ofBufferObject particlesBuffer;

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
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
    std::vector<std::shared_ptr<ofEventListener>> axisListeners;
    std::vector<std::shared_ptr<ofEventListener>> buttonPressedListeners;
    std::vector<std::shared_ptr<ofEventListener>> buttonReleasedListeners;
    void axisChanged(ofxGamepadAxisEvent &e, int gamepadIndex);
    void buttonPressed(ofxGamepadButtonEvent &e, int gamepadIndex);
    void buttonReleased(ofxGamepadButtonEvent &e, int gamepadIndex);
    void recordGamepadActivity(int gamepadIndex);
    std::array<float, 2> latestActivtyTimeArray;
    std::array<int, 2> isActiveArray;

    ofTrueTypeFont myFont, myFontBold;
    ofImage gamepadControlsImage, informationImage;

    // int numFrames = 4000;
};
