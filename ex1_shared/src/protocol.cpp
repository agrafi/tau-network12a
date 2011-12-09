/*
 * protocol.cpp
 *
 *  Created on: Dec 1, 2011
 *      Author: aviv
 */

#include "headers/protocol.h"

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

int SendDataToSocket(int fd, const char* data, int size)
{
	int nsent;

	nsent = send(fd, data, size, MSG_NOSIGNAL);
	do
	{
		if (nsent == -1)
		{
			fprintf(stderr, "socket send error: %s", strerror(errno));
			return -1;
		}

		if (nsent == size)
			break;

		size-= nsent;
		data+= nsent;

	} while(1);

	return 0;
}


int SendLineToSocket(int fd, string str)
{
	char newline = '\n';
	// TODO: make more robust
	send(fd, str.c_str(), str.size(), 0);
	send(fd, &newline, sizeof(newline), 0);

	return 0;
}

int ReceiveDataFromSocket(int fd, char *data, int size, int receiveall)
{
	int nread;
	int totread= 0;

	// If requested to get zero bytes, return immediately
	if (size == 0)
	{
		return 0;
	}

	do
	{
		nread= recv(fd, &(data[totread]), size - totread, 0);

		if (nread == -1)
		{
			fprintf(stderr, "recv error: %s\n", strerror(errno));
			return -1;
		}

		if (nread == 0)
		{
			printf("The other host terminated the connection.\n");
			return -1;
		}

		// If we want to return as soon as some data has been received,
		// let's do the job
		if (!receiveall)
			return nread;

		totread += nread;
	}
	while (totread != size);

	return totread;
}

// TODO: Replace with RecvDataFromSocket
int RecvLineFromSocket(int fd, string* ret)
{
	char buffer[MAX_LINE];
	int nRecv = 0, nTotal = 0;
	// init buffer
	memset(buffer, 0, MAX_LINE);

	// Read MAX_LINE chars or until newline
	while (nTotal < MAX_LINE)
	{
		nRecv = recv(fd, buffer + nTotal, 1, 0);
		if (nRecv == -1)
		{
			// try again
			if (errno == EAGAIN)
				continue;
			// error occurred
			perror(strerror(errno));
			return errno;
		}
		if ((buffer[nTotal] == '\r') || (buffer[nTotal] == '\n'))
		{
			// done.
			buffer[nTotal] = '\0';
			break;
		}
		nTotal += nRecv;
	}

	*ret = buffer;
	return 0;
}

int WriteFileFromSocket(int fd, char *path, unsigned long long size)
{
	fstream myfile;
	char buffer[MAX_BUFFER] = {0};
	unsigned long long nleft = size;

	myfile.open(path,ios::out | ios::binary);

	//verify that file opened correctly
	if (!myfile.is_open())
	{
		cout << "Could not open file " << path << endl;
		return -1;
	}

	while(nleft > 0)
	{
		unsigned int nwritten = (nleft > MAX_BUFFER ? MAX_BUFFER : nleft);
		if (-1 == ReceiveDataFromSocket(fd, buffer, nwritten, 1))
			return -1;
		myfile.write (buffer, nwritten);
		nleft -= nwritten;
	}
 	myfile.close();
	cout << "Attachment saved" << endl;

	return 0;
}

// Using the STL
int strtoint(string s)
{
  int ret;
  std::stringstream strStream(s);
  strStream >> ret;
  return ret;
}

int GetNextMessage(int fd, raw_msg* msg)
{
	int ret = -1;
	// receive entire header
	memset(msg, 0, sizeof(raw_msg));
	ret = ReceiveDataFromSocket(fd, (char*)msg, sizeof(raw_msg), 1);
	if ((0 == msg->code) || (msg->magic != MSG_MAGIC))
	{
		cout << "Malformed message. Aborting" << endl;
		return -1;
	}
	return ret;

}

int SendNextMessage(int fd, raw_msg* msg)
{
	msg->magic = MSG_MAGIC;
	return SendDataToSocket(fd, (char*)msg, sizeof(raw_msg));
}

char GetNextCommand(string command)
{
	if (command == "SHOW_INBOX")
		return SHOW_INBOX_C;
	if (command == "GET_MAIL")
		return GET_MAIL_C;
	if (command == "GET_ATTACHMENT")
		return GET_ATTACHMENT_C;
	if (command == "DELETE_MAIL")
		return DELETE_MAIL_C;
	if (command == "COMPOSE")
		return COMPOSE_C;
	if (command == "QUIT")
		return QUIT_C;
	return -1;
}

