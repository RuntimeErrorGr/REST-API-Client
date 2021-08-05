#ifndef _HELPERS_
#define _HELPERS_


#define BUFLEN 4096
#define LINELEN 1000
#define SERVER "34.118.48.238"
#define PORT 8080
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)
#define ERROR_MSG "{\"error\":"
#define JWT_MSG "{\"token\":\""
#define COOKIE_MSG "Set-Cookie: "
#define COOKIE_SUFIX " Path=/; HttpOnly"
#define JSON_URL "application/json"

#include <iostream>
#include <algorithm>
#include <vector>
#include <bits/stdc++.h>

#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(const char *host_ip, int portno, int ip_type, int socket_type, int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);

void Register(int sockfd);

void LogIn(int sockfd, string& cookie, json& user);

void EnterLibrary(int sockfd, string cookie, string& jwt);

void GetBooks(int sockfd, string jwt);

void GetBook(int sockfd, string jwt);

void AddBook(int sockfd, string jwt);

void DeleteBook(int sockfd, string jwt);

void Logout(int sockfd, string& cookie, string& jwt, json user);

#endif
