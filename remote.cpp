#include <string>
#include <getopt.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <queue>
#include <iostream>
#include <RF24/RF24.h>

#define read_File "toC.txt"
#define write_File "toPy.txt"
#define messageType unsigned long

using namespace std;
//RF24 radio("/dev/spidev0.0",8000000 , 25);  
//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);
//const int role_pin = 7;
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
//const uint8_t pipes[][6] = {"1Node","2Node"};

// hack to avoid SEG FAULT, issue #46 on RF24 github https://github.com/TMRh20/RF24.git
unsigned long  got_message;

bool writeToFile(string fileName, messageType message);
queue<messageType> readFromFile(string);

void setup(void){
	//Prepare the radio module
	printf("\nPreparing interface\n");
	radio.begin();
	radio.setRetries( 15, 15);
	//	radio.setChannel(0x4c);
	//	radio.setPALevel(RF24_PA_MAX);
	//	radio.setPALevel(RF24_PA_MAX);

	radio.printDetails();
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1,pipes[0]);
	//	radio.startListening();

}

bool sendMessage(unsigned long action){
	//This function send a message, the 'action', to the arduino and wait for answer
	//Returns true if ACK package is received
	//Stop listening
	radio.stopListening();
	unsigned long message = action;
	printf("Now sending  %lu...", message);

	//Send the message
	bool ok = radio.write( &message, sizeof(unsigned long) );
	if (!ok){
		printf("failed...\n\r");
	}else{
		printf("ok!\n\r");
	}	
	//Listen for ACK
	radio.startListening();/*
	//Let's take the time while we listen
	unsigned long started_waiting_at = millis();
	bool timeout = false;
	while ( ! radio.available() && ! timeout ) {
		//printf("%d", !radio.available());
		if (millis() - started_waiting_at > 1000 ){
			timeout = true;
		}
	}

	if( timeout ){
		//If we waited too long the transmission failed
		printf("Oh gosh, it's not giving me any response...\n\r");
		return false;
	}else{
		//If we received the message in time, let's read it and print it
		radio.read( &got_message, sizeof(unsigned long) );
		printf("Yay! Got this response %lu.\n\r",got_message);
		return true;
	}*/

}

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
	printf("Receiving Message.\n");
		
	messageType in;
		
	radio.read( &in, sizeof(messageType) );
		
	printf("Received message: %d.\n", in);
	
	radio.stopListening();
	messageType message = in;
	
	bool ok = radio.write( &message, sizeof(messageType) );
	
	if (!ok){
		printf("failed to respond...\n\r");
	}else{
		printf("ok!\n\r");
	}	
	//Listen for ACK
	radio.startListening();
		
	writeToFile(write_File, in);
}

int main( int argc, char ** argv){

	char choice;
	setup();
	bool switched = false;
	int counter = 0;
	queue<messageType> toSend;

	//printf("Char Size: %d.", sizeof(char));
	
	while (true) {
		// If a message has become available, receive it.
		if (radio.available()) {
			receiveMessage();
		}
		
		queue<messageType> fromFile = readFromFile(read_File);
		
		while (!fromFile.empty()) {
			toSend.push(fromFile.front());
			fromFile.pop();
		}
		
		// If a message has become available, receive it.
		if (radio.available()) {
			receiveMessage();
		}
		
		if (!toSend.empty()) {
			printf("To send size %d.\n", toSend.size());
			if (sendMessage(toSend.front()))
					toSend.pop();
		}
	}
}

/**********************************
 * Write To File
 **********************************
 * Opens the given file, and adds
 * the passed-in string into end
 * of the file.
 **********************************/
bool writeToFile(string fileName, messageType data) {
	/*if (testingFiles) {
		printf("Writing to file.\n");
	}*/
	// Save all the data already in the file.
	queue<messageType> oldData = readFromFile(fileName);

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
queue<messageType> readFromFile(string fileName) {
	//printf("Reading from file.\n");
	
	// Allocate local variables.
	queue<messageType> *out = new queue<messageType>();
	string ln;
	ifstream in;
	
	// Access the file.
	in.open(fileName.c_str(), ios_base::in);
	while(!in.eof()) {
		getline(in, ln);
		if (ln != "") {
			out->push(atoi(ln.c_str()));
			printf("Read %s.\n", ln.c_str());
		}
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

