#include "ofApp.h"

void ofApp::mousePressed(int x, int y, int button)
{
    switch (button)
    {
    case 0:
        actionSpawnParticles(2);
        break;
    case 2:
        actionSpawnParticles(1);
        break;
    case 1:
        actionTriggerWave();
        break;
    }
}