#include "db.h"
#include "file.h"
#include "page.h"
#include "bpt.h"

#include <ctime>
#include <gtest/gtest.h>

#include <string>
#define SIZE 10000

/*******************************************************************************
 * The test structures stated here were written to give you and idea of what a
 * test should contain and look like. Feel free to change the code and add new
 * tests of your own. The more concrete your tests are, the easier it'd be to
 * detect bugs in the future projects.
 ******************************************************************************/


/*
 * TestFixture for page allocation/deallocation tests
 */
class BpTree : public ::testing::Test {
protected:
    /*
     * NOTE: You can also use constructor/destructor instead of SetUp() and
     * TearDown(). The official document says that the former is actually
     * perferred due to some reasons. Checkout the document for the difference
     */
    BpTree() { 
        init_db();
        fd = open_table(pathname.c_str()); 
    }

    ~BpTree() {
        if (fd >= 0) {
            shutdown_db();
        }
    }

    int64_t fd;                // file descriptor
    std::string pathname = "../randominsert.db";  // path for the file
};





/* Insert Test Code */


// TEST_F(BpTree, AscendingInsert) {
//     Header_page* hp = (Header_page *)malloc(sizeof(Header_page));
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));

//     file_read_page(fd, 0, (page_t*)hp);

//     ASSERT_EQ(hp->magic_number, 2022);
    
//     srand((unsigned int)time(NULL));
//     for (int i = 1; i <= SIZE; i++) {
//         int value_size = rand() % 63 + 50;
//         // int value_size = 50;
//         // int value_size = 112;
//         // EXPECT_EQ(db_insert(fd, i, "Bukayo Saka", value_size), 0);
//         db_insert(fd, i, "Bukayo Saka", value_size);
//     }

//     file_read_page(fd, 0, (page_t*)hp);
//     Internal_page* root = (Internal_page *)malloc(sizeof(Internal_page));
//     file_read_page(fd, hp->root_page_number, (page_t *)root);

//     free(ret_val);
//     free(val_size);
//     free(hp);
//     // free(root);

//     // int is_removed = remove(pathname.c_str());
//     // ASSERT_EQ(is_removed, 0);
// }



TEST_F(BpTree, DescendingInsert) {
    Header_page* hp = (Header_page *)malloc(sizeof(Header_page));
    char* ret_val = (char*)malloc(sizeof(char) * 112);
    uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));

    file_read_page(fd, 0, (page_t*)hp);

    ASSERT_EQ(hp->magic_number, 2022);
    
    srand((unsigned int)time(NULL));
    for (int i = SIZE; i >= 1; i--) {
        int value_size = rand() % 63 + 50;
        // int value_size = 50;
        EXPECT_EQ(db_insert(fd, i, "Eddie Nketia", value_size), 0);
        // db_insert(fd, i, "Eddie Nketia", value_size);
        // printf("insert %d end!\n", i);
        // EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
        // printf("insert %d 끝!\n\n", i);
    }

    file_read_page(fd, 0, (page_t*)hp);
    Internal_page* root = (Internal_page *)malloc(sizeof(Internal_page));
    file_read_page(fd, hp->root_page_number, (page_t *)root);
}


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
//         db_insert(fd, key, "Bukayo Saka", value_size);
//         // EXPECT_EQ(db_insert(fd, key, "Bukayo Saka", value_size), 0);
//         // db_insert(fd, i, "Eddie Nketia", value_size);
//         // printf("insert %d end!\n", i);
//         // EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
//     }

//     file_read_page(fd, 0, (page_t*)hp);
//     Internal_page* root = (Internal_page *)malloc(sizeof(Internal_page));
//     file_read_page(fd, hp->root_page_number, (page_t *)root);

//     free(ret_val);
//     free(val_size);
//     free(hp);
//     // free(root);

// }









/* Find Test Code */

// TEST_F(BpTree, FindSingleKey) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
//     uint16_t* val_size = (uint16_t*)malloc(sizeof(uint16_t));
//     for (int i = 1; i <= SIZE; i++) {
//         // std::cout << "find " << i << "시작\n";
//         EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
//     }
//     free(ret_val);
//     free(val_size);
// }











/* Scan Test Code */

TEST_F(BpTree, Scan) {
    std::vector<int64_t> keys;
    std::vector<char*> values;
    std::vector<uint16_t> val_sizes;
    EXPECT_EQ(db_scan(fd, 5624, SIZE, &keys, &values, &val_sizes), 0);

    std::cout << "test code  keys.size(): " << keys.size() << '\n';
    EXPECT_EQ(SIZE - 5624 + 1, keys.size());

    printf("keys\n");
    printf("keys.size(): %ld\n", keys.size());
    printf("values.size(): %ld\n", values.size());
    printf("val_sizes.size(): %ld\n", val_sizes.size());

    for (int i = 0; i < keys.size(); i++) {
        std::cout << keys[i] << ' ';
    }
    printf("\n");

    printf("values\n");
    for (int i = 0; i < values.size(); i++) {
        std::cout << values[i] << '\n';
    }
    printf("\n");

    printf("val_sizes\n");
    for (int i = 0; i < val_sizes.size(); i++) {
        std::cout << keys[i] << ": " ;
        std::cout << val_sizes[i] << ' ';
    }
    printf("\n");

}





/* Delete Test Code */


// TEST_F(BpTree, DeleteAscending) {
//     char* ret_val = (char*)malloc(sizeof(char) * 112);
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
//         // std::cout << "find " << i << "시작\n";
//         EXPECT_EQ(db_find(fd, i, ret_val, val_size), 0);
//         std::cout << ret_val << '\n';
//     }
//     free(ret_val);
//     free(val_size);
// }

