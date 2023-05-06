#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(ofKeyEventArgs& key);
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
		
		ofxTCPClient tcpClient;
		string msgTx, msgRx;

		ofTrueTypeFont  mono;
		ofTrueTypeFont  monosm;

		float counter;
		int connectTime;
		int deltaTime;


		//client's instruction status
		int status;


		void parseMetadata(string message);
		int imgWidth;
		int imgHeight;
		int imgSize;
		ofImage receivedImg;
		ofImageType receivedImgType;
		string receivedImgName;
		vector <string> msgStore;

		

		//schedule of display
		//position in list = order displayed
		//stores tuple of image name and display duration
		//when switching to new image, timer is set to attached duration value
		vector<tuple<string, int>> schedule;

		
		//image being displayed
		ofImage image;
		ofImage fadeImage;

		//fading between images
		int getNextImage();
		bool fading;
		int fadeDuration = 2000;
		int fadeStart;
		int fadeElapsed;
		

		//image (number) being displayed currently (position counter for scheduling)
		int displayedImgNum;
		//duration to display current image (in milliseconds)
		int displayTime;

		//time current image was displayed
		int displayStartTime;
		//time current image has been displayed for
		int displayElapsedTime;



	int size;
};
