#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:



	// each client machine has a directory with all current content
	// when polling server for new content, check against all existing files 
	// (unless server knows already what's on each client, therefore will only send things that are not present)
	// server should know which files/content are on each client, and in what order they're being displayed
	// therefore, control can be easily set up
	// commands to send to clients:
	// add new content
	// replace existing file (?)
	// remove file
	// change order (sends a complete new order which overrides existing)
	// stop/start/turn off etc (shutoff command)

	ofRunApp(new ofApp());

}
