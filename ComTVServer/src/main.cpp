#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context
	// current goals:
	// client and server files
	// communicate over same wifi network
	// send custom bit stream 
	// (perhaps a very small image that is then displayed)
	// server gets pings of what devices are connected
	// client gets pings of what server it's connected to

	// store received images in local directory
	// display images from local directory
	// then move onto infrequent (or a button for testing) pings of server for new images
	// 
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
