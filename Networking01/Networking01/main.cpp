#include <iostream>
#include <string>
#include "ftpclient.h"
using namespace std;


int main(int argc , char **argv) {
	string filePath = argv[0]; // this is the path to where this application is running + the app name
	//Here we find out how long the application name is
	int appNameLen = 0;
	for (int i = filePath.length();i>0;i--)
	{
		char c = filePath[i];
		if (c == '\\')
			break;
		appNameLen++;
	}
	//now we got the current directory only
	filePath = filePath.substr(0,filePath.length()-appNameLen+1);
	Client client;
	client.setFilePath(filePath);
	client.initCommands();
	char quit[] = "quit";
	string input;
	int port = 21;
	char* host = "88.99.32.10";
	if (client.Connect(21, host) != 0) {
		cout << "Failed to connect!" << endl;
	}
	else {
		cout << "Forbundet til " << host << ":" << port << endl;
	}
	do {
		input = "";
		cout << "$ ftp>";
		getline(cin, input);
		if (input.size() == 0) {
			continue;
		}
		input.append("\r\n\0");
		client.ParseMsg((input.c_str()), input.size());
	} while (input.compare(quit) != 0);
	client.CloseCon();
	return 0;
}