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
		if (connectStatus == 0) {
			tcpClient.send("Client connected!");
			connectStatus = 1;
		}
		// client waiting for bytestream
		if (imgReceiveStatus == 2) {
			//number of bytes in the image, assuming 3-channel colour (JPG)
			// 
			//width x height x 3 when image type is JPG
			int imgSize = imgHeight * imgWidth * 3;
			if (receivedImageType == OF_IMAGE_COLOR_ALPHA) {
				//if png has been sent, 4 bytes per pixel
				imgSize = imgHeight * imgWidth * 4;
			}
			
			char* receivedBytes{};
			ofBuffer buffer;
			buffer.allocate(imgSize);
			int totalBytes = 0;
			while (totalBytes < imgSize) {
				int result = tcpClient.receiveRawBytes(&buffer.getBinaryBuffer()[totalBytes], imgSize - totalBytes);
				if (result > 0) {
					totalBytes += result;
					if (totalBytes == imgSize) {
						receivedImg.setFromPixels((unsigned char*)buffer.getData() , imgWidth, imgHeight, receivedImageType);
						receivedImg.save(receivedImgName);
						imgReceiveStatus = 0;
					}
				}
			}
		}
		//otherwise client will be receiving string values
		else {
			// receive string/text message from server
			string str = tcpClient.receive();
			
			if (str.length() > 0) {
				msgStore.push_back(str);
				if (str == "Sending Image" && imgReceiveStatus == 0) {
					//begin receiving image
					imgReceiveStatus = 1;
				}
				else if (imgReceiveStatus == 1) {
					//expecting metadata string = width, height, filename
					
					//split on commas to get list/vector of values
					vector <string> metadata = ofSplitString(str, ",");

					//if message has split into 3 values, (assume) message is the metadata
					if (metadata.size() == 3) {
						imgWidth = ofToInt(metadata[0]);
						imgHeight = ofToInt(metadata[1]);
						receivedImgName = metadata[2];
						string filetype = receivedImgName.substr(receivedImgName.size() - 3);
						receivedImageType = OF_IMAGE_COLOR;
						if (filetype == "png") {
							receivedImageType = OF_IMAGE_COLOR_ALPHA;
						}
						//image metadata has been received, wait for image bytestream
						imgReceiveStatus = 2;
					}
				}
			}
		}
		

		//while connected, do poll to server every (?) 10 seconds
		//
	}
	else {
		connectStatus = 0;
		msgTx = "";
		// if we are not connected lets try and reconnect every 5 seconds
		deltaTime = ofGetElapsedTimeMillis() - connectTime;
		if (deltaTime > 5000) {
			tcpClient.setup("127.0.0.1", 11999);
			tcpClient.setMessageDelimiter("\n");
			connectTime = ofGetElapsedTimeMillis();
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
	//draw received messages from server
	for (unsigned int i = 0; i < msgStore.size(); i++) {
		ofDrawBitmapString(msgStore[i], 300, 300 + (15 * i));
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
