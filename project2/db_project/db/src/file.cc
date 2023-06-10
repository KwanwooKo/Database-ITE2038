#include "file.h"

using namespace std;

// Open existing database file or create one if it doesn't exist
/**
 *  Open the database file
 *  It opens an existing database file using 'pathname'(parameter) or create a new file if absent
 *  If a new file needs to be created, the default file size should be 10MiB.
 *  Then it returns the file descriptor of the opened database file.
 *  All other 5 commands below should be handled after open data file.
 *  If it opens an existing database file, it must check the magic number of database file.
 *  If the checked number is different from the expected value, then it returns a negative value for error handling.
 */

int64_t file_open_table_file(const char* pathname) { 

    // find the file whether it is opened
    // fd == file descriptor
    int64_t fd = open(pathname, O_RDWR | O_SYNC);
    // if the file is not existed create new file
    if (fd == -1) {
        printf("create new database file.\n");
        // considering to input the value of header_page and free_page
        // make the file size to 10MiB => using ftruncate method
        // ftruncate(created_fd, INITIAL_DB_FILE_SIZE); I think it is not necessary
        // allocate header_page first
        int created_fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0666);
        if (created_fd == -1) {
            printf("database file is not created.\n");
            return -1;
        }
        fd_lists.push_back(created_fd);
        Header_page hp;
        hp.free_page_number = INITIAL_DB_FILE_SIZE - PAGE_SIZE;
        hp.number_of_pages = (INITIAL_DB_FILE_SIZE / PAGE_SIZE);
        hp.root_page_number = 0;
        pwrite(created_fd, &hp, PAGE_SIZE, 0);  // allocate header_page
        fsync(created_fd);
        
        // input the value of free_page
        for (pagenum_t pagenum = (INITIAL_DB_FILE_SIZE / PAGE_SIZE) - 1; pagenum > 0; pagenum--) {
            Free_page fp;
            if (pagenum == 1) {
                fp.next_free_page_number = 0;
                pwrite(created_fd, &fp, PAGE_SIZE, PAGE_SIZE * pagenum);
                fsync(created_fd);
                continue;
            }
            // need to check this value
            fp.next_free_page_number = (pagenum - 1) * PAGE_SIZE;
            pwrite(created_fd, &fp, PAGE_SIZE, PAGE_SIZE * pagenum);    // allocate free_page
            fsync(created_fd);
        }
        // then returns the fd of the opened database file.
        return created_fd;
    }
    // from this code this fd is not created file, but it is opened file
    bool fd_check = false;
    for (int i = 0; i < fd_lists.size(); i++) {
        if (fd_lists[i] == fd) {
            fd_check = true;
        }
    }
    if (!fd_check) fd_lists.push_back(fd);
    pagenum_t magic_number;
    // read magic_number from header_page and compare with the expected value which is 2022
    pread(fd, &magic_number, 8, 0);
    if (magic_number != 2022) {
        // if the magic_number is not equal to expected value
        // then return negative value
        printf("this file is not equal to expected magic number.\n");
        return -1;
    }
    // else return fd
    return fd;
}

// Allocate an on-disk page from the free page list
/**
 *  Allocate a page
 *  It returns a new page #(id) from the free page list
 *  If the free page list is empty, then it should grow the database file and return a free page #.
 */
pagenum_t file_alloc_page(int64_t fd) {
    pagenum_t ret = -1;
    Header_page hp;
    pread(fd, &hp, PAGE_SIZE, 0);
    // if the free page list is empty => increase file sizes 2 times for each process
    if (hp.free_page_number == 0) {
        // if there are no other free_page, then the created free_page.next_free_page_number is 0
        hp.free_page_number = (hp.number_of_pages * 2 - 2) * PAGE_SIZE;
        for (pagenum_t pagenum = hp.number_of_pages * 2 - 1; pagenum >= hp.number_of_pages; pagenum--) {
            Free_page fp;
            if (pagenum == hp.number_of_pages) {
                fp.next_free_page_number = 0;
                pwrite(fd, &fp, PAGE_SIZE, PAGE_SIZE * pagenum);
                fsync(fd);
                continue;
            }
            fp.next_free_page_number = (pagenum - 1) * PAGE_SIZE;
            pwrite(fd, &fp, PAGE_SIZE, PAGE_SIZE * pagenum);
            fsync(fd);
        }
        // increase the total number of pages
        ret = (hp.number_of_pages * 2 - 1) * PAGE_SIZE;
        hp.number_of_pages *= 2;
        pwrite(fd, &hp, PAGE_SIZE, 0);
        fsync(fd);
    }
    else {
        ret = hp.free_page_number;
        Free_page fp;
        pread(fd, &fp, PAGE_SIZE, hp.free_page_number);
        hp.free_page_number = fp.next_free_page_number;
        pwrite(fd, &hp, PAGE_SIZE, 0);
        fsync(fd);
    }
    return ret; 
}

// Free an on-disk page to the free page list
/**
 *  Free a page
 *  It informs the disk space manager of returning the page with 'page_number' for freeing it to the free page list.
 */
void file_free_page(int64_t fd, pagenum_t pagenum) {
    Free_page fp;
    pread(fd, &fp, PAGE_SIZE, pagenum);
    Header_page hp;
    pread(fd, &hp, PAGE_SIZE, 0);
    fp.next_free_page_number = hp.free_page_number;
    hp.free_page_number = pagenum;
    pwrite(fd, &hp, PAGE_SIZE, 0);
    pwrite(fd, &fp, PAGE_SIZE, pagenum);
    fsync(fd);
}

// Read an on-disk page into the in-memory page structure(dest)
/**
 *  Read a page
 *  It fetches the disk page corresponding to 'page_number' to the in-memory buffer (i.e., 'dest').
 */
void file_read_page(int64_t fd, pagenum_t pagenum, struct page_t* dest) {
    pread(fd, dest, PAGE_SIZE, pagenum);
}

// Write an in-memory page(src) to the on-disk page
/**
 *  Write a page
 *  It writes the in-memory page content in the buffer (i.e., 'src') to the disk page pointed by 'page_number'.
 */
void file_write_page(int64_t fd, pagenum_t pagenum, const struct page_t* src) {
    int error_code = pwrite(fd, src, PAGE_SIZE, pagenum);
    if (error_code == -1) {
        printf("page is not written well\n");
        return;
    }
    fsync(fd);
}

// Close the database file
/**
 *  Close the database file
 *  This API doesn't receive a file descriptor as a parameter.
 *  So a means for referencing the descriptor of the opened file(i.e., global variable) is required
 */
void file_close_table_files() {
    int size = fd_lists.size();
    for (int i = 0; i < size; i++) {
        int check_closed = close(fd_lists[i]);
        if (check_closed == -1) {
            printf("file is not closed well\n");
        }
    }
    fd_lists.clear();
}
