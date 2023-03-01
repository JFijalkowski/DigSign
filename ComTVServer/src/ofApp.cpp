#include "ofApp.h"
#include <string>
#include <iostream>
#include "ofPixels.h"

using namespace std;

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetBackgroundColor(20);

	// setup the server to listen on 11999
	ofxTCPSettings settings(11999);

	// set other options
	//settings.blocking = false;
	//settings.reuse = true;
	//settings.messageDelimiter = "\n";

	TCP.setup(settings);

	// optionally set the delimiter to something else.  The delimiter in the client and the server have to be the same, default being [/TCP]
	TCP.setMessageDelimiter("\n");
	lastSent = 0;

	gui.setup();
	gui.add(clientPing.setup("Ping client"));

}

//--------------------------------------------------------------
void ofApp::update() {
	// for each client lets send them a message letting them know what port they are connected on
	// we throttle the message sending frequency to once every 100ms
	uint64_t now = ofGetElapsedTimeMillis();
	if (now - lastSent >= 100) {
		for (int i = 0; i < TCP.getLastID(); i++) {
			if (!TCP.isClientConnected(i)) continue;

			TCP.send(i, "hello client - you are connected on port - " + ofToString(TCP.getClientPort(i)));

			// receive all the available messages, separated by \n
			// and keep only the last one
			string str;
			string tmp;
			do {
				str = tmp;
				tmp = TCP.receive(i);
			} while (tmp != "");
			if (str == "Client Connected!") {
				//client has just connected
				//load image to send
				string imagename = "testimg.jpg";
				img.load(imagename);
				//convert image to bytestream
				int width = img.getWidth();
				int height = img.getHeight();

				string type = ofToString(img.getImageType());
				TCP.send(i, "Sending Image");
				TCP.send(i, (ofToString(width) + "," + ofToString(height) + "," + type));
				//img.getPixels().getPixels();
				TCP.sendRawBytes(i, (const char*)img.getPixels().getData(), (width * height * 3));
			}
			lastSent = now;
		}

	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(220);
	ofDrawBitmapString("COM TV SERVER \nConnect on port: " + ofToString(TCP.getPort()), 10, 20);
	ofDrawBitmapString("Connected Clients : " + ofToString(TCP.getNumClients()), 10, 50);
	ofSetColor(0);
	ofDrawRectangle(10, 60, ofGetWidth() - 24, ofGetHeight() - 65 - 15);

	ofSetColor(220);

	// for each connected client lets get the data being sent and lets print it to the screen
	for (unsigned int i = 0; i < (unsigned int)TCP.getLastID(); i++) {

		if (!TCP.isClientConnected(i))continue;

		// give each client its own color
		ofSetColor(255 - i * 30, 255 - i * 20, 100 + i * 40);

		// calculate where to draw the text
		int xPos = 15;
		int yPos = 80 + (12 * i * 4);

		// get the ip and port of the client
		string port = ofToString(TCP.getClientPort(i));
		string ip = TCP.getClientIP(i);
		string info = "client " + ofToString(i) + " -connected from " + ip + " on port: " + port;


		// if we don't have a string allocated yet
		// lets create one
		if (i >= storeText.size()) {
			storeText.push_back(string());
		}

		// receive all the available messages, separated by \n
		// and keep only the last one
		string str;
		string tmp;
		do {
			str = tmp;
			tmp = TCP.receive(i);
		} while (tmp != "");

		// if there was a message set it to the corresponding client
		if (str.length() > 0) {
			storeText[i] = str;
		}

		// draw the info text and the received text bellow it
		ofDrawBitmapString(info, xPos, yPos);
		if (str == "Client Connected") {
			ofDrawBitmapString("Client connected", 100, 100);
		}
		ofDrawBitmapString(storeText[i], 25, yPos + 20);
	}
	gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	//add UI elements for different features
	
	//send update ping (loop through all images the client doesn't have)
		//first message contains what the image is called, its size etc (metadata)
		//second message is the image
		//stego would need to be applied before sending,
		//then client can either decode directly from the stream being sent or check the image after receiving it
		//client can send back msg when it's successfully read the image
	
	//change image order 
		//send new list (of image names) to client
		//may need its own UI element for this, either dragging&dropping, typing in some text box or selecting order from dropdowns

	//update/replace image - may be needed, but not 100% necessary

	//remove image
		//specify image to be deleted, and pass in a new image order (just with the removed image taken out)



}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
