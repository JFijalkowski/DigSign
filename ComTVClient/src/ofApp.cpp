#include "ofApp.h"
#include <iostream>

#define RECONNECT_TIME 400

//STATUS CODES
const int DISCONNECTED = -1;
const int IDLE = 0;
const int RECEIVE_IMG_METADATA = 2;
const int RECEIVE_IMG_DATA = 3;
const int SAVE_IMG_DATA = 4;
const int RECEIVE_DISPLAY_SCHEDULE = 5;
const int REMOVE_IMAGE = 6;

map<int, string> statusNames = {
	{IDLE, "Idle"},
	{RECEIVE_IMG_METADATA, "Receiving Image Metadata..."},
	{RECEIVE_IMG_DATA, "Receiving Image Data..."},
	{SAVE_IMG_DATA, "Saving Image..."},
	{RECEIVE_DISPLAY_SCHEDULE, "Receiving Display Schedule"},
	{REMOVE_IMAGE, "Removing Image"}};



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

	status = DISCONNECTED;

	schedule = { {"diamond.jpg", 5000}, {"regent.jpg", 5000} };
	displayTime = get<1>(schedule[0]);
	image.load(get<0>(schedule[0]));
	displayElapsedTime = 0;
	displayStartTime = ofGetElapsedTimeMillis();
	fading = false;
}

//--------------------------------------------------------------
void ofApp::update(){
	if (tcpClient.isConnected()) {

		//client has just connected to server
		if (status == DISCONNECTED) {
			tcpClient.send("Client connected!");
			cout << "Connected! \n";
			status = IDLE;
		}
		// client waiting for bytestream
		if (status == RECEIVE_IMG_DATA) {

			ofBuffer buffer;
			buffer.allocate(imgSize);
			int receivedBytes = 0;
			int remainingBytes = imgSize;
			int messageSize = 256;
			cout << "Started img receive \n";
			

			//listen for message chunks until all bytes received
			while (remainingBytes > 0) {
				//receive full size chunk
				if (remainingBytes > messageSize) {
					int result = tcpClient.receiveRawBytes((char*)&buffer.getBinaryBuffer()[receivedBytes], messageSize);
					if (result > 0){
						remainingBytes -= messageSize;
						receivedBytes += messageSize;
					}
				}
				//receive last chunk
				else {
					cout << "one chunk left: " << remainingBytes << "\n";
					int result = tcpClient.receiveRawBytes((char*)&buffer.getBinaryBuffer()[receivedBytes], remainingBytes);
					if (result > 0) {
						receivedBytes += remainingBytes;
						remainingBytes = 0;
					}
				}
			}
			//if all data obtained, save image
			if (remainingBytes==0) {
				cout << "saving image\n";
				receivedImg.setFromPixels((unsigned char*)buffer.getData(), imgWidth, imgHeight, receivedImgType);
				
				status = SAVE_IMG_DATA;
			}
		}
		//prioritise saving image before receiving new data
		else if (status == SAVE_IMG_DATA) {
			if (decodeImage(receivedImg, authKey)) {
				receivedImg.save(receivedImgName);
				cout << "Image has been saved\n";
			}
			//send confirmation now image has been received
			tcpClient.send("Image Received");
			status = IDLE;
			
		}
		
		//otherwise client will be receiving string values
		else {
			// receive string/text message from server
			string str = tcpClient.receive();
			
			//if message received from server
			if (str.length() > 0) {
				cout << "received: \n";
				cout << str << "\n";
				msgStore.push_back(str);

				if (str == "Sending Image" && status == IDLE) {
					//begin receiving image (starting with metadata
					status = RECEIVE_IMG_METADATA;
				}

				else if (str == "Send Schedule" && status == IDLE) {
					//receive new image schedule
					status = RECEIVE_DISPLAY_SCHEDULE;
				}

				else if (status == RECEIVE_IMG_METADATA) {
					parseMetadata(str);
				}

				else if (status == RECEIVE_DISPLAY_SCHEDULE) {
					schedule = parseSchedule(str);
					for (tuple<string,int> pair : schedule) {
						cout << get<0>(pair) << get<1>(pair) << "\n";
					}
					status = IDLE;
				}
			}
		}
		

		//while connected, do poll to server every (?) 10 seconds
		//
	}
	else {
		status = DISCONNECTED;
		msgTx = "";
		// if not connected try and reconnect every 5 seconds
		deltaTime = ofGetElapsedTimeMillis() - connectTime;
		if (deltaTime > 5000) {
			tcpClient.setup("127.0.0.1", 11999);
			tcpClient.setMessageDelimiter("\n");
			connectTime = ofGetElapsedTimeMillis();
		}

	}
}

