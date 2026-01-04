#include <alloc.h>
#include <types/types.h>
#include <memory.h>

#define USED 1
#define FREE 0

typedef struct {
	size_t size;
	uint8_t status;
} heap_node;

heap_node *heap_start = (heap_node*)HEAP_ADDRESS;
heap_node *heap_end_pos = (heap_node*)HEAP_ADDRESS;

void *alloc(size_t size) {
	heap_node *cur_pos = heap_start;

	while (cur_pos < heap_end_pos) {
		if (cur_pos->size >= size && cur_pos->status == FREE) {
			cur_pos->status = USED;
			return (void*)(cur_pos + 1);
		}
		cur_pos = (heap_node*)((uint8_t*)(cur_pos + 1) + cur_pos->size);
	}
	heap_end_pos->size = size;
	heap_end_pos->status = USED;
	heap_node *addr_to_ret = heap_end_pos + 1;
	heap_end_pos = (heap_node*)((uint8_t*)(heap_end_pos + 1) + heap_end_pos->size);
	return (void*)addr_to_ret;
}

void free(void *ptr) {
	heap_node *node = (heap_node*)((uint8_t*)ptr - sizeof(heap_node));
	if (node->status == USED) {
		node->status = FREE;
	}
}