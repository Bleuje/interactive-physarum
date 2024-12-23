#include "ofApp.h"

void ofApp::keyPressed(int key)
{
    switch (key)
    {
    case OF_KEY_RIGHT:
        if (settingsChangeMode == 0)
            actionChangeParams(1);
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex, 1);
            latestPointSettingsActionTime = getTime();
        }
        break;
    case OF_KEY_LEFT:
        if (settingsChangeMode == 0)
            actionChangeParams(-1);
        else
        {
            pointsDataManager.changeValue(settingsChangeIndex, -1);
            latestPointSettingsActionTime = getTime();
        }
        break;
    case OF_KEY_UP:
        if (settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(1);
        else
        {
            settingsChangeIndex = (settingsChangeIndex - 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
            latestPointSettingsActionTime = getTime();
        }
        break;
    case OF_KEY_DOWN:
        if (settingsChangeMode == 0)
            pointsDataManager.changeSelectionIndex(-1);
        else
        {
            settingsChangeIndex = (settingsChangeIndex + 1 + SETTINGS_SIZE) % SETTINGS_SIZE;
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
        else
        {
            pointsDataManager.resetCurrentPoint();
        }
        break;
    case 'b':
        if (settingsChangeMode == 0)
        {
            // nothing
        }
        else
        {
            pointsDataManager.resetAllPoints();
        }
        break;
    case '5':
        settingsChangeMode = (settingsChangeMode + 1) % 2;
        latestPointSettingsActionTime = getTime();
        break;
    }

    paramsUpdate();
}