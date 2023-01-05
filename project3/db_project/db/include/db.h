#ifndef __DB_H__
#define __DB_H__
#include "file.h"
#include "page.h"
#include "bpt.h"
#include "buffer.h"
#include <cstdint>
#include <vector>
#include <cstring>
#include <queue>
// Open an existing database file or create one if not exist.
int64_t open_table(const char* pathname);
// Insert a record to the given table.
int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size);
int db_insert_into_leaf_page(int64_t table_id, pagenum_t leaf_page_num, int64_t key, const char* value, uint16_t val_size);
int db_insert_into_leaf_page_after_splitting(int64_t table_id, pagenum_t root_page_num, pagenum_t leaf_page_num, int64_t key, const char* value, uint16_t val_size);
int db_insert_into_parent_page(int64_t table_id, pagenum_t left_page_num, pagenum_t right_page_num, int64_t key);
int db_insert_into_internal_page(int64_t table_id, pagenum_t parent_page_num, int left_index, int64_t key, pagenum_t right_page_num);
int db_insert_into_internal_page_after_splitting(int64_t table_id, pagenum_t parent_page_num, int left_index, int64_t key, pagenum_t right_page_num);

// Find a record with the matching key from the given table.
int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size);

// Delete a record with the matching key from the given table.
int db_delete(int64_t table_id, int64_t key);
pagenum_t db_delete_entry(int64_t table_id, pagenum_t page_num, int64_t key);
pagenum_t db_delete_parent_entry(int64_t table_id, pagenum_t page_num, int k_prime_index);
int remove_entry_from_leaf_page(int64_t table_id, pagenum_t leaf_page_num, int64_t key);
pagenum_t db_adjust_root(int64_t table_id);
int get_neighbor_index(int64_t table_id, pagenum_t page_num);
pagenum_t merge_leaf_pages(int64_t table_id, pagenum_t leaf_page_num, pagenum_t neighbor_page_num, int neighbor_index, int64_t k_prime);
pagenum_t redistribute_leaf_pages(int64_t table_id, pagenum_t leaf_page_num, pagenum_t neighbor_page_num, int neighbor_index, int k_prime_index, int64_t k_prime);
pagenum_t merge_internal_pages(int64_t table_id, pagenum_t page_num, pagenum_t neighbor_page_num, int neighbor_index, int64_t k_prime);
pagenum_t redistribute_internal_pages(int64_t table_id, pagenum_t page_num, pagenum_t neighbor_page_num, int neighbor_index, int k_prime_index, int64_t k_prime);

// Find records with a key between the range:  begin_key ≤ key ≤ end_key
int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes);

// Initialize the database system.
int init_db(int num_buf);
// Shutdown the database system.
int shutdown_db();
// find leaf_page offset + function overloading
pagenum_t find_leaf_page(int64_t table_id, int64_t key);
pagenum_t find_leaf_page(int64_t table_id);
// make leaf page
pagenum_t make_leaf_page(int64_t table_id);
// make internal page
pagenum_t make_internal_page(int64_t table_id);
int get_left_index(int64_t table_id, pagenum_t parent_page_num, pagenum_t left_page_num);
// record all root_page offset
static pagenum_t *root_pagenum_list;
static int maximum_fdlists_size;
#endif /* __DB_H__ */