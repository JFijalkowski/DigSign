#include "ofApp.h"
#include <string>
#include <iostream>
#include "ofPixels.h"

using namespace std;
//constants

//client status codes
const int IDLE = 0;
const int START_IMG_SEND = 1;
const int SEND_IMG_METADATA = 2;
const int SEND_IMG_DATA = 3;
const int SENT_IMG_DATA = 4;
const int SEND_SCHEDULE = 5;
const int REMOVE_IMAGE = 6;

map<int,string> statusNames = {
	{IDLE, "Idle"},
	{START_IMG_SEND, "Starting Image Send..."},
	{SEND_IMG_METADATA, "Sending Image Metadata..."},
	{SEND_IMG_DATA, "Sending Image Data..."},
	{SENT_IMG_DATA, "Sent Image Data, awaiting confirmation..."},
	{SEND_SCHEDULE, "Sending Display Schedule"},
	{REMOVE_IMAGE, "Removing Image" }};


ofColor paleGreen(159, 255, 148);
ofColor paleBlue(117, 202, 255);
ofColor paleYellow(255, 253, 148);
ofColor paleOrange(255, 212, 148);
ofColor paleRed(255, 163, 150);

map<int, ofColor> statusColours = {
	{IDLE, paleGreen},
	{START_IMG_SEND, paleBlue},
	{SEND_IMG_METADATA, paleOrange},
	{SEND_IMG_DATA, paleYellow},
	{SENT_IMG_DATA, paleRed},
	{SEND_SCHEDULE, paleYellow},
	{REMOVE_IMAGE, paleRed } };

//TODO: add some constants for messages (eg: "Client Connected" so no mismatch occurs)
//removes need to directly type message strings, and prevents capitalisation/typo errors


//user display dimension constants
const int screenWidth = 1000;
const int screenHeight = 800;
const int clientPanelSize = 200;
const int previewImageSize = 125;
const int refreshButtonWidth = 200;
const int refreshButtonHeight = 50;

int refreshButtonColour[] = { 50, 150, 50 };
int scheduleButtonColour[] = { 50, 50, 150 };
const int panelCoords[4][2] = { {25,100}, {275, 100}, {525, 100}, {775, 100} };

//--------------------------------------------------------------
void ofApp::setup() {
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


	//get images from google drive

	getAvailableImages();
	/*
	img.load("testimg.jpg");
	cout << img.getPixels().getData() << "\n";
	cout << "Encoding: " << "encoded.jpg" << "\n";
	ofImage newimg = encodeImage(img, "boo");
	newimg.save("encoded.jpg", OF_IMAGE_QUALITY_BEST);
	cout << newimg.getPixels().getData() << "\n";
	newimg.load("encoded.jpg");
	cout << newimg.getPixels().getData()[20] << "\n";
	*/

}

//--------------------------------------------------------------
void ofApp::update() {
	// for each client lets send them a message letting them know what port they are connected on
	// we throttle the message sending frequency to once every 100ms
	uint64_t now = ofGetElapsedTimeMillis();
	if (now - lastSent >= 100) {
		for (int i = 0; i < TCP.getLastID(); i++) {
			if (!TCP.isClientConnected(i)) continue;

			
			// receive all the available messages, separated by \n
			// and keep only the last one
			string str;
			string tmp;
			do {
				str = tmp;
				tmp = TCP.receive(i);
				if (tmp !=""){ cout <<"RECEIVED: " << tmp << "\n"; }
			} while (tmp != "");

			if (str == "Client connected!") {
				//client has just connected, start tracking their status
				clientStatuses[i] = IDLE;
				cout << "Client Connected\n";
			}
			lastSent = now;

			//carry out client action
			int clientStatus = clientStatuses[i];

			handleClient(clientStatus, i, str);

		}

	}
}

