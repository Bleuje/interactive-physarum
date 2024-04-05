#include "ofMain.h"
#include "ofApp.h"

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// by Etienne Jacob

// Crediting:
// Work derived from mxsage's 36 points but with a different implementations
// Before studying mxsage's I already had a physarum implementation
// I modified my implementation to use their technique
// I'm using counters on pixels and this is different
// I'm using the set of parameters from 36 Points, some work well, some don't, I had to tune stuff
// This is also introducing interaction with gamepad


//========================================================================
int main( ){
    // this example uses compute shaders which are only supported since
    // openGL 4.3
    ofGLWindowSettings settings;
    settings.setGLVersion(4,3);
    settings.setSize(1024,576);
    settings.windowMode = OF_FULLSCREEN;
    ofCreateWindow(settings);


    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());

}
