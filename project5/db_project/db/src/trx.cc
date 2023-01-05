#include "trx.h"
#define DEBUG 0
#define SHARED 0
#define EXCLUSIVE 1
/* hash table */
std::unordered_map<std::pair<int64_t, int64_t>, hash_table_entry*, struct Hash> lock_table;
std::map<int, bool> visited;
std::unordered_map<int, trx_t*, struct trx_hash> trx_table;
pthread_mutex_t trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;
int current_trx_id = 1;
/* Initialize any data structures required for implementing lock table, such as hash table, lock table latch, etc. */
/* If success, return 0. Otherwise, return non-zero value */
int init_lock_table() {
    /* initialize mutex by using pthread_mutex_init function */
    /* 이거 일어나면 안되긴 해 */
    // if (pthread_mutex_init(&trx_manager_latch, NULL) != 0) {
    //     printf("mutex initialization failed\n");
    //     return -1;
    // }
    return 0;
}
/* Allocate and append a new lock object to the lock list of the record having the key */
/* If there is a predecessor's lock object in the lock list, sleep until the predecessor to release its lock */
/* If there is no predecessor's lock object, return the address of the new lock object */ 
/* If an error occurs, return NULL */
/* 여기다가 dead_lock detection 추가해줘야해 */
/* mutex 걸어 */
lock_t *lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode) {
    /* hash 된 table 이 shared resources 니까 이거 접근 못하게 막아놓고 처리 해야돼 */
    pthread_mutex_lock(&trx_manager_latch);
    // std::cout << "lock_acquire" << std::endl;
    /* table_id, page_id 로 key 찾음 */
    std::pair<int64_t, int64_t> hashing_key = {table_id, page_id};
    hash_table_entry* entry = lock_table[hashing_key];
    /* entry 가 존재하지 않는 경우 */
    /* entry 추가 + entry 설정 */
    if (entry == NULL) {
        entry = (hash_table_entry *) malloc(sizeof(hash_table_entry));
        entry->table_id = table_id;
        entry->page_id = page_id;
        entry->head = NULL;
        entry->tail = NULL;
        lock_table[hashing_key] = entry;
    }
    /* 중복체크 */
    lock_t* existing_lock = find_same_lock(entry, key, trx_id, lock_mode);
    if (existing_lock) {
        pthread_mutex_unlock(&trx_manager_latch);
        return existing_lock;
    }

    lock_t* lock = allocate_new_lock(entry, trx_id, key, lock_mode);
    bool is_exists = false;
    if (lock->lock_mode == EXCLUSIVE) {
        /* 앞에 같은 record_id 다른 trx_id 의 lock 이 있는지 확인 */
        is_exists = find_not_equal_trx_id_and_equal_record_id(lock, trx_id, key);
    }   
    else if (lock->lock_mode == SHARED) {
        /* 앞에 같은 record_id, 다른 trx_id, EXCLUSIVE lock 이 있는지 확인 */
        is_exists = find_exclusive_lock(lock);
    }

    /* 여기서 deadlock detection 하고  detect 되면 return NULL */
    visited.clear();
    if (deadlock_detection(trx_id, trx_id)) {
        trx_abort(trx_id);
        pthread_mutex_unlock(&trx_manager_latch);
        return NULL;
    }

    /* 그 lock 이 있으면 sleep */
    if (is_exists) {
        pthread_cond_wait(&lock->cond, &trx_manager_latch);
    }
    pthread_mutex_unlock(&trx_manager_latch);
    return lock;
}
/* Remove the lock_obj from the lock list */
/* If there is a successor's lock waiting for the thread releasing the lock, wake up the successor. */
/* If success, return 0. Otherwise, return non-zero value */
/* 절대 mutex 안 걸어 */
/* 이미 깨어있는 애들한텐 관심이 없어, 그래서 trx_id 가 다른 애들만 봐야돼 */
int lock_release(lock_t* lock_obj) {
    // std::cout << "lock_release" << std::endl;
    
    hash_table_entry* entry = lock_obj->sentinel;

    /* 1. lock_obj 부터 가장 가까운 lock 찾아 (같은 record 만) */
    lock_t* immediate_lock = find_immediate_lock(lock_obj);
    /* 2. lock_obj 를 리스트에서 제거 */
    cut_relation(entry, lock_obj);
    /* immediate_lock 이 일단 존재 */
    if (immediate_lock != NULL && immediate_lock->owner_trx_id != lock_obj->owner_trx_id) {
        /* lock_obj 가 X 인 경우 */
        if (lock_obj->lock_mode == EXCLUSIVE) {
            lock_t* next_to_immediate_lock = find_immediate_lock(immediate_lock);
            /* immediate_lock 이 S 인 경우 */
            /* S 이면 X 나오기 전까지 다 꺠워 */
            /* X S */
            if (immediate_lock->lock_mode == SHARED) {
                /* trx_id 가 같고, next_to_immediate_lock 이 X lock 이면 */
                if (next_to_immediate_lock != NULL && next_to_immediate_lock->owner_trx_id == immediate_lock->owner_trx_id && next_to_immediate_lock->lock_mode == EXCLUSIVE) {
                    wake_up_same_trx(immediate_lock);
                }
                else {
                    wake_up_shared_locks(immediate_lock);
                }
            }
            /* immediate_lock 이 X 인 경우 */
            /* X 이면 얘만 꺠워 */
            /* 얘만 깨우는게 아니라 같은 trx SHARED 도 다 깨워야하는거 아닌가 */
            /* X X */
            else if (immediate_lock->lock_mode == EXCLUSIVE) {
                if (next_to_immediate_lock != NULL && next_to_immediate_lock->owner_trx_id == immediate_lock->owner_trx_id &&
                        immediate_lock->lock_mode == EXCLUSIVE) {
                    wake_up_same_trx(immediate_lock);
                }
                else {
                    pthread_cond_signal(&immediate_lock->cond);
                }
            }
        }
        /* lock_obj 가 S 인 경우 */
        else {
            lock_t* next_to_immediate_lock = find_immediate_lock(immediate_lock);
            /* S S(lock_obj) X */
            if (immediate_lock->lock_mode == EXCLUSIVE) {
                if (find_prev_lock(immediate_lock)) {
                    #if DEBUG
                    std::cout << "there is more shared locks left, so do not wake up immediate lock" << std::endl;
                    #endif
                    delete lock_obj;
                    lock_obj = NULL;
                    return 0;
                }
                #if DEBUG
                std::cout << "this releasing lock is the last lock of the trx" << std::endl;
                #endif
                pthread_cond_signal(&immediate_lock->cond);
            }
            /* S(lock_obj) S X */
            else if (next_to_immediate_lock != NULL && immediate_lock->owner_trx_id == next_to_immediate_lock->owner_trx_id && immediate_lock->lock_mode == SHARED && next_to_immediate_lock->lock_mode == EXCLUSIVE) {
                wake_up_same_trx(next_to_immediate_lock);
            }
        }
    }
    delete lock_obj;
    lock_obj = NULL;
    return 0;
}
/* 새로운 lock entry->tail에 할당 */
lock_t* allocate_new_lock(hash_table_entry* entry, int trx_id, int64_t key, int lock_mode) {

    lock_t* cur = entry->head;
    lock_t* lock = new lock_t();
    lock->prev = entry->tail;
    lock->next = NULL;
    lock->sentinel = entry;
    lock->cond = PTHREAD_COND_INITIALIZER;
    lock->record_id = key;
    lock->lock_mode = lock_mode;
    lock->trx_next_lock_ptr = NULL;
    /* trx next lock 연결 */
    // pthread_mutex_lock(&trx_manager_latch);
    /* 무조건 존재해야해, 이미 trx_begin 에서 생성하고 왔어 */
    trx_t* trx = trx_table[trx_id];
    /* trx_table 에 하나도 없는 경우 */
    if (trx->tail == NULL) {
        trx->next_lock = lock;
        trx->tail = lock;
    }
    else {
        trx->tail->trx_next_lock_ptr = lock;
        trx->tail = lock;
    }
    lock->owner_trx_id = trx_id;
    
    
    if (entry->head == NULL && entry->tail == NULL) {
        entry->head = lock;
        entry->tail = lock;
    }
    else if (entry->head == entry->tail) {
        entry->tail->next = lock;
        entry->tail = lock;
    }
    else {
        entry->tail->next = lock;
        entry->tail = lock;
    }
    return lock;
}
/* helper function => 싹 다 mutex 안 걸어 */