//wrapper function for managing communication to/from a particular client
void ofApp::handleClient(int clientStatus, int clientID, string lastMessage) {
	
	//---------------------SENDING AN IMAGE--------------------------

	//notify client image is going to be sent
	if (clientStatus == START_IMG_SEND) {
		TCP.send(clientID, "Sending Image");
		clientStatuses[clientID] = SEND_IMG_METADATA;
		cout << "Sending image\n";
	}
	//load image and send metadata
	else if (clientStatus == SEND_IMG_METADATA) {
		string imgName = images[imgQueue[clientID]-1];
		cout << imgName << "\n";
		//load image to send
		img.load(imgName);
		//convert image to bytestream
		int width = img.getWidth();
		int height = img.getHeight();
		imgSize = width * height * 3;
		//format: width,height,filename, filesize
		TCP.send(clientID, (ofToString(width) + "," + ofToString(height) + "," + imgName + "," + ofToString(imgSize)));
		cout << "Sent Metadata\n";
		clientStatuses[clientID] = SEND_IMG_DATA;
	}

	else if (clientStatus == SEND_IMG_DATA) {
		img = encodeImage(img, authKey);
		ofPixels imgPixels = img.getPixels();
		unsigned char* imgData = imgPixels.getData();
		int bytesRemaining = imgSize;
		int messageSize = 256;
		int bytesSent = 0;
		cout << "Sending image";
		//send image data	to client
		while (bytesRemaining > 1) {
			if (bytesRemaining > messageSize) {
				TCP.sendRawBytes(clientID, (char*)&img.getPixels()[bytesSent], messageSize);
				bytesRemaining -= messageSize;
				bytesSent += messageSize;
			}
			else {
				cout << "Last message: " << bytesRemaining << "\n";
				TCP.sendRawBytes(clientID, (char*)&img.getPixels()[bytesSent], bytesRemaining);
				bytesSent += bytesRemaining;
				bytesRemaining = 0;
			}
		}

		cout << "sent image data\n";
		clientStatuses[clientID] = SENT_IMG_DATA;
	}
	//client sends confirmation message when image has been recieved
	else if (clientStatus == SENT_IMG_DATA && lastMessage == "Image Received") {
		//client is able to receive new instructions
		clientStatuses[clientID] = IDLE;
		cout << "Image sent\n";

		imgQueue[clientID] -= 1;
		//if there's still some images left in the client's queue, start sending again
		if (imgQueue[clientID] > 0) {
			//move to next image in queue
			clientStatuses[clientID] = START_IMG_SEND;

		}
	}

	//send new display schedule to client
	if (clientStatus == SEND_SCHEDULE) {
		//message format: imgName,duration | imgName,duration
		//pipes to separate image-duration pairs, commas to separate image name and duration

		string schedule = "outside.jpg,3000|regent.jpg,3000|diamond.jpg,3000|interior.jpg,3000";
		TCP.send(clientID, schedule);
		cout << "Sent schedule \n" << schedule << "\n";
		clientStatuses[clientID] = IDLE;
	}
}


//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(220);
	ofDrawBitmapString("COM TV SERVER \nConnect on port: " + ofToString(TCP.getPort()), 10, 20);
	ofDrawBitmapString("Connected Clients : " + ofToString(TCP.getNumClients()), 10, 50);
	ofSetColor(0);

	ofSetColor(220);
	
	//clear button collection - prevents unconnected clients being referenced
	refreshButtons = {};

	int connectedClientNum = 0;

	// for each connected client lets get the data being sent and lets print it to the screen
	for (unsigned int i = 0; i < (unsigned int)TCP.getLastID(); i++) {
		if (!TCP.isClientConnected(i))continue;

		// get location to draw the client's control box
		int xPos = panelCoords[connectedClientNum][0];
		int yPos = panelCoords[connectedClientNum][1];

		// give each client its own color
		int r = 255 - i * 30;
		int g = 255 - i * 20;
		int b = 100 + i * 40;

		ofSetColor(255 - i * 30, 255 - i * 20, 100 + i * 40);
		int panelColour[] = { r,g,b };
		drawControlPanel(xPos, yPos, i, panelColour);

	}

	//TODO: add Schedule Creation/edit UI
}

//method to draw a controls box for a connected client at a given coordinate
void ofApp::drawControlPanel(int x, int y, int clientID, int backgroundColour[3]) {
	// draw client bounding box
	ofSetColor(backgroundColour[0],backgroundColour[1],backgroundColour[2]);
	ofDrawRectangle(x, y, clientPanelSize, clientPanelSize);

	//draw status name on box of corresponding colour
	ofSetColor(statusColours[clientStatuses[clientID]]);
	ofDrawRectangle(x, y, clientPanelSize - (refreshButtonWidth / 2), refreshButtonHeight);
	ofSetColor(0);
	ofDrawBitmapString(statusNames[clientStatuses[clientID]], x + 5, y + 15);

	//draw refresh button at bottom of panel
	//add drawn button coords to collection of refresh buttons
	refreshButtons.insert({ 
		clientID, drawButton(x, (y + clientPanelSize - refreshButtonHeight), refreshButtonWidth, refreshButtonHeight, refreshButtonColour, "Refresh") 
		});
	scheduleButtons.insert({
		clientID, drawButton((x + (clientPanelSize/2)), y, (refreshButtonWidth/2), refreshButtonHeight, scheduleButtonColour, "Send Schedule")
		});
	// TODO: draw preview of displayed image

	// get the ip and port of the client
	ofSetColor(0);
	ofDrawBitmapString("Client " + ofToString(clientID), x + 5, y + 65);
	ofDrawBitmapString("Connected from:" + TCP.getClientIP(clientID), x + 5, y + 85);
	ofDrawBitmapString("On Port: " + ofToString(TCP.getClientPort(clientID)), x + 5, y + 105);
}

