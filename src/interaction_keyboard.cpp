#include "ofApp.h"

void ofApp::keyPressed(int key)
{
    switch (key)
    {
    case OF_KEY_RIGHT:
        if (settingsChangeMode == 0)
            actionChangeParams(1);
        else if (settingsChangeMode == 1)
        {
            pointsDataManager.changeValue(settingsChangeIndex, 1);
            latestPointSettingsActionTime = getTime();
        }
        else if (settingsChangeMode == 2)
        {
            actionValuesArray[settingsChangeIndex]++;
            latestPointSettingsActionTime = getTime();
        }
        break;
    case OF_KEY_LEFT:
        if (settingsChangeMode == 0)
            actionChangeParams(-1);
        else if (settingsChangeMode == 1)
        {
            pointsDataManager.changeValue(settingsChangeIndex, -1);
            latestPointSettingsActionTime = getTime();
        }
        else if (settingsChangeMode == 2)
        {
            actionValuesArray[settingsChangeIndex]--;
            latestPointSettingsActionTime = getTime();
        }
        break;
    case OF_KEY_UP:
        if (settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(1);
        else if (settingsChangeMode == 1)
        {
            settingsChangeIndex = (settingsChangeIndex - 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
        else if (settingsChangeMode == 2)
        {
            settingsChangeIndex = (settingsChangeIndex - 1 + ACTION_VALUES_SIZE) % ACTION_VALUES_SIZE;
            latestPointSettingsActionTime = getTime();
        }
        break;
    case OF_KEY_DOWN:
        if (settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(-1);
        else if (settingsChangeMode == 1)
        {
            settingsChangeIndex = (settingsChangeIndex + 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
        else if (settingsChangeMode == 2)
        {
            settingsChangeIndex = (settingsChangeIndex + 1 + ACTION_VALUES_SIZE) % ACTION_VALUES_SIZE;
            latestPointSettingsActionTime = getTime();
        }
        break;
    case ' ':
        actionRandomParams();
        break;
    case 'r':
        actionRandomParams();
        break;
    case 's':
        actionSwapParams();
        break;
    case 'd':
        actionSpawnParticles(2);
        break;
    case 'f':
        actionSpawnParticles(1);
        break;
    case 'x':
        actionChangeSigmaCount(-1);
        break;
    case 'c':
        actionChangeSigmaCount(1);
        break;
    case 'p':
        actionChangeDisplayType();
        break;
    case 'w':
        actionTriggerWave();
        break;
    case 'h':
        actionChangeDisplayType();
        break;
    case 'a':
        if (settingsChangeMode == 0)
        {
            actionChangeColorMode();
        }
        else if (settingsChangeMode == 1)
        {
            pointsDataManager.resetCurrentPoint();
        }
        else if (settingsChangeMode == 2)
        {
            actionResetCurrentActionValue();
        }
        break;
    case 'b':
        if (settingsChangeMode == 0)
        {
            // nothing
        }
        else if (settingsChangeMode == 1)
        {
            pointsDataManager.resetAllPoints();
        }
        else if (settingsChangeMode == 2)
        {
            actionResetAllActionValues();
        }
        break;
    case '5':
        settingsChangeMode = (settingsChangeMode + 1) % 3;
        latestPointSettingsActionTime = getTime();
        break;
    case 'u':
        pointsDataManager.createRandomParameters();
        break;
    case 'i':
        pointsDataManager.writeParamsToFile();
        break;
    }

    paramsUpdate();
}