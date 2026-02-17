#include <queue.h>
#include <alloc.h>
#include <types/types.h>
#include <string.h>

queue_t *queue_create(int n, int size) {
	queue_t *q = alloc(sizeof(queue_t));
	if (!q) return NULL;

	q->elems = alloc(n * size);
	if (!q->elems) {
		free(q);
		return NULL;
	}

	q->front = -1;
	q->rear = 0;
	q->n = n;
	q->size = size;

	return q;
}

int queue_is_empty(queue_t *q) {
	return (q->front == q->rear - 1);
}
int queue_is_full(queue_t *q) {
	return (q->rear == q->n);
}

int enqueue(queue_t *q, void *value) {
	if (queue_is_full(q)) return -1;

	memcpy(q->elems + q->size * q->rear, value, q->size);
	q->rear++;

	return 0;
}

int dequeue(queue_t *q) {
	if (queue_is_empty(q)) return -1;
	q->front++;

	return 0;
}

void *queue_peek(queue_t *q) {
	if (queue_is_empty(q)) return NULL;

	return q->elems + q->size * (q->front + 1);
}

void queue_delete(queue_t *q) {
	free(q->elems);
	free(q);
}

