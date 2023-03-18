/*********************************************************************************
 *      Copyright:  (C) 2023 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  test.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(03/05/23)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "03/05/23 09:21:59"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#define  FTOK_PATH  "/dev/zero"
#define  FTOK_PROJID    0x22
#define  BACKLOG 13
union semun
{
	int val;
	struct semid_ds  *buf;
	unsigned  short  *arry;
};
union semun sem_union;

int semid;
int semaphore_init(void);		
int semaphore_p(int semid);			
int semaphore_v(int semid);			

void semaphore_term(int semid);
void child_process(int cli_fd);
void print_usage(char *progname);

int main (int argc, char **argv)
{
	int i;
	int ch;
	int semid;
	int cli_fd;
	int on = 1;
	int rv =  -1;
	int port = 0;
	int listen_fd = -1;
	char buf[1024];
	pid_t  pid;
	socklen_t len;
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;

	struct option opts[] = {
		{"port", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	while((ch=getopt_long(argc, argv, "p:h", opts, NULL)) != -1 )
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'h':
				print_usage(argv[0]);
				return 0;
		}
	}
	if(!port)
	{
		printf("please input the port!\n");
		print_usage(argv[0]);
		return 0;
	}

	listen_fd = socket(AF_INET,SOCK_STREAM,0);
	if(listen_fd < 0)
	{
		printf("create socket failure: %s\n",strerror(errno));
		return -1;
	}
	printf("create socket[%d] successfully!\n",listen_fd);
	
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	rv = bind(listen_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if(rv < 0)
	{
		printf("bind socket failure: %s\n",strerror(errno));
		return -1;
	}
	printf("socket[%d] bind on port[%d] successfully!\n",listen_fd,port);

	if(listen(listen_fd,100) < 0)
	{
		printf("listen failure:%s\n",strerror(errno));
		return -1;
	}
	listen(listen_fd,BACKLOG);

	while(1)
	{
		cli_fd = accept(listen_fd,(struct sockaddr *)&cliaddr,&len);
		if(cli_fd <0 )
		{
			printf("accept client failure:%s\n",strerror(errno));
			return -1;
		}
		memset(buf,0,sizeof(buf));

		if((semid = semaphore_init()) < 0)
		{
			printf("semaphore initial failure:%s\n",strerror(errno));
			return -1;
		}

		if((pid = fork()) < 0)
		{
			printf("create child process failure:%s\n",strerror(errno));
			close(cli_fd);
			return -1;
		}
		else if(pid >0 )
		{
			close(cli_fd);
			continue;
		}
		else if(0 == pid)	
		{
			printf("child process[%d] start running and do something now...\n",getpid());
			sleep(3);
			printf("child process done.\n");
			semaphore_v(semid);		
			sleep(1);
			printf("child process exit now\n");          
			child_process(cli_fd);
			close(listen_fd);
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
	close(listen_fd);
	return 0;
}

void child_process(int cli_fd)
{
	int rv = -1;
	char buf[1024];
	
	printf("child PID[%d] start to commuicate whit client...\n",getpid());
	while(1)
	{
		rv = read(cli_fd,buf,sizeof(buf));
		if(rv < 0 )
		{
			printf("read data from client failure:%s\n",strerror(errno));
			close(cli_fd);
			exit(0);
		}
		else if(rv == 0)
		{
			printf("socket[%d] get disconnect.\n",cli_fd);
			close(cli_fd);
			exit(0);
		}
		else if(rv > 0)
		{
			printf("read %d data from client:%s\n",strlen(buf),buf);
		}

		rv = write(cli_fd,buf,sizeof(buf));
		if(rv < 0 )
		{
			printf("write data  failure:%s\n",strerror(errno));
			close(cli_fd);
			exit(0);
		}
		close(cli_fd);
		exit(0);
	}
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

	semid = semget(key,1,IPC_CREAT|0644);		
	if(semid < 0)
	{
		printf("semget() get semid  failure:%s\n",strerror(errno));
		return -1;
	}

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

void print_usage(char *progname)
{
	  printf("%s usage: \n",progname);
	  printf("-p(--port):sepcify server port.\n");
	  printf("-h(--Help):print this help information.\n");
	  return ;
}

