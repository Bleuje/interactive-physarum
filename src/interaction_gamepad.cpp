#include "ofApp.h"

void ofApp::buttonPressed(ofxGamepadButtonEvent &e, int gamepadIndex)
{
    // cout << "BUTTON " << e.button << " PRESSED" << endl;

    int buttonId = e.button;
    if (buttonId == 0)
    {
        if (settingsChangeMode == 0)
            actionRandomParams();
        else
        {
            pointsDataManager.resetCurrentPoint();
        }
    }
    if (buttonId == 1)
    {
        if (settingsChangeMode == 0)
            actionSwapParams();
        else
        {
            pointsDataManager.resetAllPoints();
        }
    }
    if (buttonId == 2)
    {
        // actionTriggerWave();
        spawnGamepadIndex = gamepadIndex;
        actionSpawnParticles(2);
    }
    if (buttonId == 3)
    {
        // actionChangeDisplayType();
        spawnGamepadIndex = gamepadIndex;
        actionSpawnParticles(1);
    }
    if (buttonId == 4)
    {
        actionChangeSigmaCount(1, gamepadIndex);
    }
    if (buttonId == 5)
    {
        actionChangeSigmaCount(-1, gamepadIndex);
    }
    if (buttonId == 6)
    {
        if (numberOfActiveGamepads <= 1)
        {
            settingsChangeMode = (settingsChangeMode + 1) % 2;
            latestPointSettingsActionTime = getTime();
        }
    }
    if (buttonId == 7)
    {
        actionChangeColorMode();
    }
    if (buttonId == 10)
    {
        spawnGamepadIndex = gamepadIndex;
        actionTriggerWave();
        // actionSpawnParticles(2);
    }
    if (buttonId == 9)
    {
        actionChangeDisplayType();
    }

    recordGamepadActivity(gamepadIndex);

    paramsUpdate();
}

void ofApp::axisChanged(ofxGamepadAxisEvent &e, int gamepadIndex)
{
    // cout << "AXIS " << e.axis << " VALUE " << ofToString(e.value) << endl;

    int axisType = e.axis;
    float value = e.value;

    if (abs(value) > 0.5)
        recordGamepadActivity(gamepadIndex);

    if (axisType == 6 && value > 0.5)
    {
        if (settingsChangeMode == 0)
        {
            if (numberOfActiveGamepads == 1)
                actionChangeParams(1);
            else
            {
                pointsDataManager.currentSelectionIndex = gamepadIndex;
                actionChangeParams(1);
            }
        }
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex, 1);
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 6 && value < -0.5)
    {
        if (settingsChangeMode == 0)
        {
            if (numberOfActiveGamepads == 1)
                actionChangeParams(-1);
            else
            {
                pointsDataManager.currentSelectionIndex = gamepadIndex;
                actionChangeParams(-1);
            }
        }
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex, -1);
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 7 && value > 0.5)
    {
        if (settingsChangeMode == 0)
        {
            if (numberOfActiveGamepads == 1)
                pointsDataManager.changeSelectionIndex(-1);
            else
            {
                pointsDataManager.currentSelectionIndex = gamepadIndex;
                actionChangeParams(-1);
            }
        }
        else
        {
            settingsChangeIndex = (settingsChangeIndex + 1 + GlobalSettings::SETTINGS_SIZE) % GlobalSettings::SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 7 && value < -0.5)
    {
        if (settingsChangeMode == 0)
        {
            if (numberOfActiveGamepads == 1)
                pointsDataManager.changeSelectionIndex(1);
            else
            {
                pointsDataManager.currentSelectionIndex = gamepadIndex;
                actionChangeParams(1);
            }
        }
        else
        {
            settingsChangeIndex = (settingsChangeIndex - 1 + GlobalSettings::SETTINGS_SIZE) % GlobalSettings::SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 0 || axisType == 1)
    {
        if (axisType == 0)
            translationAxis1Array[gamepadIndex] = 0;
        if (axisType == 1)
            translationAxis2Array[gamepadIndex] = 0;
        if (abs(value) > 0.09)
        {
            if (axisType == 0)
                translationAxis1Array[gamepadIndex] = value;
            if (axisType == 1)
                translationAxis2Array[gamepadIndex] = value;

            penMoveLatestTime = getTime();
        }
    }

    if (axisType == 3 || axisType == 4)
    {
        if (axisType == 3)
            moveBiasActionXArray[gamepadIndex] = 0;
        if (axisType == 4)
            moveBiasActionYArray[gamepadIndex] = 0;

        if (abs(value) > 0.09)
        {
            if (axisType == 3)
                moveBiasActionXArray[gamepadIndex] = value;
            if (axisType == 4)
                moveBiasActionYArray[gamepadIndex] = value;

            penMoveLatestTime = getTime();
        }
    }

    if (axisType == 2)
    {
        curL2Array[gamepadIndex] = value;
    }
    if (axisType == 5)
    {
        curR2Array[gamepadIndex] = value;
    }

    paramsUpdate();
}

void ofApp::buttonReleased(ofxGamepadButtonEvent &e, int gamepadIndex)
{
    // cout << "BUTTON " << e.button << " RELEASED" << endl;
}

void ofApp::recordGamepadActivity(int gamepadIndex)
{
    latestActivtyTimeArray[gamepadIndex] = getTime();
    isActiveArray[gamepadIndex] = true;
}