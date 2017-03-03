#pragma once
#include <string.h>
typedef char* string;

void connect(string host, int port);
void disconnect();

void download(string target);
void displayFile(string filePath);	//Reads and prints up to 1KB of file

void upload(string filePath, string targetDir);