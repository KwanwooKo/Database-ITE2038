#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "page.h"
#include "file.h"
#include <algorithm>
#include <pthread.h>


/* Buffer structure */
typedef struct Buffer {
    page_t page;
    int64_t table_id;
    pagenum_t page_num;
    bool is_dirty;
    bool is_pinned;
    pthread_mutex_t page_latch;
    Buffer* next;
    Buffer* prev;
} Buffer;

typedef struct Buffer_List {
    Buffer* head;
    Buffer* tail;
} Buffer_List;

/* global variable buffer_queue */
static Buffer *buffer_queue;
static Buffer *buffer;
extern int buffer_capacity;
extern int buffer_size;

/* Buffer function */
void buffer_init(int num_buf);
void buffer_destroy();
Buffer* find_buffer_page(int64_t table_id, pagenum_t page_num);
void buffer_register(int64_t table_id, pagenum_t page_num);
void buffer_read_page(int64_t table_id, pagenum_t page_num, page_t* dest);
void buffer_write_page(int64_t table_id, pagenum_t page_num, const struct page_t* src);
pagenum_t buffer_alloc_page(int64_t table_id);
void buffer_free_page(int64_t table_id, pagenum_t page_num);

void buffer_pin_off(int64_t table_id, pagenum_t pagenum);

void print_buffer();
#endif /* __BUFFER_H__*/


/*

1. 단독으로 쓰이는 함수
buffer_init
buffer_read_page
buffer_write_page
buffer_alloc_page
buffer_free_page
buffer_pin_off

2. 다른 함수에서 호출되는 함수
find_buffer_page
buffer_register

buffer_destroy


evict

*/