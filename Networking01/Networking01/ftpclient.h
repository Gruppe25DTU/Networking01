#pragma once
#include <string.h>
#include <winsock.h>
typedef char* string;

class Client {
public:
	void connect(string host, int port);
	void disconnect();

	void download(string target);
	void displayFile(string filePath);	//Reads and prints up to 1KB of file

	void upload(string filePath, string targetDir);
private:
	int socket;
	sockaddr_in saddr;
	WSADATA wlib;
	int success;
};