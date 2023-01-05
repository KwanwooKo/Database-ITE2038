#include "db.h"
#include "file.h"
#include "page.h"
#include "buffer.h"
#include "bpt.h"
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

// TEST_F(BpTree, BufferReadTest) {
//     printf("BufferReadTest 시작\n");
//     Header_page* hp = (Header_page *)malloc(sizeof(Header_page));
//     buffer_read_page(fd, 0, (page_t *) hp);
//     printf("test code line 46\n");
//     std::cout << "root_page_num: " << hp->root_page_number << '\n';
//     std::cout << "magic_number: " << hp->magic_number << '\n';
//     ASSERT_EQ(hp->magic_number, 2022);
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
    Header_page* hp = (Header_page *)malloc(sizeof(Header_page));
    char* ret_val = (char*)malloc(sizeof(char) * 112);
    uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));

    buffer_read_page(fd, 0, (page_t*) hp);
    ASSERT_EQ(hp->magic_number, 2022);


    srand((unsigned int)time(NULL));
    for (int i = 1; i <= SIZE; i++) {
        printf("insert %d\n", i);
        int value_size = rand() % 63 + 50;
        // int value_size = 50;
        // int value_size = 112;
        EXPECT_EQ(db_insert(fd, i, "Bukayo Saka", value_size), 0);
        EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
        // db_insert(fd, i, "Bukayo Saka", value_size);
    }

    free(ret_val);
    free(val_size);
    free(hp);
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
//    // ASSERT_EQ(is_removed, 0);
// }


/* Scan Test Code */
// TEST_F(BpTree, Scan) {
//     std::vector<int64_t> keys;
//     std::vector<char*> values;
//     std::vector<uint16_t> val_sizes;
//     EXPECT_EQ(db_scan(fd, 5624, 6379, &keys, &values, &val_sizes), 0);
//     std::cout << "test code  keys.size(): " << keys.size() << '\n';
//     EXPECT_EQ(6379 - 5624 + 1, keys.size());
//     EXPECT_EQ(6379 - 5624 + 1, val_sizes.size());
//     EXPECT_EQ(6379 - 5624 + 1, values.size());
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
