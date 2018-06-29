#include <string>
#include <getopt.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <queue>
#include <iostream>
#include <map>
#include <RF24/RF24.h>

#define read_File "toC.txt"
#define write_File "toPy.txt"
#define sizeType unsigned short
#define idType unsigned short
#define testing true
#define testingConverters true

using namespace std;

// Gather RF settings.
RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Declare global variables.
idType  id, numDevices;
sizeType messageSize;
map<idType, string> comms_collection;
bool size;
unsigned int waitTimeId = 10000;	// How long to wait (in ms) for an ID  before assuming lead.

// Pre-declare all base functions.
void receiveMessage();
bool sendMessage(string);
bool writeToFile(string fileName, string message);
queue<string> readFromFile(string);
bool sendSize(sizeType);
bool sendString(string);
sizeType convertSize(string);
string convertId(idType);
idType convertId(string);
void analyzeMessage(idType);

/**********************************
 * Receive Message
 **********************************
 * Pulls in messages and determines
 *	what to do based on circumstances.
 **********************************/
void receiveMessage() {
	if (!radio.available()) {
		return;
	}
	if (testing) {
		printf("Receiving Message.\n");
	}
	if (size) {
		radio.read( &messageSize, sizeof(sizeType) );
		if (messageSize == 0) {
			messageSize = sizeof(sizeType);
			return;
		}
		size = false;
		if (testing) {
			printf("Received size: %d.\n", messageSize);
		}
	}
	else {
		string in;
		idType sentId;

		radio.read( &in, messageSize);
		
		if (testing) {
			printf("Message size= %d.\n", in.length());
		}
		
		if (in.length() <= sizeof(sizeType)) {
			messageSize = convertSize(in);
			return;
		}
		
		if (testing) {
			printf("Received message: %s.\n", in.c_str());
		}
		
		size = true;
		
		if (testing) {
			printf("Grabbing id.\n");
		}
		
		sentId = convertId(in.substr(0, sizeof(idType)));
		
		if (testing) {
			printf("ID: %d. Grabbing message.\n", sentId);
		}
		
		in = in.substr(sizeof(idType), in.size() - sizeof(idType));

		
		if (testing) {
			printf("Message: %s. Analyzing message.\n", in.c_str());
		}
		// If the given message is a command, then store it for action decision making.
		if (!in.find("id-")) {
			// If the command comes from the main device, then collect it in, and clear all old messages..
			if (sentId <= 1) {
				for(idType i = 1; i < numDevices; i++) {
					comms_collection[i] = "";
				}
				
				comms_collection[sentId] = in;
				analyzeMessage(sentId);
				
			// If the received message doesn't come from the main device, then count it's command.
			}
			else if (sentId > 1 && sentId <= numDevices) {
				comms_collection[sentId] = in;
				
				// If over half of the devices have sent commands, then check to see if they can stop.
				if (sentId >= numDevices / 2) {
					analyzeMessage(sentId);
				}
			}
			
			return;
		// If the message is a request for id, then respond if this device is the head device.
		}
		else if (id == 1 && in.find("rid-")) {
			// Adjust accordingly.
			numDevices++;
			sendMessage("sid-" + convertId(numDevices));
			comms_collection[numDevices] = "";
			writeToFile(write_File, "nid= " + convertId(numDevices));
			printf("NumDevices = %s.\n", std::to_string(numDevices).c_str());
			return;
			
		// If the message is a response of a new id given, then keep track of how many devices are active.
		}
		else if (sentId == 1 && in.find("sid-")) {
			numDevices = convertId(in.substr(4, sizeof(idType)));
			comms_collection[numDevices] = "";
			writeToFile(write_File, "nid= " + convertId(numDevices));
			printf("NumDevices = %s.\n", std::to_string(numDevices).c_str());
			return;
			
		// If message is gps of other device, then pass the message on to python.
		}
		else if (in.find("gid-") && convertId(in.substr(4, sizeof(idType))) == id) {
			writeToFile(write_File, convertId(sentId) + "-" + in);
		}
	}
}

/**********************************
 * Analyze Message
 **********************************
 * Determines what to do based on the
 * id of the sender, and on what the
 * message is.
 **********************************/
