#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "page.h"
#include "file.h"
#include <algorithm>


/* Buffer structure */
typedef struct Buffer {
    page_t page;
    int64_t table_id;
    pagenum_t page_num;
    bool is_dirty;
    pthread_mutex_t page_latch;
    Buffer* next;
    Buffer* prev;
} Buffer;

typedef struct Buffer_List {
    Buffer* head;
    Buffer* tail;
} Buffer_List;

/* global variable buffer_queue */
extern Buffer *buffer_queue;
static int buffer_capacity;
static int buffer_size;


/* Buffer function */
void buffer_init(int num_buf);
void buffer_destroy();

/* 종속 */
Buffer* find_buffer_page(int64_t table_id, pagenum_t page_num);
/* 종속 */
void buffer_register(int64_t table_id, pagenum_t page_num);
/* 종속 */
void buffer_page_evict(Buffer* cur);
/* 종속 */
void move_buffer_page_position(Buffer* cur);

/* 단독 */
void buffer_read_page(int64_t table_id, pagenum_t page_num, page_t* dest);
/* 단독 */
void buffer_write_page(int64_t table_id, pagenum_t page_num, const struct page_t* src);
/* 몰?루 */
pagenum_t buffer_alloc_page(int64_t table_id);
/* 몰?루 */
void buffer_free_page(int64_t table_id, pagenum_t page_num);


/* 단독 */
void buffer_read_leaf_page(int64_t table_id, pagenum_t page_num, page_t* dest);
/* 단독 */
void buffer_write_leaf_page(int64_t table_id, pagenum_t page_num, const struct page_t* src);

#endif /* __BUFFER_H__*/