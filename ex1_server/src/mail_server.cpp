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

int Compose(int fd, user* u, raw_msg* arguments)
{
	char temp_buffer[MAX_BUFFER]; // temp buffer for reading/storing operations
	raw_msg msg;
	field f;

	msg.code = COMPOSE_RESPONSE;
	msg.magic = MSG_MAGIC;

	message_t *m = new message_t;
	m->from = u->username;

	// Receive recipients list
	for (unsigned int i=0; i<arguments->compose.recipients_num; i++)
	{

		if (-1 == ReceiveDataFromSocket(server_s.client_fd, (char*)&f, sizeof(f), 1))
		{
			delete m;
			return -1;
		}

		if (f.code != RECIPIENT)
		{
			cout << "Error. Expecting RECIPIENT field but I got " << f.code << endl;
			delete m;
			return -1;
		}

		memset(temp_buffer, 0, sizeof(temp_buffer));
		if (-1 == ReceiveDataFromSocket(server_s.client_fd, temp_buffer, f.len, 1))
		{
			return -1;
		}
		string recipient = temp_buffer;
		if (users_map.count(recipient) == 0)
		{
			cout << "Unknown recipient " << recipient << endl;
			return -1;
		}

		m->to.push_front(users_map[recipient]);
	}

	// Read subject
	memset(temp_buffer, 0, sizeof(temp_buffer));
	if (-1 == ReceiveDataFromSocket(server_s.client_fd, temp_buffer, arguments->compose.subject_len, 1))
	{
		return -1;
	}

	m->subject = temp_buffer;

	// Read text
	memset(temp_buffer, 0, sizeof(temp_buffer));
	if (-1 == ReceiveDataFromSocket(server_s.client_fd, temp_buffer, arguments->compose.text_len, 1))
	{
		return -1;
	}

	m->body = temp_buffer;

	// Receive attachment list
	for (unsigned int i=0; i<arguments->compose.attachments_num; i++)
	{
		msg_data_attachment_field f;
		if (-1 == ReceiveDataFromSocket(server_s.client_fd, (char*)&f, sizeof(f), 1))
		{
			delete m;
			return -1;
		}
		// f.filename_len = attachment_list[i].size();
		// f.attachment_size = GetFileSize((char*)attachment_list[i].c_str());


		// Receive Filename
		memset(temp_buffer, 0, sizeof(temp_buffer));
		if (-1 == ReceiveDataFromSocket(server_s.client_fd, temp_buffer, f.filename_len, 1))
		{
			// Loop over attachments and free them all
			attachment_pool::iterator attachmentsiterator;
			for(attachmentsiterator = m->attachments.begin();
					attachmentsiterator != m->attachments.end();
					attachmentsiterator++)
			{

				if ((*attachmentsiterator).second->data != NULL)
					free( (*attachmentsiterator).second->data);
			}
			delete m;
			return -1;
		}

		attachment* at = new attachment;
		at->filename = ExtractFilename(temp_buffer);
		at->data = calloc(1, f.attachment_size);
		at->size = f.attachment_size;
		at->refcount = m->to.size(); // Set ref count to number of recipients

		if (-1 == ReceiveDataFromSocket(server_s.client_fd, (char*)at->data, at->size, 1))
		{
			// Loop over attachments and free them all
			attachment_pool::iterator attachmentsiterator;
			for(attachmentsiterator = m->attachments.begin();
					attachmentsiterator != m->attachments.end();
					attachmentsiterator++)
			{

				if ((*attachmentsiterator).second->data != NULL)
					free ((*attachmentsiterator).second->data);
			}
			delete m;
			return -1;
		}

		m->attachments[i+1] = at; // list is one based
	}

	// Store mail in db for each recipient
	list <user*>::iterator listiterator;
	for(listiterator = m->to.begin();
			listiterator != m->to.end();
			listiterator++)
	{
		message_t *queued_msg = new message_t;
		*queued_msg = *m; // Clone message
		queued_msg->id = (*listiterator)->next_msg_id++;
		(*listiterator)->messages[queued_msg->id] = queued_msg;
	}
	msg.code = COMPOSE_RESPONSE;
	msg.compose_response.result = 1;

	if (-1 == SendNextMessage(server_s.client_fd, &msg))
	{
		// We keeps the mail in db event the reponse could not be sent.
		return -1;
	}

	cout << "Mail received" << endl;
	return 0;
}

