/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  semaphore.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/03/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/03/23 09:38:10"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define  FTOK_PATH	"/dev/zero"
#define  FTOK_PROJID	0x22

union semun
{
	int	val;
	struct semid_ds  *buf;
	unsigned  short  *arry;
};
union semun sem_union;

int semid;
int semaphore_init(void);		
int semaphore_p(int semid);			
int semaphore_v(int semid);			
void semaphore_term(int semid);

int main(int argc,char **argv)
{
	int i;
	pid_t pid;

	if((semid = semaphore_init()) < 0)
	{
		printf("semaphore initial failure:%s\n",strerror(errno));
		return -1;
	}

	if((pid = fork()) < 0)
	{
		printf("create child process failure:%s\n",strerror(errno));
		return -1;
	}
	else if(0 == pid)
	{
		printf("child process[%d] start running and do something now...\n",getpid());
		sleep(3);
		printf("child process do something over...\n");
		semaphore_v(semid);		
	
		sleep(1);

		printf("child process exit now\n");
		exit(0);
	}
	printf("parent process P operator wait child process over.\n");
	semaphore_p(semid);
	printf("parent process[%d] start running and do something now...\n",getppid());
	sleep(2);
	printf("parent process destroy semaphore and exit\n");

	semaphore_term(semid);
	return 0;
}

int semaphore_init(void)
{
	key_t   key;
	sem_union.val = 0;
	key = ftok(FTOK_PATH,FTOK_PROJID);		
	
	if(key < 0)
	{
		printf("ftok() get key failure:%s\n",strerror(errno));
		return -1;
	}
	printf("ftok() get key successfully!\n");

	semid = semget(key,1,IPC_CREAT | 0644);		
	if(semid < 0)
	{
		printf("semget() get semid  failure:%s\n",strerror(errno));
		return -1;
	}

	sem_union.val = 0;
	if(semctl(semid,0,SETVAL,sem_union) < 0)	
	{
		printf("semctl() set initial value failure: %s\n", strerror(errno));		
		return -1;
	}

	printf("semaphore get key_t[0x%x] and semid[%d]\n", key, semid);	
	return semid;	
}

void semaphore_term(int semid)
{
	if(semctl(semid,0,IPC_RMID,sem_union) < 0)
	{
		printf("semctl() delete semaphore ID failure:%s\n",strerror(errno));
	}
	return ;
}

int semaphore_p(int semid)
{
	struct sembuf  _sembuf;
	_sembuf.sem_num = 0;		
	_sembuf.sem_op  =  -1;		
	_sembuf.sem_flg  =  SEM_UNDO;	

	if(semop(semid,&_sembuf,1) < 0)
	{
		printf("semop  P  operator   failure:%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

int semaphore_v(int semid)
{
	struct sembuf  _sembuf;
	_sembuf.sem_num = 0;		
	_sembuf.sem_op  =  1;		
	_sembuf.sem_flg  =  SEM_UNDO;	
	if(semop(semid,&_sembuf,1) < 0)
	{
		printf("semop V  operator   failure:%s\n",strerror(errno));
		return -1;
	}
	return 0;
}


