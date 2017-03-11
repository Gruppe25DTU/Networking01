#include<sstream>
#include<locale>
#include "ftpclient.h"
using namespace std;
#pragma warning(disable : 4996)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
string reply;


Client::~Client()
{
	CloseCon();
}

int Client::Connect(int port, char *adr) {
	WSAStartup(0x0101, &wlib);
	cs = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr(adr);
	nOk = connect(cs, (sockaddr *)&saddr, sizeof(saddr));
	if (nOk != 0) {
		cout << "Kunne ikke oprette forbindelse!" << endl;
		exit(1);
	}
	GetReply(cs);
	cout << reply.c_str();
	return nOk;
}

void Client::DataConnect() {
	ds = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	string pasv = "EPSV\r\n\0";
	SendMsg(pasv.c_str(), pasv.size(), cs);
	GetReply(cs);
	cout << reply;
	int start = reply.find("(|||") + 4;
	int end = reply.find_last_of("|");
	if (start == string::npos || end == string::npos) {
		cout << "Kunne ikke aflæse svar på PASV (EPSV)!" << endl;
		exit(1);
	}
	int port = stoi(reply.substr(start, end - start));
	if (port < 0) {
		cout << "Kunne ikke aflæse svar på PASV (EPSV)!" << endl;
		exit(1);
	}
	saddr.sin_port = htons(port);
	nOk = connect(ds, (sockaddr *)&saddr, sizeof(saddr));
	if (nOk != 0) {
		cout << "Kunne ikke oprette socket til Data-forbindelse!" << endl;
		exit(1);
	}
}

void Client::DataCloseCon() {
	closesocket(ds);
}


void Client::ParseMsg(const char *pmsg, int size) {
	locale loc;
	string upcmd = pmsg;
	int fspace = upcmd.find_first_of(' ');
	//upcmd becomes the first part of the FTP Command
	if (fspace != string::npos) {
		upcmd = upcmd.substr(0, fspace);
	}
	else {
		//Remove CRLF from cmd
		if (upcmd.back() == '\n') {
			upcmd.pop_back();
		}
		if (upcmd.back() == '\r') {
			upcmd.pop_back();
		}
	}
	//Capitalizes upcmd
	for (string::size_type i = 0; i < upcmd.length(); i++) {
		upcmd[i] = toupper(upcmd[i], loc);
	}
	std::map<std::string, Command>::iterator it = cmdMap.begin();
	if ((it = cmdMap.find(upcmd)) != cmdMap.end())
	{
		it->second.run(pmsg, size);
	}
	else
	{
		cout << "Ukendt kommando" << endl;
	}
}

void Client::list(const char *pmsg, int size)
{
	GetList(string(pmsg));
}

void Client::quit(const char *pmsg, int size)
{
	string quit = "quit\r\n\0";
	SendMsg(quit.c_str(), quit.size(), cs);
	GetReply(cs);
	cout << reply.c_str();
	CloseCon();
	exit(0);
}

void Client::genericCmds(const char *pmsg, int size)
{
	SendMsg(pmsg, size, cs);
	GetReply(cs);
	cout << reply.c_str();
}

std::string Client::getArg(const char *pmsg, int argNum) const {
	string tmp (pmsg);
	int ndelim;
	int fdelim = 0;
	for (int i = 0; fdelim != string::npos && i < argNum; i++) {
		fdelim = tmp.find(" ", fdelim);
		if (fdelim == string::npos) {
			return "";
		}
		fdelim++;
		while (fdelim != string::npos && fdelim < tmp.size() && tmp.at(fdelim) == ' ') {
			fdelim++;
		}
	}
	if (tmp.at(fdelim) == '"') {
		fdelim++;
		ndelim = tmp.find('"', fdelim);
	}
	else {
		ndelim = tmp.find(" ", fdelim);
	}
	return (tmp.substr(fdelim, ndelim - fdelim)).c_str();
}

void Client::upload(const char* pmsg, int size)
{
	string tmp = pmsg;
	string fname = getArg(pmsg, 1);
	ifstream file;
	ostringstream filestrbuf;
	string filestr;
	file.open(fname, ios::binary);
	filestrbuf;
	if (file.is_open()) {
		string cmd = "STOR " + getArg(pmsg, 2);
		DataConnect();
		SendMsg(cmd.c_str(), cmd.size(), cs);
		GetReply(cs);
		cout << reply;
		if (reply.substr(0, 3).compare("553") == 0) {
			cout << "Kunne ikke oprette filen: " << cmd.substr(5, string::npos) << endl;
			DataCloseCon();
			return;
		}
		//Put the file in a string buffer
		filestrbuf << file.rdbuf();
		file.close();
		//Convert to string
		filestr = filestrbuf.str();
		//Send file to Data-Socket as char*
		SendMsg(filestr.c_str(), filestr.size(), ds);
		//Close connection to indicate EOF
		DataCloseCon();

		GetReply(cs);
		cout << reply;
		DataCloseCon();
	}
	else {
		cout << "Kunne ikke finde filen: " << fname << endl;
	}
}

