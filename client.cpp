#include <sys/types.h> 
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <stdio.h>
#include <netdb.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>
using namespace std;

#define BUFFERLENGTH 1024

int fillServerAddress(const char* ipAddr, const char* portNumber, struct sockaddr_in* address);
bool has_digit(string &s);

int main(int argc, char* argv[]) {

	int numGuesses = 0;	
	string input;
	char name[BUFFERLENGTH];
	char response[BUFFERLENGTH];
	char sendmessage[BUFFERLENGTH];
	char guess[BUFFERLENGTH];

	if (argc != 3) {
		cerr << "Usage: <IP Address> <Port Number>";
		return 1;
	}
	//get address of server
	struct sockaddr_in server_address; 
	int success = fillServerAddress(argv[1], argv[2], &server_address);
	if (success != 0) {
		cerr << "Failed to find DNS\n";
		return 1;
	}
	//Socket Endpoint
	int sockNum = socket(AF_INET, SOCK_STREAM, 0);
	if (sockNum == -1) {
		perror("Socket failure");
		return 1;
	}
	//Connect
	//Make sure to cast sockaddr_in to sockaddr
	success = connect(sockNum, (struct sockaddr*) &server_address, sizeof(server_address)); 
	if (success == -1) { 
		perror("Connection Failure");
		return 1;
	}
	//Ask for name
	cout << "\nPlease Enter Name: ";
	getline(cin, input);
	strcpy(name, input.c_str());
	//send name
	if ((send(sockNum, &name, sizeof(name), 0)) < 0) {
		cerr << "Send Error\n";
		return 1;
	}

	//Ask for initial guess progress
	int msgLength = 0;
	size_t bytesRead = 0;
	while ((bytesRead = recv(sockNum, guess, sizeof(guess), 0)) > 0) {
		msgLength += bytesRead;
		if (msgLength > BUFFERLENGTH-1) break;
	}
	if (bytesRead < 0) {
		cerr << "Message Receive Failure\n";
		return 1;
	}
	int guessLength = strlen(guess);
	
	cout << "Only first character of unput taken\n";
	bool done = false;
	while (!done && numGuesses < 1000) {
			//Ask for guess
			cout << "Number of Guesses: " << numGuesses++ << endl;
			cout << "Word: " << guess << endl;
			cout << "Enter Guess: ";
			
				
			cin >> input;
			strcpy(sendmessage, input.c_str());
			//send guess
			send(sockNum, &sendmessage, sizeof(sendmessage), 0);
			//recieve response
			msgLength = 0;
			bytesRead = 0;
			while ((bytesRead = recv(sockNum, response, sizeof(response), 0)) > 0) {
				msgLength += bytesRead;
				if (msgLength > BUFFERLENGTH-1) break;
			}
			if (bytesRead < 0) {
				cerr << "Message Receive Failure\n";
				return 1;
			}
			response[msgLength-1] = '\0';
			//Check guess
			//if theyre the same there was no update to guess
			if (strcmp(guess, response) == 0) {
				cout << "Incorrect\n\n";
				continue;
			}
			//if theyre different update then check done
			else {
				cout << "Correct\n\n";
				for (int i = 0; i < guessLength; i++) {
					guess[i] = response[i];
				}		
				done = true;
				for (int i = 0; i < guessLength; i++) {
					if (guess[i] == '-') {        //If any character is '-' then continue
						done = false;
					}
				}
			}		
	}
	cout << "Congrats you guessed the word " << guess << " in only "
		 << numGuesses << " tries!\n\n";		
	
	//recieve leaderboard
	msgLength = 0;
	bytesRead = 0;
	while ((bytesRead = recv(sockNum, &response, sizeof(response), 0)) > 0) {
		msgLength += bytesRead;
		if (msgLength > BUFFERLENGTH - 1) {
			break;
		}
	}
	if (bytesRead < 0) { 
		cerr << "Message Receive Failure\n" << strerror(errno) << endl;
		return 1;
	}
	//print leaderboard
	string leaderB;
	leaderB = response;
	cout << "LEADERBOARD:\n";
	
	string word;
	stringstream iss(leaderB);
	while (iss >> word) {
		cout << word << " ";		
		if (has_digit(word)) {
			cout << endl;
		}
	}

	return 0;	
	
}


bool has_digit(string &s) {
	return any_of(s.begin(), s.end(), ::isdigit);
}



int fillServerAddress(const char* ipAddr, const char* portNumber, struct sockaddr_in* address) {

	//getaddrinfo will fill addr_list with linked list
	//Used warmup as example becasue I didnt want to set the fields manually
	//although I could have done that without calling getaddrinfo 

	struct addrinfo* addr_list = NULL;
	int ret = getaddrinfo(ipAddr, portNumber, NULL, &addr_list);
	if (ret != 0) { 
		cerr << "Error with getaddrinfo: " << gai_strerror(ret); 
		return -1;
	}
	struct addrinfo* helper = addr_list;
	while ( helper != NULL && helper->ai_family != AF_INET) {
		helper = helper->ai_next;
	}
	if (helper == NULL) {
		cerr << "Address not found\n";
		return -1;
	}
	assert(helper->ai_addrlen == sizeof(struct sockaddr_in));
	memcpy(address, helper->ai_addr, sizeof(*address));
	freeaddrinfo(addr_list);
	return 0;

}

