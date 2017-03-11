#include <winsock2.h>
#include <iostream>
#include<fstream>
#include <string.h>
#include <map>
#define DEFAULT_BUFFER_SIZE 100

class Client {
private:

	class Command
	{
	private:
		Client* parent;
		void (Client::*Method)(const char*, int);
	public:
		Command() = delete;
		Command(Client* parent, void (Client::*M)(const char*, int));
		void run(const char* pmsg, int size);
	};

	//These member functions handles FTP Commands
	void download(const char *pmsg, int size);		//Handles the "RETR" Command, which we call "DOWNLOAD"
	void upload(const char *pmsg, int size);		//Handles the "STOR" Command, which we call "UPLOAD"
	void quit(const char *pmsg, int size);			//Handles the "QUIT" Command
	void list(const char *pmsg, int size);			//Handles the "LIST" Command
	void genericCmds(const char *pmsg, int size);	//Handles any FTP Commands that doesn't require us to do anything
													//Other than send the message and recieve a reply
													//Generic FTP Commands implemented so far:
													//CWD, CDUP, PASS, USER.
	//---------------------------------------------------------------------------------------------------------

	std::map<std::string, Command> cmdMap; //The key is the FTP Command as a string, and the value is
	                                               //A Command object

	int SendMsg(const char *pmsg, int size, int socket);
	void GetFile(std::string cmd);
	void GetList(std::string cmd);
	std::string GetData(std::string cmd, bool print, std::ostream& out);
	void DataConnect();
	void DataCloseCon();
	std::string getArg(const char *pmsg, int argNumber) const;
	std::string filepath;
	int ds;	//Datasocket
	int cs;	//Controlsocket
	sockaddr_in saddr;
	WSADATA wlib;
	int nOk;

public:
	~Client();
	void ParseMsg(const char *pmsg, int size); //All messages from our controlsocket runs through here where they get 
											   //redirected to other member functions that handles specific FTP commands
	int Connect(int port, char *adr);
	void CloseCon();
	const char* GetReply(int socket);
	char* Help();
	void initCommands();
	void setFilePath(std::string fileP) { filepath = fileP;}
};
