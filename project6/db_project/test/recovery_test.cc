#include "file.h"
#include "page.h"
#include "buffer.h"
#include "trx.h"
#include "log.h"
#include "bpt.h"
#include <ctime>
#include <gtest/gtest.h>
#include <string>
#define SIZE 10000
#define VAL_SIZE 60
#define BUFFER_SIZE 512
#define THREADNUM 5
#define UPDATE_SIZE 100

// /*******************************************************************************
//  * The test structures stated here were written to give you and idea of what a
//  * test should contain and look like. Feel free to change the code and add new
//  * tests of your own. The more concrete your tests are, the easier it'd be to
//  * detect bugs in the future projects.
//  ******************************************************************************/
// /*
//  * TestFixture for page allocation/deallocation tests    
//  */
class Recovery : public ::testing::Test {
protected:
    /*
     * NOTE: You can also use constructor/destructor instead of SetUp() and
     * TearDown(). The official document says that the former is actually
     * perferred due to some reasons. Checkout the document for the difference
     */
    Recovery() { 
        if (std::remove(pathname.c_str()) == 0) {
            std::cout << "File Already Exists. Remove File" << std::endl;
        }
        // if (std::remove("logfile.data") == 0) {
        //     std::cout << "erase logfile" << std::endl;
        // }
        init_db(BUFFER_SIZE, NORMAL_RECOVERY, 100, "logfile.data", "logmsg.txt");
        table_id = open_table(pathname.c_str()); 
    }
    ~Recovery() {
        if (table_id >= 0) {
            shutdown_db();
        }
    }
    int64_t table_id;                // file descriptor
    std::string pathname = "DATA13";  // path for the file
};

void *x_lock_only(void *data) {
    int64_t table_id = ((int*)data)[0];
    int trx_id = trx_begin();
    uint16_t o_size;
    for (int i = 1; i <= UPDATE_SIZE; i++) {
        EXPECT_EQ(db_update(table_id, i, "Gabriel Jesus", VAL_SIZE, &o_size, trx_id), 0);
        EXPECT_EQ(o_size, VAL_SIZE);
    }
    char ret_val[112];
    uint16_t ret_val_size;
    for (int i = 1; i <= UPDATE_SIZE; i++) {
        /* find 되는지 확인 */
        EXPECT_EQ(db_find(table_id, i, ret_val, &ret_val_size), 0);
        /* value 비교 */
        EXPECT_EQ(strcmp(ret_val, "Gabriel Jesus"), 0);
    }
    trx_commit(trx_id);



    int new_id = trx_begin();
    
    for (int i = 1; i <= UPDATE_SIZE; i++) {
        EXPECT_EQ(db_update(table_id, i, "Bukayo Saka", VAL_SIZE, &o_size, new_id), 0);
        EXPECT_EQ(o_size, VAL_SIZE);
    }
    for (int i = 1; i <= UPDATE_SIZE; i++) {
        EXPECT_EQ(db_find(table_id, i, ret_val, &ret_val_size), 0);
        EXPECT_EQ(strcmp(ret_val, "Bukayo Saka"), 0);
    }

    trx_commit(new_id);

    return NULL;
}

TEST_F(Recovery, X_LOCK_ONLY) {
    /* insert 부터 진행 */
    for (int i = 1; i <= SIZE; i++) {
        EXPECT_EQ(db_insert(table_id, i, "Gabriel Martinelli", VAL_SIZE), 0);
    }
    std::cout << "[INFO] Insert finished" << std::endl;

    int thread_num = THREADNUM;
    uint16_t val_size;
    pthread_t threads[thread_num];

    int* param_table_id = (int*)malloc(sizeof(int));
    *param_table_id = table_id;
    for (int i = 0; i < thread_num; i++) {
        pthread_create(&threads[i], NULL, x_lock_only, param_table_id);
    }

    for (int i = 0; i < thread_num; i++) {
        pthread_join(threads[i], NULL);
    }

}

TEST_F(Recovery, X_LOCK_ONLY_CHECK) {
    
}
