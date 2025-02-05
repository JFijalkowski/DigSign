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
		uint64_t lastSent;

		ofxPanel gui;
		ofxButton clientPing;

		//link client id to its status
		map <int, int> clientStatuses;
		//link client id to image displayed on client (to draw thumbnail of what's displaying)
		map <int, string> displayedImages;

		void handleClient(int clientStatus, int clientID, string lastMessage);


		void drawControlPanel(int x, int y, int clientID, int backgroundColour[3]);
		tuple<int, int, int, int> drawButton(int x, int y, int width, int height, int backgroundColour[3], string text);
		bool checkCollides(int x, int y, tuple<int, int, int, int> buttonCoords);

		
		//list/vector of images in filesystem
		vector <string> images;
		void getAvailableImages();
		
		ofImage img;
		int imgSize = 0;
		
		//link client id to img send queue (for sending multiple images while refreshing)
		//value is number of images remaining in queue (send all available images in order)
		map <int, int> imgQueue;


		string authKey = "H2SYBBKWTJ";
		ofImage encodeImage(ofImage image, string key);


		//link client id to coords for its refresh button 
		map <int, tuple<int, int, int, int>> refreshButtons;
		map <int, tuple<int, int, int, int>> scheduleButtons;
};
