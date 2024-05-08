#include "ofMain.h"
#include "ofApp.h"

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// by Etienne Jacob

// Crediting:
// Work derived 36 Points by Sage Jenson (mxsage) but with a different implementation.
// Before studying mxsage's code I already had a physarum implementation (already inspired by their work),
// I modified it to use the most important aspects of their algorithm in 36 Points.

// This project is using counters on pixels and this is different.
// It's using the set of parameters from 36 Points, some points work well, some don't, I had to tune stuff and I kept what worked.
// It's introducing spatial interpolation between points and a lot of interaction features.


//========================================================================
int main( ){
    // Uses compute shaders which are only supported since
    // openGL 4.3
    ofGLWindowSettings settings;
    settings.setGLVersion(4,3);
    settings.setSize(1024,576);
    settings.windowMode = OF_FULLSCREEN;
    ofCreateWindow(settings);

    ofRunApp(new ofApp());
}
