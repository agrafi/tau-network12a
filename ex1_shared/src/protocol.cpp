/*
 * protocol.cpp
 *
 *  Created on: Dec 1, 2011
 *      Author: aviv
 */

#include "protocol.h"

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

unsigned long long GetFileSize(char *pre_expanded_path)
{
	wordexp_t exp_result;
	struct stat filestatus;
	string path = "";

	// expand and open filename by full path
	wordexp(pre_expanded_path, &exp_result, 0);

	for (unsigned int i=0; i<exp_result.we_wordc ; i++)
	{
		path += exp_result.we_wordv[i];
		if (i<exp_result.we_wordc-1)
			path += " ";
	}

	if (-1 == stat( path.c_str(), &filestatus ))
	{
		wordfree(&exp_result);
		return 0;
	}

	wordfree(&exp_result);
	return filestatus.st_size;
}

int WriteFileFromSocket(int fd, char *pre_expanded_path, unsigned long long size)
{
	wordexp_t exp_result;
	string path = "";
	fstream myfile;
	char buffer[MAX_BUFFER] = {0};
	unsigned long long nleft = size;

	// expand and open filename by full path
	wordexp(pre_expanded_path, &exp_result, 0);

	for (unsigned int i=0; i<exp_result.we_wordc ; i++)
	{
		path += exp_result.we_wordv[i];
		if (i<exp_result.we_wordc-1)
			path += " ";
	}
	myfile.open(path.c_str(),ios::out | ios::binary);
	wordfree(&exp_result);

	//verify that file opened correctly
	if (!myfile.is_open())
	{
		cout << "Could not open file " << path << " " << strerror(errno) << endl;
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

int WriteFileToSocket(int fd, char *pre_expanded_path, unsigned long long size)
{
	wordexp_t exp_result;
	string path;
	fstream myfile;
	char buffer[MAX_BUFFER] = {0};
	unsigned long long nleft = size;
	streamsize nread = 0;

	// Trim \" from both sides
	if(pre_expanded_path[strlen(pre_expanded_path) - 1] == '\"')
	{
		pre_expanded_path[strlen(pre_expanded_path) - 1] = '\0';
	}
	if(pre_expanded_path[0] == '\"')
	{
		pre_expanded_path++;
	}

	// expand and open filename by full path
	wordexp(pre_expanded_path, &exp_result, 0);

	for (unsigned int i=0; i<exp_result.we_wordc ; i++)
	{
		path += exp_result.we_wordv[i];
		if (i<exp_result.we_wordc-1)
			path += " ";
	}

	myfile.open(path.c_str(), ios::in | ios::binary);
	wordfree(&exp_result);

	//verify that file opened correctly
	if (!myfile.is_open())
	{
		cout << "Could not open file " << path << " " << strerror(errno) << endl;
		return -1;
	}

	while(myfile.good())
	{
		myfile.read(buffer, MAX_BUFFER);
		nread = myfile.gcount();

		if (-1 == SendDataToSocket(fd, buffer, nread))
			return -1;

		nleft -= nread;
	}

	myfile.close();

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

string ExtractFilename( const string& path )
{
	string ret = path;
	if(ret[ret.length() - 1] == '\"')
	{
		ret.erase(ret.length()-1);
	}
	if(path[0] == '\"')
	{
		ret = ret.substr(1);
	}

	return ret.substr( ret.find_last_of( '/' ) +1 );
}


int GetNextMessage(int fd, raw_msg* msg)
{
	int ret = -1;
	// receive entire header
	memset(msg, 0, sizeof(raw_msg));
	ret = ReceiveDataFromSocket(fd, (char*)msg, sizeof(raw_msg), 1);
	if ((0 == msg->code) || (msg->magic != MSG_MAGIC))
	{
		//cout << "Malformed message. Aborting" << endl;
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

