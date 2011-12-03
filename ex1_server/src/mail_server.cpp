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
#include <errno.h>
#include <string.h>
#include "protocol.h"
#include "mail_server.h"

using namespace std;

void GetAttachment(int fd, user* u, string arguments)
{
	unsigned int id = 1;
	string id_str = "";
	unsigned int attachment_num = 1;
	string attachment_num_str = "";
	string attachment_path = "";
	string formatted_message = "";

    std::stringstream   linestream(arguments);
    std::getline(linestream, id_str, ' ');  // read up-to the first space
    std::getline(linestream, attachment_num_str, ' ');  // read up-to the second space
    std::getline(linestream, attachment_path, '\n');  // read up-to the third space

    id = strtoint(id_str);
    attachment_num = strtoint(attachment_num_str);
    if (u->messages.count(id) == 0)
    {
    	// message id does not exists, abort
    	return;
    }

    message* m = u->messages[id];

    if (m->attachments.count(attachment_num) == 0)
    {
    	// attachment num does not exists, abort
    	return;
    }

	send(fd, m->attachments[attachment_num]->data, m->attachments[attachment_num]->size, 0);
	return;
}

string FullMessage(message* m)
{
	std::stringstream ret;
	ret << "From: " << m->from << endl;

	ret << "To: ";
	list <user*>::iterator usersiterator;
	for(usersiterator = m->to.begin();
			usersiterator != m->to.end();
			usersiterator++)
	{
		if (usersiterator != m->to.begin())
			ret << ",";
		ret << (*usersiterator)->username;
	}
	ret << endl;

	ret << "Subject: " << m->subject << endl;

	ret << "Attachments: ";
	attachment_pool::iterator attachmentsiterator;
	for(attachmentsiterator = m->attachments.begin();
			attachmentsiterator != m->attachments.end();
			attachmentsiterator++)
	{
		if (attachmentsiterator != m->attachments.begin())
			ret << ",";
		ret << (*attachmentsiterator).second->filename;
	}
	ret << endl;

	ret << "Text: " << m->body << endl;
	return ret.str();
}

void GetMail(int fd, user* u, string arguments)
{
	unsigned int id = 1;
	string id_str = "";
	string formatted_message = "";

    std::stringstream   linestream(arguments);
    std::getline(linestream, id_str, ' ');  // read up-to the first space
    std::getline(linestream, id_str, '\n');  // read up-to the second space

    id = strtoint(id_str);
    if (u->messages.count(id) == 0)
    {
    	// message id does not exists, abort
    	return;
    }

	formatted_message = FullMessage(u->messages[id]);
	send(fd, formatted_message.c_str(), formatted_message.size(), 0);
	return;
}

string MessageSummary(message* m)
{
	std::stringstream ret;
	ret << m->id << " ";
	ret << m->from << " ";
	ret << "\"" << m->subject << "\" ";
	ret << m->attachments.size() << endl;
	return ret.str();
}

void ShowInbox(int fd, user* u, string arguments)
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

char GetNextMessage(string command)
{
	if (command == "SHOW_INBOX")
		return SHOW_INBOX_C;
	if (command == "GET_MAIL")
		return GET_MAIL_C;
	if (command == "GET_ATTACHMENT")
		return GET_ATTACHMENT_C;
	if (command == "QUIT")
		return QUIT_C;
	return 0;
}

int Login()
{
	string greeting = "‫.‪Welcome! I am simple-mail-server‬‬";
	string username = "";
	string password = "";

	if (-1 == SendLineToSocket(server_s.client_fd, greeting))
		return 1;

	// parse next request
	string line;
	if (0 != RecvLineFromSocket(server_s.client_fd, &line))
		return 1;

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
	string command, arguments;

	if (-1 == Login())
		return;

	while (!quit)
	{
		// parse next request
		string line;
		if (0 != RecvLineFromSocket(server_s.client_fd, &line))
			break;

	    std::stringstream   linestream(line);
	    std::getline(linestream, command, ' ');  // read up-to the first space
	    std::getline(linestream, arguments, '\n');  // read up-to the newline
	    cout << "cmd " << command << endl;
	    cout << "args " << arguments << endl;
		code = GetNextMessage(command);
		if (0 == code)
		{
			cout << "Malformed message. Aborting" << endl;
			continue;
		}

		switch (code)
		{
			case SHOW_INBOX_C:
				ShowInbox(server_s.client_fd, server_s.current_user, arguments);
				break;
			case GET_MAIL_C:
				GetMail(server_s.client_fd, server_s.current_user, arguments);
				break;
			case GET_ATTACHMENT_C:
				GetAttachment(server_s.client_fd, server_s.current_user, arguments);
				break;
			case DELETE_MAIL_C:
			case COMPOSE_C:
			case QUIT_C:
				quit = 1;
				break;
			default:
				cout << "Undefined msg code " << code << endl;
				continue;
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

	user* u = users_map["Moshe"];
	message_t *m = new message_t;
	m->id = u->next_msg_id;
	m->from = "Yossi";
	m->subject = "Funny Pictures";
	m->body = "How are you? Long time no see!";
	m->to.push_front(users_map["Moshe"]);

	attachment* a1 = new attachment;
	a1->filename = "funny1.jpg";
	a1->data = malloc(50000);
	a1->size = 50000;
	attachment* a2 = new attachment;
	a2->filename = "funny2.jpg";
	a2->data = malloc(100);
	a2->size = 100;
	m->attachments[1] = a1;
	m->attachments[2] = a2;

	u->messages[u->next_msg_id++] = m;


	message_t *m2 = new message_t;
	m2->id = u->next_msg_id;
	m2->from = "Yossi";
	m2->subject = "Hi There!";
	m2->body = "Just simple text";
	m2->to.push_front(users_map["Esther"]);

	user* u2 = users_map["Moshe"];
	u2->messages[u->next_msg_id++] = m2;


	return;
}

void StartListeningServer(unsigned short port)
{
	if ((server_s.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Could not initialize socket. Aborting." << endl;
		return;
	}

	// For debuggind only
	int opt = 1;
	setsockopt(server_s.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	server_s.sin.sin_port = htons(port);
	server_s.sin.sin_addr.s_addr = 0;
	server_s.sin.sin_addr.s_addr = INADDR_ANY;
	server_s.sin.sin_family = AF_INET;

	if (bind(server_s.server_fd, (struct sockaddr *)&server_s.sin,sizeof(struct sockaddr_in) ) == -1)
	{
	    cout << "Error binding socket to port " << port << ". Aborting. (" << strerror(errno) << ")" << endl;
	    return;
	}

	if (listen(server_s.server_fd, 10) == -1)
	{
	    cout << "Error while trying to listen(). Aborting." << ". Aborting. (" << strerror(errno) << ")" << endl;
	    return;
	}

    // Server blocks on this call until a client tries to establish connection.
    // When a connection is established, it returns a 'connected socket descriptor' different
    // from the one created earlier.
	socklen_t size = sizeof(server_s.client_addr);
    server_s.client_fd = accept(server_s.server_fd, (struct sockaddr *)&server_s.client_addr, &size);
    if (server_s.client_fd == -1)
    	cout << "Failed accepting connection" << ". Aborting. (" << strerror(errno) << ")" << endl;
    else
    	cout << "Client Connected" << endl;

	HandleClient();

	close(server_s.client_fd);
	close(server_s.server_fd);
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
