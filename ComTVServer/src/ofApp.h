#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxGui.h"
#include "ofImage.h"
#include "ofPixels.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		void getAvailableImages();

		void drawControlPanel(int x, int y, int clientID, int backgroundColour[3]);
		tuple<int, int, int, int> drawButton(int x, int y, int width, int height, int backgroundColour[3], string text);
		bool checkCollides(int x, int y, tuple<int, int, int, int> buttonCoords);

		ofxTCPServer TCP;

		ofTrueTypeFont  mono;
		ofTrueTypeFont  monosm;

		ofxPanel gui;
		ofxButton clientPing;

		vector <string> storeText;
		vector <string> images;
		uint64_t lastSent;
		ofImage img;
		int imgSize = 0;

		map <int, int> clientStatuses;
		map <int, string> displayedImages;
		map <int, tuple<int, int, int, int>> refreshButtons;

		
};
