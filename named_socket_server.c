/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  named_socket_server.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/03/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/03/23 07:30:48"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SOCKET_PATH	 "/tmp/socket.domain"
#define BACKLOG       13

void print_usage(char *progname);

int main(int argc,char **argv)
{
	int rv = -1;
	int listen_fd = -1;
	int client_fd = -1;
	char buf[1024];
	struct sockaddr_un serv_addr;
	struct sockaddr_un cli_addr;
	socklen_t cli_addr_len = sizeof(struct sockaddr);

	if((listen_fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0 )
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("socket create fd[%d]\n",listen_fd);

	if(!access(SOCKET_PATH,F_OK))
	{
		unlink(SOCKET_PATH);
	}

	memset(&serv_addr,0,sizeof(serv_addr));
	memset(&cli_addr,0,sizeof(cli_addr));
	serv_addr.sun_family = AF_UNIX;
	strncpy(serv_addr.sun_path,SOCKET_PATH,sizeof(serv_addr.sun_path)-1);

	if((bind(listen_fd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0 )
	{
		printf("bind socket failure:%s\n",strerror(errno));
		unlink(SOCKET_PATH);
		return -1;
	}
	printf("socket[%d] bind on path \"%s\"  ok\n",listen_fd, SOCKET_PATH);

	listen(listen_fd,BACKLOG);

	while(1)
	{
		printf("start waiting and accept new client[%d] connect...\n",listen_fd);
		client_fd = accept(listen_fd,(struct sockaddr *)&cli_addr,&cli_addr_len);
		if(client_fd < 0 )
		{
			printf("accept new socket failure:%s\n",strerror(errno));
			return -1;
		}

		memset(buf,0,sizeof(buf));
		if((rv = read(client_fd,buf,sizeof(buf))) < 0)
		{
			printf("read data from client socket[%d] failure:%s\n", client_fd,strerror(errno));
			close(client_fd);
			continue;
		}
		else if( 0 == rv )
		{
			printf("client socket[%d] disconnect\n",client_fd);
			close(client_fd);
			continue;
		}
		printf("read %d bytes data from client[%d]\nThe temperature('C) is :%s\n",rv,client_fd,buf);

		if((write(client_fd,buf,strlen(buf))) <0 )
		{
			printf("write %d bytes data back to client[%d] failure: %s\n",  rv,client_fd,strerror(errno));
			close(client_fd);
		}
		sleep(1);
		close(client_fd);
	}
	close(listen_fd);
	return 0 ;
}

void print_usage(char *progname)
{
	printf("%s usage: \n",progname);
	printf("-p(--port): sepcify server port \n");
	printf("-h(--help): printf this help information \n");
}