void Client::download(const char* pmsg, int size)
{
	string tmp = pmsg;
	string cmd = "RETR ";
	cmd.append(tmp.substr(9, string::npos));
	GetData(cmd);

}

int Client::SendMsg(const char *pmsg, int size, int socket) {
	if ((nOk = send(socket, pmsg, size, 0)) == -1) {
		cout << "Kunne ikke sende!" << endl;
		exit(1);
	}
	return nOk;
}

void Client::GetData(string cmd) {
	ostringstream strbuf;
	bool namegiven = true;
	string filestr;
	string filename = getArg(cmd.c_str(), 2);
	if (filename.empty()) {
		filename = getArg(cmd.c_str(), 1);
		namegiven = false;
	}
	if (filename.back() == '\n') {
		filename.pop_back();
	}
	if (filename.back() == '\r') {
		filename.pop_back();
	}
	if (namegiven) {
		int pos = cmd.find(filename);
		int finish = filename.length();
		while (cmd.at(pos - 1) == ' ') {
			pos--;
			finish++;
		}
		cmd.erase(pos, finish);
	}
	ofstream file(filename, ios::binary|ios::trunc);
	if (!(file.is_open())) {
		cout << "Kunne ikke åbne filen!" << endl;
		return;
	}
	int rval;
	DataConnect();
	SendMsg(cmd.c_str(), cmd.size(), cs);
	GetReply(cs);
	cout << reply;
	if (reply.substr(0, 3).compare("550") == 0) {
		cout << "Kunne ikke åbne filen!" << endl;
		DataCloseCon();
		return;
	}
	char partial_reply[DEFAULT_BUFFER_SIZE];
	do {
		rval = (int)recv(ds, partial_reply, DEFAULT_BUFFER_SIZE - 1, 0);
		if (rval > 0) {
			file.write(partial_reply, rval);
			filestr.append(string(partial_reply).substr(0,rval));
			
		}
	} while (rval > 0);
	//file << strbuf.rdbuf();
	file.close();
	//filestr = strbuf.str();
	//prints out the first 1KB 
	int sublen = 1024 > filestr.size()? filestr.size(): 1024;
	cout << filestr.substr(0, sublen) << endl;;
	//----------------------------------------------------------
	if (filestr.size() == 0 || (filestr.size() > 0 && filestr.at(filestr.size() - 1) != '\n')) {
		cout << endl;
	}
	GetReply(cs);
	cout << reply;
	DataCloseCon();
}

void Client::GetList(string cmd)
{
	string listStr;
	string fullreply;
	int rval;
	int total;
	DataConnect();
	SendMsg(cmd.c_str(), cmd.size(), cs);
	GetReply(cs);
	cout << reply;
	if (reply.substr(0, 3).compare("500") == 0 || 
		reply.substr(0, 3).compare("501") == 0 || 
		reply.substr(0, 3).compare("502") == 0 ||
		reply.substr(0, 3).compare("530") == 0 ||
		reply.substr(0, 3).compare("421") == 0) 
	{
		cout << "Kunne ikke hente listen" << endl;
		DataCloseCon();
		return;
	}
	char partial_reply[DEFAULT_BUFFER_SIZE];
	do {
		rval = (int)recv(ds, partial_reply, DEFAULT_BUFFER_SIZE - 1, 0);
		if (rval > 0) {
			fullreply.append(string(partial_reply).substr(0,rval));
		}
	} while (rval > 0);
	cout << fullreply <<endl;
	GetReply(cs);
	cout << reply;
	DataCloseCon();

}

const char* Client::GetReply(int socket) {
	reply = "";
	string tmp;
	int fnint = 0;
	int rval;
	char partial_reply[DEFAULT_BUFFER_SIZE];
	do {
		rval = (int)recv(socket, partial_reply, DEFAULT_BUFFER_SIZE - 1, 0);
		if (rval > 0) {
			partial_reply[rval] = '\0';
			tmp = partial_reply;
			reply.append(tmp);
			fnint = tmp.find_first_not_of("0123456789");
		}
	} while (!(fnint == 3 && tmp.at(3) == ' '));
	return reply.c_str();
}

void Client::CloseCon() {
	closesocket(cs);
	closesocket(ds);
	WSACleanup();
	exit(0);
}

Client::Command::Command(Client *p, void (Client::*M)(const char*, int))
	: parent(p), Method(M) { }

void Client::Command::run(const char* pmsg, int size)
{
	(parent->*Method)(pmsg, size);
}

void Client::initCommands()
{
	cmdMap.emplace("UPLOAD", Command(this, &Client::upload));
	cmdMap.emplace("DOWNLOAD", Command(this, &Client::download));
	cmdMap.emplace("QUIT", Command(this, &Client::quit));
	cmdMap.emplace("LIST", Command(this, &Client::list));
	cmdMap.emplace("USER", Command(this, &Client::genericCmds));
	cmdMap.emplace("PASS", Command(this, &Client::genericCmds));
	cmdMap.emplace("CWD", Command(this, &Client::genericCmds));
	cmdMap.emplace("CDUP", Command(this, &Client::genericCmds));
}