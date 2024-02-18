#include "../db/include/file.h"
#include "../db/include/page.h"
#include "../db/include/buffer.h"
#include "../db/include/trx.h"
#include "../db/include/bpt.h"
#include <ctime>
#include <gtest/gtest.h>
#include <string>
#define SIZE 100000


// /*******************************************************************************
//  * The test structures stated here were written to give you and idea of what a
//  * test should contain and look like. Feel free to change the code and add new
//  * tests of your own. The more concrete your tests are, the easier it'd be to
//  * detect bugs in the future projects.
//  ******************************************************************************/
// /*
//  * TestFixture for page allocation/deallocation tests    
//  */
class BpTree : public ::testing::Test {
protected:
    /*
     * NOTE: You can also use constructor/destructor instead of SetUp() and
     * TearDown(). The official document says that the former is actually
     * perferred due to some reasons. Checkout the document for the difference
     */
    BpTree() { 
        init_db(512);
        fd = open_table(pathname.c_str()); 
    }
    ~BpTree() {
        if (fd >= 0) {
            shutdown_db();
        }
    }
    int64_t fd;                // file descriptor
    std::string pathname = "../insertTest.db";  // path for the file
};


// TEST_F(BpTree, UpdateFindRandomTest) {
//     char* value = (char *) malloc(sizeof(char) * 112);
//     // strcpy(value, "Eddie Nketia");
//     strcpy(value, "Gabriel Martinelli");
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     int trx_id = -1;
//     trx_id = trx_begin();
//     int command = 0;
//     for (int i = 1; i <= SIZE; i++) {
//         uint16_t value_size = 112;
//         uint16_t new_val_size = 112;
//         // int key = rand() % SIZE;
//         int key = i;
//         if (i % 100 == 0) {
//             trx_commit(trx_id);
//             trx_id = trx_begin();
//         }
//         command = rand() % 2;
//         if (command == 0) {
//             std::cout << "find " << key << '\n';
//             EXPECT_EQ(db_find(fd, i, ret_val, val_size, trx_id), 0);
//         }
//         else if (command == 1) {
//             std::cout << "update " << key << '\n';
//             EXPECT_EQ(db_update(fd, key, value, new_val_size, &value_size, trx_id), 0);
//             EXPECT_EQ(db_find(fd, key, ret_val, val_size, trx_id), 0);
//             ASSERT_EQ(strcmp(ret_val, value), 0);
//         }
//     }
//     trx_commit(trx_id);
// }

// TEST_F(BpTree, UpdateFindTest) {
//     char* value = (char *) malloc(sizeof(char) * 112);
//     strcpy(value, "Eddie Nketia");
//     // strcpy(value, "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     int trx_id = 1;
//     trx_id = trx_begin();
//     for (int i = 1; i <= SIZE; i++) {
//         // std::cout << "find " << i << "시작\n";
//         if (i % 1000 == 0) {
//             trx_commit(trx_id);
//             trx_id = trx_begin();
//         }
//         uint16_t value_size = 112;
//         uint16_t new_val_size = 112;
//         std::cout << "update " << i << '\n';
//         EXPECT_EQ(db_find(fd, i, ret_val, val_size, trx_id), 0);
//         EXPECT_EQ(db_update(fd, i, value, new_val_size, &value_size, trx_id), 0);
//         std::cout << "find " << i << '\n';
//         EXPECT_EQ(db_find(fd, i, ret_val, val_size, trx_id), 0);
//         ASSERT_EQ(strcmp(ret_val, value), 0);
//         // std::cout << "value: " << ret_val << '\n';
//     }
//     trx_commit(trx_id);
// }

