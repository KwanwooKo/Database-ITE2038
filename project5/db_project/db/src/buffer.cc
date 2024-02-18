#include "../include/bpt.h"
#include "../include/buffer.h"
#include <pthread.h>
pthread_mutex_t buffer_manager_latch = PTHREAD_MUTEX_INITIALIZER;
Buffer *buffer_queue;
/* buffer_manager_latch lock XXXX */
void buffer_init(int num_buf) {
    buffer_queue = (Buffer *) malloc(sizeof(Buffer));
    buffer_queue->table_id = -1;
    buffer_queue->page_num = -1;
    buffer_queue->page_latch = PTHREAD_MUTEX_INITIALIZER;
    buffer_queue->is_dirty = false;
    buffer_queue->next = buffer_queue;
    buffer_queue->prev = buffer_queue;
    buffer_capacity = num_buf;
    buffer_size = 0;
}
/* buffer_manager_latch lock XXXX */
void buffer_destroy() {
    printf("buffer destroy\n");
    Buffer *cur = buffer_queue->next;
    if (cur == buffer_queue) {
        free(buffer_queue);
        return;
    }
    while (1) {
        if (cur == buffer_queue) {
            break;
        }
        if (cur->is_dirty) {
            file_write_page(cur->table_id, cur->page_num, &cur->page);
        }
        Buffer *prev = cur;
        cur = cur->next;
        free(prev);
        prev = NULL;
    }
    free(buffer_queue);
}
/**
 * Case 1. existing page
 *      replace the page
 *      return buffer
 * Case 2. non-existing page
 * return NULL
 */
/* buffer_manager_latch lock XXXX */
Buffer* find_buffer_page(int64_t table_id, pagenum_t page_num) {
    Buffer* cur = buffer_queue->next;
    while (cur != buffer_queue) {
        if (cur->table_id == table_id && cur->page_num == page_num) {
            break;
        }
        cur = cur->next;
    }
    return cur;
}
/* buffer_manager_latch lock XXXX */
void buffer_register(int64_t table_id, pagenum_t page_num) {
    Buffer *cur = (Buffer *) malloc(sizeof(Buffer));
    file_read_page(table_id, page_num, &cur->page);
    cur->table_id = table_id;
    cur->page_num = page_num;
    cur->is_dirty = false;
    cur->page_latch = PTHREAD_MUTEX_INITIALIZER;
    
    /* 제일 처음으로 register 되는 경우 */
    if (buffer_queue->next == buffer_queue) {
        buffer_queue->next = cur;
        buffer_queue->prev = cur;
        cur->prev = buffer_queue;
        cur->next = buffer_queue;
    }
    /* 이미 존재하는 list 에 register 되는 경우 */
    else {
        cur->next = buffer_queue->next;
        cur->prev = buffer_queue;
        buffer_queue->next->prev = cur;
        buffer_queue->next = cur;
    }
    buffer_size++;
}
/* dirty 이면 기록, 그리고 제일 앞으로 까지만 보냄 */
void buffer_page_evict(Buffer* cur) {
    /* 수정된 버퍼 라면 디스크에 기록 */
    if (cur->is_dirty) {
        file_write_page(cur->table_id, cur->page_num, &cur->page);
        cur->is_dirty = false;
    }
    move_buffer_page_position(cur);
}
/* buffer_queue->prev 를 맨 앞으로 보냄 */
void move_buffer_page_position(Buffer* cur) {
    Buffer *prev = cur->prev;
    Buffer *next = cur->next;
    prev->next = next;
    next->prev = prev;
    cur->prev = buffer_queue;
    cur->next = buffer_queue->next;
    buffer_queue->next->prev = cur;
    buffer_queue->next = cur;   
}
/* 여기서는 page_latch 안거나?? */
/* buffer_manager_latch lock OOOO */
void buffer_read_page(int64_t table_id, pagenum_t page_num, page_t* dest) {
    pthread_mutex_lock(&buffer_manager_latch);
    Buffer *cur = find_buffer_page(table_id, page_num);
    /**
     * buffer 에 해당 페이지가 존재하지 않는 경우
     */
    if (cur == buffer_queue) {
        /**
         * buffer pool 에 공간이 남은 경우
         */
        if (buffer_size < buffer_capacity) {
            /* buffer size 는 buffer_register 에서 증가시켜줌 */
            buffer_register(table_id, page_num);
            cur = find_buffer_page(table_id, page_num);
            memcpy(dest, &cur->page, PAGE_SIZE);
        }
        /**
         * buffer pool 에 공간이 남지 않은 경우
         */
        else {
            cur = buffer_queue->prev;
            buffer_page_evict(cur);
            file_read_page(table_id, page_num, &cur->page);
            memcpy(dest, &cur->page, PAGE_SIZE);
            cur->table_id = table_id;
            cur->page_num = page_num;
        }
    }
    /* buffer list 에 페이지가 존재하는 경우 */
    else {
        /* buffer_queue 바로 다음에 존재하는 경우 */
        if (cur == buffer_queue->next) {
            memcpy(dest, &cur->page, PAGE_SIZE);
        }
        else {
            move_buffer_page_position(cur);
            memcpy(dest, &cur->page, PAGE_SIZE);
        }
    }
    pthread_mutex_unlock(&buffer_manager_latch);
}
/**
 * 무조건 존재 안 할수도 있다고 치자
 */
