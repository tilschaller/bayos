#ifndef _QUEUE_H
#define _QUEUE_H

typedef struct {
	void *elems;
	int rear;
	int front;
	int n;
	int size;
} queue_t;

queue_t *queue_create(int n, int size);
void queue_delete(queue_t *q);
int queue_is_empty(queue_t *q);
int queue_is_full(queue_t *q);
int enqueue(queue_t *q, void *value);
int dequeue(queue_t *q);
void *queue_peek(queue_t *q);

#endif // _QUEUE_H
