/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  fifo_chat.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/02/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/02/23 13:55:26"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_FILE1		".fifo_chat1"
#define FIFO_FILE2		".fifo_chat2"

int	g_stop = 0;

void sig_pipe(int signum)
{
	if(SIGPIPE == signum)
	{
		printf("get pipe broken signal and let programe exit\n");
		g_stop = 1;
	}
}

int main(int argc,char **argv)
{
	int rv;
	int	fdr_fifo;
	int	fdw_fifo;
	int	mode = 0;
	char buf[1024];
	fd_set	rdset;

	if(argc != 2)	
	{
		printf("please select a fifo to chat(fifo_chat 0 or 1)\n");
		printf("This chat program need run twice,1st time run with [0] and 2nd run with [1]\n");
		return -1;
	} 
	mode = atoi(argv[1]);	

	if( access(FIFO_FILE1,F_OK) != 0)		
	{
		printf("%s not exist and create it now\n ",FIFO_FILE1);
		mkfifo(FIFO_FILE1,0666);		
	}
	if( access(FIFO_FILE2,F_OK) != 0)		
	{
		printf("%s not exist and create it now\n ",FIFO_FILE2);
		mkfifo(FIFO_FILE2,0666);		
	}
	signal(SIGPIPE,sig_pipe);

	if( 0 == mode)
	{
		printf("start open '%s' for read and it will blocked untill write endpoint opened...\n",FIFO_FILE1);
		if( (fdr_fifo=open(FIFO_FILE1, O_RDONLY)) < 0 )		
		{
			printf("Open fifo[%s] for chat read endpoint failure: %s\n", FIFO_FILE1, strerror(errno));
			return -1	;
		}
		printf("start open '%s' for write...\n", FIFO_FILE2);		
		if( (fdw_fifo=open(FIFO_FILE2, O_WRONLY)) < 0 )	
		{
			printf("Open fifo[%s] for chat write endpoint failure: %s\n", FIFO_FILE2, strerror(errno));
			return -1;
		}
	}
	else
	{
		printf("start open '%s' for write and it will blocked untill read endpoint opened...\n",FIFO_FILE1);
		if( (fdw_fifo=open(FIFO_FILE1, O_WRONLY)) < 0 )		
		{
			printf("Open fifo[%s] for chat write endpoint failure: %s\n", FIFO_FILE1, strerror(errno));
			return -1	;
		}
		printf("start open '%s' for read...\n", FIFO_FILE2);		
		if( (fdr_fifo=open(FIFO_FILE2, O_RDONLY)) < 0 )	
		{
			printf("Open fifo[%s] for chat read endpoint failure: %s\n", FIFO_FILE2, strerror(errno));
			return -1;
		}
	}

	printf("start  chating with another program now,please input message now:\n");
	while(!g_stop)
	{
		FD_ZERO(&rdset);		
		FD_SET(STDIN_FILENO,&rdset);	
		FD_SET(fdr_fifo,&rdset);		

		rv = select(fdr_fifo+1,&rdset,NULL,NULL,NULL);
		if( rv <= 0)
		{
			printf("select  get  timeout  or error:%s\n",strerror(errno));
			continue;
		}
		if(FD_ISSET(fdr_fifo,&rdset))		
		{
			memset(buf,0,sizeof(buf));
			rv = read(fdr_fifo,buf,sizeof(buf));
			if(rv < 0)
			{
				printf("read data from FIFO failure:%s\n",strerror(errno));
				break;
			}
			else if(0 == rv)
			{
				printf("another side of FIFO get closed and progarm will exit now\n");
				break;
			}
		
			printf("read %d word--%s",rv,buf);
		}
		if(FD_ISSET(STDIN_FILENO,&rdset))
		{
			memset(buf,0,sizeof(buf));
			fgets(buf,sizeof(buf),stdin);	
			write(fdw_fifo,buf,strlen(buf));
		}
	}
}