/* 같은 record_id, 다른 trx_id */
/* 추가되는 lock 이 X lock 인 경우 */
bool find_not_equal_trx_id_and_equal_record_id(lock_t* lock, int trx_id, int64_t key) {
    lock_t* cur = lock->sentinel->head;
    trx_t* lock_trx = trx_table[lock->owner_trx_id];
    bool is_exists = false;
    while (1) {
        if (cur == lock) {
            break;
        }
        /* key 가 같은 애들 + trx_id 가 다른 애들끼리 */
        /* 다른 trx_id 가 존재하는 경우 간선연결을 해야하니까 */
        if (cur->record_id == key && cur->owner_trx_id != trx_id) {
            lock_trx->wait_for_graph.insert(cur->owner_trx_id);
            is_exists = true;
        }
        cur = cur->next;
    }
    return is_exists;
}

/* entry->head 를 기준으로 뒤에 lock 과 record_id 가 같고 trx_id 가 다른 exclusive lock 찾기 */
/* 추가되는 lock 이 SHARED 인 경우 */
bool find_exclusive_lock(lock_t* lock) {
    lock_t* cur = lock->sentinel->head;
    trx_t* lock_trx = trx_table[lock->owner_trx_id];
    bool is_exists = false;
    while (1) {
        /* exclusive lock 없음 */
        if (cur == lock) {
            break;
        }
        /* record_id 가 같고 trx_id 다르고 lock_mode == EXCLUSIVE */
        else if (cur->record_id == lock->record_id && cur->lock_mode == EXCLUSIVE) {
            if (cur->owner_trx_id != lock->owner_trx_id) {
                lock_trx->wait_for_graph.insert(cur->owner_trx_id);
                is_exists = true;
            }
        }
        cur = cur->next;
    }
    return is_exists;
}