//draw a button at coordinates x,y of specified width, height and colour
//draws provided text on left of button (with some padding)
//could add an offset parameter to jank in some text alignment
tuple<int, int, int, int> ofApp::drawButton(int x, int y, int width, int height, int backgroundColour[3], string label) {
	
	ofSetColor(backgroundColour[0], backgroundColour[1], backgroundColour[2]);
	ofDrawRectangle(x, y, width, height);
	
	//draw refresh button text
	ofSetColor(255);
	ofDrawBitmapString(label, (x + (width * 0.1)), (y + (height * 0.66)));

	tuple <int, int, int, int> buttonCoords = { x,y, x + width, y + width };
	return buttonCoords;

}

//get names of image files in filesystem and update stored list
void ofApp::getAvailableImages() {
	vector<string> imageList = {};
	ofDirectory dir = ofDirectory("");
	cout << "Getting available images\n";
	for (ofFile file : dir.getFiles()) {
		//add check for just image files (in case other files are accidentally in the directory)
		if ((file.getExtension() == "jpg")||(file.getExtension() == "png")) {
			imageList.push_back(file.getFileName());
		}
	}
	
	images = imageList;
}

//steganography: encode authentication key into image data using Least Significant Bit
//1 bit of key is encoded per byte of image data
ofImage ofApp::encodeImage(ofImage image, string key) {
	//get input image pixel data (list of chars)
	unsigned char *pix = image.getPixels().getData();
	//index of pixel data to be encoded to (encode 1 bit per byte of image)
	int encoded = 0;

	//encode each character from key
	for (int i = 0; i < key.length(); i++ ) {
		char x = key[i];
		//convert char-to-be-encoded to binary string
		string charBinary = ofToBinary(x);

		//encode each bit from char binary string
		for (int j = 0; j < charBinary.length(); j++) {

			//get next pixel to be encoded into
			string newPixelByte = ofToBinary(pix[encoded]);
			//set last bit to new binary value
			newPixelByte[7] = charBinary[j];

			//overwrite original pixel data
			pix[encoded] = ofBinaryToChar(newPixelByte);

			//increment pixel encoding count
			encoded++;
		}
		
	}
	ofPixels newPixels;
	newPixels.setFromPixels(pix, image.getWidth(), image.getHeight(), image.getImageType());
	image.setFromPixels(newPixels);
	return image;
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
	
	//loops through each client, ignores if client is no longer connected
	for (unsigned int i = 0; i < (unsigned int)TCP.getLastID(); i++) {
		if (!TCP.isClientConnected(i))continue;
		int clientStatus = clientStatuses[i];
		bool idle = (clientStatus == IDLE);
		//will only accept new instructions if the client is currently idle
		if (clientStatus != IDLE)continue;
		//----------------------------send image-------------------
		//check if a refresh button has been pressed
		if (checkCollides(x, y, refreshButtons[i])) {
			//update list of available images to send
			getAvailableImages();
			imgQueue[i] = images.size();
			clientStatuses[i] = START_IMG_SEND;
			//call func for refreshing client i
			
			//for now, send image

			//break, as only one button can be pressed at a given time
			break;
		}
		else if (checkCollides(x, y, scheduleButtons[i])) {
			TCP.send(i, "Send Schedule");
			clientStatuses[i] = SEND_SCHEDULE;
			break;
		}
		//check if <button type> button has been pressed ...
		// 
		//Create / edit schedule
		//
		//update/replace image - already done with Refresh
		// 
		//remove image (including reference in schedule)
		// 
	}
}



bool ofApp::checkCollides(int x, int y, tuple<int, int, int, int> buttonCoords) {
	int x1 = get<0>(buttonCoords);
	int y1 = get<1>(buttonCoords);
	int x2 = get<2>(buttonCoords);
	int y2 = get<3>(buttonCoords);


	//check if coordinates lie within specified box
	if ((x > x1) && (x < x2) && (y > y1) && (y < y2)) {
		return true;
	}
	else {
		return false;
	}

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
