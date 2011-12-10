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
#include <algorithm>
#include <string>
#include <errno.h>
#include <string.h>
#include <netdb.h>          /* hostent struct, gethostbyname() */
#include <arpa/inet.h>      /* inet_ntoa() to format IP address */
#include <netinet/in.h>     /* in_addr structure */
#include "protocol.h"
#include "mail_client.h"

using namespace std;

int ShowInbox(string arguments)
{
	raw_msg msg;
	msg.magic = MSG_MAGIC;
	msg.code = SHOW_INBOX;

	// Send header
	if (-1 == SendNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Get result header
	if (-1 == GetNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Verify message
	if (msg.code != SHOW_INBOX_RESPONSE)
	{
		return -1;
	}

	char* inbox_buffer = (char*) calloc(1, msg.show_inbox_response.inbox_len);
	if (-1 == ReceiveDataFromSocket(client_s.client_fd, inbox_buffer, msg.show_inbox_response.inbox_len, 1))
	{
		delete inbox_buffer;
		return -1;
	}

	cout << inbox_buffer;

	// dispose buffer
	free(inbox_buffer);
	return 0;
}

int GetMail(string arguments)
{
	raw_msg msg;
	msg.magic = MSG_MAGIC;
	msg.code = GET_MAIL;

	string id_str = "";
	std::stringstream   linestream(arguments);
	std::getline(linestream, id_str, ' ');  // read up-to the first space
	std::getline(linestream, id_str, '\n');  // read up-to the second space

	msg.get_mail.mail_id = strtoint(id_str);

	// Send header
	if (-1 == SendNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Get result header
	if (-1 == GetNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Verify message
	if (msg.code != GET_MAIL_RESPONSE)
	{
		return -1;
	}

	char* mail_buffer = (char*)calloc(1, msg.get_mail_response.mail_len + 1);
	if (-1 == ReceiveDataFromSocket(client_s.client_fd, mail_buffer, msg.get_mail_response.mail_len, 1))
	{
		free(mail_buffer);
		return -1;
	}

	cout << mail_buffer;

	// dispose buffer
	free (mail_buffer);
	return 0;
}

int GetAttachment(string arguments)
{
	raw_msg msg;
	msg.magic = MSG_MAGIC;
	msg.code = GET_ATTACHMENT;

	string mail_id_str = "";
	string attachment_id_str = "";
	string path_str = "";
	string full_path = "";

	std::stringstream   linestream(arguments);
	std::getline(linestream, mail_id_str, ' ');  // read up-to the first space
	std::getline(linestream, attachment_id_str, ' ');  // read up-to the second space
	std::getline(linestream, path_str, '\n');  // read up-to the newline

	if (path_str.length() == 0)
		return -1;

	msg.get_attachment.mail_id = strtoint(mail_id_str);
	msg.get_attachment.attachment_id = strtoint(attachment_id_str);

	if(path_str[path_str.length() - 1] == '\"')
	{
		path_str.erase(path_str.length()-1);
	}
	if(path_str[0] == '\"')
	{
		path_str = path_str.substr(1);
	}


	// Send header
	if (-1 == SendNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Get result header
	if (-1 == GetNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Verify message
	if (msg.code != GET_ATTACHMENT_RESPONSE)
	{
		return -1;
	}

	char* filename = (char*)calloc(1, msg.get_attachment_response.filename_len);
	if (-1 == ReceiveDataFromSocket(client_s.client_fd, filename, msg.get_attachment_response.filename_len, 1))
	{
		free(filename);
	}

	full_path = path_str.append("/");
	full_path.append(filename);

	if (-1 == WriteFileFromSocket(client_s.client_fd, (char*) full_path.c_str(),
			msg.get_attachment_response.attachment_size))
	{
		free(filename);
		return -1;
	}

	free(filename);
	return 0;
}

int DeleteMail(string arguments)
{
	raw_msg msg;
	msg.magic = MSG_MAGIC;
	msg.code = DELETE_MAIL;

	string id_str = "";
	std::stringstream   linestream(arguments);
	std::getline(linestream, id_str, ' ');  // read up-to the first space
	std::getline(linestream, id_str, '\n');  // read up-to the second space

	msg.delete_mail.mail_id = strtoint(id_str);

	// Send header
	if (-1 == SendNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	return 0;
}

int Compose(string arguments)
{
	raw_msg msg;
	msg.code = COMPOSE;
	msg.magic = MSG_MAGIC;

	string line;
	string to, subject, attachments, text;

	if (arguments.length() > 0)
	{
		cout << "COMPOSE arguments should be empty" << endl;
		return -1;
	}

	getline (cin,line);
	stringstream linestream_to(line);
	getline(linestream_to, to, ' ');  // read up-to the first space
	getline(linestream_to, to, '\n');  // read up-to the newline

	getline (cin,line);
	stringstream linestream_subject(line);
	getline(linestream_subject, subject, ' ');  // read up-to the first space
	getline(linestream_subject, subject, '\n');  // read up-to the newline

	getline (cin,line);
	stringstream linestream_attachments(line);
	getline(linestream_attachments, attachments, ':');  // read up-to the first :
	getline(linestream_attachments, attachments, ' ');  // read up-to the first space
	getline(linestream_attachments, attachments, '\n');  // read up-to the newline

	getline (cin,line);
	stringstream linestream_text(line);
	getline(linestream_text, text, ' ');  // read up-to the first space
	getline(linestream_text, text, '\n');  // read up-to the newline

	// Split To: line
	vector<string> to_list = split(to, ',');
	vector<string> attachment_list = split(attachments, ',');

	msg.compose.recipients_num = to_list.size();
	msg.compose.subject_len = subject.size();
	msg.compose.text_len = text.size();
	msg.compose.attachments_num = attachment_list.size();

	// Send header
	if (-1 == SendNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	field f;

	// Send recipients list
	for (unsigned int i=0; i<to_list.size(); i++)
	{
		f.code = RECIPIENT;
		f.len = to_list[i].size();

		if (-1 == SendDataToSocket(client_s.client_fd, (char*)&f, sizeof(f)))
		{
			return -1;
		}

		if (-1 == SendDataToSocket(client_s.client_fd, to_list[i].c_str(), to_list[i].size()))
		{
			return -1;
		}
	}

	// Send subject
	if (-1 == SendDataToSocket(client_s.client_fd, subject.c_str(), subject.size()))
	{
		return -1;
	}

	// Send text
	if (-1 == SendDataToSocket(client_s.client_fd, text.c_str(), text.size()))
	{
		return -1;
	}

	// Send attachment list
	for (unsigned int i=0; i<attachment_list.size(); i++)
	{
		msg_data_attachment_field f;
		string filename = ExtractFilename(attachment_list[i]);
		f.filename_len = filename.size();
		f.attachment_size = GetFileSize((char*)attachment_list[i].c_str());

		if (-1 == SendDataToSocket(client_s.client_fd, (char*)&f, sizeof(f)))
		{
			return -1;
		}

		// Send filename without the full path
		if (-1 == SendDataToSocket(client_s.client_fd, filename.c_str(), filename.size()))
		{
			return -1;
		}

		// Send file
		if (-1 == WriteFileToSocket(client_s.client_fd, (char*) (attachment_list[i].c_str()),
				f.attachment_size))
		{
			return -1;
		}

	}

	// Get result header
	if (-1 == GetNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Verify message
	if (msg.code != COMPOSE_RESPONSE)
	{
		cout << "msg.code != COMPOSE_RESPONSE" << endl;
		return -1;
	}

	// Check compose result
	if (msg.compose_response.result != 1)
	{
		cout << "msg.compose_response.result != 1" << endl;
		return -1;
	}

	cout << "Mail sent" << endl;
	return 0;
}

int HandleCommands()
{
	char quit = 0;
	int code;
	string command, arguments;
	while (!quit)
	{
		command = arguments = "";
		// parse next request
		string line;
		getline(cin, line);

		std::stringstream   linestream(line);
		std::getline(linestream, command, ' ');  // read up-to the first space
		std::getline(linestream, arguments, '\n');  // read up-to the newline

		if (-1 == (code = GetNextCommand(command)))
			continue;

		switch (code)
		{
			case SHOW_INBOX_C:
				quit = ShowInbox(arguments);
				break;
			case GET_MAIL_C:
				quit = GetMail(arguments);
				break;
			case GET_ATTACHMENT_C:
				quit = GetAttachment(arguments);
				break;
			case DELETE_MAIL_C:
				quit = DeleteMail(arguments);
				break;
			case COMPOSE_C:
				quit = Compose(arguments);
				break;
			case QUIT_C:
				quit = 1;
				break;
			default:
				cout << "Undefined msg code " << code << endl;
				continue;
		}
	}
	return 0;
}


int Login()
{
	string line;
	string username, password;
	getline (cin,line);
	vector<string> x = split(line, ' ');
	if (x.size() < 2)
		return -1;
	username = x[1];

	getline (cin,line);
	x = split(line, ' ');
	if (x.size() < 2)
		return -1;
	password = x[1];

	raw_msg msg;
	msg.code = LOGIN;
	msg.magic = MSG_MAGIC;
	msg.login_request.username_len = username.size();
	msg.login_request.password_len = password.size();

	// Send header
	if (-1 == SendNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Send username
	if (-1 == SendDataToSocket(client_s.client_fd, username.c_str(), username.size()))
	{
		return -1;
	}

	// Send password
	if (-1 == SendDataToSocket(client_s.client_fd, password.c_str(), password.size()))
	{
		return -1;
	}

	// Get result header
	if (-1 == GetNextMessage(client_s.client_fd, &msg))
	{
		return -1;
	}

	// Verify message
	if (msg.code != LOGIN_RESPONSE)
	{
		return -1;
	}

	// Check login result
	if (msg.login_response.result != 1)
	{
		return -1;
	}

	cout << "Connected to server" << endl;
	return 0;
}

int ConnectToServer(const char* hostname, unsigned short port)
{
      struct sockaddr_in echoServAddr; /* Echo server address */

      struct hostent *host;     /* host information */
        if ((host = gethostbyname(hostname)) == NULL)
        {
  		  cout << "Could not resolve server name: " << strerror(errno) << endl;
  		  return -1;
        }

      /* Create a reliable, stream socket using TCP */
      if ((client_s.client_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
		  cout << "Could not connect to server: " << strerror(errno) << endl;
		  return -1;
      }

      /* Construct the server address structure */
      memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
      echoServAddr.sin_family      = AF_INET;             /* Internet address family */
      echoServAddr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);   /* Server IP address */
      echoServAddr.sin_port        = htons(port); /* Server port */

      /* Establish the connection to the echo server */
      if (connect(client_s.client_fd, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
      {
		  cout << "Could not connect to server: " << strerror(errno) << endl;
		  return -1;
      }

	return 0;
}

int main(int argc, char** argv) {
	unsigned short port = 6423;
	const char* host = "localhost";


	if (argc > 3)
	{
		cout << "Usage: " << argv[0] << " [host] [port]" << endl;
		return 2;
	}

	// Get non default port
	if (argc == 3)
	{
		port = atoi(argv[2]);
	}

	// Get non default host
	if (argc == 2)
	{
		host = argv[1];
	}

	if (-1 == ConnectToServer(host, port))
	{
		return 1;
	}

	cout << "Welcome! I am simple-mail-server." << endl;

	if (-1 == Login())
	{
		cout << "Login failed. Aborting." << endl;
		return 1;
	}

	HandleCommands();

	close(client_s.client_fd);

	return 0;
}
