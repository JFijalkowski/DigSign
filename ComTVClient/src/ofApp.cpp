#include "ofApp.h"
#include <iostream>

#define RECONNECT_TIME 400

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetBackgroundColor(255);

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

	schedule = { {"testimg.jpg", 5000}, {"testimg2.jpg", 5000} };
}

//--------------------------------------------------------------
void ofApp::update(){
	if (tcpClient.isConnected()) {
		//client has just connected to server
		if (connectStatus == 0) {
			tcpClient.send("Client connected!");
			cout << "Connected! \n";
			connectStatus = 1;
		}
		// client waiting for bytestream
		if (imgReceiveStatus == 2) {

			bool receivedAll = false;
			ofBuffer buffer;
			buffer.allocate(imgSize);
			int receivedBytes = 0;
			int remainingBytes = imgSize;
			int messageSize = 256;
			cout << "Started img receive \n";
			//listen for message chunks until all bytes received
			while (remainingBytes > 0) {
				cout << remainingBytes << "\n";
				if (remainingBytes > messageSize) {
					int result = tcpClient.receiveRawBytes((char*)&buffer.getBinaryBuffer()[receivedBytes], messageSize);
					if (result > 0){
						remainingBytes -= messageSize;
						receivedBytes += messageSize;
					}
				}
				else {
					cout << "last chunk left: " << remainingBytes << "\n";
					tcpClient.receiveRawBytes((char*)&buffer.getBinaryBuffer()[receivedBytes], remainingBytes);
					receivedBytes += remainingBytes;
					remainingBytes = 0;
					receivedAll = true;
				}
			}
			//if all data obtained, save image
			if (receivedAll) {
				cout << "saving image\n";
				receivedImg.setFromPixels((unsigned char*)buffer.getData(), imgWidth, imgHeight, receivedImgType);
				receivedImg.save(receivedImgName);
				//send confirmation
				tcpClient.send("Image Received");
				imgReceiveStatus = 0;
			}
		}
		
		//otherwise client will be receiving string values
		else {
			// receive string/text message from server
			string str = tcpClient.receive();
			
			if (str.length() > 0) {
				cout << "received: \n";
				cout << str << "\n";
				msgStore.push_back(str);
				if (str == "Sending Image" && imgReceiveStatus == 0) {
					//begin receiving image
					imgReceiveStatus = 1;
				}
				else if (imgReceiveStatus == 1) {
					//expecting metadata string = width, height, filename, filesize (in bytes)
					
					//split on commas to get list/vector of values
					vector <string> metadata = ofSplitString(str, ",");

					//if message has split into 4 values, (assume) message is the metadata
					if (metadata.size() == 4) {
						imgWidth = ofToInt(metadata[0]);
						imgHeight = ofToInt(metadata[1]);
						receivedImgName = metadata[2];
						//imgSize automatically adjusts if more bytes per pixel (Eg: PNG)
						imgSize = ofToInt(metadata[3]);
						string filetype = receivedImgName.substr(receivedImgName.size() - 3);
						receivedImgType = OF_IMAGE_COLOR;
						if (filetype == "png") {
							receivedImgType = OF_IMAGE_COLOR_ALPHA;
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
			ofDrawBitmapString("status:" + ofToString(imgReceiveStatus), 15, 55);
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



	//draw stored images on schedule
	displayElapsedTime = ofGetElapsedTimeMillis() - displayStartTime;
	//if image has not been displayed for full scheduled time yet
	if (displayElapsedTime < displayTime) {
		//for now, just draw image as normal size (may need to be stretched to fit, or cropped for full-size)
		image.draw(500, 500);
	}
	//current image has been displayed for scheduled duration
	else {
		displayedImgNum++;
		//if end of schedule, start at beginning
		if (displayedImgNum >= schedule.size()) {
			displayedImgNum = 0;
		}
		//update current display start time
		displayStartTime = ofGetElapsedTimeMillis();
		//get filename and duration for new image
		tuple<string, int> newSchedule = schedule[displayedImgNum];
		//load new image
		image.load(get<0>(newSchedule));
		//set display duration
		displayTime = get<1>(newSchedule);
		cout << "displaying image: " << get<0>(newSchedule) << "\n";
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
