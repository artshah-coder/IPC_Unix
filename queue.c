#include"queue.h"
#include<stdio.h>
#include<stdlib.h>

void InitializeQueue (Queue * pq)
{
	pq->head = pq->tail = NULL;
	pq->count = 0;
}

bool QueueIsEmpty (const Queue * pq)
{
	if (pq->count)
		return false;
	else
		return true;
}

bool EnQueue (pid_t pid, Queue * pq)
{
	Node * new = (Node *) malloc(sizeof(Node));

	if (NULL == new)
	{
		printf("Can't allocate memory.\n");
		return false;
	}

	if (QueueIsEmpty(pq))
	{
		new->payload = pid;
		new->next = NULL;
		new->prev = NULL;
		pq->head = new;
		pq->tail = new;
		pq->count++;
		return true;
	}
	else
	{
		new->payload = pid;
		new->next = NULL;
		pq->tail->next = new;
		new->prev = pq->tail;
		pq->tail = new;
		pq->count++;
		return true;
	}
}

bool DeQueue (pid_t * pid, Queue * pq)
{
	if (QueueIsEmpty(pq))
		return false;
	else if (pq->count == 1)
	{
		*pid = pq->head->payload;
		Node * tmp = pq->head;
		pq->head = pq->tail = NULL;
		free(tmp);
		pq->count--;
		return true;
	}
	else
	{
		*pid = pq->head->payload;
		Node * tmp = pq->head;
		pq->head = pq->head->next;
		pq->head->prev = NULL;
		free(tmp);
		pq->count--;
		return true;
	}
}

void EmptyQueue (Queue * pq)
{
	pid_t tmp;
	while(DeQueue(&tmp, pq));
}
