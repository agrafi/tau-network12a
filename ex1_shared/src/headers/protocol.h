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
	SHOW_INBOX = 1,
	GET_MAIL,
	GET_ATTACHMENT,
	DELETE_MAIL,
	QUIT,
	COMPOSE
} msg_code;

// Show Inbox Message structure
typedef struct {
// none
} msg_data_show_inbox;

// Get Mail Message structure
typedef struct {
	int mail_id;
} msg_data_get_mail;

// Get Attachment Message structure
typedef struct {
	int mail_id;
	int attachment_id;
} msg_data_get_attachment;

// Delete Mail Message structure
typedef struct {
	int mail_id;
} msg_data_delete_mail;

// Compose Message structure
typedef struct {
// TODO: fill me
} msg_data_compose;


// Raw message to be sent on socket
typedef struct {
	msg_code code;
	int data_size;
	union data_t {
		msg_data_show_inbox show_inbox;
		msg_data_get_mail get_mail;
		msg_data_get_attachment get_attachment;
		msg_data_delete_mail delete_mail;
		msg_data_compose compose;
	} data;
} raw_msg;

#endif /* PROTOCOL_H_ */
