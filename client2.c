#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include"mymsq.h"

int main (void)
{
	int * array;
	int shmid, msqid;
	int new = 1;
	char pathname[] = "server.c";
	key_t key;
	MSG mybuf;
	size_t len = sizeof(mybuf.info);
	pid_t mypid = getpid();

	if ((key = ftok(pathname, 0)) < 0)
	{
		printf("Can't generate key for messages queue.\n");
		exit(-1);
	}

	if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)
	{
		printf("Can't get msqid");
		exit(-1);
	}

	if ((key = ftok(pathname, 1)) < 0)
	{
		printf("Can't generate key for shared memory.\n");
		exit(-1);
	}

	if ((shmid = shmget(key, 3*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL))
			< 0)
	{
		if (errno != EEXIST)
		{
			printf("Can't create shared memory for data.\n");
			exit(-1);
		}
		else
		{
			if ((shmid = shmget(key, 3*sizeof(int), 0)) < 0)
			{
				printf("Can't find shared memory.\n");
				exit(-1);
			}
			new = 0;
		}
	}

	if ((array = (int *)shmat(shmid, NULL, 0)) == (int *)(-1))
	{
		printf("Can't attach shared memory\n");
		exit(-1);
	}

	if (new)
	{
		array[0] = 0;
		array[1] = 1;
		array[2] = 1;
	}
	else
	{
		mybuf.mtype = CLIENT_TYPE;
		mybuf.info.c = 'P';
		mybuf.info.pid = mypid;
		if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
		{
			printf("Can't send message to queue.\n");
			exit(-1);
		}
		if (msgrcv(msqid, (void *) &mybuf, len, (long) mypid, 0) < 0)
		{
			printf("Can't receive message from server.\n");
			exit(-1);
		}
		array[1] += 1;
		for (long i = 0; i < 10000000000; i++);
		array[2] += 1;
		mybuf.mtype = CLIENT_TYPE;
		mybuf.info.c = 'V';
		mybuf.info.pid = mypid;
		if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
		{
			printf("Can't send message to queue.\n");
			exit(-1);
		}
		if (msgrcv(msqid, (void *) &mybuf, len, (long) mypid, 0) < 0)
		{
			printf("Can't receive message from srver.\n");
			exit(-1);
		}
	}

	printf("Program 1 was spawn %d times, " 
			"program 2 - %d times, total - %d times.\n", array[0], array[1], array[2]);
	if (shmdt(array) < 0)
	{
		printf("Can't detach shared memory.\n");
		exit(-1);
	}

	return 0;
}
