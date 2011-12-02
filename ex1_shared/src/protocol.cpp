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

string RecvLineFromSocket(int fd)
{
	char buffer[MAX_LINE];
	string ret;
	recv(fd, buffer, MAX_LINE, 0);
	ret = buffer;
	std::vector<std::string> x = split(ret, '\r');
	ret = x[0];
	x = split(ret, '\n');
	ret = x[0];
	return ret;
}