void analyzeMessage(idType sentId) {
	// If from main, determine if it is a command or not.
	if (sentId == 1) {
		writeToFile(write_File, comms_collection[sentId]);
	}
	// If the message is a command and the voting has come to a conclusion, then progress on.
	else {
		for (idType i = 1; i <= sentId; i++) {
			map<string, idType> votes;
			string command = comms_collection[i].substr(0, comms_collection[i].find("-"));
			if (command != "agg") {
				votes[command] = 1;
			}
			else {
				int loc = comms_collection[i].find("-");
				idType cc = (idType) strtoul(comms_collection[i].substr(loc, comms_collection[i].size() - loc).c_str(), NULL, 0);
				string aggc = comms_collection[cc].substr(0, comms_collection[cc].find("-"));
				votes[aggc]++;
				
				if (votes[aggc] > (numDevices / votes.size())) {
					writeToFile(write_File, comms_collection[cc]);
					return;
				}
			}
		}
	}
}

/**********************************
 * Send Message
 **********************************
 * Sends the string over RF in two
 * parts, first, how large the
 * message is, and second, the actual
 * message.
 **********************************/
bool sendMessage(string message) {
	// Declare local variables.
	radio.stopListening();
	
	if (message.length() == 0) {
		return true;
	}
	
	sizeType size = sizeof(message);
	bool sSent = false, mSent = false;
	
	if (testing) {
		// Show the user what is being sent.
		printf("Sending size: %d\n", size);
		printf("Sending message: %s\n", message.c_str());
	}
	
	// Send the size of the message, then the message.
	sSent = sendSize(size);
	
	if (/*sSent*/false) {
		if (testing) {
		}
	
		mSent = sendString(message);
	
		if (!message.find("id-") && !message.find("gps"))
			comms_collection[id] = message;
	}
	
	radio.startListening();
	// Return that sending the message was successful.
	return (sSent && mSent);
}

/**********************************
 * Get Id
 **********************************
 * Requests an ID, and sets id to
 * received id. If response does not
 * give local id, then wait for timeout,
 * or id. On timeout, or ensurance that
 * local is first, set local id to 1.
 **********************************/
void getId() {
	// Request id.
	printf("\nRequesting id...\n");
	
	sendSize(128);
	return;
	sendMessage("rid-");
	int time = millis();
	
	// Wait for a local id.
	while (id == 0 && (millis() - time) < waitTimeId) {
		if (radio.available()) {
			receiveMessage();
			if (comms_collection[(idType)1].find("sid-")) {
				id = convertId(comms_collection[1].substr(4, sizeof(idType)));
				numDevices = id;
			}
			else if (comms_collection[(idType)1].find("rid-")) {
				id = 1;
				numDevices = 2;
				sendMessage("sid-" + convertId(numDevices));
			}
		}
	}
	
	// Ensure that id is no longer zero.
	id = (id == 0 ? 1: id);

	// Output this devices id.
	printf("ID: %d.\n", id);
}

/**********************************
 * Setup
 **********************************
 * Sets up the entire system to be
 * ready to work properly.
 **********************************/
void setup(){
	if (testing) {
		printf("\nSetting up device.\n");
	}
	// Initialize ID.
	id = 0;
	size = true;
	messageSize = sizeof(idType);

	//Prepare the radio module
	radio.begin();
	radio.setRetries(15, 15);
	//radio.setChannel(0x4c);
	//radio.setPALevel(RF24_PA_MAX);
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1,pipes[1]);
	
	// Last line before system breaks if wired wrong.
	if (testing) {
		printf("Radio configured, starting radio.\n");
	}

	radio.printDetails();
	radio.startListening();

	getId();
}

/**********************************
 * Main
 **********************************
 * Set up, and then send messages
 * as often as they come up.
 **********************************/
