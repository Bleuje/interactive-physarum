#include "ofApp.h"

void ofApp::drawTextBox(const std::string &stringToShow, ofTrueTypeFont *pFont, float col, float alpha)
{
    ofPushMatrix();
    ofSetColor(col, 150);
    ofTranslate(-10 * u, -32 * u);
    ofDrawRectangle(0, 0, 20 * u + pFont->stringWidth(stringToShow), 41 * u);
    ofPopMatrix();

    ofSetColor(255 - col);
    ofPushMatrix();
    pFont->drawString(stringToShow, 0, 0);
    ofPopMatrix();
}

std::string ofApp::roundedString(float value)
{
    std::stringstream stream;
    // Set fixed-point notation and round to three decimal places
    stream << std::fixed << std::setprecision(DIGITS_PRECISION) << value;
    std::string result = stream.str();
    return result;
}

void ofApp::drawCustomCircle(ofVec2f pos, float R, float r)
{
    int mCircle = 14;
    float r2 = ofMap(R, 0, 700, 0.5 * r, 1.5 * r) * 1.0;

    R *= u;

    float alphaFactor = 1.0;
    if (ACTIVATE_PEN_FADE)
    {
        alphaFactor = ofMap(getTime() - penMoveLatestTime, 0, PEN_FADE_DURATION, 1, 0, true);
    }

    for (int i = 0; i < mCircle; i++)
    {
        float rot = 0.3 * sin(PI * getTime() * 0.18) + ofMap(i, 0, mCircle, 0, TWO_PI);

        ofPushMatrix();
        ofTranslate(pos.x, pos.y);
        ofRotateRad(rot);
        ofTranslate(R, 0);
        ofScale(u);

        ofFill();
        ofSetRectMode(OF_RECTMODE_CENTER);
        ofSetColor(255, 120 * alphaFactor);
        ofDrawRectangle(ofVec2f(0, 0), r2 + 3, r2 * 6 + 3);
        ofSetColor(0, 190 * alphaFactor);
        ofDrawRectangle(ofVec2f(0, 0), r2, r2 * 6);
        ofSetRectMode(OF_RECTMODE_CORNER);
        ofPopMatrix();
    }
}

// draw pad with arrows to indicate controls
void ofApp::drawPad(float col, float alpha)
{
    ofPushMatrix();

    ofScale(u);

    for (int i = 0; i < 4; i++)
    {
        ofPushMatrix();
        ofRotateRad(HALF_PI * i);
        float rectangleHeight = 25;
        ofSetColor(255 - col, 255 * pow(alpha / 255.0, 2.0));
        ofDrawRectangle(0, -rectangleHeight / 2, 30, rectangleHeight);
        ofSetColor(col, 255 * pow(alpha / 255.0, 2.0));
        ofBeginShape();
        ofVertex(16, rectangleHeight / 3);
        ofVertex(25, 0);
        ofVertex(16, -rectangleHeight / 3);
        ofEndShape(true);
        ofPopMatrix();
    }
    ofPopMatrix();
}