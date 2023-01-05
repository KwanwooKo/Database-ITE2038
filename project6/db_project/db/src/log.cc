#include "log.h"
#include "file.h"
#include <stdlib.h>
#define DEBUG 0
// FILE* log_file;
int log_file_fd;
FILE* log_msg_file;
/* log msg file 도 새로 만들어야 될듯? */
pthread_mutex_t log_buffer_latch = PTHREAD_MUTEX_INITIALIZER;
int64_t current_LSN = 0;
int64_t last_compensate_log_LSN = 0;
/* log_buffer */
std::vector<void*> log_buffer;
std::set<int> winner;
std::set<int> loser;
std::map<int, int64_t> loser_map;
/* trx 의 LSN 을 계속 갱신 + prev_LSN 의 정보를 이걸 통해서 참조 */
std::map<int, int64_t> trx_prev_LSN;
/* 0: begin | 1: update | 2: commit | 3: rollback | 4: compensate */
int init_log(int flag, int log_num, const char* log_path, const char* logmsg_path) {
    std::cout << "[INFO] init log start" << std::endl;
    /* log_data 파일 열기 */
    log_file_fd = open(log_path, O_RDWR | O_SYNC);
    if (log_file_fd == -1) {
        log_file_fd = open(log_path, O_RDWR | O_CREAT | O_SYNC, 0666);
    }
    /* msg file 열기 */
    log_msg_file = fopen(logmsg_path, "w");
    log_buffer_latch = PTHREAD_MUTEX_INITIALIZER;
    /* analysis pass */
    current_LSN = 0;
    trx_prev_LSN.clear();
    analysis_pass(flag, log_num);
    std::cout << "[INFO] analysis pass finished" << std::endl;
    /* redo pass */
    int redo_flag = redo_pass(flag, log_num);
    if (redo_flag == 0) return 0;
    std::cout << "[INFO] redo pass finished" << std::endl;
    /* undo pass */
    int undo_flag = undo_pass(flag, log_num);
    if (undo_flag == 0) return 0;
    std::cout << "[INFO] undo pass finished" << std::endl;
    fflush(log_msg_file);
    return 1;
}
void shutdown_log() {
    flush_log();
    close(log_file_fd);
    fclose(log_msg_file);
}
/* latch X */
/* init_log 에서 current_LSN 을 0으로 초기화 */
void analysis_pass(int flag, int log_num) {
    /* file 둘 다 맨 앞으로 수정 */
    rewind(log_msg_file);
    lseek(log_file_fd, 0, SEEK_SET);
    fprintf(log_msg_file, "[ANALYSIS] Analysis pass start\n");
    while (1) {
        general_log_t general_log;
        /* log_file에서 정보 읽어오기 */
        if (read(log_file_fd, &general_log, GENERAL_LOG_SIZE) <= 0) break;
        lseek(log_file_fd, -GENERAL_LOG_SIZE, SEEK_CUR);
        lseek(log_file_fd, general_log.log_size, SEEK_CUR);
        current_LSN += general_log.log_size;
        /* compensate log 가 나타나면 이걸로 계속 갱신 */
        if (general_log.type == COMPENSATE) {
            last_compensate_log_LSN = general_log.LSN;
        }
        /* trx 의 자기 자신 LSN 을 계속 저장해줌 */
        trx_prev_LSN[general_log.trx_id] = general_log.LSN;
        loser_map[general_log.trx_id] = general_log.LSN;
        if (general_log.type == BEGIN) {
            loser.insert(general_log.trx_id);
        }
        else if (general_log.type == COMMIT) {
            loser.erase(general_log.trx_id);
            winner.insert(general_log.trx_id);
        }
        else if (general_log.type == ROLLBACK) {
            loser.erase(general_log.trx_id);
            winner.insert(general_log.trx_id);
        }

    }
    #if DEBUG
    std::cout << "[DEBUG] last_compensate_log_LSN: " << last_compensate_log_LSN << std::endl;
    #endif
    fprintf(log_msg_file, "[ANALYSIS] Analysis success. Winner:");
    for (auto iter : winner) {
        fprintf(log_msg_file, " %d", iter);
    }
    fprintf(log_msg_file, ", Loser:");
    for (auto iter : loser) {
        fprintf(log_msg_file, " %d", iter);
    }
    fprintf(log_msg_file, "\n");
    
}
/* latch X */
int redo_pass(int flag, int log_num) {
    /* log data 파일은 처음으로 커서 돌려주기 */
    lseek(log_file_fd, 0, SEEK_SET);
    fprintf(log_msg_file, "[REDO] Redo pass start\n");
    int redo_log_num = 0;

    while (1) {
        if (redo_log_num > log_num && flag == REDO_CRASH) break;
        redo_log_num++;
        general_log_t general_log;
        update_log_t update_log;
        compensate_log_t compensate_log;
        /* log_file에서 정보 읽어오기 */
        if (read(log_file_fd, &general_log, GENERAL_LOG_SIZE) <= 0) break;

        /* 읽은 general_size 만큼 다시 돌려줘 */
        lseek(log_file_fd, -GENERAL_LOG_SIZE, SEEK_CUR);
        int log_type = general_log.type;
        int cur_trx_id = general_log.trx_id;
        if (log_type == BEGIN || log_type == COMMIT || log_type == ROLLBACK) {
            lseek(log_file_fd, GENERAL_LOG_SIZE, SEEK_CUR);
        }
        /* 만약 type 이 general 이 아니라면 다시 읽어야 해 */
        else if (log_type == UPDATE) {
            read(log_file_fd, &update_log, UPDATE_LOG_SIZE);
            update_log.old_image = new char[update_log.data_length];
            read(log_file_fd, update_log.old_image, update_log.data_length);
            update_log.new_image = new char[update_log.data_length];
            read(log_file_fd, update_log.new_image, update_log.data_length);

            delete update_log.old_image;
            delete update_log.new_image;
        }
        else if (log_type == COMPENSATE) {
            read(log_file_fd, &compensate_log, UPDATE_LOG_SIZE);
            compensate_log.old_image = new char[compensate_log.data_length];
            read(log_file_fd, compensate_log.old_image, compensate_log.data_length);
            compensate_log.new_image = new char[compensate_log.data_length];
            read(log_file_fd, compensate_log.new_image, compensate_log.data_length);
            read(log_file_fd, &(compensate_log.next_undo_LSN), 8);

            delete compensate_log.old_image;
            delete compensate_log.new_image;
        }
        if (log_type == BEGIN) {
            fprintf(log_msg_file, "LSN %lu [BEGIN] Transaction id %d\n", general_log.LSN, general_log.trx_id);
        }
        else if (log_type == ROLLBACK) {
            fprintf(log_msg_file, "LSN %lu [ROLLBACK] Transaction id %d\n", general_log.LSN, general_log.trx_id);
        }
        else if (log_type == COMMIT) {
            fprintf(log_msg_file, "LSN %lu [COMMIT] Transaction id %d\n", general_log.LSN, general_log.trx_id);
        }
        else if (log_type == UPDATE) {
            std::string pathname = "DATA";
            pathname += std::to_string(update_log.table_id);
            open_table(pathname.c_str());

            /* 이거 update 어케 진행함? => buffer_write_leaf_page */
            Leaf_page lp;
            buffer_read_leaf_page(update_log.table_id, update_log.page_number, (page_t *)&lp);
            memcpy(lp.page_body + update_log.offset - 128, update_log.new_image, update_log.data_length);

            std::cout << "log's LSN: " << update_log.LSN << std::endl;
            std::cout << "page's LSN: " << lp.page_LSN << std::endl;
            std::cout << "parent_page_number: " << lp.parent_page_number << std::endl;
            std::cout << "is_leaf: " << lp.is_leaf << std::endl; 

            if (update_log.LSN <= lp.page_LSN) {
                fprintf(log_msg_file, "LSN %lu [CONSIDER-REDO] Transaction id %d\n", update_log.LSN, update_log.trx_id);   
            }
            else {
                buffer_write_leaf_page(update_log.table_id, update_log.page_number, (page_t *)&lp);
                /* table_id 이용해서 찾고, page_number 로 접근하고, */
                fprintf(log_msg_file, "LSN %lu [UPDATE] Transaction id %d redo apply\n", update_log.LSN, update_log.trx_id);
            }
        }
        else if (log_type == COMPENSATE) {
            fprintf(log_msg_file, "LSN %lu [CLR] next undo lsn %lu\n", compensate_log.LSN, compensate_log.next_undo_LSN);
        }
    }
    if (flag == REDO_CRASH) {
        return 0;
    } 
    fprintf(log_msg_file, "[REDO] Redo pass end\n");
    return 1;
}
/* loser 에 대해서 다시 원래대로 돌려놔야 해 */
int undo_pass(int flag, int log_num) {
    fprintf(log_msg_file, "[UNDO] Undo pass start\n");
    /* 매 사이클마다 마지막 LSN 을 찾아 */
    int undo_log_num = 0;
    while (1) {
        /* 탈출 조건 => loser 가 없는 경우 or log_num or flag 관련 */
        if (loser.size() == 0) break;
        if (undo_log_num > log_num && flag == UNDO_CRASH) break;
        undo_log_num++;
        /* 가장 큰 LSN 을 가진 loser 를 찾아 */
        int trx_id = -1;
        int64_t last_undo_LSN = -1;
        for (auto iter : loser) {
            if (last_undo_LSN < loser_map[iter]) {
                trx_id = iter;
                last_undo_LSN = loser_map[trx_id];
            }
        }

        lseek(log_file_fd, last_undo_LSN, SEEK_SET);

        general_log_t general_log;
        update_log_t update_log;
        compensate_log_t compensate_log;
        read(log_file_fd, &general_log, GENERAL_LOG_SIZE);
        // std::cout << "log_type: " << general_log.type << std::endl;
        lseek(log_file_fd, -GENERAL_LOG_SIZE, SEEK_CUR);
        /* 여기서 타입에 따라 읽는 게 달라져 */
        int log_type = general_log.type;
        /* 다시 원래 위치로 */
        if (log_type == BEGIN || log_type == COMMIT || log_type == ROLLBACK) {
            lseek(log_file_fd, GENERAL_LOG_SIZE, SEEK_CUR);
        }
        /* 만약 type 이 general 이 아니라면 다시 읽어야 해 */
        else if (log_type == UPDATE) {
            read(log_file_fd, &update_log, UPDATE_LOG_SIZE);
            update_log.old_image = new char[update_log.data_length];
            read(log_file_fd, update_log.old_image, update_log.data_length);
            update_log.new_image = new char[update_log.data_length];
            read(log_file_fd, update_log.new_image, update_log.data_length);
        }
        else if (log_type == COMPENSATE) {
            read(log_file_fd, &compensate_log, UPDATE_LOG_SIZE);
            compensate_log.old_image = new char[compensate_log.data_length];
            read(log_file_fd, compensate_log.old_image, compensate_log.data_length);
            compensate_log.new_image = new char[compensate_log.data_length];
            read(log_file_fd, compensate_log.new_image, compensate_log.data_length);
            read(log_file_fd, &(compensate_log.next_undo_LSN), 8);
            loser_map[compensate_log.trx_id] = compensate_log.next_undo_LSN;
        }
        
        /* 여기서부터 undo 과정 */
        if (log_type == UPDATE) {
            /* undo 를 진행하고 CLR를 작성 */
            /* undo 진행 */
            std::string pathname = "DATA";
            pathname += std::to_string(update_log.table_id);
            open_table(pathname.c_str());

            Leaf_page* lp = new Leaf_page();
            buffer_read_leaf_page(update_log.table_id, update_log.page_number, (page_t *) lp);
            /* 원래 값으로 돌려줘 (일반 update 랑 달라) )*/
            memcpy(lp->page_body + update_log.offset - 128, update_log.new_image, update_log.data_length);
            buffer_write_leaf_page(update_log.table_id, update_log.page_number, (page_t *) lp);

            write_compensate_log(update_log.trx_id, update_log.table_id, update_log.page_number, update_log.offset, update_log.data_length, update_log.old_image, update_log.new_image);

            loser_map[update_log.trx_id] = update_log.prev_LSN;
            fprintf(log_msg_file, "LSN %lu [UPDATE] Transaction id %d undo apply\n", update_log.LSN, update_log.trx_id);
            delete lp;
            /* CLR 작성*/
        }
        else if (log_type == BEGIN) {
            /* BEGIN 만 하고 다른 연산을 안함 => 위너 취급 (실제로 위너는 아님) */
            /* 출력문 적고 rollback log 를 작성 */
            write_general_log(general_log.trx_id, ROLLBACK);
            fprintf(log_msg_file, "LSN %lu [BEGIN] Transaction id %d\n", general_log.LSN, general_log.trx_id);
            loser.erase(general_log.trx_id);
        }
    }
    if (flag == UNDO_CRASH) {
        flush_log();
        return 0;
    } 
    fprintf(log_msg_file, "[UNDO] Undo pass end\n");
    return 1;
}
/* begin, commit, rollback 셋이 같은 맥락 => 써보다가 뒤의 commit, rollback 함수가 필요없으면 그냥 이거 general 로 바꾸고 이걸로 적자  */
void write_general_log(int trx_id, int type) {
    pthread_mutex_lock(&log_buffer_latch);
    int64_t prev_LSN = -1;
    if (type != BEGIN) {
        prev_LSN = trx_prev_LSN[trx_id];
    }
    
    general_log_t* log = allocate_new_general_log(current_LSN, prev_LSN, trx_id, type);
    /* buffer가 다 찬 경우 -> flush */
    if (log_buffer.size() == LOG_BUFFER_SIZE) {
        flush_log();
    }
    log_buffer.push_back((void*) log);
    pthread_mutex_unlock(&log_buffer_latch);
}
/* 이거 old_image 무조건 무조건 무조건 memcpy 해야돼 => 이렇게 했음(allocate_new_update_log 함수에서) */
int64_t write_update_log(int trx_id, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image) {
    pthread_mutex_lock(&log_buffer_latch);
    int64_t prev_LSN = trx_prev_LSN[trx_id];
    
    update_log_t* log = allocate_new_update_log(current_LSN, prev_LSN, trx_id, UPDATE, table_id, page_number, offset, data_length, old_image, new_image);
    // /* buffer가 다 찬 경우 -> flush */
    if (log_buffer.size() == LOG_BUFFER_SIZE) {
        flush_log();
    }
    log_buffer.push_back((void*) log);
    pthread_mutex_unlock(&log_buffer_latch);
    return log->LSN;
}
void write_compensate_log(int trx_id, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image) {
    pthread_mutex_lock(&log_buffer_latch);
    int64_t prev_LSN = trx_prev_LSN[trx_id];
    compensate_log_t* log = allocate_new_compensate_log(current_LSN, prev_LSN, trx_id, COMPENSATE, table_id, page_number, offset, data_length, old_image, new_image);
    if (log_buffer.size() == LOG_BUFFER_SIZE) {
        flush_log();
    }
    log_buffer.push_back((void*) log);
    pthread_mutex_unlock(&log_buffer_latch);
}
/* latch X */
general_log_t* allocate_new_general_log(int64_t LSN, int64_t prev_LSN, int trx_id, int type) {
    general_log_t* log = new general_log_t();
    log->log_size = GENERAL_LOG_SIZE;
    log->LSN = LSN;
    log->prev_LSN = prev_LSN;
    log->trx_id = trx_id;
    log->type = type;
    /* LSN 저장, current_LSN 은 현재 LSN 을 위해, trx_prev_LSN 은 prev LSN */
    trx_prev_LSN[trx_id] = LSN;
    current_LSN += log->log_size;
    return log;
}
update_log_t* allocate_new_update_log(int64_t LSN, int64_t prev_LSN, int trx_id, int type, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image) {
    update_log_t* log = new update_log_t();
    log->log_size = UPDATE_LOG_SIZE + 2 * data_length;
    log->LSN = LSN;
    log->prev_LSN = prev_LSN;
    log->trx_id = trx_id;
    log->type = type;
    log->table_id = table_id;
    log->page_number = page_number;
    log->offset = offset;
    log->data_length = data_length;
    log->old_image = new char[data_length];
    memcpy(log->old_image, old_image, data_length);
    log->new_image = new char[data_length];
    memcpy(log->new_image, new_image, data_length);
    /* LSN 저장, current_LSN 은 현재 LSN 을 위해, trx_prev_LSN 은 prev LSN */
    trx_prev_LSN[trx_id] = LSN;
    current_LSN += log->log_size;
    return log;
}
compensate_log_t* allocate_new_compensate_log(int64_t LSN, int64_t prev_LSN, int trx_id, int type, int64_t table_id, pagenum_t page_number, uint16_t offset, uint16_t data_length, const char* old_image, const char* new_image) {
    compensate_log_t* log = new compensate_log_t();
    log->log_size = COMPENSATE_LOG_SIZE + 2 * data_length;
    log->LSN = LSN;
    log->prev_LSN = prev_LSN;
    log->trx_id = trx_id;
    log->type = type;
    log->table_id = table_id;
    log->page_number = page_number;
    log->offset = offset;
    log->data_length = data_length;
    log->old_image = new char[data_length];
    memcpy(log->old_image, old_image, data_length);
    log->new_image = new char[data_length];
    memcpy(log->new_image, new_image, data_length);
    log->next_undo_LSN = last_compensate_log_LSN;
    last_compensate_log_LSN = log->LSN;
    /* LSN 저장, current_LSN 은 현재 LSN 을 위해, trx_prev_LSN 은 prev LSN */
    trx_prev_LSN[trx_id] = LSN;
    current_LSN += log->log_size;
    return log;
}
/* log 에 형변환 해서 parameter 로 넣어 */
int get_type_from_log(void* log) {
    int ret;
    memcpy(&ret, (char*)log + 24, 4);
    return ret;
}

