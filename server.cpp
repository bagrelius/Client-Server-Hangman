
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
#include <string>
#include <cstring>
#include <pthread.h>

#include <vector>
#include <fstream>
#include <time.h>
using namespace std;

#define BUFFERSIZE 1024
#define NUMBERLINES 57127

struct arg_struct {
	int client_soc;
	string w;
};

struct player {
	string name;
	float score;
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int numRecord = 0;
player leaderboard[3];

void * fulfillRequest(void * args_ptr);


int main(int argc, char* argv[]) {

	for (int i = 0; i < 3; i++) {
		leaderboard[i].score = 1000.0; //Just needs to be high for comparison
	}
	srand(time(0));
	//get words
	string line;
	vector<string> lines;
	//fill word vector
	ifstream file("This needs to be the file path to word list of your choice");
	while (getline(file, line)) {
		lines.push_back(line);
	}
	
	if (argc != 2) {
		cout << "Usage: <Port Number>\n";
		return 1;
	}
	//input portnumber
	unsigned short portNum = (unsigned short) strtoul(argv[1], NULL, 0);
	int sockNum = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address, client_address;
	socklen_t addrLen = sizeof(client_address);
	//zero out server_address and fill variables
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portNum);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//Bind
	if (bind(sockNum, (struct sockaddr*) &server_address, sizeof(server_address)) != 0) {
		cerr << "Bind Error\n";
		return 1;
	}
	//Listen
	if (listen(sockNum, 5) != 0) {
		cerr << "Listen Error\n";
		return 1;
	}

	while (true) {
		//Accept
		int client_sock = accept(sockNum, (struct sockaddr*) &client_address, &addrLen);
		if (client_sock < 0) {
			cerr << "Not Accepted\n";
		} else { 
			//Handle with thread
			struct arg_struct *args = new arg_struct();	
			args->client_soc = client_sock;
			int randNum = rand() % NUMBERLINES;
			args->w = lines[randNum];
			
			pthread_t thread;
			pthread_create(&thread, NULL, fulfillRequest, (void *)args);
		}
	}
}

void * fulfillRequest(void* args_ptr) {
	
	string nameString;
	int numGuess = 0;
	char name[BUFFERSIZE];
	char buffer[BUFFERSIZE];
	char word[BUFFERSIZE];
	char guess[BUFFERSIZE];
	int wordLen;
	size_t bytesRead;
	int msgLength = 0;

	
	struct arg_struct* args = (struct arg_struct*)args_ptr;
	int client_socket = args->client_soc;
	string wrd = args->w;
	delete args;
	strcpy(word, wrd.c_str());
	wordLen = strlen(word);
	cout << "Word Selected: " << word << endl;

	//recieve name
	while ((bytesRead = recv(client_socket, name, sizeof(name), 0)) > 0) {
		msgLength += bytesRead;
		if (msgLength > BUFFERSIZE-1) break;
	}
	if (bytesRead < 0) {
		cerr << "Message Receive Failure\n";
		return NULL;
	}
	nameString = name;  
	//Copy Word into Guess
	strcpy(guess, word);
	//Set all of guess to '-'
	for (int i = 0; i < wordLen; i++) {
		guess[i] = '-';
	}
	guess[wordLen] = '\0';
	//Send Initial blank word
	int ret = send(client_socket, &guess, sizeof(guess), 0);
	if (ret == -1) {
		cerr << "Guess Send Error\n";
		return NULL;
	}

	//Recieve guesses until won
	bool won = false;
	while (!won) {
			//recieve guess
			bytesRead = 0;
			msgLength = 0;
			while ((bytesRead = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
				msgLength += bytesRead;
				if (msgLength > BUFFERSIZE-1) break;
			}
			if (bytesRead < 0) {
				cerr << "Message Receive Failure\n";
				return NULL;
			}
			buffer[msgLength] = '\0';
			numGuess++;
			//Check Guess
			for (int i = 0; i < wordLen; i++) {
				if (word[i] == toupper(buffer[0])) { //Only first letter checked
					guess[i] = word[i];
				}
			}
			if (strcmp(guess, word) == 0) {
				ret = send(client_socket, &word, sizeof(word), 0);
				if (ret == -1) {
					cerr << "Guess Send Error\n";
					return NULL;
				}
				won = true;
			} else {
				ret = send(client_socket, &guess, sizeof(guess), 0);
				if (ret == -1) {
					cerr << "Guess Send Error\n";
					return NULL;
				}	
			}
	}
	//begin critical section
	pthread_mutex_lock(&lock);
	//update leaderboard if necessary
	if (numRecord < 3) {
		numRecord++;
	}
	float score = (float)numGuess / (float)wordLen;
	for (int i = 0; i < numRecord; i++) {
		if (score < leaderboard[i].score) {
			for (int j = i; j < numRecord-1; j++) {
				leaderboard[j+1].name = leaderboard[j].name;
				leaderboard[j+1].score = leaderboard[j].score;
			}
			leaderboard[i].name = nameString;
			leaderboard[i].score = score;
			break;
		}
	}		
	//	send leaderboard
	string bstring;
	for (int index = 0; index < numRecord; index++) {
		bstring += leaderboard[index].name;
		bstring += " ";
		bstring += to_string(leaderboard[index].score);
		bstring += " ";
	}
	//end critical section
	pthread_mutex_unlock(&lock);
	//Send Leaderboard
	strcpy(buffer, bstring.c_str());	
	if (send(client_socket, &buffer, sizeof(buffer), 0) == -1) {
		cerr << "Leaderboard Send Error\n";
		return NULL;
	}

	close(client_socket);

	return NULL;
}