/* 이거 진짜 모르겠다;;;; */
/* immediate_lock 부터 X 나오기 전까지 shared lock 을 다 꺠워 */
/* 이거 문제가 결국에는 exclusive 도 깨워야 될 수도 있어서 그런거 */
/* 시작할 때 무조건 trx_list.size() = 1 은 보장이 돼 */
/* T2(X) T4(S) T4(X) T3(S) 이 경우에 어떻게 할거야? */
void wake_up_shared_locks(lock_t* immediate_lock) {
    lock_t* cur = immediate_lock;

    while (1) {
        if (cur == NULL) {
            break;
        }
        /* 처음으로 만나는 exclusive lock 에서 break (trx 신경 쓰지 않고) */
        if (cur->record_id == immediate_lock->record_id && cur->lock_mode == EXCLUSIVE) {
            break;
        }
        if (cur->record_id == immediate_lock->record_id && cur->lock_mode == SHARED) {
            pthread_cond_signal(&cur->cond);
        }
        cur = cur->next;
    }
}
/* lock_obj 다음부터 같은 record_id 만 */
lock_t* find_immediate_lock(lock_t* lock_obj) {
    lock_t* cur = lock_obj->next;
    while (1) {
        if (cur == NULL) {
            break;
        }
        if (cur->record_id == lock_obj->record_id) {
            break;
        }
        cur = cur->next;
    }
    return cur;
}

