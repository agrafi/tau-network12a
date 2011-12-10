/*
 * protocol.h
 *
 *  Created on: Dec 1, 2011
 *      Author: aviv
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <wordexp.h>
#include <sys/stat.h>

using namespace std;

// Message code
#define SHOW_INBOX_C '1'
#define GET_MAIL_C '2'
#define GET_ATTACHMENT_C '3'
#define DELETE_MAIL_C '4'
#define QUIT_C '5'
#define COMPOSE_C '6'
#define LOGIN_C '7'

typedef enum {
	// requests
	LOGIN = '0',
	SHOW_INBOX,
	GET_MAIL,
	GET_ATTACHMENT,
	DELETE_MAIL,
	QUIT,
	COMPOSE,
	// responds
	LOGIN_RESPONSE,
	SHOW_INBOX_RESPONSE,
	GET_MAIL_RESPONSE,
	GET_ATTACHMENT_RESPONSE,
	DELETE_MAIL_RESPONSE,
	QUIT_RESPONSE,
	COMPOSE_RESPONSE
} msg_code;

typedef enum field_code {
	RECIPIENT,
	SUBJECT,
	USERNAME,
	PASSWORD
};

#pragma pack(push, 1)

typedef struct {
	field_code code;
	unsigned int len;
} field;

// Login structure
typedef struct {
	unsigned int username_len;
	unsigned int password_len;
} msg_data_login_request;

// Login structure
typedef struct {
	unsigned int result;
} msg_data_login_response;

// compose result structure
typedef struct {
	unsigned int result;
} msg_data_compose_response;


// Show Inbox Message structure
typedef struct {
// none
} msg_data_show_inbox;

// Get Mail Message structure
typedef struct {
	unsigned int mail_id;
} msg_data_get_mail;

// Get Attachment Message structure
typedef struct {
	unsigned int mail_id;
	unsigned int attachment_id;
} msg_data_get_attachment;

// Delete Mail Message structure
typedef struct {
	unsigned int mail_id;
} msg_data_delete_mail;

// Compose Message structure
typedef struct {
	unsigned int recipients_num;
	unsigned int subject_len;
	unsigned int text_len;
	unsigned int attachments_num;
} msg_data_compose;

typedef struct {
	unsigned int num_of_mails;
	unsigned int inbox_len;
} msg_data_show_inbox_response;

// Get Attachment Response Message structure
typedef struct {
	unsigned long long attachment_size;
	unsigned int filename_len;
} msg_data_get_attachment_respose;

typedef msg_data_get_attachment_respose msg_data_attachment_field;

typedef struct {
	unsigned int mail_id;
	unsigned int sender_len;
	unsigned int subject_len;
	unsigned int num_of_attachments;
	// followed by variable length data:
	// multiple fields with sender structure
	// field with subject structure
} msg_data_show_inbox_response_record;

typedef struct {
	unsigned int mail_len;
} msg_data_get_mail_response;

// Raw message to be sent on socket
typedef struct {
	// message magic
	int magic;
	// message code
	msg_code code;
	union {
		msg_data_login_request login_request;
		msg_data_login_response login_response;
		msg_data_show_inbox show_inbox;
		msg_data_get_mail get_mail;
		msg_data_delete_mail delete_mail;
		msg_data_get_attachment get_attachment;
		msg_data_compose compose;
		msg_data_compose_response compose_response;
		msg_data_get_attachment_respose get_attachment_response;
		msg_data_show_inbox_response show_inbox_response;
		msg_data_get_mail_response get_mail_response;
	};
} raw_msg;

#pragma pack(pop)

#define MAX_LINE 1024
#define MAX_BUFFER (1024*1024)

#define MSG_MAGIC 0xCAFEBABE

// helper functions - obsolete
int SendLineToSocket(int fd, string str);
int RecvLineFromSocket(int fd, string* ret);
int SendPacketToSocket(int fd, raw_msg* msg);

// Message/Socket operations
int GetNextMessage(int fd, raw_msg* msg);
int SendNextMessage(int fd, raw_msg* msg);
int SendDataToSocket(int fd, const char* data, int size);
int ReceiveDataFromSocket(int fd, char *data, int size, int receiveall);


// File operations
unsigned long long GetFileSize(char *pre_expanded_path);
string ExtractFilename( const string& path );
int WriteFileFromSocket(int fd, char *pre_expanded_path, unsigned long long size);
int WriteFileToSocket(int fd, char *pre_expanded_path, unsigned long long size);

char GetNextCommand(string command);

int strtoint(string s);
std::vector<std::string> split(const std::string &s, char delim);

#endif /* PROTOCOL_H_ */
