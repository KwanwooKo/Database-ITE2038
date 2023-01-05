#ifndef __TRX_H__
#define __TRX_H__

#include <stdint.h>
#include <pthread.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <set>
#include <unordered_map>
#include "bpt.h"



typedef struct hash_table_entry hash_table_entry;
typedef struct lock_t lock_t;
typedef struct trx_t trx_t;
typedef struct log_t log_t;

// 여기서 hash function 참고
// https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
struct Hash {
    std::size_t operator()(const std::pair<int64_t, int64_t>& key) const {
        return (std::hash<int64_t>()(key.first)
                ^ (std::hash<int64_t>()(key.second) << 1));
    }
};

struct trx_hash {
    std::size_t operator()(const int key) const {
        return std::hash<int64_t>()(key);
    }
};

struct graph_hash {
    std::size_t operator()(const int key) const {
        return std::hash<int>()(key);
    }
};

/* hash table 원소 하나당 lock table 이 구성되어 있어 */
struct lock_t {
    lock_t* prev;
    lock_t* next;
    /* Sentinel pointer */      // hash table 을 가리키는 pointer
    hash_table_entry* sentinel;
    /* Conditional Variable */  // pthread_cond?
    pthread_cond_t cond;
    /* record_id */ 
    int64_t record_id;
    /* lock mode (shared == 0 or exclusive == 1) */
    int lock_mode;
    /* trx's next lock ptr */
    lock_t* trx_next_lock_ptr;
    /* Owner Transaction ID */
    int owner_trx_id;
};


struct hash_table_entry {
    int64_t table_id;
    pagenum_t page_id;
    lock_t* head;
    lock_t* tail;
};


struct log_t {
    char* original_value;
    uint16_t value_size;
    pagenum_t page_num;
    int64_t table_id;
    uint16_t offset;
};

struct trx_t {
    int trx_id;
    lock_t* tail;
    lock_t* next_lock;
    // trx_id 를 넣으면 true 인지 false 인지 나옴
    // std::unordered_map<int, bool> wait_for_graph;
    // std::map<int, int> wait_for_graph;
    std::set<int> wait_for_graph;
    // record_id 를 집어넣으면 log 출력 => log(value, size, table_id, offset)
    std::unordered_map<int64_t, log_t*, struct trx_hash> roll_back;
};



/* APIs for lock table */
int init_lock_table();
lock_t *lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t* lock_obj);

/* APIs for trx */
int trx_begin();
int trx_commit(int trx_id);

/* abort 도 만들어야돼 */
/* trx_table[trx_id] 가 null 이 아닌 애들 싹 다 roll back */
void trx_abort(int trx_id);
void store_log(int trx_id, int64_t key, int64_t table_id, Slot_page sp, Leaf_page lp, pagenum_t lp_pagenum);



/* lock_acquire 에서 사용하는 helper function */

bool find_not_equal_trx_id_and_equal_record_id(lock_t* lock, int trx_id, int64_t key);
bool find_exclusive_lock(lock_t* lock);
lock_t* allocate_new_lock(hash_table_entry* entry, int trx_id, int64_t key, int lock_mode);
bool deadlock_detection(int start, int here);
void wake_up_same_trx(lock_t* immediate_lock);
lock_t* find_same_lock(hash_table_entry* entry, int64_t key, int trx_id, int lock_mode);

/* lock_release 에서 사용하는 helper function */

void wake_up_shared_locks(lock_t* immediate_lock);
lock_t* find_immediate_lock(lock_t* lock_obj);
void cut_relation(hash_table_entry* entry, lock_t* existing_lock);
bool find_prev_lock(lock_t* immediate_lock);
void print_locks(hash_table_entry* entry);
void print_trx(trx_t* trx);

// void erase_edge(int trx_id);
#endif /* __TRX_H__ */


// here가 이미 visited됐는데 시작정점이라면 cycle이 존재하는 것이다
// 모든 정점에 대해서 확인을 해준다(visited는 매번 새롭게 초기화)
// bool findCycleAlgorithm(int start, int here) {
//   if (visitied[here]) {
//     if (here == start) {
//       return true;
//     }
//     return false;
//   }
   
//   visitied[here] = true;
//   for (int there : node[here]) {
//     if (findCycleAlgorithm(start, there)) {
//       return true;
//     }
//   }
   
//   return false;
// }