/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  named_socket_client.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/03/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/03/23 08:04:53"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SOCKET_PATH	"/tmp/socket.domain"

void print_usage(char *progname);
int get_temp(float *temp);

int main(int argc, char **argv)
{
	int rv = -1;
	int conn_fd =-1;
	float temp = 0;
	char  str[1024];
	char  buf[1024];
	char  *serverip = 0;
	struct  sockaddr_un serv_addr;

	while(1)
	{
		if((conn_fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
		{
			printf("create  socket failure:%s\n",strerror(errno));
			return  -1;
		}
		printf("create  socket[%d]  successfully\n",conn_fd);
		memset(&serv_addr,0,sizeof(serv_addr));
		serv_addr.sun_family=AF_UNIX;
		strncpy(serv_addr.sun_path,SOCKET_PATH,sizeof(serv_addr.sun_path)-1);

		if((connect(conn_fd,(struct sockaddr *)&serv_addr,  sizeof(serv_addr)))<0)
		{
			printf("connect to unix domain socket server on \"%s\" failure:%s\n",SOCKET_PATH,strerror(errno));
			return  -1;
		}
		printf("connect to unix domain socket server on \"%s\" successfully!\n",SOCKET_PATH);
		
		rv= get_temp(&temp);
		if( rv  < 0)
		{
			printf("get temperature failure:%s\n",strerror(errno));
			return -1;
		}
		sprintf(buf,"%f\n",temp);

		rv = write(conn_fd,buf,sizeof(buf));
		if(rv<0 )
		{
			printf("write failure:%s\n",strerror(errno));
			goto cleanup;
		}
		memset(buf,0,sizeof(buf));
		rv= read(conn_fd,buf,sizeof(buf));
		if(rv<0)
		{
			printf("read failure:%s\n",strerror(errno));
			goto cleanup;
		}
		printf("read data from unix socket server!\nthe temperature('c) is:%s\n", buf);
		sleep(10);
	}
cleanup:
	close(conn_fd);
	return 0;
}

void print_usage(char *progname)
{
	printf("%s usage: \n",progname);
	printf("--h(--hostname): sepcify server hostname \n");
	printf("-p(--port): sepcify server port \n");
	printf("-H(--Help): print this help information \n");
}

int get_temp(float *temp)
{
	int     rv = 0;
	int     fd = -1;
	int     found = 0;
	DIR     *dirp = 0;
	char    *ptr = 0;
	char    chip[20];
	char    buf[1024];
	char    ds_path[50];
	char    *w1_path = "/sys/bus/w1/devices";
	struct  dirent  *direntp;

	if((dirp = opendir(w1_path)) == NULL )
	{
		printf("opendir error: %s\n",strerror(errno));
		return -1;
	}

	while((direntp = readdir(dirp)) != NULL)
	{
		if(strstr(direntp->d_name,"28-"))
		{
			strcpy(chip,direntp->d_name);
			found = 1;
			break;
		}
	}
	closedir(dirp);
	if(!found)
	{
		printf("can not find ds18b20 in %s\n",w1_path);
		return -1;
	}

	snprintf(ds_path,sizeof(ds_path),"%s/%s/w1_slave",w1_path,chip);
	if((fd = open(ds_path,O_RDONLY)) < 0 )
	{
		printf("open %s error : %s\n",ds_path,strerror(errno));
		return -1;
	}
	if(read(fd,buf,sizeof(buf)) < 0)
	{
		printf("read %s error:%s\n",w1_path,strerror(errno));
		rv = -1;
		goto cleanup;
	}

	ptr = strstr(buf,"t=");
	if(!ptr)
	{
		printf("error:can not get temperature\n");
		rv = -1;
		goto cleanup;
	}
	ptr += 2;
	*temp = atof(ptr)/1000;
	snprintf(buf,sizeof(buf),"%f",*temp);

cleanup:
	close(fd);
	return rv;
}
