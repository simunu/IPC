/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  shared_read_server.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/05/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/05/23 02:38:03"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>

#define LEN   1024
#define FTOK_PATH   "/dev/zero"
#define FTOK_PROJID     0x22

typedef struct shared_use_st
{
	int  written;
	char T[LEN];
}shared_use;

int main(int argc,char **argv)
{
	int shmid;
	void *shm = NULL;
	key_t key;
	shared_use *shared;

	if((key = ftok(FTOK_PATH,FTOK_PROJID)) < 0)
	{
		printf("ftok() get key failure:%s\n",strerror(errno));
		return -1;
	}
	shmid = shmget(key,sizeof(shared_use),IPC_CREAT | 0666);
	if(shmid < 0)
	{
		printf("shmget() creat shared memroy failure:%s\n",strerror(errno));
		return -1;
	}
	printf("get key_t[0x%x] and shmid[%d]\n",key,shmid);

	shared = shmat(shmid,NULL,0);
	if((void *) -1 == shared)
	{
		printf("shmat() alloc shared memroy failure:%s\n",strerror(errno));
		return -1;
	}
	
	shared-> written =0;
	while(1)
	{
		if(shared-> written == 1)
		{
			printf("the data from shared memroy :%s\n",shared->T);
			sleep(1);
			shared->written =0;
		}
		else
		{
			sleep(1);
		}
	}
  	if(shmdt(shared)  == -1)
	{
		printf("shmdt() failure:%s",strerror(errno));
		return -1;
	}
	shmctl(shmid,IPC_RMID,NULL);
	return 0;
}