/* buffer_manager_latch lock OOOO */
void buffer_write_page(int64_t table_id, pagenum_t page_num, const struct page_t* src) {
    pthread_mutex_lock(&buffer_manager_latch);
    Buffer *cur = find_buffer_page(table_id, page_num);
    if (cur == buffer_queue) {
        cur = buffer_queue->prev;
        buffer_page_evict(cur);
        memcpy(&cur->page, src, PAGE_SIZE);
        cur->table_id = table_id;
        cur->page_num = page_num;
        cur->is_dirty = true;
    }
    else {
        move_buffer_page_position(cur);
        memcpy(&cur->page, src, PAGE_SIZE);
        cur->is_dirty = true;
    }
    pthread_mutex_unlock(&buffer_manager_latch);
}
/* 얘는 확정이라 치면 다른걸 찾아야돼 */
pagenum_t buffer_alloc_page(int64_t table_id) {
    Buffer *header_buf = find_buffer_page(table_id, 0);
    pagenum_t page_num;
    /* header_buf 를 찾고 그 header_buf 가 값이 변경돼있는 경우 */
    if (header_buf != buffer_queue && header_buf->is_dirty) {
        file_write_page(table_id, 0, &header_buf->page);
        header_buf->is_dirty = false;
        page_num = file_alloc_page(table_id);
        file_read_page(table_id, 0, &header_buf->page);
    }
    /* header_buf 를 못 찾은 경우 */
    else if (header_buf == buffer_queue) {
        page_num = file_alloc_page(table_id);
    }
    /* header_buf 를 찾고 header_buf 가 값이 변경돼있지 않은 경우 */
    else {
        page_num = file_alloc_page(table_id);
        file_read_page(table_id, 0, &header_buf->page);
    }
    Buffer *cur = buffer_queue;
    /* 버퍼에 공간이 남아있는 경우 */
    if (buffer_size < buffer_capacity) {
        buffer_register(table_id, page_num);
        cur = find_buffer_page(table_id, page_num);
    }
    /* 버퍼에 공간이 남아있지 않은 경우 */
    else {
        cur = buffer_queue->prev;
        if (cur->is_dirty) {
            file_write_page(cur->table_id, cur->page_num, &cur->page);
            cur->is_dirty = false;
        }
        Buffer *prev = cur->prev;
        Buffer *next = cur->next;
        prev->next = next;
        next->prev = prev;
        cur->prev = NULL;
        cur->next = NULL;
        buffer_register(table_id, page_num);
        buffer_size--;
        cur = find_buffer_page(table_id, page_num);
        // cur->is_pinned = false;
    }
    return page_num;
}
void buffer_free_page(int64_t table_id, pagenum_t page_num) {
    Buffer *cur = find_buffer_page(table_id, page_num);
    Buffer *header_buf = find_buffer_page(table_id, 0);
    if (header_buf != buffer_queue && header_buf->is_dirty) {
        file_write_page(table_id, 0, &header_buf->page);
        header_buf->is_dirty = false;
        file_free_page(table_id, page_num);
        file_read_page(table_id, 0, &header_buf->page);
    }
    else if (header_buf == buffer_queue) {
        file_free_page(table_id, page_num);
    }
    else {
        file_free_page(table_id, page_num);
        file_read_page(table_id, 0, &header_buf->page);
    }
    if (cur == buffer_queue) {
        return;
    }
    Buffer *prev = cur->prev;
    Buffer *next = cur->next;
    prev->next = next;
    next->prev = prev;
    cur->next = NULL;
    cur->prev = NULL;
    buffer_size--;
    free(cur);
}
/* leaf page 를 읽는 경우 page_latch 를 걸어야 함 */
void buffer_read_leaf_page(int64_t table_id, pagenum_t page_num, page_t* dest) {
    /* is_leaf 인 경우 page_latch 에 lock 걸어야 함 */
    pthread_mutex_lock(&buffer_manager_latch);
    Buffer *cur = find_buffer_page(table_id, page_num);
    /**
     * buffer 에 해당 페이지가 존재하지 않는 경우
     */
    if (cur == buffer_queue) {
        /**
         * buffer pool 에 공간이 남은 경우
         */
        if (buffer_size < buffer_capacity) {
            /* buffer size 는 buffer_register 에서 증가시켜줌 */
            buffer_register(table_id, page_num);
            cur = find_buffer_page(table_id, page_num);
            pthread_mutex_lock(&cur->page_latch);
            pthread_mutex_unlock(&buffer_manager_latch);
            memcpy(dest, &cur->page, PAGE_SIZE);
            pthread_mutex_unlock(&cur->page_latch);
        }
        /**
         * buffer pool 에 공간이 남지 않은 경우
         */
        else {
            /* 끝으로 보내고 해당 page 가 mutex_unlock 되는 순간 접근 가능 */
            cur = buffer_queue->prev;
            buffer_page_evict(cur);
            pthread_mutex_lock(&cur->page_latch);
            pthread_mutex_unlock(&buffer_manager_latch);
            file_read_page(table_id, page_num, &cur->page);
            memcpy(dest, &cur->page, PAGE_SIZE);
            cur->table_id = table_id;
            cur->page_num = page_num;
            pthread_mutex_unlock(&cur->page_latch);
        }
    }
    /* buffer list 에 페이지가 존재하는 경우 */
    else {
        /* buffer_queue 바로 다음에 존재하는 경우 */
        if (cur == buffer_queue->next) {
            pthread_mutex_lock(&cur->page_latch);
            pthread_mutex_unlock(&buffer_manager_latch);
            memcpy(dest, &cur->page, PAGE_SIZE);
            pthread_mutex_unlock(&cur->page_latch);
        }
        else {
            move_buffer_page_position(cur);
            
            pthread_mutex_lock(&cur->page_latch);
            pthread_mutex_unlock(&buffer_manager_latch);
            memcpy(dest, &cur->page, PAGE_SIZE);
            pthread_mutex_unlock(&cur->page_latch);
        }
    }
}
void buffer_write_leaf_page(int64_t table_id, pagenum_t page_num, const struct page_t* src) {
    pthread_mutex_lock(&buffer_manager_latch);
    Buffer *cur = find_buffer_page(table_id, page_num);
    if (cur == buffer_queue) {
        cur = buffer_queue->prev;
        
        // evict 할 때만 추가 시켜 봤어
        buffer_page_evict(cur);
        pthread_mutex_lock(&cur->page_latch);
        pthread_mutex_unlock(&buffer_manager_latch);
        memcpy(&cur->page, src, PAGE_SIZE);
        cur->table_id = table_id;
        cur->page_num = page_num;
        cur->is_dirty = true;
        pthread_mutex_unlock(&cur->page_latch);
    }
    else {
        move_buffer_page_position(cur);
        pthread_mutex_lock(&cur->page_latch);
        pthread_mutex_unlock(&buffer_manager_latch);
        memcpy(&cur->page, src, PAGE_SIZE);
        cur->is_dirty = true;
        pthread_mutex_unlock(&cur->page_latch);
    }
}


