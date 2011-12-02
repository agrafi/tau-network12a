/*
 * mail_server.cpp
 *
 *  Created on: Dec 1, 2011
 *      Author: aviv
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include "protocol.h"
#include "mail_client.h"

using namespace std;

void ReadUsersFile(string path)
{
	ifstream file(path.c_str());
	string line;

	while(std::getline(file, line))
	{
		user_t* u = new user_t;
	    std::stringstream   linestream(line);

	    // If you have truly tab delimited data use getline() with third parameter.
	    // If your data is just white space separated data
	    // then the operator >> will do (it reads a space separated word into a string).
	    std::getline(linestream, u->username, '\t');  // read up-to the first tab (discard tab).
	    std::getline(linestream, u->password, '\t');  // read up-to the second tab (discard tab).
	    users_list.push_back(*u);
	}

	return;
}

void StartListeningServer(unsigned short port)
{
	if ((server_s.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Could not initialize socket. Aborting." << endl;
		return;
	}

	server_s.sin.sin_port = htons(port);
	server_s.sin.sin_addr.s_addr = 0;
	server_s.sin.sin_addr.s_addr = INADDR_ANY;
	server_s.sin.sin_family = AF_INET;

	if (bind(server_s.fd, (struct sockaddr *)&server_s.sin,sizeof(struct sockaddr_in) ) == -1)
	{
	    cout << "Error binding socket to port " << port << ". Aborting." << endl;
	    return;
	}

	return;
}


int main(int argc, char** argv) {
	string users_file_path = "";
	unsigned short port = 6423;

	if ((argc != 2) && (argc != 3))
	{
		cout << "Usage: " << argv[0] << " users_file [port]" << endl;
		return 2;
	}

	// Get users file path
	users_file_path = argv[1];

	// Get non default port
	if (argc > 2)
	{
		port = atoi(argv[2]);
	}

	// Read users list from file to fresh users data structure
	ReadUsersFile(users_file_path);

	StartListeningServer(port);

	// We should never get here.
	return 0;
}
