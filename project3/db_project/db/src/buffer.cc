#include "db.h"
#include "buffer.h"

int buffer_capacity;
int buffer_size;
void buffer_init(int num_buf) {
    buffer_queue = (Buffer *) malloc(sizeof(Buffer));
    buffer_queue->table_id = -1;
    buffer_queue->page_num = -1;
    buffer_queue->is_pinned = true;
    buffer_queue->is_dirty = false;
    buffer_queue->next = buffer_queue;
    buffer_queue->prev = buffer_queue;
    buffer_capacity = num_buf;
    buffer_size = 0;
}

void buffer_destroy() {
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


void buffer_register(int64_t table_id, pagenum_t page_num) {
    Buffer *cur = (Buffer *) malloc(sizeof(Buffer));
    file_read_page(table_id, page_num, &cur->page);
    cur->table_id = table_id;
    cur->page_num = page_num;
    cur->is_dirty = false;
    cur->is_pinned = false;
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


void buffer_read_page(int64_t table_id, pagenum_t page_num, page_t* dest) {
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
            cur->is_pinned = true;
            memcpy(dest, &cur->page, PAGE_SIZE);
        }

        /**
         * buffer pool 에 공간이 남지 않은 경우
         */
        else {
            /* 일단 끝으로 보내 */
            cur = buffer_queue->prev;
            /* 뒤에서 부터 차례대로 넘어와서 pin 이 안박힌 버퍼를 찾아 */
            while (cur != buffer_queue) {
                if (!cur->is_pinned) break;
                cur = cur->prev;
            }

            /* 수정된 버퍼 라면 디스크에 기록 */
            if (cur->is_dirty) {
                file_write_page(cur->table_id, cur->page_num, &cur->page);
                cur->is_dirty = false;
            }

            Buffer *prev = cur->prev;
            Buffer *next = cur->next;
            prev->next = next;
            next->prev = prev;
            cur->prev = buffer_queue;
            cur->next = buffer_queue->next;
            buffer_queue->next->prev = cur;
            buffer_queue->next = cur;

            file_read_page(table_id, page_num, &cur->page);
            memcpy(dest, &cur->page, PAGE_SIZE);
            cur->table_id = table_id;
            cur->page_num = page_num;
            cur->is_pinned = true;
        }
    }
    /* buffer list 에 페이지가 존재하는 경우 */
    else {
        /* buffer_queue 바로 다음에 존재하는 경우 */
        if (cur == buffer_queue->next) {
            cur->is_pinned = true;
            memcpy(dest, &cur->page, PAGE_SIZE);
        }
        else {
            Buffer *prev = cur->prev;
            Buffer *next = cur->next;
            prev->next = next;
            next->prev = prev;
            cur->prev = buffer_queue;
            cur->next = buffer_queue->next;
            buffer_queue->next->prev = cur;
            buffer_queue->next = cur;

            memcpy(dest, &cur->page, PAGE_SIZE);
        }
    }

}


/**
 * 무조건 존재 안 할수도 있다고 치자
 */
void buffer_write_page(int64_t table_id, pagenum_t page_num, const struct page_t* src) {
    Buffer *cur = find_buffer_page(table_id, page_num);


    if (cur == buffer_queue) {
        printf("이런 케이스가 많나?\n");
        printf("page_num: %ld\n", page_num);
        file_write_page(table_id, page_num, src);
    }
    else {
        memcpy(&cur->page, src, PAGE_SIZE);
        cur->is_pinned = false;
        cur->is_dirty = true;
    }

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
        cur = buffer_queue->next;
        // cur = find_buffer_page(table_id, page_num);
        cur->is_pinned = false;
    }
    /* 버퍼에 공간이 남아있지 않은 경우 */
    else {
        cur = buffer_queue->prev;
        while (cur != buffer_queue) {
            if (!cur->is_pinned) break;
            cur = cur->prev;
        }

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
        cur->is_pinned = false;
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
        // buffer_write_page(table_id, 0, &header_buf->page);
    }
    else if (header_buf == buffer_queue) {
        file_free_page(table_id, page_num);
    }
    else {
        file_free_page(table_id, page_num);
        file_read_page(table_id, 0, &header_buf->page);
        // buffer_write_page(table_id, 0, &header_buf->page);
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

void buffer_pin_off(int64_t table_id, pagenum_t page_num) {
    Buffer *cur = find_buffer_page(table_id, page_num);
    cur->is_pinned = false;
}



void print_buffer() {
    Buffer *cur = buffer_queue->next;
    while (cur != buffer_queue) {
        if (cur->is_pinned) {
            printf("pinned  ");
        }
        else {
            printf("not pinned  ");
        }
        printf("offset: %ld\n", cur->page_num);
        cur = cur->next;
    }
}