//TEST_F(BpTree, UpdateTest) {
//    char* value = (char *) malloc(sizeof(char) * 112);
//    memcpy(value, "Gabriel Martinelli", 112);
//    int trx_id = trx_begin();
//    uint16_t value_size = 112;
//    uint16_t new_val_size = 112;
//    for (int i = 1; i <= SIZE; i++) {
//        if (i % 100 == 0) {
//            EXPECT_NE(trx_commit(trx_id), 0);
//            trx_id = trx_begin();
//        }
//        printf("update %d\n", i);
//        EXPECT_EQ(db_update(fd, i, value, new_val_size, &value_size, trx_id), 0);
//        // db_insert(fd, i, "Gabriel Jesus", value_size);
//    }
//    EXPECT_NE(trx_commit(trx_id), 0);
//    printf("trx commit\n");
//
//
//    char* ret_val = (char*)malloc(sizeof(char) * 112);
//    uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//
//    for (int i = 1; i <= SIZE; i++) {
//        std::cout << "find " << i << "시작\n";
//        EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
//        // ASSERT_EQ(strcmp(ret_val, "Gabriel Martinelli"), 0);
//        std::cout << "val: " << ret_val << '\n';
//    }
//}

// TEST_F(BpTree, FindSingleKey) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     int trx_number = -1;
//     trx_number = trx_begin();
//     for (int i = 1; i <= SIZE; i++) {
//         // if (i % 10 == 0) {
//         //     EXPECT_NE(trx_commit(trx_number), 0);
//         //     trx_number = trx_begin();
//         // }
//         std::cout << "find " << i << "시작\n";
//         EXPECT_EQ(db_find(fd, 1, ret_val, val_size, trx_number), 0);
//         std::cout << "value: " << ret_val << '\n';
//     }
//     // for (int i = 1; i <= trx_number; i++) {
//         EXPECT_NE(trx_commit(trx_number), 0);
//     // }
//     free(ret_val);
//     free(val_size);
// }

// TEST_F(BpTree, FindSingleKey) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     int trx_number = -1;
//     for (int i = 1; i <= SIZE; i++) {
//         trx_number = trx_begin();
//         std::cout << "find " << i << "시작\n";
//         EXPECT_EQ(db_find(fd, 1, ret_val, val_size, trx_number), 0);
//         std::cout << "value: " << ret_val << '\n';
//         EXPECT_NE(trx_commit(trx_number), 0);
//     }
//     free(ret_val);
//     free(val_size);
// }


/* Insert Test Code */
// TEST_F(BpTree, RandomInsert) {
//     Header_page* hp = (Header_page *)malloc(sizeof(Header_page));
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     file_read_page(fd, 0, (page_t*)hp);
//     ASSERT_EQ(hp->magic_number, 2022);
    
//     srand((unsigned int)time(NULL));
//     for (int i = 1; i <= SIZE; i++) {
//         int value_size = rand() % 63 + 50;
//         int key = rand() % SIZE + 1;
//         // int value_size = 50;
//         // int value_size = 112;
//         if (db_find(fd, key, ret_val, val_size) != 0) {
//             db_insert(fd, key, "Bukayo Saka", value_size);
//         }
//     }
//     file_read_page(fd, 0, (page_t*)hp);
//     Internal_page* root = (Internal_page *)malloc(sizeof(Internal_page));
//     file_read_page(fd, hp->root_page_number, (page_t *)root);
//     free(ret_val);
//     free(val_size);
//     free(hp);
//     // free(root);
// }



 TEST_F(BpTree, AscendingInsert) {
     char* ret_val = (char*)malloc(sizeof(char) * 112);
     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));

     // srand((unsigned int)time(NULL));
     for (int i = 1; i <= SIZE; i++) {
         printf("insert %d\n", i);
         // int value_size = rand() % 63 + 50;
         // int value_size = 50;
         int value_size = 112;
         EXPECT_EQ(db_insert(fd, i, "Bukayo Saka", value_size), 0);
         // EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
         // db_insert(fd, i, "Bukayo Saka", value_size);
     }

     free(ret_val);
     free(val_size);
     // int is_removed = remove(pathname.c_str());
     // ASSERT_EQ(is_removed, 0);
 }



// TEST_F(BpTree, DescendingInsert) {
//     Header_page* hp = (Header_page *)malloc(sizeof(Header_page));
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     buffer_read_page(fd, 0, (page_t*)hp);
//     ASSERT_EQ(hp->magic_number, 2022);
    
