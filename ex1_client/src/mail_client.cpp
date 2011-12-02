#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#define DEFAULT_PORT "6423"

#define DIG_NUM 10
#define MAX_BUFF_LEN 100
#define MAX_PATH 150
#define NAME_LEN 25
#define PASS_LEN 20
#define COMM_LEN 20
#define LIST_LEN 50
#define ATTACH_LEN 256//???????????????????????????????? or char*
#define TEXT_LEN 256
#define SUB_LEN 100
#define RECV_FAILED_MSG "data receiving failed"
#define SEND_FAILED_MSG  "data sendeing failed"
#define MEM_ALLOC_ERROR_EXIT "memory allocation error"
#define STDIN_ERROR "stdin error"
#define SOCKET_CLOSE_ERROR "socket close failed "
#define ERROR_CODE -1
#define SHOW_INBOX "SHOW INBOX"
#define GET_MAIL "GET_MAIL"
#define GET_ATTACHMENT "GET_ATTACHMENT"
#define DELETE_MAIL "DELETE_MAIL"
#define QUIT "QUIT"
#define COMPOSE "COMPOSE"

//////////////////////////////////////////////////////////////////
///////////////////////STRUCTS DEFINITION/////////////////////////
//////////////////////////////////////////////////////////////////

#pragma pack(push,1)
typedef struct {
	char comm_name[COMM_LEN];
	int mail_id;
	int attach_num;
	char filePath[MAX_PATH];
} COMMAND;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
	char* receivers[LIST_LEN];
	char subject[SUB_LEN];
	char* attachments[LIST_LEN];
	char* text;
} MSG;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
	char* user;
	char* password;
} IDENT;
#pragma pack(pop)

//////////////////////////////////////////////////////////////////
////////////////////MAIL FUNCTIONS////////////////////////////////
//////////////////////////////////////////////////////////////////

//receives string , fills the command struct
void strToCommStruct(COMMAND* comm, char* strComm) {
	int i = 0, j = 0, k = 0;
	int str_len = strlen(strComm);
	char str_mail_id[6];
	for (i = 0; i < str_len; i++) {
		if (strComm[i] != ' ') {
			if (j == 0) {
				comm->comm_name[k] = strComm[i];
				k++;
			} else if (j == 1) {
				str_mail_id[k] = strComm[i];
				k++;
			} else if (j == 2) {
				comm->filePath[k] = strComm[i];
				k++;
			}
		} else {
			j++;
			k = 0;
		}
	}
	comm->mail_id = atoi(str_mail_id);
}

//receives string , fills  receivers list in message
void strToReceiversList(char* list_receivers[LIST_LEN], char* str_receivers) {
	int i = 0, j = 0;
	char str[NAME_LEN];
	while (str_receivers[i] != '\n') {
		if (str_receivers[i] != ',') {
			str[j] = str_receivers[i];
			j++;
		} else {
			strcpy(list_receivers[j], str);
			j = 0;
			bzero(str, NAME_LEN);

		}
		i++;
	}

}

//receives string , fills  attachment list in message
void strToAttachList(char* list_attachments[LIST_LEN], char* str_attachment) {
	int i = 0, j = 0, k = 0; //when k even , start new word to parse
	char str[MAX_PATH];
	while (str_attachment[i] != '\n') {
		if (str_attachment[i] == '"') {
			k++;
			i++;
			if (k != 0 && k % 2 == 0) { // finished copy a path
				strcpy(list_attachments[j], str);
				j = 0;
				bzero(str, MAX_PATH);
			}
		} else if ((str_attachment[i - 1] == 34)
				&& (str_attachment[i] == ',')) {
			i++; //after comma increase i
		} else {
			str[j] = str_attachment[i];
			j++;
			i++;
		}
	}
}

// function calculates length of the list in bytes (all elements)
int sizeElemList(char* list[LIST_LEN]) {
	int list_size = sizeof(list);
	int i, size = 0;
	for (i = 0; i < list_size; i++) {
		size += strlen(list[i]) + 1;
	}
	return size;
}

//////////////////////////////////////////////////////////////////
//////////////CONNECTION FUNCTIONS////////////////////////////////
//////////////////////////////////////////////////////////////////

