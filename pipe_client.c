/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  pipe_client.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/10/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/10/23 09:10:19"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <getopt.h>
#include <netdb.h>

#define MSG_STR   "Get the temperature successfully!"

void printf_usage(char *progname);
int get_temp(float *temp);

int main(int argc, char **argv)
{
	int     T = 0;
	int     ch= 0 ;
	int     conn_fd =-1;
	int     rv = -1;
	int     port = 0;
	int     pipe_fd[2];
	int     wstatus; 
	char   buf[1024];
	char   str[1024];
	char   ipstr[1024];
	char   *hostname = 0;
	char   *serverip = 0;
	char   **hostip; 
	float   temp = 0;
	pid_t  pid;
	struct hostent  *servhost;
	struct  sockaddr_in serv_addr;

	struct option  opts[] = {
		{"hostname",required_argument,NULL,'h'},
		{"port",required_argument,NULL,'p'},
		{"Help",no_argument,NULL,'H'},
		{NULL,0,NULL,0}
	};

	while((ch = getopt_long(argc,argv,"h:p:H",opts,NULL)) != -1)
	{
		switch(ch)
		{
			case'h':
				hostname = optarg;
				break;
			case'p':
				port=atoi(optarg);
				break;
			case'H':
				printf_usage(argv[0]);
				return 0;
		}
	}

	if((inet_aton(hostname,&serv_addr.sin_addr)) == 0)
	{
		if((servhost = gethostbyname(hostname)) == NULL)
		{
			printf("Get hostname error: %s\n", strerror(errno));
			return -1;
		}
		switch(servhost -> h_addrtype)
		{
			case AF_INET6:
			case AF_INET:
				hostip = servhost -> h_addr_list;
				for (; *hostip != NULL; hostip++)
					printf("IP: %s\n",inet_ntop(servhost ->h_addrtype, servhost -> h_addr,ipstr,sizeof(ipstr)));
				serverip = ipstr;
				break;
			default:
				printf("error address!\n");
				break;
		}
	}
	else
	{
		serverip = hostname;
	}
	if(pipe(pipe_fd)<0)
	{
		printf("create pipe failure\n");
		return -1;
	}
	printf("create pipe successfully!\n");


	while(1)
	{
		if((conn_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
		{
			printf("create  socket failure:%s\n",strerror(errno));
			return  -1;
		}
		printf("create  socket[%d]  successfully\n",conn_fd);
		memset(&serv_addr,0,sizeof(serv_addr));
		serv_addr.sin_family=AF_INET;
		serv_addr.sin_port=htons(port);
		inet_aton(serverip,&serv_addr.sin_addr);
		if((connect(conn_fd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)))<0)
		{
			printf("connect to server[%s:%d] failure:%s\n",serverip,port,   strerror(errno));
			return  -2;
		}
		printf("connect to  server[%s:%d]  successfully!\n",serverip,port);
		pid = fork();
		if(pid < 0)
		{
			printf("create child process failure\n");
			return -1;
		}
		else if(pid==0)
		{
			close(pipe_fd[1]);
			memset(buf,0,sizeof(buf));
			rv = read (pipe_fd[0],buf,sizeof(buf));
			if(rv< 0)
			{
				printf("child process read data from pipe failure\n");
				return -1;
			}
			printf("child process read %d bytes from pipe:%s\n",rv,buf);
			return 0;
		}
		else if(pid >0)
		{
			sleep(1);
			close(pipe_fd[0]);
			memset(buf,0,sizeof(buf));
			rv = get_temp(&temp);
			sprintf(buf,"%f\n",temp);
			if(write(pipe_fd[1],buf,sizeof(buf)) < 0)
			{
				printf("parent process write data to pipe failure\n");
				return   -1;
			}
		}
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
		printf("the  temperature('c) is:%s\n", buf);
		sleep(10);
	}

	printf("parent process start waiting child process exit...\n");
	wait(&wstatus);
	return 0;
cleanup:
	close(conn_fd);
	return 0;
}
void printf_usage(char *progname)
{
	printf("%s usage: \n",progname);
	printf("--h(--hostname): sepcify server hostname \n");
	printf("-p(--port): sepcify server port \n");
	printf("-H(--Help): print this help information \n");
}

int get_temp(float *temp)
{
	char    *w1_path = "/sys/bus/w1/devices";
	char    ds_path[50];
	char    chip[20];
	char    buf[1024];
	DIR     *dirp = 0;
	struct  dirent  *direntp;
	int     ds18b20_fd = -1; 
	char    *ptr = 0;
	int     found = 0;
	int     ds18b20_rv = 0;
	float   t = 0;

	if((dirp = opendir(w1_path)) == NULL )
	{
		printf("opendir error: %s\n",strerror(errno));
		return -2;
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
		return -3;
	}

	snprintf(ds_path,sizeof(ds_path),"%s/%s/w1_slave",w1_path,chip);
	if((ds18b20_fd = open(ds_path,O_RDONLY)) < 0 )
	{
		printf("open %s error : %s\n",ds_path,strerror(errno));
		return -4;
	}
	if(read(ds18b20_fd,buf,sizeof(buf)) < 0)
	{
		printf("read %s error:%s\n",ds_path,strerror(errno));
		ds18b20_rv = -5;
		goto cleanup;
	}

	ptr = strstr(buf,"t=");
	if(!ptr)
	{
		printf("error:can not get temperature\n");
		ds18b20_rv = -7;
		goto cleanup;
	}
	ptr += 2;
	*temp = atof(ptr)/1000;
	snprintf(buf,sizeof(buf),"%f",*temp);

cleanup:
	close(ds18b20_fd);
	return ds18b20_rv;
}