//     srand((unsigned int)time(NULL));
//     for (int i = SIZE; i >= 1; i--) {
//         int value_size = rand() % 63 + 50;
//         // int value_size = 50;
//         // int value_size = 112;
//         // EXPECT_EQ(db_insert(fd, i, "Bukayo Saka", value_size), 0);
//         db_insert(fd, i, "Bukayo Saka", value_size);
//     }
//     buffer_read_page(fd, 0, (page_t*)hp);
//     Internal_page* root = (Internal_page *)malloc(sizeof(Internal_page));
//     buffer_read_page(fd, hp->root_page_number, (page_t *)root);

//     free(ret_val);
//     free(val_size);
//     free(hp);
//     // free(root);
//     // int is_removed = remove(pathname.c_str());
//     // ASSERT_EQ(is_removed, 0);
// }


/* Scan Test Code */
// TEST_F(BpTree, Scan) {
//     const int from = 1;
//     const int to = 500;
//     std::vector<int64_t> keys;
//     std::vector<char*> values;
//     std::vector<uint16_t> val_sizes;
//     EXPECT_EQ(db_scan(fd, from, to, &keys, &values, &val_sizes), 0);
//     std::cout << "test code  keys.size(): " << keys.size() << '\n';
//     EXPECT_EQ(to - from + 1, keys.size());
//     EXPECT_EQ(to - from + 1, val_sizes.size());
//     EXPECT_EQ(to - from + 1, values.size());
//     printf("keys\n");
//     for (int i = 0; i < keys.size(); i++) {
//         std::cout << keys[i] << ' ';
//     }
//     printf("\n");

//     printf("values\n");
//     for (int i = 0; i < values.size(); i++) {
//         std::cout << values[i] << '\n';
//     }
//     printf("\n");

//     printf("val_sizes\n");
//     for (int i = 0; i < val_sizes.size(); i++) {
//         std::cout << val_sizes[i] << ' ';
//     }
//     printf("\n");
// }





/* Delete Test Code */
// TEST_F(BpTree, DeleteAscending) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     for (int i = 1; i <= SIZE; i++) {
//         EXPECT_EQ(db_delete(fd, i), 0);
//     }
//     for (int i = 1; i <= SIZE; i++) {
//         EXPECT_NE(db_find(fd, i, ret_val, val_size), 0); 
//     }
//     free(ret_val);
//     free(val_size);
// }


// TEST_F(BpTree, DeleteRandom) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     for (int i = 1; i <= SIZE; i++) {
//         int num = rand() % SIZE + 1;
//         if (db_find(fd, num, ret_val, val_size) == 0) {
//             db_delete(fd, num);
//             EXPECT_NE(db_find(fd, num, ret_val, val_size), 0);    
//         }   
//     }
//     free(ret_val);
//     free(val_size);
// }


// TEST_F(BpTree, DeleteDescending) {
//      char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     for (int i = 1; i <= SIZE; i++) {
//         if (db_find(fd, i, ret_val, val_size) == 0) {
//             db_delete(fd, i);
//             EXPECT_NE(db_find(fd, i, ret_val, val_size), 0);    
//         }   
//     }
//     free(ret_val);
//     free(val_size);
// }




/* Find Test */
// TEST_F(BpTree, NotFindSingleKey) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     for (int i = 1; i <= SIZE; i++) {
//         // std::cout << "find " << i << "시작\n";
//         EXPECT_NE(db_find(fd, i, ret_val, val_size), 0);
//     }
//     free(ret_val);
//     free(val_size);
// }

// TEST_F(BpTree, FindSingleKey) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     for (int i = 1; i <= SIZE; i++) {
//         std::cout << "find " << i << "시작\n";
//         EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
//         std::cout << "value: " << ret_val << '\n';
//     }
    
//     free(ret_val);
//     free(val_size);
// }

// TEST_F(BpTree, BufferPrint) {
//     Buffer *cur = buffer_queue;
//     if (cur == NULL) {
//         printf("buffer is NULL\n");
//     }
//     else {
//         while (cur->next) {
//             std::cout << "offset: " << cur->page_num << ' ';
//             cur = cur->next;
//         }
//     }   
// }
