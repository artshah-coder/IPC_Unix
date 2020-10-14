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
	int * array;			// указатель для разделяемой памяти
	int shmid, msqid;		// переменные для хранения IPC-дескрипторов
					// очереди разделяемой памяти и очереди сообщений
	int new = 1;			// переменная-флаг, определяющая, необходимо
					// инициализировать shared memory или нет
	char pathname[] = "server.c";	// имя файла для генерации IPC-ключей
	key_t key;			// ключ - имя IPC объектов из пространства имен IPC
	MSG mybuf;			// структура данных для передаваемого сообщения
					// описана в заголовочном файле "mymsq.h"
	size_t len = sizeof(mybuf.info);// длина передаваемого сообщения
	pid_t mypid = getpid();		// PID текущего процесса

	if ((key = ftok(pathname, 0)) < 0)	// Пытаемся сгенерировать ключ
						// очереди сообщений
	{
		printf("Can't generate key for messages queue.\n");
		exit(-1);
	}

	if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)	// Пытаемся получить дескриптор
	       							// для очереди сообщений	
	{
		printf("Can't get msqid");
		exit(-1);
	}

	if ((key = ftok(pathname, 1)) < 0)	// Пробуем сгенерировать ключ
						// для разделяемой памяти
	{
		printf("Can't generate key for shared memory.\n");
		exit(-1);
	}

	// Пробуем получить IPC-дескриптор для разделяемой памяти
	if ((shmid = shmget(key, 3*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL))
			< 0)
	{
		// если ошибка, анализируем, может разделяемая память
		// с данным дескриптором уже есть
		if (errno != EEXIST)
		{
			/* Если разделяемой памяти нет, и создать ее не удалось, 
			 * покидаем программу */
			printf("Can't create shared memory for data.\n");
			exit(-1);
		}
		else
		{
			/* В противном случае, получаем снова ее дескриптор
			 * и сбрасываем флаг new в 0: память по ключу key
			 * уже создавалась ранее, инициализировать ее 
			 * не надо */
			if ((shmid = shmget(key, 3*sizeof(int), 0)) < 0)
			{
				printf("Can't find shared memory.\n");
				exit(-1);
			}
			new = 0;
		}
	}

	// Пробуем включить shared memory в адресное пространство
	// текущего процесса
	if ((array = (int *)shmat(shmid, NULL, 0)) == (int *)(-1))
	{
		printf("Can't attach shared memory\n");
		exit(-1);
	}

	if (new) // Если флаг new взведен, инициализируем
		 // shared memory начальными значениями
	{
		array[0] = 1;
		array[1] = 0;
		array[2] = 1;
	}
	else
	{
		/* В противном случае входим в критическую секцию.
		 * Будем менять содержимое разделяемой памяти.
		 * Но перед этим отправим запрос серверу на операцию 
		 * P над семафором mutex */
		/* Заполним структуру сообщения для сервера */
		mybuf.mtype = CLIENT_TYPE;	// Тип сообщения
		mybuf.info.c = 'P';		// Операция P
		mybuf.info.pid = mypid;		// PID процесса,
						// отправившего запрос
		/* Пытаемся отправить сообщение в очередь */
		if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
		{
			printf("Can't send message to queue.\n");
			exit(-1);
		}
		/* Дожидаемся ответа от сервера */
		if (msgrcv(msqid, (void *) &mybuf, len, (long) mypid, 0) < 0)
		{
			printf("Can't receive message from server.\n");
			exit(-1);
		}
		/* После получения подтверждения от сервера, входим
		 * в критическую секцию и модифицируем разделяемую
		 * память */
		array[0] += 1;
		for (long i = 0; i < 10000000000; i++);
		array[2] += 1;
		/* После прохода критической секции,
		 * засылаем в очередь сообщений запрос
		 * на операцию V над "семафором" mutex*/
		mybuf.mtype = CLIENT_TYPE;
		mybuf.info.c = 'V';
		mybuf.info.pid = mypid;
		if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
		{
			printf("Can't send message to queue.\n");
			exit(-1);
		}
		/* Ждем подтверждения от сервера */
		if (msgrcv(msqid, (void *) &mybuf, len, (long) mypid, 0) < 0)
		{
			printf("Can't receive message from srver.\n");
			exit(-1);
		}
	}

	printf("Program 1 was spawn %d times, " 
			"program 2 - %d times, total - %d times.\n", array[0], array[1], array[2]);
	// Пытаемся отсоединить разделяемую память из
	// адресного пространства текущего процесса
	if (shmdt(array) < 0)
	{
		printf("Can't detach shared memory.\n");
		exit(-1);
	}

	return 0;
}
