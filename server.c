#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include"queue.h"	// Заголовочный файл, хранящий типы данных очереди и
			// прототипы функций для работы с очередями
#include"mymsq.h"	// Файл с типом данных для сообщений
			// и константой CLIENT_TYPE

int main (void)
{
	unsigned int mutex = 1;	// Семафор, инициализированный начальным значением 1
	Queue wait;		// Очередь из PID'ов заблокированных процессов
	InitializeQueue(&wait);
	
	int msqid;		// IPC-дескриптор очереди сообщений
	key_t key;		// Ключ для генерации имени в пространстве IPC имен
				// для очереди сообщений
	MSG mybuf;		// структура сообщения
	char pathname[] = "server.c";
	size_t len = sizeof(mybuf.info);

	if ((key = ftok(pathname, 0)) < 0) // пытаемся сгенерировать IPC-ключ для
					   // очереди сообщений по имени данного
					   // файла и числа 0 для текущей очереди
					   // сообщений
	{
		printf("Can't generate key for messages queue.\n");
		exit(-1);
	}

	if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0) // Пытаемся получить
							 // IPC-дескриптор очереди
							 // сообщений
	{
		printf("Can't get mesqid.\n");
		exit(-1);
	}

	/* В бесконечном цикле ожидаем сообщений от взаимодействующих процессов */
	while (1)
	{
		// пытаемся получить сообщение от клиента
		if (msgrcv(msqid, (void *) &mybuf, len, CLIENT_TYPE, 0) < 0)
		{
			printf("Can't recieve message from queue.\n");
			exit(-1);
		}

		// Если клиент прислал запрос на операцию V над семафором
		if (mybuf.info.c == 'V')
		{
			/* Отправляем приславшему запрос клиенту сообщение
			 * о том, что он может продолжить свою работу.
			 * Сообщение имеет тип PID приславшего запрос клиента
			 * и нулевую длину */
			mybuf.mtype = (long) mybuf.info.pid;
			if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
			{
				printf("Can't send message to queue.\n");
				EmptyQueue(&wait);
				exit(-1);
			}

			if (!QueueIsEmpty(&wait)) // Проверяем, не пуста ли очередь ожидающих процессов
			{
				/* Если очередь не пуста, выбираем из нее PID процесса в порядке FIFO
				 * и отправляем сообщение ожидающему процессу с выбранным PID
				 * в очередь сообщений */
				pid_t res;
				if (!DeQueue(&res, &wait))
				{
					printf("Undefinded error while deleting element from queue.\n");
					EmptyQueue(&wait);
					exit(-1);
				}
				mybuf.mtype = (long) res;
				if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
				{
					printf("Can't send message to queue.\n");
					EmptyQueue(&wait);
					exit(-1);
				}
			}
			else
				/* В случае пустой очереди просто увеличиваем семафор на 1 */
				mutex++;
		}
		else if (mybuf.info.c == 'P') // Если клиент прислал запрос на операцию P
		{
			if (mutex == 0)	/* Если семафор равен 0, процесс блокируется,
					   добавляется в очередь ожидания
					   обратное сообщение не посылается */
			{
				if (!EnQueue(mybuf.info.pid, &wait))
				{
					printf("Undefinded error while adding elemrnt to queue.\n");
					EmptyQueue(&wait);
					exit(-1);
				}
			}
			else
			{
				/* В противном случае значение семафра уменьшается на 1
				 * и отправившему запрос клиенту отсылается пустое сообщение
				 * с типом, равным значению PID клиента, сигнализирующее
				 * о возможности продолжения работы клиентом */
				mutex--;
				mybuf.mtype = (long) mybuf.info.pid;
				if (msgsnd(msqid, (void *) &mybuf, len, 0) < 0)
				{
					printf("Can't send message to queue.\n");
					EmptyQueue(&wait);
					exit(-1);
				}
			}
		}
	}

	return 0;
}
