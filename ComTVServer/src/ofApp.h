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
		
		ofxTCPServer TCP;

		ofTrueTypeFont  mono;
		ofTrueTypeFont  monosm;

		ofxPanel gui;
		ofxButton clientPing;

		vector <string> storeText;
		uint64_t lastSent;
		ofImage img;

		map <int, int> clientStatuses;
		map <int, tuple<int, int, int, int>> refreshButtons;


		const int IDLE = 0;
		const int START_IMAGE_SEND = 1;
		const int SEND_IMG_METADATA = 2;
		const int SEND_IMG_DATA = 3;
		const int SEND_DISPLAY_SCHEDULE = 4;
		const int REMOVE_IMAGE = 5;
};
