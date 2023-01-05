#ifndef __PAGE_H__
#define __PAGE_H__


#include <stdint.h>

typedef uint64_t pagenum_t;

// buffer maybe
typedef struct page_t {
    // in-memory page structure
    char reserved[4096];
} page_t;

typedef struct Header_page {
    pagenum_t magic_number = 2022;            // compare with the value which I expected
    pagenum_t free_page_number;               // the first free_page_number
    pagenum_t number_of_pages;                // the total number of page include header page
    pagenum_t root_page_number = 0;           // root page number
    char reserved[4096 - 32];
} Header_page;

typedef struct Free_page {
    // next_free_page_number = 0 => last free page
    pagenum_t next_free_page_number;
    char reserved[4088];
} Free_page;

typedef struct Page_header {
    // parent_page_number == 0 => root page
    pagenum_t parent_page_number;
    int is_leaf;
    uint32_t number_of_keys;
    char reserved[128 - 16];
} Page_header;

typedef struct Slot_page {
    int64_t key;
    uint16_t size; // unsigned short 랑 uint16_t 중 뭐가 맞냐
    uint16_t offset;
    int trx_id;
} Slot_page;

typedef struct Leaf_page {
    pagenum_t parent_page_number;
    int is_leaf = 1;
    uint32_t number_of_keys = 0;
    char reserved[112 - 16];
    pagenum_t amount_of_free_space = 4096 - 128;    // 3968
    pagenum_t right_sibling_page_number;
    char page_body[4096 - 128];
} Leaf_page;

typedef struct Entry {
    int64_t key;
    pagenum_t page_number;
} Entry;

typedef struct Internal_page {
    pagenum_t parent_page_number;
    int is_leaf;
    uint32_t number_of_keys;
    char reserved[120 - 16];
    pagenum_t left_most_page_number;
    Entry entry[248];   // 128 ~ 4096
} Internal_page;

#endif  /* __PAGE_H__*/