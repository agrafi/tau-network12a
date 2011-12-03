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


int SendLineToSocket(int fd, string str)
{
	char newline = '\n';
	// TODO: make more robust
	send(fd, str.c_str(), str.size(), 0);
	send(fd, &newline, sizeof(newline), 0);
	return 0;
}

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

// Using the STL
int strtoint(string s)
{
  int ret;
  std::stringstream strStream(s);
  strStream >> ret;
  return ret;
}