int DeleteMail(int fd, user* u, raw_msg* arguments)
{
	unsigned int id = 1;

    id = arguments->delete_mail.mail_id;
    if (u->messages.count(id) == 0)
    {
    	// message id does not exists, abort
    	return -1;
    }

    // delete attachments from memory
	attachment_pool::iterator attachmentsiterator;
	for(attachmentsiterator = u->messages[id]->attachments.begin();
			attachmentsiterator != u->messages[id]->attachments.end();
			attachmentsiterator++)
	{

		if ((*attachmentsiterator).second->data != NULL)
			if (--(*attachmentsiterator).second->refcount == 0) // if ref count reached zero, delete
				free((*attachmentsiterator).second->data);
	}
	// delete message from memory
	delete(u->messages[id]);

	// remove from user inbox
	u->messages.erase(id);

	return 0;
}


int GetAttachment(int fd, user* u, raw_msg* arguments)
{
	raw_msg msg;

    if (u->messages.count(arguments->get_attachment.mail_id) == 0)
    {
    	// message id does not exists, abort
    	cout << "message id does not exists, abort" << endl;
    	return -1;
    }

    message* m = u->messages[arguments->get_attachment.mail_id];

    if (m->attachments.count(arguments->get_attachment.attachment_id) == 0)
    {
    	// attachment id does not exists, abort
    	cout << "attachment id " << arguments->get_attachment.attachment_id << " does not exists, abort" << endl;
    	return -1;
    }

    msg.code = GET_ATTACHMENT_RESPONSE;
    msg.get_attachment_response.attachment_size =
    		m->attachments[arguments->get_attachment.attachment_id]->size;
    msg.get_attachment_response.filename_len = m->attachments[arguments->get_attachment.attachment_id]->filename.size();

	if (-1 == SendNextMessage(server_s.client_fd, &msg))
	{
		return -1;
	}

	if (-1 == SendDataToSocket(fd, m->attachments[arguments->get_attachment.attachment_id]->filename.c_str(),
			msg.get_attachment_response.filename_len))
	{
		return -1;
	}

	cout << "Sending " << m->attachments[arguments->get_attachment.attachment_id]->size << " of data" << endl;
    return SendDataToSocket(fd, (char*) m->attachments[arguments->get_attachment.attachment_id]->data,
    		m->attachments[arguments->get_attachment.attachment_id]->size);
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
		ret << "\"" << (*attachmentsiterator).second->filename << "\"";
	}
	ret << endl;

	ret << "Text: " << m->body << endl;
	return ret.str();
}

