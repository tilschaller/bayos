#include <alloc.h>
#include <types/types.h>
#include <memory.h>
#include <lock.h>

#define USED 1
#define FREE 0

typedef struct heap_node {
	size_t size;
	uint8_t status;
	struct heap_node *prev;
	struct heap_node *next;
} heap_node;

heap_node *heap_start = (heap_node*)HEAP_ADDRESS;

spinlock_t heap_lock = {0};

void *alloc(size_t size) {
	acquire(&heap_lock);

	if (size < 0x20) {
		size = 0x20;
	}

	heap_node *cur_node = heap_start;

	while (cur_node != NULL) {
		if (cur_node->size == size && cur_node->status == FREE) {
			cur_node->status = USED;
			release(&heap_lock);
			return (void*)(cur_node + 1);
		}
		if (cur_node->size >= (size + sizeof(heap_node) + 0x20) && cur_node->status == FREE) {
			heap_node *new_node = (heap_node*)((uintptr_t)cur_node + size + sizeof(heap_node));
			new_node->size = cur_node->size - size - sizeof(heap_node);
			new_node->status = FREE;
			new_node->prev = cur_node;
			new_node->next = cur_node->next;
			if (cur_node->next != NULL) {
				cur_node->next->prev = new_node;
			}
			cur_node->next = new_node;
			cur_node->status = USED;
			cur_node->size = size;
			release(&heap_lock);
			return (void*)(cur_node + 1);
		}
		cur_node = cur_node->next;
	}

	release(&heap_lock);

	return 0;
}

void free(void *ptr) {
	if (ptr == NULL) return;
	if ((uintptr_t)ptr < HEAP_ADDRESS || (uintptr_t)ptr >= HEAP_ADDRESS + HEAP_LENGTH) return;
	heap_node *node = (heap_node*)((uint8_t*)ptr - sizeof(heap_node));
	if (node->status == FREE) return;
	acquire(&heap_lock);
	node->status = FREE;
	heap_node *prev = node->prev;
	heap_node *next = node->next;
	if (prev != NULL && prev->status == FREE) {
		prev->size = prev->size + node->size + sizeof(heap_node);
		prev->next = next;
		if (next != NULL) {
			next->prev = prev;
		}
		node = prev;
	}
	if (next != NULL && next->status == FREE) {
		node->size = next->size + node->size + sizeof(heap_node);
		node->next = next->next;
		if (node->next != NULL) {
			node->next->prev = node;
		}
	}

	release(&heap_lock);
}

void heap_init() {
	heap_node *node = heap_start;
	node->prev = NULL;
	node->next = NULL;
	node->status = FREE;
	node->size = HEAP_LENGTH - sizeof(heap_node);
}
