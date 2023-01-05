#ifndef __LOG_H__
#define __LOG_H__
#include "page.h"
#include "trx.h"
#include "bpt.h"
#include <string>
#include <map>
#include <vector>
#include <set>

#define NORMAL_RECOVERY 0
#define REDO_CRASH 1
#define UNDO_CRASH 2

#define BEGIN 0
#define UPDATE 1
#define COMMIT 2
#define ROLLBACK 3
#define COMPENSATE 4
#define GENERAL_LOG_SIZE 28
#define UPDATE_LOG_SIZE 48
#define COMPENSATE_LOG_SIZE 56
#define LOG_BUFFER_SIZE 50

extern pthread_mutex_t log_buffer_latch;

typedef struct general_log_t general_log_t;
typedef struct update_log_t update_log_t;
typedef struct compensate_log_t compensate_log_t;

#pragma pack (push, 1)
struct general_log_t {
    uint32_t log_size;
    int64_t LSN;
    int64_t prev_LSN;
    int trx_id;
    int type;
};

struct update_log_t {
    uint32_t log_size;
    int64_t LSN;
    int64_t prev_LSN;
    int trx_id;
    int type;
    int64_t table_id;
    pagenum_t page_number;
    uint16_t offset;
    uint16_t data_length;
    char* old_image;
    char* new_image;

};

struct compensate_log_t {
    uint32_t log_size;
    int64_t LSN;
    int64_t prev_LSN;
    int trx_id;
    int type;
    int64_t table_id;
    pagenum_t page_number;
    uint16_t offset;
    uint16_t data_length;
    char* old_image;
    char* new_image;
    int64_t next_undo_LSN;
};
#pragma pack(pop)

int init_log(int flag, int log_num, const char* log_path, const char* logmsg_path);
void shutdown_log();
void analysis_pass(int flag, int log_num);
int redo_pass(int flag, int log_num);
int undo_pass(int flag, int log_num);


void flush_log();
void flush_log_with_latch();
void flush_general_log(general_log_t* log);
void flush_update_log(update_log_t* log);
void flush_compensate_log(compensate_log_t* log);
int get_type_from_log(void* log);


void get_old_image(update_log_t* log, char* src);
void get_old_image(compensate_log_t* log, char* src);
void get_new_image(update_log_t* log, char* src);
void get_new_image(compensate_log_t* log, char* src);

/* function overloading */
general_log_t* allocate_new_general_log(int64_t LSN, int64_t prev_LSN, int trx_id, int type);
update_log_t* allocate_new_update_log(int64_t LSN, int64_t prev_LSN, int trx_id, int type, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image);
compensate_log_t* allocate_new_compensate_log(int64_t LSN, int64_t prev_LSN, int trx_id, int type, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image);

/* log file 을 적는 함수, log_msg 아님! */
void write_general_log(int trx_id, int type);
int64_t write_update_log(int trx_id, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image);
void write_compensate_log(int trx_id, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image);

/* log 의 종류에 관계없이 log 타입을 반환 */
int get_type_from_log(void* log);

#endif