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
#include <map>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include "protocol.h"

using namespace std;

#define MAX_LINE 1024

// Attachment structure
typedef struct
{
	string filename;
	unsigned long size;
	void* data;
	unsigned short refcount;
} attachment;

// Forward declaration
struct message;
typedef map <unsigned int, message*> message_pool;
typedef map <unsigned int, attachment*> attachment_pool;

// User structure
typedef struct {
	string username;
	string password;
	message_pool messages;
	unsigned int next_msg_id;
} user;

// Message structure
typedef struct message
{
	unsigned int id;
	string from;
	list <user*> to;
	string subject;
	attachment_pool attachments;
	string body;
} message_t;


// Server structure
typedef struct
{
	int server_fd;
	int client_fd;
	struct sockaddr_in sin;
    struct sockaddr_in client_addr;
    user* current_user;
} server;


// Globals
typedef map <string,user*> users_db;
users_db users_map;
server server_s;

#endif /* MAIL_SERVER_H_ */
