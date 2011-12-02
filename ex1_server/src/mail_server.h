/*
 * mail_server.h
 *
 *  Created on: Dec 1, 2011
 *      Author: aviv
 */


#ifndef MAIL_SERVER_H_
#define MAIL_SERVER_H_

#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

#define MAX_LINE 1024

// Attachment structure
struct attachment_t
{
	string filename;
	void* data;
} attachment;

// Forward declaration
struct message_t;

// User structure
struct user_t {
	string username;
	string password;
	list <message_t> messages;
} user;

// Message structure
struct message_t
{
	string from;
	list <user_t> to;
	string subject;
	list <attachment_t> attachments;
	string body;
} message;


// Server structure
struct server_t
{
	int server_fd;
	int client_fd;
	struct sockaddr_in sin;
    struct sockaddr_in client_addr;
} server;


// Globals
list <user_t> users_list;
server_t server_s;

#endif /* MAIL_SERVER_H_ */
