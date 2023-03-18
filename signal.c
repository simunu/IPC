/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  signal.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(02/27/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "02/27/23 12:52:08"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

int child_stop = 0;
int parent_run = 0;

void sig_child(int signum)
{
	if(SIGUSR1 == signum)
	{
		child_stop = 1;
	}
}

void sig_parent(int signum)
{
	if(SIGUSR2 == signum)
	{
		parent_run = 1;
	}
}

int main (int argc,char **argv)
{
	int wstatus;
	pid_t pid;
	signal(SIGUSR1,sig_child);
	signal(SIGUSR2,sig_parent);

	if((pid = fork()) < 0)
	{
		printf("create child process failure:%s\n",strerror(errno));
		return -1;
	}
	else if(pid == 0)
	{
		printf("child process start running.\n");
		kill(getppid(),SIGUSR2);

		while(!child_stop)
		{
			sleep(1);
		}
		printf("child process receive signal from parent and exit now.\n");
		return 0;
	}
	printf("parent hangs up untill receive signal from child!\n");
	while(!parent_run)
	{
		sleep(1);
	}
	printf("parent start running now and send child a signal to exit.\n");
	kill(pid,SIGUSR1);

	wait(&wstatus);
	printf("parent wait child process die and exit now.\n");
	return 0;
}
