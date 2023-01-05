#include "lock_table.h"

/* hash table 원소 하나당 lock table 이 구성되어 있어 */
struct lock_t {
    /* GOOD LOCK :) */
    lock_t* prev;
    lock_t* next;
    /* Sentinel pointer */      // hash table 을 가리키는 pointer
    hash_table_entry* sentinel;
    /* Conditional Variable */  // pthread_cond?
    pthread_cond_t cond;
};

struct hash_table_entry {
    int64_t table_id;
    int64_t record_id;
    lock_t* head;
    lock_t* tail;
};

// 여기서 hash function 참고
// https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
struct Hash {
    std::size_t operator()(const std::pair<int64_t, int64_t>& key) const {
        return (std::hash<int64_t>()(key.first)
                ^ (std::hash<int64_t>()(key.second) << 1));
    }
};

/* mutex */
pthread_mutex_t lock_table_latch;

/* hash table */
std::unordered_map<std::pair<int64_t, int64_t>, hash_table_entry*, struct Hash> lock_table;


/* Initialize any data structures required for implementing lock table, such as hash table, lock table latch, etc. */
/* If success, return 0. Otherwise, return non-zero value */
int init_lock_table() {
    /* initialize mutex by using pthread_mutex_init function */
    /* 근데 이거 함수로 해야 하는 건가? => 함수로 해야될듯 성공하면 0 아닌 경우에 -1 */
    /* 이거 일어나면 안되긴 해 */
    if (pthread_mutex_init(&lock_table_latch, NULL) != 0) {
        printf("mutex initialization failed\n");
        return -1;
    }
    return 0;
}

/* Allocate and append a new lock object to the lock list of the record having the key */
/* If there is a predecessor's lock object in the lock list, sleep until the predecessor to release its lock */
/* If there is no predecessor's lock object, return the address of the new lock object */ 
/* If an error occurs, return NULL */
lock_t* lock_acquire(int64_t table_id, int64_t key) {
    // printf("lock acquire function\n");
    /* hash 된 table 이 shared resources 니까 이거 접근 못하게 막아놓고 처리 해야돼 */
    pthread_mutex_lock(&lock_table_latch);
    std::pair<int64_t, int64_t> hashing_key = {table_id, key};
    hash_table_entry* entry = lock_table[hashing_key];

    /* entry 가 존재하지 않는 경우 */
    /* entry 추가 + entry 설정 */
    if (entry == NULL) {
        entry = (hash_table_entry *) malloc(sizeof(hash_table_entry));
        entry->table_id = table_id;
        entry->record_id = key;
        entry->head = NULL;
        entry->tail = NULL;
        lock_table[hashing_key] = entry;
    }

    /* lock 추가 + lock 설정 */
    lock_t* lock = (lock_t *) malloc(sizeof(lock_t));
    /* 일어나진 않겠지만, NULL 인 경우에 return NULL */
    if (lock == NULL) {
        return NULL;
    }
    lock->prev = entry->tail;
    lock->next = NULL;
    lock->sentinel = entry;
    lock->cond = PTHREAD_COND_INITIALIZER;
    /* lock 이 제일 첫번째인 경우 */
    /* prev 가 존재하지 않으니 */
    if (lock->prev != NULL) {
        lock->prev->next = lock;
    }
    /* head 가 NULL */
    if (entry->head == NULL) {
        entry->head = lock;
    }
    entry->tail = lock;    

    if (entry->head != lock) {
        pthread_cond_wait(&lock->cond, &lock_table_latch);
    }

    // printf("line 103\n");
    pthread_mutex_unlock(&lock_table_latch);
    return lock;
};

/* Remove the lock_obj from the lock list */
/* If there is a successor's lock waiting for the thread releasing the lock, wake up the successor. */
/* If success, return 0. Otherwise, return non-zero value */
int lock_release(lock_t* lock_obj) {
    // printf("lock release function\n");
    pthread_mutex_lock(&lock_table_latch);
    /* wake up => pthread_cond_signal(lock) */
    /* param 에 release 할 lock 을 그대로 받았어 */
    /* 무조건 head 를 release 하지 않나? */
    /* entry->head == NULL 이면 return -1 */
    hash_table_entry* entry = lock_obj->sentinel;
    if (entry->head == NULL) {
        return -1;
    }
    /* lock_obj == entry->head */
    /* wake up 해야 될 lock 이 더이상 없어 */
    if (lock_obj->next == NULL) {
        entry->head = NULL;
        entry->tail = NULL;       
    }
    /* wake up 해야 될 lock 이 있는 경우 => 깨워야 해 */
    else {
        entry->head = lock_obj->next;
        pthread_cond_signal(&lock_obj->next->cond);
    }

    free(lock_obj);
    pthread_mutex_unlock(&lock_table_latch);
    return 0;
}