//steganography: decode steganography-encoded image authentication key into image data using Least Significant Bit
//1 bit of key is encoded per byte of image data
//starts at front of image data
bool ofApp::decodeImage(ofImage image, string key) {
	//get input image pixel data (list of chars)
	unsigned char* pix = image.getPixels().getData();
	string receivedKey = "";
	//index of pixel data to be decoded from
	int decoded = 0;
	//decode per character from key
	for (int i = 0; i < key.length(); i++) {
		//build binary string of encoded char
		string charBinary = "";

		//get each bit of encoded char
		for (int j = 0; j <8; j++) {
			string encodedPixelByte = ofToBinary(pix[decoded]);
			//get last bit, append to bit string
			charBinary += encodedPixelByte[7];
			cout << "byte: " << ofToBinary(pix[decoded]) << "\n";
			cout << "bit: " << encodedPixelByte[7] << "\n";
			//increment decoding count
			decoded++;
		}
		//add decoded character to received key
		receivedKey += ofBinaryToChar(charBinary);
		cout << "\n" << ofBinaryToChar(charBinary) << "\n";
	}
	cout << "\n Received Key: " << receivedKey <<"\n";
	return (receivedKey == key);
}

void ofApp::parseMetadata(string message) {
	//expecting metadata string = width, height, filename, filesize (in bytes)

	//split on commas to get list/vector of values
	vector <string> metadata = ofSplitString(message, ",");

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
		status = RECEIVE_IMG_DATA;
	}
}

vector<tuple<string, int>> ofApp::parseSchedule(string message) {
	vector<string> pairs = ofSplitString(message, "|");
	vector<tuple<string, int>> newSchedule = {};
	for (string pair : pairs) {
		vector<string> data = ofSplitString(pair, ",");
		newSchedule.push_back({ data[0], ofToInt(data[1]) });
		cout << pair << "\n";
	}
	return newSchedule;
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
			ofDrawBitmapString("status:" + (statusNames[status]), 15, 55);
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

	//reset colour for drawing image (affects image tint)
	ofSetColor(255);

	int currentTime = ofGetElapsedTimeMillis();
	int width = ofGetWindowWidth();
	int height = ofGetWindowHeight();
	//draw stored images on schedule
	displayElapsedTime = currentTime - displayStartTime;
	fadeElapsed = currentTime - fadeStart;
	//if image has not been displayed for full scheduled time yet
	if (displayElapsedTime < displayTime) {
		//draw image as normal
		image.draw(0, 0, width, height);
	}

	//blend images if doing fade
	else if (fading && (fadeElapsed < fadeDuration)) {
		int fade = ((float) fadeElapsed / fadeDuration) * 255;

		ofSetColor(255, 255, 255, 255-fade);
		image.draw(0, 0, width, height);

		ofSetColor(255, 255, 255, fade);
		fadeImage.draw(0, 0, width, height);
	}

	//if image had finished fade, set up new image
	else if (fading && (fadeElapsed >= fadeDuration)) {
		ofDisableAlphaBlending();
		cout << "finished fade \n";
		fading = false;
		
		displayedImgNum = getNextImage();

		//update current display start time
		displayStartTime = currentTime;
		//get filename and duration for new image
		tuple<string, int> newSchedule = schedule[displayedImgNum];
		//load new image
		//image.load(get<0>(newSchedule));
		image = fadeImage;
		image.draw(0, 0, width, height);
		//set display duration
		displayTime = get<1>(newSchedule);
		
		cout << "displaying image: " << get<0>(newSchedule) << "\n";
	}

	//image has displayed for scheduled time, set up fade to next image
	else {
		cout << "Started Fade \n";
		image.draw(0, 0, width, height);
		//set next image in queue to fade-in
		fadeImage.load(get<0>(schedule[getNextImage()]));
		cout << get<1>(schedule[getNextImage()]);
		//set start of fade
		fadeStart = currentTime;
		fading = true;
		ofEnableAlphaBlending();

	}
}

int ofApp::getNextImage() {
	int imgNum = displayedImgNum + 1;
	//if end of schedule, start at beginning
	if (imgNum >= schedule.size()) {
		imgNum = 0;
	}
	return imgNum;
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