void flush_log_with_latch() {
    pthread_mutex_lock(&log_buffer_latch);
    flush_log();
    pthread_mutex_unlock(&log_buffer_latch);
}

/* log buffer 비우기, log_buffer.size 가 최대치를 넘어갈 때 확인하고 flush */
void flush_log() {
    int count = 1;
    for (void* log : log_buffer) {
        // std::cout << "count:     " << count++ << std::endl;
        int type = get_type_from_log(log);
        switch(type) {
            case BEGIN:
            case COMMIT:
            case ROLLBACK:
                flush_general_log((general_log_t *) log);
                // delete (general_log_t *) log;
                break;
            case UPDATE:
                flush_update_log((update_log_t *) log);
                // delete (update_log_t *) log;
                break;
            case COMPENSATE:
                flush_compensate_log((compensate_log_t *) log);
                // delete (compensate_log_t *) log;
                break;
        }
    }
    for (void* log : log_buffer) {
        delete log;
    }
    log_buffer.clear();
}
void flush_general_log(general_log_t* log) {
    int cur = lseek(log_file_fd, 0, SEEK_CUR);
    lseek(log_file_fd, 0, SEEK_END);
    write(log_file_fd, log, GENERAL_LOG_SIZE);
    lseek(log_file_fd, cur, SEEK_SET);
}
void flush_update_log(update_log_t* log) {
    int cur = lseek(log_file_fd, 0, SEEK_CUR);
    lseek(log_file_fd, 0, SEEK_END);
    /* 1. 48바이트 만큼 옮기기 */
    write(log_file_fd, log, UPDATE_LOG_SIZE);
    /* 2. old_image 옮기기 */
    write(log_file_fd, log->old_image, log->data_length);
    /* 3. new_image 옮기기 */
    write(log_file_fd, log->new_image, log->data_length);
    lseek(log_file_fd, cur, SEEK_SET);
    
}
void flush_compensate_log(compensate_log_t* log) {
    int cur = lseek(log_file_fd, 0, SEEK_CUR);
    lseek(log_file_fd, 0, SEEK_END);
    /* 1. 48바이트 만큼 옮기기 */
    write(log_file_fd, log, UPDATE_LOG_SIZE);
    /* 2. old_image 옮기기 */
    write(log_file_fd, log->old_image, log->data_length);
    /* 3. new_image 옮기기 */
    write(log_file_fd, log->new_image, log->data_length);
    /* 4. next_undo_LSN 옮기기 */
    write(log_file_fd, &(log->next_undo_LSN), 8);
    lseek(log_file_fd, cur, SEEK_SET);
}
/*
1. bcr => 똑같이 적으면 돼
2. update && compensate
48 byte 는 읽어 
1) 48 byte 를 읽고
2) data_length old_image
3) data_length new_image
4) next_undo_LSN
*/

