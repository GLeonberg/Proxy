//-----------------------------------------------------------------------
// Program: proxy.c
// Author: Gregory Leonerg
//
// This program works by creating a socket on the port given as an
// argument and then listening for incoming connections on that port.
// Once a connection is made, the message from the client is read into
// a buffer. The buffer is then parsed to check the request type as well
// as extract the various necesarry tokens (such as url, path, etc).
// Once the tokens are extracted, a new GET request is constructed.
//
// The website address is used to find the IP of the website, and then
// a socket connecction to that website is created. The newly constructed
// GET request is sent over that connection and the reponse is read
// into a new buffer. The contents of that buffer are then sent back
// to the client.
//-----------------------------------------------------------------------

// required libraries
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "csapp.h"

static const char* progName = "proxy";

// Generic error function
void err()
{
	perror(progName);
	exit(2);
}

int main(int argc, char* argv[])
{
	// check argument
	if(argc == 1)
	{
		printf("Please enter the port number as an argument!\n");
		exit(1);
	}

	// create log file
	FILE* log;
	if ( (log = fopen("log.txt", "w")) == NULL )
		err();

	// make socket
	int fd = 0;
	if( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err();

	// bind
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	if( bind(fd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
		err();

	// listen
	if( listen(fd, 5) < 0)
		err();

	// infinite loop of accepting and processing clients
	while(1)
	{
		// accept request, store client info
		int cfd = 0;

		struct sockaddr_in client_addr;
		int client_size = sizeof(client_addr);
		if( (cfd = accept(fd, (struct sockaddr*) &client_addr, &client_size)) < 0)
			err();

		// read request
		int reqlen = 0;
		char reqbuf[10000];
		bzero(&reqbuf, sizeof(reqbuf));

		if( (reqlen = read(cfd, reqbuf, sizeof(reqbuf))) < 0)
			err();

		// check if valid request
		if(strncmp(reqbuf, "GET", 3) != 0)
		{
			fprintf(stderr, "Not a GET request\n");
			continue;
		}

		// get file path/port and protocol/version
		char* s = strdup(reqbuf);
		char* scopy = s;
		char* firstLine = strsep(&s, "\r");
		char* get = strsep(&firstLine, " ");
		char* filepathport = strsep(&firstLine, " ");
		char* protVer = strsep(&firstLine, " ");

		// check for specified port number
		int port = 0;
		char* portstr = strdup(filepathport);
		char* portstrcopy = portstr;
		strsep(&portstr, ":");
		strsep(&portstr, ":");// attempt to jump to port number

		if(portstr == NULL) // implied port detected
		{
			port = 80;
			portstr = "80";
		}
		else // explicit port detected
		{
			portstr = strsep(&portstr, "/");
			port = atoi(portstr);
		}

		// get client IP address
		char* clientIP = inet_ntoa(client_addr.sin_addr);

		// get time information
		time_t t = time(NULL);
		struct tm* time;
		time = localtime(&t);
		char* timeStr = asctime(time);
		timeStr[strlen(timeStr)-1] = 0;

		// grab website name out of request
		char* temp = strdup(filepathport);
		strsep(&temp, "/"); // remove the "http://"
		strsep(&temp, "/");
		char* url = strsep(&temp, ":"); // remove the port info
		url = strsep(&url, "/");
		free(temp);

		// grab path out of request
		char* path = strdup(filepathport);
		strsep(&path, "/");
		strsep(&path, "/");
		strsep(&path, "/");
		path--;
		path[0] = '/';

		// construct get request
		char request[1000];
		bzero(&request, 1000);
		strcat(request, "GET ");
		strcat(request, path);
		strcat(request, " ");
		strcat(request, protVer);
		strcat(request, "\n");
		strcat(request, "Host: ");
		strcat(request, url);
		strcat(request, "\n");
		strcat(request, "Connection: close\n\n");

		// prep connection info
		struct addrinfo hints, *res;
		bzero(&hints, sizeof(hints));

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		getaddrinfo(url, portstr, &hints, &res);

		// make the socket
		int siteFD;
		if((siteFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
			err();

		// connect to website
		if(connect(siteFD, res->ai_addr, res->ai_addrlen) < 0)
			err();

		// send GET request
		int len = strlen(request);
		if(send(siteFD, request, len, 0) < 0)
			err();

		// receive server response
		char sitebuf[10000];
		bzero(&sitebuf, sizeof(sitebuf));
		int readVal, size = 0;

		// receive each chunk and immediately send it to client
		while( (readVal = Read(siteFD, sitebuf, sizeof(sitebuf))) != (size_t) NULL)
		{
			Write(cfd, sitebuf, readVal);
			size += readVal;
			bzero(&sitebuf, sizeof(sitebuf));
		}

		// write access to log file
		fprintf(log, "%s: %s %s %d\n", timeStr, clientIP, filepathport, size);
		fflush(log); // flush buffer to make sure written immediately

		// free memory
		free(scopy);
		free(portstrcopy);
		free(temp);

		// close sockets
		close(cfd);
		close(siteFD);
	}

	// close log file and socket and exit program
	if(fclose(log) == EOF) err();
	close(fd);
	return 0;
}