int GetMail(int fd, user* u, raw_msg* arguments)
{
	unsigned int id = 1;
	string formatted_message = "";
	raw_msg msg;

	id =  arguments->get_mail.mail_id;
    cout << "Getting mail number " << id << endl;

    if (u->messages.count(id) == 0)
    {
    	// message id does not exists, abort
    	return -1;
    }

	formatted_message = FullMessage(u->messages[id]);
	msg.code = GET_MAIL_RESPONSE;
	msg.get_mail_response.mail_len = formatted_message.size();

	cout << "Sending " << msg.get_mail_response.mail_len << " chars:" << endl;

	if (-1 == SendNextMessage(server_s.client_fd, &msg))
	{
		return -1;
	}

	if (-1 == SendDataToSocket(server_s.client_fd, formatted_message.c_str(), msg.get_mail_response.mail_len))
	{
		return -1;
	}

	return 0;
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

int ShowInbox(int fd, user* u, raw_msg* arguments)
{
	string summary = "";
	raw_msg msg;
	msg.code = SHOW_INBOX_RESPONSE;
	msg.show_inbox_response.num_of_mails = u->messages.size();

	message_pool::iterator listiterator;
	for(listiterator = u->messages.begin();
			listiterator != u->messages.end();
			listiterator++)
	{
		summary += MessageSummary(listiterator->second);
		continue;
	}

	msg.show_inbox_response.inbox_len = summary.size();
	if (-1 == SendNextMessage(server_s.client_fd, &msg))
	{
		return -1;
	}
	// Send subject
	if (-1 == SendDataToSocket(server_s.client_fd, summary.c_str(), msg.show_inbox_response.inbox_len))
	{
		return -1;
	}

	return 0;
}

int Login()
{
	raw_msg msg;
	char temp_buffer[MAX_LINE+1];
	unsigned int username_len = 0, password_len = 0;
	string username = "";
	string password = "";

	if (-1 == GetNextMessage(server_s.client_fd, &msg))
	{
		return -1;
	}

	if (msg.code != LOGIN)
	{
		return -1;
	}

	username_len = (msg.login_request.username_len > MAX_LINE ? MAX_LINE : msg.login_request.username_len);
	password_len = (msg.login_request.password_len > MAX_LINE ? MAX_LINE : msg.login_request.password_len);

	memset(temp_buffer, 0, MAX_LINE+1);
	if (-1 == ReceiveDataFromSocket(server_s.client_fd, temp_buffer, username_len, 1))
	{
		return -1;
	}

	cout << temp_buffer << endl;
	username = temp_buffer;

	memset(temp_buffer, 0, MAX_LINE+1);
	if (-1 == ReceiveDataFromSocket(server_s.client_fd, temp_buffer, password_len, 1))
	{
		return -1;
	}

	password = temp_buffer;

	msg.code = LOGIN_RESPONSE;
	msg.magic = MSG_MAGIC;
	msg.login_response.result = 0;

	users_db::iterator listiterator;

	cout << "Client username is " << username << " and password " << password << endl;

	for(listiterator = users_map.begin();
			listiterator != users_map.end();
			listiterator++)
	{
		if ((listiterator->first.compare(username) == 0) && (listiterator->second->password.compare(password) == 0))
		{
			// found a match
			server_s.current_user = listiterator->second;
			msg.login_response.result = 1;
			cout << "Authenticated username is " << listiterator->first << endl;
			break;
		}
	}

	// Send login result
	if (-1 == SendNextMessage(server_s.client_fd, &msg))
	{
		return -1;
	}

	return 0;
}

void HandleClient()
{
	char quit = 0;
	raw_msg msg;

	if (-1 == Login())
		return;

	while (!quit)
	{
		// parse next request
		if(-1 == GetNextMessage(server_s.client_fd, &msg))
		{
			quit = 1;
			continue;
		}

		switch (msg.code)
		{
			case SHOW_INBOX_C:
				quit = ShowInbox(server_s.client_fd, server_s.current_user, &msg);
				break;
			case GET_MAIL_C:
				quit = GetMail(server_s.client_fd, server_s.current_user, &msg);
				break;
			case GET_ATTACHMENT_C:
				quit = GetAttachment(server_s.client_fd, server_s.current_user, &msg);
				break;
			case DELETE_MAIL_C:
				quit = DeleteMail(server_s.client_fd, server_s.current_user, &msg);
				break;
			case COMPOSE_C:
				quit = Compose(server_s.client_fd, server_s.current_user, &msg);
				break;
			case QUIT_C:
				quit = 1;
				break;
			default:
				// cout << "Undefined msg code " << msg.code << endl;
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

	// FOR DEBUGGING
	/*
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
	memset(a1->data, 'A', 50000);
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
	*/

	return;
}

void StartListeningServer(unsigned short port)
{
	if ((server_s.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Could not initialize socket. Aborting." << endl;
		return;
	}

	// For debugging only
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


	while (1)
	{
		socklen_t size = sizeof(server_s.client_addr);
		server_s.client_fd = accept(server_s.server_fd, (struct sockaddr *)&server_s.client_addr, &size);
		if (server_s.client_fd == -1)
		{
			cout << "Failed accepting connection" << ". Aborting. (" << strerror(errno) << ")" << endl;
			break;
		}
		else
		{
			cout << "Client Connected" << endl;
		}

		HandleClient();

		close(server_s.client_fd);
	}
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
