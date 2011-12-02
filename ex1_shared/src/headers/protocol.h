/*
 * protocol.h
 *
 *  Created on: Dec 1, 2011
 *      Author: aviv
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

// Message code
typedef enum {
	// requests
	SHOW_INBOX = 1,
	GET_MAIL,
	GET_ATTACHMENT,
	DELETE_MAIL,
	QUIT,
	COMPOSE,
	// responds
	SHOW_INBOX_RESPONSE,
	GET_MAIL_RESPONSE,
	GET_ATTACHMENT_RESPONSE,
	DELETE_MAIL_RESPONSE,
	QUIT_RESPONSE,
	COMPOSE_RESPONSE
} msg_code;

typedef enum field_code {
	SENDER,
	SUBJECT
};

typedef struct {
	field_code code;
	unsigned int len;
} field;

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
	int mail_id;
	int attachment_id;
} msg_data_get_attachment;

// Delete Mail Message structure
typedef struct {
	unsigned int mail_id;
} msg_data_delete_mail;

// Compose Message structure
typedef struct {
// TODO: fill me
} msg_data_compose;

typedef struct {
	unsigned int mail_id;
	unsigned int num_of_senders;
	unsigned int subject_len;
	unsigned int num_of_attachments;
	// followed by variable length data:
	// multiple fields with sender structure
	// field with subject structure
} msg_data_show_inbox_response;

typedef struct {
	// TODO: fill me
} msg_data_get_mail_response;

// Raw message to be sent on socket
typedef struct {
	// message code
	msg_code code;
	// request/response total size including trailers
	unsigned int  data_size;
} raw_msg;

#endif /* PROTOCOL_H_ */
