#include "ofApp.h"

#define RECONNECT_TIME 400

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetBackgroundColor(230);

	// our send and recieve strings
	msgTx = "";
	msgRx = "";

	ofxTCPSettings settings("127.0.0.1", 11999);

	// set other options:
	//settings.blocking = false;
	//settings.messageDelimiter = "\n";

	// connect to the server - if this fails or disconnects
	// we'll check every few seconds to see if the server exists
	tcpClient.setup(settings);

	// optionally set the delimiter to something else.  The delimiter in the client and the server have to be the same
	tcpClient.setMessageDelimiter("\n");

	connectTime = 0;
	deltaTime = 0;

	connectStatus = 0;
	imgReceiveStatus = 0;
}

//--------------------------------------------------------------
void ofApp::update(){
	if (tcpClient.isConnected()) {
		//client has just connected to server
		if (connectStatus == 1) {
			tcpClient.send("Client connected!");
		}
		// we are connected - lets try to receive from the server
		string str = tcpClient.receive();
		if (str.length() > 0) {
			if (str == "Sending Image" && imgReceiveStatus == 0) {
				//begin receiving image
			}
			//"Sending image" -> status = 1
			//width,height,type -> status = 2
			//image (3xwidthxheight bytes) -> status = 0 (after receiving the whole bytestream)

			//convert raw bytes to image
			//convert type string to imgtype 
		}

		//while connected, do poll to server every (?) 10 seconds
		//
	}
	else {
		msgTx = "";
		// if we are not connected lets try and reconnect every 5 seconds
		deltaTime = ofGetElapsedTimeMillis() - connectTime;

		if (deltaTime > 5000) {
			tcpClient.setup("127.0.0.1", 11999);
			tcpClient.setMessageDelimiter("\n");
			connectTime = ofGetElapsedTimeMillis();
			connectStatus = 1;
		}

	}
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(20);
	ofDrawBitmapString("openFrameworks TCP Send Example", 15, 30);

	if (tcpClient.isConnected()) {
		if (!msgTx.empty()) {
			ofDrawBitmapString("sending:", 15, 55);
			ofDrawBitmapString(msgTx, 85, 55);
		}
		else {
			ofDrawBitmapString("status: type something to send data to port 11999", 15, 55);
		}
		ofDrawBitmapString("from server: \n" + msgRx, 15, 270);
	}
	else {
		ofDrawBitmapString("status: server not found. launch server app and check ports!\n\nreconnecting in " + ofToString((5000 - deltaTime) / 1000) + " seconds", 15, 55);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs& key){
	// you can only type if you're connected
	// we accumulate 1 line of text and send every typed character
	// on the next character after a breakline we clear the buffer
	if (tcpClient.isConnected()) {
		if (key.key == OF_KEY_BACKSPACE || key.key == OF_KEY_DEL) {
			if (!msgTx.empty()) {
				msgTx = msgTx.substr(0, msgTx.size() - 1);
			}
		}
		else if (key.codepoint != 0) {
			ofUTF8Append(msgTx, key.codepoint);
		}
		tcpClient.send(msgTx);
		if (!msgTx.empty() && msgTx.back() == '\n') {
			msgTx.clear();
		}
	}
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
