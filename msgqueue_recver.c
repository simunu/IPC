/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  msgqueue_recver.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/05/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/05/23 13:25:19"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define FTOK_PATH	"/dev/zero"
#define FTOK_PROJID		0x22

typedef struct  msgbuf
{
	long mtype;
	char  mtext[512];
}msg;

int main(int argc,char  **argv)
{
	int i;
	int msgid;
	int msgtype;
	key_t	key;
	msg	msgbuf;

	if((key = ftok(FTOK_PATH,FTOK_PROJID)) < 0)
	{
		printf("ftok() get key failure:%S\n",strerror(errno));
		return  -1;
	}
	
	msgid = msgget(key,IPC_CREAT | 0666);
	if(msgid < 0)
	{
		printf("msgget() creat shared memroy failure:%s\n",strerror(errno));
		return -1;
	}
	msgtype = (int)key;
	printf("key_t[%d]	msgid[%d]	msgtype[%d]\n",(int)key,msgid,msgtype);

	while(1)
	{
		memset(&msgbuf,0,sizeof(msgbuf));
		if(msgrcv(msgid,&msgbuf,sizeof(msgbuf.mtext),msgtype,IPC_NOWAIT) < 0)
		{
			printf("msgrsnd() receive message failure:%s\n",strerror(errno));
			break;
		}
		printf("receive message:the temperature is  %s\n",msgbuf.mtext);
		sleep(1);
	}
	msgctl(msgid,IPC_RMID,NULL);
	return 0;
}

