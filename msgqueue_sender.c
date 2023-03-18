/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  msgqueue_sender.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/05/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/05/23 12:24:38"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define FTOK_PATH	"/dev/zero"
#define FTOK_PROJID		0x22

typedef struct msgbuf
{
	long mtype;
	char mtext[512];
}msg;

int get_temp(float *temp);
int main(int argc,char **argv)
{
	int i;
	int rv;
	int	msgid;
	int	msgtype;
	float temp;
	msg	msgbuf;
	key_t key;

	if((key =ftok(FTOK_PATH,FTOK_PROJID)) < 0)
	{
		printf("ftok() get key failure:%s\n",strerror(errno));
		return -1;
	}
	msgid = msgget(key,IPC_CREAT | 0666);
	if(msgid < 0)
	{
		printf("shmget() create shared memroy failure:%S\n",strerror(errno));
		return -1;
	}
	msgtype =(int)key;//why?
	printf("key_t[%d]  msgid[%d]  msgtype[%d]\n",(int)key,msgid,msgtype);

	rv= get_temp(&temp);
	if(rv <0)
	{
		printf("get temperature failure:%s\n",strerror(errno));
		return -1;
	}
	while(1)
	{
		msgbuf.mtype = msgtype;
		sprintf(msgbuf.mtext,"%f\n",temp);
		if(msgsnd(msgid,&msgbuf,sizeof(msgbuf.mtext),IPC_NOWAIT) < 0)
		{
			printf("msgsnd() send message failure:%s\n",strerror(errno));
			break;
		}
		printf("send message:get the temperature is %s\n",msgbuf.mtext);
		sleep(1);
	}
	msgctl(msgid,IPC_RMID,NULL);
	return 0;
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
		printf("read %s error:%s\n",ds_path,strerror(errno));
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
