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
#include "mail_server.h"

using namespace std;

string MessageSummary(message* m)
{
	std::stringstream ret;
	ret << m->id << " ";
	ret << m->from << " ";
	ret << "\"" << m->subject << "\" ";
	ret << m->attachments.size() << endl;
	return ret.str();
}

void ShowInbox(int fd, user* u)
{
	message_pool::iterator listiterator;
	for(listiterator = u->messages.begin();
			listiterator != u->messages.end();
			listiterator++)
	{
		string summary = MessageSummary(listiterator->second);
		send(fd, summary.c_str(), summary.size(), 0);
	}
	return;
}

char GetNextMessage(int fd)
{
	char ret;
	read(fd, &ret, sizeof(msg_code));
	return ret;
}

int Login()
{
	string greeting = "‫.‪Welcome! I am simple-mail-server‬‬";
	string username = "";
	string password = "";

	if (-1 == SendLineToSocket(server_s.client_fd, greeting))
		return 1;
	string line = RecvLineFromSocket(server_s.client_fd);

    std::stringstream   linestream(line);
    std::getline(linestream, username, ' ');  // read up-to the first space
    std::getline(linestream, password, ' ');  // read up-to the second space

	users_db::iterator listiterator;
	for(listiterator = users_map.begin();
			listiterator != users_map.end();
			listiterator++)
	{
		if ((listiterator->first == username) && (listiterator->second->password == password))
		{
			// found a match
			if (-1 == SendLineToSocket(server_s.client_fd, "Connected to server"))
				return 1;
			server_s.current_user = listiterator->second;
			return 0;
		}
	}
	return 0;
}

void HandleClient()
{
	char quit = 0;
	char code;

	if (-1 == Login())
		return;

	while (!quit)
	{
		// parse next request
		code = GetNextMessage(server_s.client_fd);
		if (0 == code)
		{
			cout << "Malformed message. Aborting" << endl;
			return;
		}

		switch (code)
		{
			case SHOW_INBOX_C:
				ShowInbox(server_s.client_fd, server_s.current_user);
				break;

			case GET_MAIL_C:
			case GET_ATTACHMENT_C:
			case DELETE_MAIL_C:
			case COMPOSE_C:
			case QUIT_C:
				quit = 1;
				break;
			default:
				cout << "Undefined msg code " << code << endl;
				return;
		}
	}
	return;
}

void ReadUsersFile(string path)
{
	ifstream file(path.c_str());
	string line;

	while(std::getline(file, line))
	{
		user* u = new user;
	    std::stringstream   linestream(line);

	    // If you have truly tab delimited data use getline() with third parameter.
	    // If your data is just white space separated data
	    // then the operator >> will do (it reads a space separated word into a string).
	    std::getline(linestream, u->username, '\t');  // read up-to the first tab (discard tab).
	    std::getline(linestream, u->password, '\t');  // read up-to the second tab (discard tab).
	    users_map[u->username] = u;
	    u->next_msg_id = 1;
	}


	message_t *m = new message_t;
	m->id = 1;
	m->from = "Yossi";
	m->subject = "Funny Pictures";
	m->body = "How are you? Long time no see!";
	m->to.push_front(users_map["Moshe"]);

	attachment* a1 = new attachment;
	a1->filename = "funny1.jpg";
	a1->data = malloc(100);
	attachment* a2 = new attachment;
	a2->filename = "funny2.jpg";
	a2->data = malloc(100);
	m->attachments.push_front(a1);
	m->attachments.push_front(a2);

	user* u = users_map["Moshe"];
	u->messages[u->next_msg_id++] = m;


	return;
}

void StartListeningServer(unsigned short port)
{
	if ((server_s.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Could not initialize socket. Aborting." << endl;
		return;
	}

	server_s.sin.sin_port = htons(port);
	server_s.sin.sin_addr.s_addr = 0;
	server_s.sin.sin_addr.s_addr = INADDR_ANY;
	server_s.sin.sin_family = AF_INET;

	if (bind(server_s.server_fd, (struct sockaddr *)&server_s.sin,sizeof(struct sockaddr_in) ) == -1)
	{
	    cout << "Error binding socket to port " << port << ". Aborting." << endl;
	    return;
	}

	if (listen(server_s.server_fd, 10) == -1)
	{
	    cout << "Error while trying to listen(). Aborting." << endl;
	    return;
	}

    // Server blocks on this call until a client tries to establish connection.
    // When a connection is established, it returns a 'connected socket descriptor' different
    // from the one created earlier.
	socklen_t size = sizeof(server_s.client_addr);
    server_s.client_fd = accept(server_s.server_fd, (struct sockaddr *)&server_s.client_addr, &size);
    if (server_s.client_fd == -1)
    	printf("Failed accepting connection\n");
    else
    	printf("Connected\n");

	HandleClient();

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
