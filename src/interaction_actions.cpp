#include "ofApp.h"

void ofApp::actionChangeSigmaCount(int dir)
{
    sigmaCountArray[singleActiveGamepadIndex] = (sigmaCountArray[singleActiveGamepadIndex] + sigmaCountModulo + dir) % sigmaCountModulo;
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
    waveXarray[currentWaveIndex] = actionXArray[singleActiveGamepadIndex];
    waveYarray[currentWaveIndex] = actionYArray[singleActiveGamepadIndex];
    waveTriggerTimes[currentWaveIndex] = getTime();
    waveSavedSigmas[currentWaveIndex] = actionAreaSizeSigmaArray[singleActiveGamepadIndex];

    currentWaveIndex = (currentWaveIndex + 1) % MAX_NUMBER_OF_WAVES;

    waveActionAreaSizeSigma = actionAreaSizeSigmaArray[singleActiveGamepadIndex];
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
    if (spawnType == 2)
    {
        setRandomSpawn();
    }
}