#include "ofApp.h"

void ofApp::buttonPressed(ofxGamepadButtonEvent &e)
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
        actionSpawnParticles(2);
    }
    if (buttonId == 3)
    {
        // actionChangeDisplayType();
        actionSpawnParticles(1);
    }
    if (buttonId == 4)
    {
        actionChangeSigmaCount(1);
    }
    if (buttonId == 5)
    {
        actionChangeSigmaCount(-1);
    }
    if (buttonId == 6)
    {
        settingsChangeMode = (settingsChangeMode + 1) % 2;
        latestPointSettingsActionTime = getTime();
    }
    if (buttonId == 7)
    {
        actionChangeColorMode();
    }
    if (buttonId == 10)
    {
        actionTriggerWave();
        // actionSpawnParticles(2);
    }
    if (buttonId == 9)
    {
        actionChangeDisplayType();
    }

    paramsUpdate();
}

void ofApp::axisChanged(ofxGamepadAxisEvent &e)
{
    // cout << "AXIS " << e.axis << " VALUE " << ofToString(e.value) << endl;

    int axisType = e.axis;
    float value = e.value;
    if (axisType == 6 && value > 0.5)
    {
        if (settingsChangeMode == 0)
            actionChangeParams(1);
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex, 1);
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 6 && value < -0.5)
    {
        if (settingsChangeMode == 0)
            actionChangeParams(-1);
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex, -1);
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 7 && value > 0.5)
    {
        if (settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(-1);
        else
        {
            settingsChangeIndex = (settingsChangeIndex + 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 7 && value < -0.5)
    {
        if (settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(1);
        else
        {
            settingsChangeIndex = (settingsChangeIndex - 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
    }
    if (axisType == 0 || axisType == 1)
    {
        if (axisType == 0)
            curTranslationAxis1 = 0;
        if (axisType == 1)
            curTranslationAxis2 = 0;
        if (abs(value) > 0.09)
        {
            if (axisType == 0)
                curTranslationAxis1 = value;
            if (axisType == 1)
                curTranslationAxis2 = value;

            penMoveLatestTime = getTime();
        }
    }

    if (axisType == 3 || axisType == 4)
    {
        if (axisType == 3)
            curMoveBiasActionX = 0;
        if (axisType == 4)
            curMoveBiasActionY = 0;

        if (abs(value) > 0.09)
        {
            if (axisType == 3)
                curMoveBiasActionX = value;
            if (axisType == 4)
                curMoveBiasActionY = value;

            penMoveLatestTime = getTime();
        }
    }

    if (axisType == 2)
    {
        curL2 = value;
    }
    if (axisType == 5)
    {
        curR2 = value;
    }

    paramsUpdate();
}

void ofApp::buttonReleased(ofxGamepadButtonEvent &e)
{
    // cout << "BUTTON " << e.button << " RELEASED" << endl;
}