int main(int argc, char ** argv) {
	setup();
	queue<string> toSend;
	if (testing) {
		printf("Entering Loop.\n");
	}
	
	while (true) {
		// If a message has become available, receive it.
		if (radio.available()) {
			receiveMessage();
		}
		
		queue<string> fromFile = readFromFile(read_File);
		
		while (!fromFile.empty()) {
			toSend.push(fromFile.front());
			fromFile.pop();
		}
		
		if (testing) {
			printf("Reading from file.\n");
		}
		
		// If a message has become available, receive it.
		if (radio.available()) {
			receiveMessage();
		}
		
		if (!toSend.empty())
			if (sendSize(toSend.front().length()))
					toSend.pop();
	}
}

/**********************************
 * Write To File
 **********************************
 * Opens the given file, and adds
 * the passed-in string into end
 * of the file.
 **********************************/
bool writeToFile(string fileName, string data) {
	// Save all the data already in the file.
	queue<string> oldData = readFromFile(fileName);

	// Initialize the filestream.
	ofstream file;
	file.open(fileName.c_str());
	
	// Add the data to the back of the file.
	oldData.push(data);

	// Fill in the file.
	while(!oldData.empty()) {
		file << oldData.front();
		oldData.pop();
	}

	// Wrap up the writing process.
	file.close();
	return true;
}

/**********************************
 * Read From File
 **********************************
 * Opens the given file, returns a
 * list of all the lines in the given
 * file.
 **********************************/
queue<string> readFromFile(string fileName) {
	// Allocate local variables.
	queue<string> *out = new queue<string>();
	string ln;
	ifstream in;
	
	if (testing) {
		printf("Reading from file.\n");
	}
	
	// Access the file.
	in.open(fileName.c_str());
	while(!in.eof()) {
		std::getline(in, ln);
		out->push(ln);
	}
	
	// Close the file, and pass-back the lines.
	in.close();
	
	// Initialize the filestream.
	ofstream file;
	file.open(fileName.c_str());

	// Empty the file.
	file << "";

	// Wrap up the writing process.
	file.close();
	
	return *out;
}

/**********************************
 * Send Long
 **********************************
 * Sends the passed-in long over RF.
 **********************************/
bool sendSize(sizeType out) {
	if (testing) {
		printf("Sending size: %d.\n", out);
	}
	
	// Declare local variables.
	bool message_posted;

	// Prep the radio to send.
	radio.stopListening();

	// Attempt to send the message.
	message_posted = radio.write( &out, sizeof(out) );

	// Start listening for returning messages.
	radio.startListening();

	// Return whether the message-post worked.
	return message_posted;
}

/**********************************
 * Send String
 **********************************
 * Sends the passed-in string over RF.
 **********************************/
bool sendString(string out) {
	if (testing) {
		printf("Sending message: %s.\n", out.c_str());
	}
	
	// Declare local variables.
	bool message_posted;
	out = convertId(id) + "-" + out;

	// Prep the radio to send.
	radio.stopListening();

	// Attempt to send the message.
	message_posted = radio.write( &out, sizeof(out) );

	// Start listening for returning messages.
	radio.startListening();

	// Return whether the message-post worked.
	return message_posted;
}

/**********************************
 * Convert Size
 **********************************
 * Turns a string into the value of
 * the sizeType.
 **********************************/
sizeType convertSize(string in) {
	if (testingConverters) {
		printf("Converting %s to decimal.", in.c_str());
	}
	
	sizeType out = 0;
	
	// Convert each character to a value.
	for (char a : in)	{
		out = out << 8;
		out += (sizeType)a;
	}
	
	return out;
}

/**********************************
 * Convert Id
 **********************************
 * Turns a short into two chars.
 **********************************/
string convertId(idType in) {
	if (testingConverters) {
		printf("Converting %d to id string.", in);
	}
	string out;
	
	// Add in each character.
	for (idType i = ~0; i > 1; i /= 256) {
		out = (char)in + out;
		in /= 256;
	}
	
	return out;
}

/**********************************
 * Convert Id
 **********************************
 * Turns two chars into a short.
 **********************************/
idType convertId(string in) {
	if (testingConverters) {
		printf("Converting %s to number.", in.c_str());
	}
	
	idType out = 0;
	
	// Convert each character to a value.
	for (char a : in)	{
		out = out << 8;
		out += (idType)a;
	}
	
	return out;
}