// verify send() for TCP connection  (from recitation)
int sendAll(int s, char *buf, int *len) {
	int total = 0; // how many bytes we've sent
	int bytesleft = *len; // how many have left to send
	int n;
	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent
	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

// receive  data from buff, return number of received bits or error -1
int receiveAll(int s, char *buf) {
	int indexPos = 0, numBytes;
	while (indexPos == 0 || !(buf[indexPos - 1] == '\n')) {
		numBytes = recv(s, buf + indexPos, MAX_BUFF_LEN, 0);
		if (numBytes == 0 || numBytes == -1)
			return numBytes;
		indexPos += numBytes;
	}
	return indexPos;
}

// function sends error message
void error(int sock, char* msg) {
	perror(msg);
	close(sock);
	exit(0);
}

//////////////////////////////////////////////////////////////////
//////////////OTHER FUNCTIONS/////////////////////////////////////
//////////////////////////////////////////////////////////////////

//function receives list of receivers, attachments and returns string of receivers, attachments
char* makeString(char* list[LIST_LEN], int num_recipients) {
	char* str = malloc(sizeof(list) + 50);
	int i;
	for (i = 0; i < LIST_LEN; i++) {
		strcpy(str, list[i]); //TODO take care of "" in attachments
		str += ',';
	}
	return str;
}

//function receives list and returns number od elements in it
int numElem(char* arr[LIST_LEN]) {
	int count = 0;
	int i;
	for (i = 0; arr[i]; i++) {
		count++;
	}
	return count;
}

//the programs receives 2 parameters: host & port
int main(int argc, char *argv[]) {
	typedef COMMAND *command;
	command com;
	typedef IDENT *ident;
	ident id;
	typedef MSG *mess;
	mess message;

	char command_obj[COMM_LEN];
	char str[MAX_PATH];
	char str_tmp_int[MAX_PATH];
	char str_mail_id[DIG_NUM]; // mail id
	char str_attach_num[DIG_NUM]; // number of attchments
	char str_int[DIG_NUM];
	char *str_rec; //receivers string
	char *buf; //text buffer
	char *attchStr;
	int ln, mail_id_int, ln_path, num_int, ln_com;
	char c, host_name[80] = { '\0' }, port[6] = DEFAULT_PORT, in_buf[MAX_BUFF_LEN];
	int mySock = 0, res, num_lines, i, approved;
	struct addrinfo hints, *servinfo, *p;

	if (argc == 2) {
		strcpy(host_name, argv[1]);
	} else if (argc > 2) {
		strcpy(host_name, argv[1]);
		strcpy(port, argv[2]);
	}
	printf("mail_client \n");
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (host_name[0] == '\0') {
		hints.ai_flags = AI_PASSIVE; // use my IP address
		res = getaddrinfo(NULL, port, &hints, &servinfo);
	} else {
		res = getaddrinfo(host_name, port, &hints, &servinfo);
	}
	if (res != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((mySock = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== ERROR_CODE) {
			fprintf(stderr, "socket() error : %d\n", errno);
			continue;
		}
		if (connect(mySock, p->ai_addr, p->ai_addrlen) == ERROR_CODE) {
			if (close(mySock) == ERROR_CODE)
				fprintf(stderr, "close() error : %d\n", errno);
			fprintf(stderr, "connect() error : %d\n", errno);
			continue;
		}
		printf("Welcome! I am simple-mail-server. \n");
		break; // if we get here- connected successfully
	}
	if (p == NULL) { // looped off the end of the list with no connection
		fprintf(stderr, "failed to connect\n");
		return 1;
	}
	freeaddrinfo(servinfo);
	//user input user name & password
	id = (ident) calloc(1, sizeof(IDENT));
	if (id == NULL) {
		error(mySock, MEM_ALLOC_ERROR_EXIT);
	}
	printf("user: "); //TODO:take care of type of input user print USER:USER NAME
	memset(id->user, 0, NAME_LEN);
	if (fgets(id->user, NAME_LEN, stdin) == NULL) {
		free(id);
		error(mySock, STDIN_ERROR);
	}
	printf("password: "); //TODO:take care of type of input user print USER:USER NAME
	memset(id->password, 0, PASS_LEN);
	if (fgets(id->password, PASS_LEN, stdin) == NULL) {
		free(id);
		error(mySock, STDIN_ERROR);
	}

	//sending user name & password to server
	ln = NAME_LEN;
	if (sendAll(mySock, id->user, &ln) == ERROR_CODE) {
		free(id);
		error(mySock, SEND_FAILED_MSG);
	}
	ln = PASS_LEN;
	if (sendAll(mySock, id->password, &ln) == ERROR_CODE) {
		free(id);
		error(mySock, SEND_FAILED_MSG);
	}
	free(id);

	//receiving approval for connection
	if (receiveAll(mySock, in_buf) == ERROR_CODE) {
		error(mySock, RECV_FAILED_MSG);
	}
	approved = atoi(in_buf);
	if (approved == 1) {
		printf("Connected to  server");
	} else
		printf("Error: connection to server failed.");

	com = (command) calloc(1, sizeof(COMMAND));
	if (com == NULL) {
		error(mySock, MEM_ALLOC_ERROR_EXIT);
	}
	// get command name from the user
	while (strcmp(com->comm_name, QUIT) != 0) {
		if (fgets(command_obj, COMM_LEN, stdin) == NULL) {
			free(com);
			error(mySock, STDIN_ERROR);
		}
		strToCommStruct(com, command_obj);
		bzero(command_obj, COMM_LEN); /* init to command_obj */
		ln_com = COMM_LEN;
		if (strcmp(com->comm_name, SHOW_INBOX) == 0) {
			if (sendAll(mySock, com->comm_name, &ln_com) == ERROR_CODE) {//send to server message SHOW_INBOX
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}
			if (receiveAll(mySock, in_buf) == ERROR_CODE) {//receive from server number of lines in inbox
				free(com);
				error(mySock, RECV_FAILED_MSG);
			}
			num_lines = atoi(in_buf);
			i = 1;
			while (i <= num_lines) {//receive line after line with mails
				if (receiveAll(mySock, in_buf) == ERROR_CODE) {
					free(com);
					error(mySock, RECV_FAILED_MSG);
				}
				printf("%s \n", in_buf);// print mails in inbox in order: mail_id sender "subject" num_attachments
				i++;
			}

		} else if (strcmp(com->comm_name, GET_MAIL) == 0) {
			if (sendAll(mySock, com->comm_name, &ln_com) == ERROR_CODE) {//send to server message GET_MAIL
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}
			memset(str_mail_id, 0, DIG_NUM);
			sprintf(str_mail_id, "%d", com->mail_id);
			mail_id_int = DIG_NUM;
			if (sendAll(mySock, str_mail_id, &mail_id_int) == ERROR_CODE) {//send to server id of the mail taht we want to receive
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}

			//???????????????Does client need to save mails or just print them?
			i = 1;
			while (i <= 5) {// receiving five parts of mail: From, To, Subject, Attachments, Text
				if (receiveAll(mySock, in_buf) == ERROR_CODE) {
					free(com);
					error(mySock, RECV_FAILED_MSG);
				}
				printf("%s \n", in_buf);//print each part
				i++;
			}

		} else if (strcmp(com->comm_name, GET_ATTACHMENT) == 0) {
			if (sendAll(mySock, com->comm_name, &ln_com) == ERROR_CODE) {//send to server message GET_ATTACHMENT
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}
			memset(str_mail_id, 0, DIG_NUM);
			sprintf(str_mail_id, "%d", com->mail_id);
			if (sendAll(mySock, str_mail_id, &mail_id_int) == ERROR_CODE)//send to server message with mail id,
				free(com);
			error(mySock, SEND_FAILED_MSG);

			memset(str_attach_num, 0, DIG_NUM);
			sprintf(str_attach_num, "%d", com->attach_num);
			if (sendAll(mySock, str_attach_num, &num_int) == ERROR_CODE)
				free(com);
			error(mySock, SEND_FAILED_MSG);

			ln_path = MAX_PATH;
			if (sendAll(mySock, com->filePath, &ln_path) == ERROR_CODE) {
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}

			if (receiveAll(mySock, in_buf) == ERROR_CODE) {
				free(com);
				error(mySock, RECV_FAILED_MSG);
			}
			//safeAttacment(in_buf)
			//TODO get path + safe attachment and print Atachment saved
			printf("Attachment saved");
		} else if (strcmp(com->comm_name, DELETE_MAIL) == 0) {
			if (sendAll(mySock, com->comm_name, &ln_com) == ERROR_CODE) {
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}

			memset(str_mail_id, 0, DIG_NUM);
			sprintf(str_mail_id, "%d", com->mail_id);
			if (sendAll(mySock, str_mail_id, &mail_id_int) == ERROR_CODE) {
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}

			if (receiveAll(mySock, in_buf) == ERROR_CODE) {
				free(com);
				error(mySock, RECV_FAILED_MSG);
			}
			if (in_buf == NULL) {
				printf("Problem with deleting mail ");
			}

			/* the user choose QUIT command, so mail client will exit
			 * he will send to server QUIT command and will close connection */

		} else if (strcmp(com->comm_name, QUIT) == 0) {
			if (sendAll(mySock, com->comm_name, &ln_com) == ERROR_CODE) {
				free(com);
				error(mySock, SEND_FAILED_MSG);
			}
			if (close(mySock) == ERROR_CODE) {
				error(mySock, SOCKET_CLOSE_ERROR);

			} else if (strcmp(com->comm_name, COMPOSE) == 0) {
				if (sendAll(mySock, com->comm_name, &ln_com) == ERROR_CODE) {
					free(com);
					error(mySock, SEND_FAILED_MSG); //TODO: do i need this ?
				}
				message = (mess) calloc(1, sizeof(MSG));
				if (message == NULL) {
					free(com);
					error(mySock, MEM_ALLOC_ERROR_EXIT);
				}
				printf("To: ");
				fgets(str, NAME_LEN * LIST_LEN, stdin); //TODO: check error
				strToReceiversList(message->receivers, str);
				bzero(str, MAX_PATH);
				printf("Subject: ");
				fgets(message->subject, SUB_LEN, stdin); //TODO: check error
				bzero(str, MAX_PATH);
				printf("Attachments: ");
				fgets(str, ATTACH_LEN * LIST_LEN, stdin);
				strToAttachList(message->attachments, str); //TODO:maybe enter it to a list of attach
				bzero(str, MAX_PATH);
				printf("Text: ");
				if ((buf = (char*) calloc(TEXT_LEN,
						sizeof(char))) == NULL) {
					free(com);
					free(message);
					error(mySock, MEM_ALLOC_ERROR_EXIT);
				}
				i = 0;
				while ((c = getchar()) != '\n') {
					buf[i] = c;
					i++;
				}
				message->text = buf;
				/*  send each part of the struct as independent */
				//sending receivers
				// client send the size of elements inside receiversList
				ln = sizeElemList(message->receivers);
				//itoa(,,10);
				snprintf(str, LIST_LEN, "%d", ln);

				if (sendAll(mySock, str, &num_int) == ERROR_CODE) {
					free(message);
					error(mySock, SEND_FAILED_MSG);
				}
				ln = numElem(message->receivers);
//				itoa(str, ln, 10);
				str_rec = (char*) calloc(1, sizeof(COMMAND));
				if (message == NULL) {
					free(com);
					error(mySock, MEM_ALLOC_ERROR_EXIT);
				}
				str_rec = makeString(message->receivers, ln);
				if (sendAll(mySock, str_rec,
						&num_int) == ERROR_CODE) {
					free(message);
					error(mySock, SEND_FAILED_MSG);
				}

				//ln = sizeElemList(message->receivers);
				//memset(str_int, 0, DIG_NUM);
				//sprintf(str_int, "%d", ln);
				//if (sendAll(mySock, str_int, &num_int) == ERROR_CODE) {
				//	free(message);
				//	error( mySock, SEND_FAILED_MSG);
				//}
				//n=numElem (char** arr
				// Second client send all list

				//sending subject
				ln= SUB_LEN;
				sendAll(mySock, message->subject, &ln);
				//sending attachments

				ln = sizeElemList(message->attachments);
				snprintf(str_tmp_int, LIST_LEN, "%d",ln );
				if (sendAll(mySock, str_tmp_int, &num_int) == ERROR_CODE) {
					free(message);
					error(mySock, SEND_FAILED_MSG);
				}
				ln = numElem(message->attachments);
				//itoa(str_tmp_int, ln, 10);
				buf = makeString(message->attachments, ln);
				if (sendAll(mySock, buf, &num_int) == ERROR_CODE) {
					free(message);
					error(mySock, SEND_FAILED_MSG);
				}
				//sendAllAttachments(mySock, message->attachments);
				//sending text
				memset(str_int, 0, DIG_NUM);
				sprintf(str_int, "%d", i + 1);
				num_int = DIG_NUM;
				sendAll(mySock, str_int, &num_int); //sends number of bytes in text
				ln = i + 1;
				sendAll(mySock, message->text, &ln);
				free(message->text);
				free(message);
				printf("Mail sent");
			} else {
				/* if user does not enters the right command, he as another chance */
				printf("No such command.\nTry again.\n");
			}
			memset(com->comm_name, 0, COMM_LEN);
			memset(com->filePath, 0, MAX_PATH);
		}
		free(com);
		exit(0);
	}
}
