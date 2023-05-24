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
//add some constants for messages (eg: "Client Connected" so no mismatch occurs)
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


	//get images from google drive

	getAvailableImages();
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
		//cout << img.getPixels().getData();
		//format: width,height,filename, filesize
		TCP.send(clientID, (ofToString(width) + "," + ofToString(height) + "," + imgName + "," + ofToString(imgSize)));
		cout << "Sent Metadata\n";
		clientStatuses[clientID] = SEND_IMG_DATA;
	}

	else if (clientStatus == SEND_IMG_DATA) {
		ofPixels imgPixels = img.getPixels();
		unsigned char* imgData = imgPixels.getData();
		cout << "imgSize: " << imgSize << "\n";
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

		//TCP.sendRawBytes(i, (const char*)imgData, imgSize);
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

	//send new display schedule to client - chain with refresh?
	if (clientStatus == SEND_SCHEDULE) {
		//message format: imgName,duration | imgName,duration
		//pipes to separate image-duration pairs, commas to separate image name and duration
		//create GUI to enter schedule?
		string schedule = "regent.jpg,3000|diamond.jpg,3000|interior.jpg,3000";
		TCP.send(clientID, schedule);
		cout << schedule << "\n";
		cout << "Sent schedule \n";
		clientStatuses[clientID] = IDLE;
	}
}


//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(220);
	ofDrawBitmapString("COM TV SERVER \nConnect on port: " + ofToString(TCP.getPort()), 10, 20);
	ofDrawBitmapString("Connected Clients : " + ofToString(TCP.getNumClients()), 10, 50);
	ofSetColor(0);
	//ofDrawRectangle(10, 60, ofGetWidth() - 24, ofGetHeight() - 65 - 15);

	ofSetColor(220);
	int boxcolour[3] = { 100,100,100 };

	
	//clear button collection
	refreshButtons = {};

	int connectedClientNum = 0;

	// for each connected client lets get the data being sent and lets print it to the screen
	for (unsigned int i = 0; i < (unsigned int)TCP.getLastID(); i++) {
		if (!TCP.isClientConnected(i))continue;

		
		// give each client its own color
		int r = 255 - i * 30;
		int g = 255 - i * 20;
		int b = 100 + i * 40;

		ofSetColor(255 - i * 30, 255 - i * 20, 100 + i * 40);
		int panelColour[] = { r,g,b };
		drawControlPanel(panelCoords[connectedClientNum][0], panelCoords[connectedClientNum][1], i, panelColour);

		// calculate where to draw the text
		int xPos = 15;
		int yPos = 80 + (12 * i * 4);

		// get the ip and port of the client
		string port = ofToString(TCP.getClientPort(i));
		string ip = TCP.getClientIP(i);
		string info = "client " + ofToString(i) + " -connected from " + ip + " on port: " + port;

	}
	//gui.draw();
}

//method to draw a controls box for a connected client at a given coordinate
void ofApp::drawControlPanel(int x, int y, int clientID, int backgroundColour[3]) {
	// draw client bounding box
	ofSetColor(backgroundColour[0],backgroundColour[1],backgroundColour[2]);
	ofDrawRectangle(x, y, clientPanelSize, clientPanelSize);
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
	// draw preview of displayed image
	// small green square/corner to identify it's running (may be already implied that it's running since unconnected clients will not be drawn)
	// maybe have a popup gui that's created when neede	d - allows you to input image display orders (probably)
	//add coordinates for buttons to data structure for buttons (and which client they belong to)
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
//datastructure to store button info should be:
//split by button function
//key matches client id, so same value can be used to determine which client to send messages to
// 

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
		//change image order 
		//send new list (of image names) to client
		//may need its own UI element for this, either dragging&dropping, typing in some text box or selecting order from dropdowns
		int mousePos = x;
	//update/replace image - may be needed, but not 100% necessary

	//remove image
		//specify image to be deleted, and pass in a new image order (just with the removed image taken out)
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
