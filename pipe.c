/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  pipe.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/02/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/02/23 08:11:09"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define	MSG_STR	    "This message is from parent:Hello,child process!"

int main(int argc,char  **argv)
{
	int	rv;
	int wstatus;
	int pipe_fd[2];
	char buf[1024];
	pid_t pid;

	if(pipe(pipe_fd) < 0)		
	{
		printf("create pipe failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create pipe successsfully!\n");

	if((pid = fork()) < 0)
	{
		printf("create child process failure:%s\n",strerror(errno));
		return -1;
	}
	else if (pid == 0)
	{
		close(pipe_fd[1]);		
		memset(buf,0,sizeof(buf));
		rv = read(pipe_fd[0],buf,sizeof(buf));		
		if( rv < 0 )
		{
			printf("child process read data from pipe failure:%s\n",strerror(errno));
			return -1;
		}
		printf("child process read %d bytes from pipe:%s\n",rv,buf);
		return 0;
	}
	close(pipe_fd[0]);		
	if(write(pipe_fd[1],MSG_STR,sizeof(MSG_STR)) < 0)	
	{
		printf("parent process write data to pipe failure:%s\n",strerror(errno));
		return -1;
	}

	printf("parent process start waiting child process exit...\n");		
	wait(&wstatus);
	return 0;
}