void cut_relation(hash_table_entry* entry, lock_t* existing_lock) {
    /* prev, next 둘 다 NULL */
    if (!existing_lock->prev && !existing_lock->next) {
        entry->head = NULL;
        entry->tail = NULL;
    }
    /* prev 는 NULL, next 는 존재, 얘가 head 라는 소린데? */
    else if (!existing_lock->prev && existing_lock->next) {
        entry->head = existing_lock->next;
        existing_lock->next->prev = NULL;
    }
    /* prev 는 존재, next 는 NULL  얘가 tail 이라는 소린데? */
    else if (existing_lock->prev && !existing_lock->next) {
        entry->tail = existing_lock->prev;
        existing_lock->prev->next = NULL;
    }
    /* prev, next 둘 다 존재 일반적인 케이스 */
    else if (existing_lock->prev && existing_lock->next) {
        existing_lock->next->prev = existing_lock->prev;
        existing_lock->prev->next = existing_lock->next;
    }
    existing_lock->next = NULL;
    existing_lock->prev = NULL;
}
void print_locks(hash_table_entry* entry) {
    std::cout << std::endl;
    printf("print_locks\n");
    lock_t* cur = entry->head;
    while (1) {
        if (cur == NULL) break;
        std::cout << "cur->record_id: " << cur->record_id << std::endl;
        cur = cur->next;
    }
}
int trx_begin() {
    init_lock_table();
    pthread_mutex_lock(&trx_manager_latch);
    int ret_trx_id = current_trx_id;
    current_trx_id++;
    trx_t* trx = new trx_t();
    trx->trx_id = ret_trx_id;
    trx->next_lock = NULL;
    trx->tail = NULL;
    trx_table[ret_trx_id] = trx;
    trx->wait_for_graph.clear();
    trx->roll_back.clear();
    pthread_mutex_unlock(&trx_manager_latch);
    return ret_trx_id;
}
int trx_commit(int trx_id) {
    pthread_mutex_lock(&trx_manager_latch);
    trx_t* trx = trx_table[trx_id];
    
    if (trx == NULL) {
        pthread_mutex_unlock(&trx_manager_latch);
        return 0;
    }
    /* 아니 for each 문을 도는데 왜 NULL 일 수가 있냐 */
    int count = 0;
    #if DEBUG
    for (auto iter = trx_table.begin(); iter != trx_table.end(); iter++) {
        count++;
        trx_t* tmp_trx = iter->second;
        if (iter->second == NULL) {
            continue;
        }
        lock_t* cur = tmp_trx->next_lock;
        std::cout << "trx_id: " << iter->first << std::endl;
        std::cout << "key: ";
        while (1) {
            if (cur == NULL) {
                break;
            } 
            std::cout << cur->record_id << ' ';
            cur = cur->trx_next_lock_ptr;
        }
        std::cout << std::endl << std::endl;
    }
    std::cout << "trx_table.size: " << trx_table.size() << std::endl;
    std::cout << "count: " << count << std::endl;
    #endif



    lock_t* cur = trx->next_lock;
    while (1) {
        if ( cur == NULL ) {
            break;
        }
        lock_t* tmp = cur->trx_next_lock_ptr;
        lock_release(cur);
        cur = tmp;
    }
    trx_table.erase(trx_id);
    delete trx;
    pthread_mutex_unlock(&trx_manager_latch);
    return trx_id;
}
// 일단 lock releasing 만 해보자
void trx_abort(int trx_id) {
    // pthread_mutex_lock(&trx_manager_latch);
    const int SLOT_SIZE = 12;
    trx_t* trx = trx_table[trx_id];
    if (trx == NULL) {
        pthread_mutex_unlock(&trx_manager_latch);
        return;
    }
    
    /* it.first 가 key it.second 가 log(value, value_size, table_id, offset) */
    /* value 찾아서 값 원래대로 돌려(Recovery) */
    for (auto iter : trx->roll_back) {
        Leaf_page* lp = (Leaf_page *) malloc(sizeof(Leaf_page));
        Slot_page sp;
        sp.key = iter.first;
        sp.size = iter.second->value_size;
        sp.offset = iter.second->offset;
        pagenum_t leaf_pagenum = iter.second->page_num;
        buffer_read_leaf_page(iter.second->table_id, leaf_pagenum, (page_t*) lp);
        /* offset 이 없다;;; */
        memcpy(lp->page_body + sp.offset - 128, iter.second->original_value, sp.size);
        buffer_write_leaf_page(iter.second->table_id, leaf_pagenum, (page_t *)lp);
        free(lp);
    }
    /* lock release */
    lock_t* cur = trx->next_lock;
    while (1) {
        if ( cur == NULL ) {
            break;
        }
        lock_t* tmp = cur->trx_next_lock_ptr;
        lock_release(cur);
        cur = tmp;
    }
    trx_table.erase(trx_id);
    delete trx;
    // pthread_mutex_unlock(&trx_manager_latch);
}
/* store_log */
void store_log(int trx_id, int64_t key, int64_t table_id, Slot_page sp, Leaf_page lp, pagenum_t lp_pagenum) {
    pthread_mutex_lock(&trx_manager_latch);
    trx_t* trx = trx_table[trx_id];
    if (trx->roll_back[key] == NULL) {
        log_t* log = new log_t();
        log->original_value = new char[sp.size];
        memcpy(log->original_value, lp.page_body + sp.offset - 128, sp.size);
        log->value_size = sp.size;
        log->table_id = table_id;
        log->offset = sp.offset;
        log->page_num = lp_pagenum;
        trx->roll_back[key] = log;
    }
    pthread_mutex_unlock(&trx_manager_latch);
}
/* 이거 다시 한번 보자 */
bool deadlock_detection(int start_trx, int cur_trx_id) {
    if (visited[cur_trx_id]) {
        if (start_trx == cur_trx_id) {
            return true;
        }
        return false;
    }
    visited[cur_trx_id] = true;
    for (auto iter = trx_table[cur_trx_id]->wait_for_graph.begin(); iter != trx_table[cur_trx_id]->wait_for_graph.end(); iter++) {
        if (trx_table[*iter] == NULL) continue;
        if (cur_trx_id == *iter) continue;
        if (deadlock_detection(start_trx, *iter)) {
            return true;
        }
    }
    return false;
}


void wake_up_same_trx(lock_t* immediate_lock) {
    lock_t* cur = immediate_lock;
    while (cur) {
        if (cur->owner_trx_id != immediate_lock->owner_trx_id && cur->record_id == immediate_lock->record_id) {
            break;
        }
        if (cur->record_id == immediate_lock->record_id) {
            pthread_cond_signal(&cur->cond);
        }
        cur = cur->next;
    }
}

lock_t* find_same_lock(hash_table_entry* entry, int64_t key, int trx_id, int lock_mode) {
    lock_t* cur = entry->head;
    while (cur) {
        if (cur->record_id == key && cur->owner_trx_id == trx_id && cur->lock_mode == lock_mode) {
            break;
        }
        cur = cur->next;
    }

    return cur;
}

bool find_prev_lock(lock_t* immediate_lock) {
    lock_t* cur = immediate_lock->prev;
    bool is_exists = false;
    while (cur) {
        if (cur->record_id == immediate_lock->record_id) {
            is_exists = true;
            break;
        }
        cur = cur->prev;
    }
    return is_exists;
}
