#include "bpt.h"
#include "trx.h"
#define SLOT_PAGE_SIZE 16
int64_t open_table(const char * pathname) {
    int64_t unique_id = file_open_table_file(pathname);
    if (unique_id == -1) {
        printf("not created\n");
        return -1;
    }
    Header_page hp;
    buffer_read_page(unique_id, 0, (page_t *)&hp);
    root_pagenum_list[unique_id] = hp.root_page_number;
    return unique_id;
}
/**
 *  Insert a record to the given table.
 *  Insert a key/value with the given size to the data file
 *  If successful, return 0. Ohterwise, return a non-zero value
 */
int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size) {
    char* ret_val_from_find = (char*)malloc(sizeof(char) * 112);
    uint16_t *tmp_size_from_find = (uint16_t *) malloc(sizeof(uint16_t));
    if (db_find(table_id, key, ret_val_from_find, tmp_size_from_find) == 0) {
        free(ret_val_from_find);
        free(tmp_size_from_find);
        printf("Duplicated key\n");
        return -1;
    }
    Header_page *hp = (Header_page *) malloc(sizeof(Header_page));
    Leaf_page *lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    buffer_read_page(table_id, 0, (page_t *) hp);
    pagenum_t leaf_page_num;
    /**
     * Case: the tree does not exist yet.
     * start a new tree.
    */
    if (hp->root_page_number == 0) {
        pagenum_t root_pagenum = make_leaf_page(table_id);
        buffer_read_page(table_id, 0, (page_t *) hp);
        hp->root_page_number = root_pagenum;
        root_pagenum_list[table_id] = root_pagenum;
        buffer_read_page(table_id, root_pagenum, (page_t *)lp);
        lp->number_of_keys++;
        lp->amount_of_free_space -= (val_size + SLOT_PAGE_SIZE);
        Slot_page sp;
        sp.key = key;
        sp.size = val_size;
        sp.offset = PAGE_SIZE - val_size;
        memcpy(lp->page_body, &sp, SLOT_PAGE_SIZE);
        memcpy(lp->page_body + sp.offset - 128, value, val_size);
        buffer_write_page(table_id, 0, (page_t *)hp);
        buffer_write_page(table_id, root_pagenum, (page_t *)lp);
        free(hp);
        free(lp);
        free(ret_val_from_find);
        free(tmp_size_from_find);
        return 0;
    }
    /**
    * Case: the tree already exists
    * (Rest of function body.)
    */
    leaf_page_num = find_leaf_page(table_id, key);
    buffer_read_page(table_id, leaf_page_num, (page_t *)lp);
    // buffer_pin_off(table_id, 0);
    // buffer_pin_off(table_id, leaf_page_num);
    /**
    * Case: leaf has room for key and pointer
    * order를 대체할 수 있는 걸 만들어야겠네 => amount of free space
    */
    if ((pagenum_t)val_size + (pagenum_t)SLOT_PAGE_SIZE < lp->amount_of_free_space) {
        free(ret_val_from_find);
        free(tmp_size_from_find);
        free(hp);
        free(lp);
        return db_insert_into_leaf_page(table_id, leaf_page_num, key, value, val_size);
    }
    /**
    * Case: leaf must be split
    */
    free(ret_val_from_find);
    free(tmp_size_from_find);
    free(hp);
    free(lp);
    return db_insert_into_leaf_page_after_splitting(table_id, root_pagenum_list[table_id], leaf_page_num, key, value, val_size);
}
/**
 * split 기준 세우기
 * @param table_id
 * @param root_page_num
 * @param leaf_page_num
 * @param key
 * @param value
 * @param val_size
 * @return
 */
int db_insert_into_leaf_page_after_splitting(int64_t table_id, pagenum_t root_page_num, pagenum_t leaf_page_num, int64_t key, const char* value, uint16_t val_size) {
    int64_t new_key;
    Leaf_page *lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    buffer_read_page(table_id, leaf_page_num, (page_t *)lp);
    Leaf_page *new_lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    pagenum_t new_leaf_page_num = make_leaf_page(table_id);
    buffer_read_page(table_id, new_leaf_page_num, (page_t *)new_lp);
    Slot_page *sp_list = (Slot_page *) malloc(sizeof(Slot_page) * (lp->number_of_keys + 1));
    Slot_page *new_sp_list;
    char **value_list = (char **) malloc(sizeof(char *) * (lp->number_of_keys + 1));
    char **new_value_list;
    uint32_t current_number_of_keys = lp->number_of_keys + 1;
    /**
     * copy lp's slot page to sp_list
     */
    for (int i = 0; i < lp->number_of_keys; i++) {
        memcpy(&sp_list[i], lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
    }
    // find insertion_index and record key, value and offset
    uint32_t insertion_index = lp->number_of_keys;
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        if (key < sp_list[i].key) {
            insertion_index = i;
            break;
        }
    }
    for (uint32_t i = lp->number_of_keys; i > insertion_index; i--) {
        sp_list[i] = sp_list[i - 1];
    }
    sp_list[insertion_index].key = key;
    sp_list[insertion_index].size = val_size;
    value_list[insertion_index] = (char *)malloc(sizeof(char) * val_size);
    memcpy(value_list[insertion_index], value, val_size);
    /**
     * insert attributes except insertion_index
     */
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        if (i == insertion_index) continue;
        uint16_t tmp_val_size = sp_list[i].size;
        char *tmp_value = (char *)malloc(sizeof(char) * tmp_val_size);
        value_list[i] = (char *)malloc(sizeof(char) * tmp_val_size);
        memcpy(tmp_value, lp->page_body + sp_list[i].offset - 128, sp_list[i].size);
        memcpy(value_list[i], tmp_value, tmp_val_size);
        free(tmp_value);
    }
    lp->number_of_keys++;
    /**
     * 여기까지가 일단 전부 다 끼워넣기
     */
    /**
     * find split index
     */
    int split_index = 0;
    uint16_t current_offset = 0;
    for (int i = 0; i < current_number_of_keys; i++) {
        if (current_offset + sp_list[i].size > 1984) {
            split_index = i;
            break;
        }
        current_offset += sp_list[i].size;
    }
    /**
     * move sp_list to new_sp_list
     */
    int new_lp_size = lp->number_of_keys - split_index;
    new_sp_list = (Slot_page *) malloc(sizeof(Slot_page) * new_lp_size);
    new_value_list = (char **) malloc(sizeof(char *) * new_lp_size);
    for (int i = split_index, j = 0; i < current_number_of_keys; i++, j++) {
        new_value_list[j] = (char *) malloc(sizeof(char) * sp_list[i].size);
        memcpy(new_value_list[j], value_list[i], sp_list[i].size);
        new_sp_list[j] = sp_list[i];
        new_lp->number_of_keys++;
        lp->number_of_keys--;
    }
    /**
     * update sp_list and new_sp_list's offset
     */
    uint16_t current_size = PAGE_SIZE;
    lp->amount_of_free_space = PAGE_SIZE - 128;
    for (int i = 0; i < lp->number_of_keys; i++) {
        sp_list[i].offset = current_size - sp_list[i].size;
        lp->amount_of_free_space -= (SLOT_PAGE_SIZE + sp_list[i].size);
        current_size -= sp_list[i].size;
    }
    current_size = PAGE_SIZE;
    new_lp->amount_of_free_space = PAGE_SIZE - 128;
    for (int i = 0; i < new_lp->number_of_keys; i++) {
        new_sp_list[i].offset = current_size - new_sp_list[i].size;
        new_lp->amount_of_free_space -= (SLOT_PAGE_SIZE + new_sp_list[i].size);
        current_size -= new_sp_list[i].size;
    }
    /**
     * update leaf page and new leaf page
     */
    for (int i = 0; i < lp->number_of_keys; i++) {
        memcpy(lp->page_body + SLOT_PAGE_SIZE*i, &sp_list[i], SLOT_PAGE_SIZE);
        memcpy(lp->page_body + sp_list[i].offset - 128, value_list[i], sp_list[i].size);
    }
    for (int i = 0; i < new_lp->number_of_keys; i++) {
        memcpy(new_lp->page_body + SLOT_PAGE_SIZE*i, &new_sp_list[i], SLOT_PAGE_SIZE);
        memcpy(new_lp->page_body + new_sp_list[i].offset - 128, new_value_list[i], new_sp_list[i].size);
    }
    new_lp->parent_page_number = lp->parent_page_number;
    new_lp->right_sibling_page_number = lp->right_sibling_page_number;
    lp->right_sibling_page_number = new_leaf_page_num;
    new_key = new_sp_list[0].key;
    buffer_write_page(table_id, leaf_page_num, (page_t *)lp);
    buffer_write_page(table_id, new_leaf_page_num, (page_t *)new_lp);
    /**
     * free memeory
     */
    free(lp);
    free(new_lp);
    free(sp_list);
    free(new_sp_list);
    for (int i = 0; i < current_number_of_keys; i++) {
        free(value_list[i]);
    }
    for (int i = 0; i < new_lp_size; i++) {
        free(new_value_list[i]);
    }
    free(value_list);
    free(new_value_list);
    return db_insert_into_parent_page(table_id, leaf_page_num, new_leaf_page_num, new_key);
}
/**
 * insert data to leaf_page
 */
int db_insert_into_leaf_page(int64_t table_id, pagenum_t leaf_page_num, int64_t key, const char* value, uint16_t val_size) {
    /**
     * value 를 value_list 에 담을 수가 없어
     * slot_page 다 조정하고
     * 걔네가 가리키는 값들을 다 받아와서 차례대로 저장하면 되겠네
     */
    /**
    *   read leaf_page
    *   update leaf_page attributes
    */
    Leaf_page* lp = (Leaf_page *)malloc(sizeof(Leaf_page));
    buffer_read_page(table_id, leaf_page_num, (page_t *)lp);
    uint32_t current_number_of_keys = lp->number_of_keys;
    lp->number_of_keys++;
    lp->amount_of_free_space -= (val_size + SLOT_PAGE_SIZE);
    Slot_page* sp_list = (Slot_page*)malloc(sizeof(Slot_page) * (current_number_of_keys + 1));
    char **value_list = (char **)malloc(sizeof(char*) * (current_number_of_keys + 1));
    // record slot_page to sp_list
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        Slot_page sp;
        // sp offset == 128 + 12 * i
        memcpy(&sp, lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        sp_list[i] = sp;
    }
    // find insertion_index and record key, value and offset
    uint32_t insertion_index = current_number_of_keys;
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        if (key < sp_list[i].key) {
            insertion_index = i;
            break;
        }
    }
    for (uint32_t i = current_number_of_keys; i > insertion_index; i--) {
        sp_list[i] = sp_list[i - 1];
    }
    /**
     * insert attributes to insertion_index
     */
    sp_list[insertion_index].key = key;
    sp_list[insertion_index].size = val_size;
    value_list[insertion_index] = (char *)malloc(sizeof(char) * val_size);
    memcpy(value_list[insertion_index], value, val_size);
    /**
     * insert attributes except insertion_index
     */
    for (uint32_t i = 0; i < current_number_of_keys + 1; i++) {
        if (i == insertion_index) continue;
        uint16_t tmp_val_size = sp_list[i].size;
        char *tmp_value = (char *)malloc(sizeof(char) * tmp_val_size);
        value_list[i] = (char *)malloc(sizeof(char) * tmp_val_size);
        memcpy(tmp_value, lp->page_body + sp_list[i].offset - 128, sp_list[i].size);
        memcpy(value_list[i], tmp_value, tmp_val_size);
        free(tmp_value);
    }
    /**
     * update slot_page's offset
     */
    uint16_t current_offset = PAGE_SIZE;
    for (uint32_t i = 0; i < current_number_of_keys + 1; i++) {
        sp_list[i].offset = current_offset - sp_list[i].size;
        current_offset -= sp_list[i].size;
    }
    /**
     *  record slot_page to leaf_page
     */
    for (uint32_t i = 0; i < lp->number_of_keys; i++) {
        memcpy(lp->page_body + SLOT_PAGE_SIZE*i, &sp_list[i], SLOT_PAGE_SIZE);
    }
    /**
     * record value to leaf_page
     */
    for (uint32_t i = 0; i < lp->number_of_keys; i++) {
        memcpy(lp->page_body + sp_list[i].offset - 128, value_list[i], sp_list[i].size);
    }
    /**
     *  write leaf_page to disk
     */
    buffer_write_page(table_id, leaf_page_num, (page_t *)lp);
    /**
     * free all the allocated memory
    */
    free(lp);
    free(sp_list);
    for (uint32_t i = 0; i < current_number_of_keys + 1; i++) {
        free(value_list[i]);
    }
    free(value_list);
    return 0;
}
int db_insert_into_parent_page(int64_t table_id, pagenum_t left_page_num, pagenum_t right_page_num, int64_t key) {
    int left_index;
    pagenum_t parent_page_num;
    Internal_page* parent = (Internal_page *)malloc(sizeof(Internal_page));
    Internal_page* left_page = (Internal_page *)malloc(sizeof(Internal_page));
    Internal_page* right_page = (Internal_page *)malloc(sizeof(Internal_page));
    buffer_read_page(table_id, left_page_num, (page_t *)left_page);
    buffer_read_page(table_id, right_page_num, (page_t *)right_page);
    /**
     * Case: new root
     */
    if (left_page->parent_page_number == 0) {
        Header_page *hp = (Header_page *) malloc(sizeof(Header_page));
        printf("\ncreate new root\n\n");
        parent_page_num = make_internal_page(table_id);
        buffer_read_page(table_id, 0, (page_t *) hp);
        hp->root_page_number = parent_page_num;
        root_pagenum_list[table_id] = parent_page_num;
        buffer_read_page(table_id, parent_page_num, (page_t *) parent);
        parent->left_most_page_number = left_page_num;
        parent->entry[0].key = key;
        parent->entry[0].page_number = right_page_num;
        parent->number_of_keys = 1;
        parent->parent_page_number = 0;
        parent->is_leaf = 0;
        left_page->parent_page_number = parent_page_num;
        right_page->parent_page_number = parent_page_num;
        buffer_write_page(table_id, parent_page_num, (page_t *)parent);
        buffer_write_page(table_id, left_page_num, (page_t *)left_page);
        buffer_write_page(table_id, right_page_num, (page_t *)right_page);
        buffer_write_page(table_id, 0, (page_t *) hp);
        free(parent);
        free(left_page);
        free(right_page);
        free(hp);
        return 0;
    }
    /**
     * Case: leaf or node. (Remainder of function body).
     */
    parent_page_num = left_page->parent_page_number;
    buffer_read_page(table_id, parent_page_num, (page_t *) parent);
    // buffer_pin_off(table_id, parent_page_num);
    // buffer_pin_off(table_id, left_page_num);
    // buffer_pin_off(table_id, right_page_num);
    left_index = get_left_index(table_id, parent_page_num, left_page_num);
    /**
     * Simple case: the new key fits into the node.
     */
    if (parent->number_of_keys < 248) {
        free(parent);
        free(left_page);
        free(right_page);
        return db_insert_into_internal_page(table_id, parent_page_num, left_index, key, right_page_num);
    }
    /**
     * Harder case: split a node in order
     * to preserve the B+ tree properties.
     */
    free(parent);
    free(left_page);
    free(right_page);
    return db_insert_into_internal_page_after_splitting(table_id, parent_page_num, left_index, key, right_page_num);
}
int db_insert_into_internal_page(int64_t table_id, pagenum_t parent_page_num, int left_index, int64_t key, pagenum_t right_page_num) {
    int i;
    Internal_page *parent_page = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, parent_page_num, (page_t *) parent_page);
    for (i = parent_page->number_of_keys; i > left_index + 1; i--) {
        parent_page->entry[i] = parent_page->entry[i - 1];
    }
    parent_page->entry[left_index + 1].key = key;
    parent_page->entry[left_index + 1].page_number = right_page_num;
    parent_page->number_of_keys++;
    buffer_write_page(table_id, parent_page_num, (page_t *) parent_page);
    free(parent_page);
    return 0;
}
/**
 * @param table_id
 * @param parent_page_num
 * @param left_index
 * @param key
 * @param right_page_num
 * @return
 */
int db_insert_into_internal_page_after_splitting(int64_t table_id, pagenum_t parent_page_num, int left_index, int64_t key, pagenum_t right_page_num) {
    int i, j, split;
    int64_t k_prime;
    Internal_page *old_page = (Internal_page *)malloc(sizeof(Internal_page));
    Internal_page *new_page = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, parent_page_num, (page_t *) old_page);
    Entry *temp = (Entry *) malloc(sizeof(Entry) * (old_page->number_of_keys + 1));
    for (i = 0, j = 0; i < old_page->number_of_keys; i++, j++) {
        if (j == left_index + 1) j++;
        temp[j] = old_page->entry[i];
    }
//    printf("left_index: %d\n", left_index);
    temp[left_index + 1].page_number = right_page_num;
    temp[left_index + 1].key = key;
    // 얘는 그냥 entry 개수 반갈죽 해서 받아와도 될거같은데? => 됨
    // split == 124
    split = old_page->number_of_keys / 2;
    pagenum_t new_internal_page_num = make_internal_page(table_id);
    buffer_read_page(table_id, new_internal_page_num, (page_t *) new_page);
    /**
     * update old_page attributes
     */
    old_page->number_of_keys = 0;
    for (i = 0; i < split; i++) {
        old_page->entry[i] = temp[i];
        old_page->number_of_keys++;
    }
    /**
     * update new_page attributes
     */
    new_page->left_most_page_number = temp[split].page_number;
    Internal_page *left_most_page = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, new_page->left_most_page_number, (page_t *) left_most_page);
    left_most_page->parent_page_number = new_internal_page_num;
    buffer_write_page(table_id, new_page->left_most_page_number, (page_t *) left_most_page);
    k_prime = temp[split].key;
    printf("k_prime: %ld\n", k_prime);
    for (++i, j = 0; i < 249; i++, j++) {
        Internal_page *cp = (Internal_page *) malloc(sizeof(Internal_page));
        new_page->entry[j] = temp[i];
        new_page->number_of_keys++;
        buffer_read_page(table_id, new_page->entry[j].page_number, (page_t *)cp);
        cp->parent_page_number = new_internal_page_num;
        buffer_write_page(table_id, new_page->entry[j].page_number, (page_t *) cp);
        free(cp);
    }
//    if (left_index + 1 < split) {
//
//    }
    new_page->parent_page_number = old_page->parent_page_number;
    buffer_write_page(table_id, parent_page_num, (page_t *) old_page);
    buffer_write_page(table_id, new_internal_page_num, (page_t *) new_page);
    /**
     * free memory
     */
    free(old_page);
    free(new_page);
    free(temp);
    free(left_most_page);
    return db_insert_into_parent_page(table_id, parent_page_num, new_internal_page_num, k_prime);
}
int get_left_index(int64_t table_id, pagenum_t parent_page_num, pagenum_t left_page_num) {
    int left_index = 0;
    Internal_page *parent = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, parent_page_num, (page_t *) parent);
    // buffer_pin_off(table_id, parent_page_num);
    if (parent->left_most_page_number == left_page_num) {
        free(parent);
        left_index = -1;
        return left_index;
    }
    while (left_index < parent->number_of_keys && parent->entry[left_index].page_number != left_page_num) {
        left_index++;
    }
    free(parent);
    return left_index;
}
/* trx_id 없음 */
int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size) {
    // read leaf_page info
    Leaf_page* lp = (Leaf_page *)malloc(sizeof(Leaf_page));
    pagenum_t leaf_pagenum = find_leaf_page(table_id, key);
    buffer_read_page(table_id, leaf_pagenum, (page_t *)lp);
    // buffer_pin_off(table_id, leaf_pagenum);
    // store the current_number_of_keys
    uint32_t current_number_of_keys = lp->number_of_keys;
    Slot_page sp;
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        // read slot page
        memcpy(&sp, lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        // if the key exists in Leaf_page
        if (key == sp.key) {
            // store size and val to ret_val and val_size
//            printf("key %ld find succeed!\n", key);
            memcpy(val_size, &sp.size, 2);
            memcpy(ret_val, lp->page_body + sp.offset - 128, sp.size);
            free(lp);
            return 0;
        }
    }
    // return non-zero value
//    printf("key is not found\n");
    free(lp);
    return -1;
}
int db_delete(int64_t table_id, int64_t key) {
    // 값이 존재하는지 확인
    char* ret_val = (char *)malloc(sizeof(char) * 112);
    uint16_t* ret_val_size = (uint16_t *)malloc(sizeof(uint16_t));
    // non-existing key
    if (db_find(table_id, key, ret_val, ret_val_size) != 0) {
        printf("non-existing key\n");
        free(ret_val);
        free(ret_val_size);
        return -1;
    }
    Header_page *hp = (Header_page *) malloc(sizeof(Header_page));
    pagenum_t leaf_page_num = find_leaf_page(table_id, key);
    pagenum_t root_page_num = db_delete_entry(table_id, leaf_page_num, key);
    /* empty tree! */
    if (root_page_num == 0) {
        free(ret_val);
        free(ret_val_size);
        free(hp);
        return 0;
    }
    buffer_read_page(table_id, 0, (page_t *) hp);
    hp->root_page_number = root_page_num;
    root_pagenum_list[table_id] = root_page_num;
    buffer_write_page(table_id, 0, (page_t *) hp);
    free(ret_val);
    free(ret_val_size);
    free(hp);
    return 0;
}
/**
 *  Delete a record with the matching key from the given table.
 *  update
 *  1. leaf_page's amount of free space
 *  2. leaf_page's number of keys
 */
pagenum_t db_delete_entry(int64_t table_id, pagenum_t page_num, int64_t key) {
    pagenum_t threshold, neighbor_page_num, capacity;
    int neighbor_index, k_prime_index;
    int64_t k_prime;
//    pagenum_t leaf_page_num = find_leaf_page(table_id, key);
    page_t *page = (page_t *) malloc(sizeof(page_t));
    Leaf_page *neighbor = (Leaf_page *) malloc(sizeof(Leaf_page));
    Internal_page *parent = (Internal_page *) malloc(sizeof(Internal_page));
    /**
     * 여기서 리프 지워
     * Remove key and pointer from node.
     * key 지우고 다시 받기 때문에 삭제됐다면 lp 읽어야 해
     */
    k_prime_index = remove_entry_from_leaf_page(table_id, page_num, key);
    buffer_read_page(table_id, page_num, (page_t *) page);
    // buffer_pin_off(table_id, page_num);
    /**
     * Case:  deletion from the root.
     */
    if (((Internal_page *)page)->parent_page_number == 0) {
        if (((Internal_page *)page)->number_of_keys == 0) {
            Header_page *hp = (Header_page *) malloc(sizeof(Header_page));
            buffer_read_page(table_id, 0, (page_t *) hp);
            hp->root_page_number = 0;
            root_pagenum_list[table_id] = 0;
            buffer_write_page(table_id, 0, (page_t *) hp);
            buffer_free_page(table_id, page_num);
            free(hp);
            free(page);
            free(neighbor);
            free(parent);
            return 0;
        }
        free(page);
        free(neighbor);
        free(parent);
        return db_adjust_root(table_id);
    }
    /**
     * 여기서 부턴 root 가 아닌 애들
     * Case:  deletion from a node below the root.
     */
    /**
     * 일단 deletion 이 진행되고 나서 최소한의 크기를 설정
     * Determine minimum allowable size of node,
     * to be preserved after deletion
     */
    threshold = 2500;
    /**
     * Case:  node stays at or above minimum.
     * 노드의 key 개수가 최소 개수보다 크면 그냥 끝내면 돼
     * 얘는 지우고 나서 정상적이면 return 0 하면 되겠다
     */
    /* leaf page */
    if (((Leaf_page *)page)->amount_of_free_space < threshold) {
        free(page);
        free(neighbor);
        free(parent);
        return root_pagenum_list[table_id];
    }
    /**
     * 노드의 key 개수가 최소 개수보다 작아지면 이때는 merge(coalesce) 하거나 새로 분배해야돼
     * minimum => threshold 설정
     * Case:  node falls below minimum
     * Either coalescence or redistribution
     * is needed
     */
    /**
     * 일단 합칠 노드를 찾고 neighbor 와 node 를 연결하는 부모 key(k_prime) 를 찾아
     * Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */
    /**
     * leaf_page_num => page_num
     */
    neighbor_index = get_neighbor_index(table_id, page_num);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    buffer_read_page(table_id, ((Internal_page *)page)->parent_page_number, (page_t *) parent);
    // buffer_pin_off(table_id, ((Internal_page *)page)->parent_page_number);
    k_prime = parent->entry[k_prime_index].key;
    if (neighbor_index == -1) {
        neighbor_page_num = parent->entry[0].page_number;
    }
    else if (neighbor_index == 0) {
        neighbor_page_num = parent->left_most_page_number;
    }
    else {
        neighbor_page_num = parent->entry[neighbor_index - 1].page_number;
    }
    buffer_read_page(table_id, neighbor_page_num, (page_t *) neighbor);
    // buffer_pin_off(table_id, neighbor_page_num);
    capacity = PAGE_SIZE;
    /* leaf_page's capacity */
    /* Merge */
    if (capacity < neighbor->amount_of_free_space + ((Leaf_page *)page)->amount_of_free_space + 128) {
        free(page);
        free(neighbor);
        free(parent);
        printf("merge leaf 호출 key: %ld\n", key);
        return merge_leaf_pages(table_id, page_num, neighbor_page_num, neighbor_index, k_prime);
    }
        /* Redistribution */
    else {
        free(page);
        free(neighbor);
        free(parent);
        printf("redistribute leaf 호출 key: %ld\n", key);
        return redistribute_leaf_pages(table_id, page_num, neighbor_page_num, neighbor_index, k_prime_index, k_prime);
    }
}
/**
 * 무조건 internal_page 확정
 */
pagenum_t db_delete_parent_entry(int64_t table_id, pagenum_t page_num, int k_prime_index) {
    pagenum_t threshold, neighbor_page_num, capacity;
    int neighbor_index, i;
    int64_t k_prime;
    Internal_page *page = (Internal_page *) malloc(sizeof(Internal_page));
    Leaf_page *neighbor = (Leaf_page *) malloc(sizeof(Leaf_page));
    Internal_page *parent = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, page_num, (page_t *) page);
    /* 여기서 키 지워줘야 할듯 */
    /**
     * Case:  deletion from the root.
     */
    if (page->parent_page_number == 0) {
        buffer_read_page(table_id, root_pagenum_list[table_id], (page_t *) parent);
        parent->number_of_keys--;
        for (i = k_prime_index; i < parent->number_of_keys; i++) {
            parent->entry[i] = parent->entry[i + 1];
        }
        buffer_write_page(table_id, root_pagenum_list[table_id], (page_t *) parent);
        free(page);
        free(neighbor);
        free(parent);
        return db_adjust_root(table_id);
    }
    /**
     * Case: remove k_prime
     */
    page->number_of_keys--;
    for (i = k_prime_index; i < page->number_of_keys; i++) {
        page->entry[i] = page->entry[i + 1];
    }
    buffer_write_page(table_id, page_num, (page_t *) page);
    /**
     * 여기서 부턴 root 가 아닌 애들
     * Case:  deletion from a node below the root.
     */
    /**
     * 일단 deletion 이 진행되고 나서 최소한의 크기를 설정
     * Determine minimum allowable size of node,
     * to be preserved after deletion
     */
    threshold = 124;
    /**
     * Case:  node stays at or above minimum.
     * 노드의 key 개수가 최소 개수보다 크면 그냥 끝내면 돼
     * 얘는 지우고 나서 정상적이면 return 0 하면 되겠다
     */
    /* Internal page */
    if (page->number_of_keys >= threshold) {
        free(page);
        free(neighbor);
        free(parent);
        return root_pagenum_list[table_id];
    }
    /**
     * 노드의 key 개수가 최소 개수보다 작아지면 이때는 merge(coalesce) 하거나 새로 분배해야돼
     * minimum => threshold 설정
     * Case:  node falls below minimum
     * Either coalescence or redistribution
     * is needed
     */
    /**
     * 일단 합칠 노드를 찾고 neighbor 와 node 를 연결하는 부모 key(k_prime) 를 찾아
     * Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */
    neighbor_index = get_neighbor_index(table_id, page_num);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    buffer_read_page(table_id, page->parent_page_number, (page_t *) parent);
    // buffer_pin_off(table_id, page->parent_page_number);
    k_prime = parent->entry[k_prime_index].key;
    if (neighbor_index == -1) {
        neighbor_page_num = parent->entry[0].page_number;
    }
    else if (neighbor_index == 0) {
        neighbor_page_num = parent->left_most_page_number;
    }
    else {
        neighbor_page_num = parent->entry[neighbor_index - 1].page_number;
    }
    buffer_read_page(table_id, neighbor_page_num, (page_t *) neighbor);
    // buffer_pin_off(table_id, neighbor_page_num);
    capacity = 248;
    /**
     * Merge
     */
    if (((Internal_page *)page)->number_of_keys + neighbor->number_of_keys < capacity) {
        free(page);
        free(neighbor);
        free(parent);
        printf("merge internal 호출\n");
        return merge_internal_pages(table_id, page_num, neighbor_page_num, neighbor_index, k_prime);
    }
        /**
         * Redistribution
         */
    else {
        free(page);
        free(neighbor);
        free(parent);
        printf("redistribute internal 호출\n");
        return redistribute_internal_pages(table_id, page_num, neighbor_page_num, neighbor_index, k_prime_index, k_prime);
    }
}
int remove_entry_from_leaf_page(int64_t table_id, pagenum_t leaf_page_num, int64_t key) {
    // existing key
    Leaf_page *lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    buffer_read_page(table_id, leaf_page_num, (page_t *) lp);
    Slot_page *sp_list = (Slot_page *) malloc(sizeof(Slot_page) * lp->number_of_keys);
    char **value_list = (char **) malloc(sizeof(char *) * lp->number_of_keys);
    /**
     * get slot_page
     */
    for (int i = 0; i < lp->number_of_keys; i++) {
        memcpy(&sp_list[i], lp->page_body + SLOT_PAGE_SIZE * i, SLOT_PAGE_SIZE);
    }
    uint32_t current_number_of_keys = lp->number_of_keys;
    int deletion_index = 0;
    for (int i = 0; i < lp->number_of_keys; i++) {
        if (sp_list[i].key == key) {
//            printf("delete func: found key!\n");
            deletion_index = i;
            break;
        }
    }
    lp->number_of_keys--;
    lp->amount_of_free_space += (sp_list[deletion_index].size + SLOT_PAGE_SIZE);
    for (int i = deletion_index; i < current_number_of_keys - 1; i++) {
        sp_list[i] = sp_list[i + 1];
    }
    /**
     * update value_list
     */
    for (int i = 0; i < current_number_of_keys - 1; i++) {
        value_list[i] = (char *) malloc(sizeof(char) * sp_list[i].size);
        memcpy(value_list[i], lp->page_body + sp_list[i].offset - 128, sp_list[i].size);
    }
    /**
     * update offset
    */
    uint16_t current_offset = PAGE_SIZE;
    for (int i = 0; i < lp->number_of_keys; i++) {
        sp_list[i].offset = current_offset - sp_list[i].size;
        current_offset -= sp_list[i].size;
    }
    /**
     * update leaf_page
     */
    for (int i = 0; i < current_number_of_keys - 1; i++) {
        memcpy(lp->page_body + sp_list[i].offset - 128, value_list[i], sp_list[i].size);
    }
    for (int i = 0; i < lp->number_of_keys; i++) {
        memcpy(lp->page_body + SLOT_PAGE_SIZE * i, &sp_list[i], SLOT_PAGE_SIZE);
    }
    /**
     * write leaf_page to disk
     */
    buffer_write_page(table_id, leaf_page_num, (page_t *) lp);
    /**
     * root 예외 처리
     */
    if (leaf_page_num == root_pagenum_list[table_id]) {
        /**
         * free memory
         */
        free(lp);
        free(sp_list);
        for (int i = 0; i < current_number_of_keys - 1; i++) {
            free(value_list[i]);
        }
        free(value_list);
        return 0;
    }
    int neighbor_index = get_neighbor_index(table_id, leaf_page_num);
    int k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    /**
     * free memory
     */
    free(lp);
    free(sp_list);
    for (int i = 0; i < current_number_of_keys - 1; i++) {
        free(value_list[i]);
    }
    free(value_list);
    return k_prime_index;
}
pagenum_t db_adjust_root(int64_t table_id) {
    Internal_page *new_root = (Internal_page *) malloc(sizeof(Internal_page));
    Header_page *hp = (Header_page *) malloc(sizeof(Header_page));
    pagenum_t new_root_page_num;
    pagenum_t root_page_num = root_pagenum_list[table_id];
    Internal_page *root = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, root_pagenum_list[table_id], (page_t *) root);
    // buffer_pin_off(table_id, root_pagenum_list[table_id]);
    /**
     * 이미 지워졌으니까 잘 지워진게 아니지
     * Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */
    if (root->number_of_keys > 0) {
        free(new_root);
        free(hp);
        free(root);
        return root_page_num;
    }
    /**
     * 지우고 나서 empty root 가 된 경우
     * root 노드가 지워져도 child 가 있을 수 있으니까
     * Case: empty root.
     */
    /* root == Internal page */
    /**
     * child 가 존재한다면 => first child 를 root로
     * header page 의 root_page_number 0 으로 갱신
     * root_pagenum_list[table_id] 값 갱신해주기
     * new_root->parent_page_number = 0 으로 해주기
     */
    if (root->is_leaf == 0) {
        new_root_page_num = root->left_most_page_number;
        buffer_read_page(table_id, new_root_page_num, (page_t *) new_root);
        buffer_read_page(table_id, 0, (page_t *) hp);
        new_root->parent_page_number = 0;
        hp->root_page_number = new_root_page_num;
        root_pagenum_list[table_id] = new_root_page_num;
        buffer_write_page(table_id, 0, (page_t *) hp);
        buffer_write_page(table_id, new_root_page_num, (page_t *) new_root);
    }
        /* root == leaf page */
        /**
         *  leaf node 이면 무조건 child 가 없으니 그냥 empty tree 되는거지
         *  If it is a leaf (has no children),
         *  then the whole tree is empty.
         */
    else {
        buffer_free_page(table_id, root_pagenum_list[table_id]);
        root_pagenum_list[table_id] = 0;
        buffer_read_page(table_id, 0, (page_t *) hp);
        hp->root_page_number = 0;
        new_root_page_num = 0;
        buffer_write_page(table_id, 0, (page_t *) hp);
    }
    free(new_root);
    free(root);
    free(hp);
    return new_root_page_num;
}
int get_neighbor_index(int64_t table_id, pagenum_t page_num) {
    int i;
    /**
     * Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.
     * If n is the leftmost child, this means
     * return -1.
     */
    Leaf_page *lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    Internal_page *parent = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, page_num, (page_t *) lp);
    buffer_read_page(table_id, lp->parent_page_number, (page_t *) parent);
    // buffer_pin_off(table_id, page_num);
    // buffer_pin_off(table_id, lp->parent_page_number);
    if (parent->left_most_page_number == page_num) {
        free(lp);
        free(parent);
        return -1;
    }
    for (i = 0; i < parent->number_of_keys; i++) {
        if (parent->entry[i].page_number == page_num) {
            free(lp);
            free(parent);
            return i;
        }
    }
    printf("Search for nonexistent pointer to node in parent.\n");
    free(lp);
    free(parent);
    return -100;
}
pagenum_t merge_leaf_pages(int64_t table_id, pagenum_t leaf_page_num, pagenum_t neighbor_page_num, int neighbor_index, int64_t k_prime) {
    int i, j, neighbor_insertion_index;
    Leaf_page *neighbor = (Leaf_page *) malloc(sizeof(Leaf_page));
    Leaf_page *n = (Leaf_page *) malloc(sizeof(Leaf_page));
    Slot_page *n_sp_list;
    char **n_value_list;
    Slot_page *neighbor_sp_list;
    char **neighbor_value_list;
    pagenum_t tmp;
    /**
     * 만약 현재 노드가 0 번째 라면 둘이 바꿔 (무조건 neighbor가 왼쪽으로 오게)
     * Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */
    if (neighbor_index == -1) {
        tmp = leaf_page_num;
        leaf_page_num= neighbor_page_num;
        neighbor_page_num = tmp;
    }
    buffer_read_page(table_id, leaf_page_num, (page_t *) n);
    buffer_read_page(table_id, neighbor_page_num, (page_t *) neighbor);
    /**
     * Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */
    neighbor_insertion_index = neighbor->number_of_keys;
    /**
     * In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */
    n_sp_list = (Slot_page *) malloc(sizeof(Slot_page) * n->number_of_keys);
    n_value_list = (char **) malloc(sizeof(char *) * n->number_of_keys);
    neighbor_sp_list = (Slot_page *) malloc(sizeof(Slot_page) * (neighbor->number_of_keys + n->number_of_keys + 2));
    neighbor_value_list = (char **) malloc(sizeof(char *) * (neighbor->number_of_keys + n->number_of_keys + 2));
    /**
     * n 의 value 랑 slot_page 다 받아와
     */
    for (i = 0; i < n->number_of_keys; i++) {
        memcpy(&n_sp_list[i], n->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        n_value_list[i] = (char *) malloc(sizeof(char) * n_sp_list[i].size);
        memcpy(n_value_list[i], n->page_body + n_sp_list[i].offset - 128, n_sp_list[i].size);
    }
    /**
     * neighbor 의 value 랑 slot_page 다 받아와
     */
    for (i = 0; i < neighbor->number_of_keys; i++) {
        memcpy(&neighbor_sp_list[i], neighbor->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        neighbor_value_list[i] = (char *) malloc(sizeof(char) * neighbor_sp_list[i].size);
        memcpy(neighbor_value_list[i], neighbor->page_body + neighbor_sp_list[i].offset - 128, neighbor_sp_list[i].size);
    }
    for (i = neighbor_insertion_index, j = 0; j < n->number_of_keys; i++, j++) {
        neighbor_sp_list[i] = n_sp_list[j];
        neighbor_value_list[i] = (char *) malloc(sizeof(char) * n_sp_list[j].size);
        memcpy(neighbor_value_list[i], n_value_list[j], n_sp_list[j].size);
        neighbor->number_of_keys++;
        neighbor->amount_of_free_space -= (n_sp_list[j].size + SLOT_PAGE_SIZE);
    }
    neighbor->right_sibling_page_number = n->right_sibling_page_number;
    for (i = 0; i < neighbor->number_of_keys; i++) {
        memcpy(neighbor->page_body + SLOT_PAGE_SIZE*i, &neighbor_sp_list[i], SLOT_PAGE_SIZE);
        memcpy(neighbor->page_body + neighbor_sp_list[i].offset - 128, neighbor_value_list[i], neighbor_sp_list[i].size);
    }
    buffer_write_page(table_id, neighbor_page_num, (page_t *) neighbor);
    buffer_free_page(table_id, leaf_page_num);
    pagenum_t parent_page_num = n->parent_page_number;
    free(n);
    free(neighbor);
    free(n_sp_list);
    free(neighbor_sp_list);
    free(n_value_list);
    free(neighbor_value_list);
    int k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    return db_delete_parent_entry(table_id, parent_page_num, k_prime_index);
}
pagenum_t merge_internal_pages(int64_t table_id, pagenum_t page_num, pagenum_t neighbor_page_num, int neighbor_index, int64_t k_prime) {
    int i, j, neighbor_insertion_index, n_end;
    int k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    Internal_page *neighbor = (Internal_page *) malloc(sizeof(Internal_page));
    Internal_page *n = (Internal_page *) malloc(sizeof(Internal_page));
    pagenum_t tmp;
    /**
     * 만약 현재 노드가 0 번째 라면 둘이 바꿔 (무조건 neighbor가 왼쪽으로 오게)
     * Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */
    if (neighbor_index == -1) {
        tmp = page_num;
        page_num= neighbor_page_num;
        neighbor_page_num = tmp;
    }
    buffer_read_page(table_id, page_num, (page_t *) n);
    // buffer_pin_off(table_id, page_num);
    buffer_read_page(table_id, neighbor_page_num, (page_t *) neighbor);
    /**
     * Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */
    neighbor_insertion_index = neighbor->number_of_keys;
    /**
     * Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */
    /**
     * Append k_prime.
     */
    pagenum_t parent_page_num = neighbor->parent_page_number;
    neighbor->entry[neighbor_insertion_index].key = k_prime;
    neighbor->entry[neighbor_insertion_index].page_number = n->left_most_page_number;
    neighbor->number_of_keys++;
    /**
     * n 의 마지막 index 를 저장
     */
    n_end = n->number_of_keys;
    /**
     * n 의 key 랑 pointers 를 neighbor 로 다 가져옴
     */
    for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
        neighbor->entry[i] = n->entry[j];
        neighbor->number_of_keys++;
        n->number_of_keys--;
    }
    buffer_write_page(table_id, neighbor_page_num, (page_t *) neighbor);
    /**
     * All children must now point up to the same parent.
     */
    Internal_page *page = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, n->left_most_page_number, (page_t *) page);
    page->parent_page_number = neighbor_page_num;
    buffer_write_page(table_id, n->left_most_page_number, (page_t *) page);
    for (i = 0; i < neighbor->number_of_keys; i++) {
        buffer_read_page(table_id, neighbor->entry[i].page_number, (page_t *) page);
        page->parent_page_number = neighbor_page_num;
        buffer_write_page(table_id, neighbor->entry[i].page_number, (page_t *) page);
    }
    /**
     * 이거 free 하는게 맞나 의문
     */
    buffer_free_page(table_id, page_num);
    free(neighbor);
    free(n);
    free(page);
    return db_delete_parent_entry(table_id, parent_page_num, k_prime_index);
}
pagenum_t redistribute_leaf_pages(int64_t table_id, pagenum_t leaf_page_num, pagenum_t neighbor_page_num, int neighbor_index, int k_prime_index, int64_t k_prime) {
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    int i;
    pagenum_t parent_page_num;
    Leaf_page *n = (Leaf_page *) malloc(sizeof(Leaf_page));
    Leaf_page *neighbor = (Leaf_page *) malloc(sizeof(Leaf_page));
    Internal_page *parent = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, leaf_page_num, (page_t *) n);
    buffer_read_page(table_id, neighbor_page_num, (page_t *) neighbor);
    parent_page_num = n->parent_page_number;
    buffer_read_page(table_id, parent_page_num, (page_t *) parent);
    Slot_page *n_sp_list = (Slot_page *) malloc(sizeof(Slot_page) * (n->number_of_keys + 1));
    Slot_page *neighbor_sp_list = (Slot_page *) malloc(sizeof(Slot_page) * neighbor->number_of_keys);
    char **n_value_list = (char **) malloc(sizeof(char *) * (n->number_of_keys + 1));
    char **neighbor_value_list = (char **) malloc(sizeof(char *) * neighbor->number_of_keys);
    for (i = 0; i < n->number_of_keys; i++) {
        memcpy(&n_sp_list[i], n->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        n_value_list[i] = (char *) malloc(sizeof(char) * n_sp_list[i].size);
        memcpy(n_value_list[i], n->page_body + n_sp_list[i].offset - 128, n_sp_list[i].size);
    }
    for (i = 0; i < neighbor->number_of_keys; i++) {
        memcpy(&neighbor_sp_list[i], neighbor->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        neighbor_value_list[i] = (char *) malloc(sizeof(char) * neighbor_sp_list[i].size);
        memcpy(neighbor_value_list[i], neighbor->page_body + neighbor_sp_list[i].offset - 128, neighbor_sp_list[i].size);
    }
    /**
     * Case: n has a neighbor to the left.
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     * 일단 왜 neighbor_index 를 -1 과 -1이 아닌걸로 구분했지? => 0 때문에
     * neighbor / n
     * 바꿔말하면 현재 노드가 0 이 아니라는 거네
     * 하나만 당겨오면 되네
     */
    if (neighbor_index != -1) {
        for (i = n->number_of_keys; i > 0; i--) {
            n_sp_list[i] = n_sp_list[i - 1];
            n_value_list[i] = n_value_list[i - 1];
        }
        n_sp_list[0] = neighbor_sp_list[neighbor->number_of_keys - 1];
        n_value_list[0] = (char *) malloc(sizeof(char) * neighbor_sp_list[neighbor->number_of_keys - 1].size);
        memcpy(n_value_list[0], neighbor_value_list[neighbor->number_of_keys - 1], neighbor_sp_list[neighbor->number_of_keys - 1].size);
        parent->entry[k_prime_index].key = n_sp_list[0].key;
        n->amount_of_free_space -= (n_sp_list[0].size + SLOT_PAGE_SIZE);
        neighbor->amount_of_free_space += (n_sp_list[0].size + SLOT_PAGE_SIZE);
    }
        /**
         * 원래는 neighbor가 왼쪽 인데 이 경우는 n 이 왼쪽인 케이스
         * => n / neighbor 이런 구조
         * Case: n is the leftmost child.
         * Take a key-pointer pair from the neighbor to the right.
         * Move the neighbor's leftmost key-pointer pair
         * to n's rightmost position.
         */
    else {
        n_sp_list[n->number_of_keys] = neighbor_sp_list[0];
        n_value_list[n->number_of_keys] = (char *) malloc(sizeof(char) * neighbor_sp_list[0].size);
        memcpy(n_value_list[n->number_of_keys], neighbor_value_list[0], neighbor_sp_list[0].size);
        parent->entry[k_prime_index].key = neighbor_sp_list[1].key;
        for (i = 0; i < neighbor->number_of_keys - 1; i++) {
            neighbor_sp_list[i] = neighbor_sp_list[i + 1];
            neighbor_value_list[i] = neighbor_value_list[i + 1];
        }
        n->amount_of_free_space -= (n_sp_list[n->number_of_keys].size + SLOT_PAGE_SIZE);
        neighbor->amount_of_free_space += (n_sp_list[n->number_of_keys].size + SLOT_PAGE_SIZE);
    }
    uint16_t current_page = PAGE_SIZE;
    for (i = 0; i < n->number_of_keys + 1; i++) {
        n_sp_list[i].offset = current_page - n_sp_list[i].size;
        current_page -= n_sp_list[i].size;
    }
    current_page = PAGE_SIZE;
    for (i = 0; i < neighbor->number_of_keys - 1; i++) {
        neighbor_sp_list[i].offset = current_page - neighbor_sp_list[i].size;
        current_page -= neighbor_sp_list[i].size;
    }
    for (i = 0; i < n->number_of_keys + 1; i++) {
        memcpy(n->page_body + SLOT_PAGE_SIZE*i, &n_sp_list[i], SLOT_PAGE_SIZE);
        memcpy(n->page_body + n_sp_list[i].offset - 128, n_value_list[i], n_sp_list[i].size);
    }
    for (i = 0; i < neighbor->number_of_keys - 1; i++) {
        memcpy(neighbor->page_body + SLOT_PAGE_SIZE*i, &neighbor_sp_list[i], SLOT_PAGE_SIZE);
        memcpy(neighbor->page_body + neighbor_sp_list[i].offset - 128, neighbor_value_list[i], neighbor_sp_list[i].size);
    }
    /**
     * n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */
    n->number_of_keys++;
    neighbor->number_of_keys--;
    buffer_write_page(table_id, parent_page_num, (page_t *) parent);
    buffer_write_page(table_id, leaf_page_num, (page_t *) n);
    buffer_write_page(table_id, neighbor_page_num, (page_t *) neighbor);
    free(n);
    free(neighbor);
    free(parent);
    free(n_sp_list);
    free(neighbor_sp_list);
    free(n_value_list);
    free(neighbor_value_list);
    return root_pagenum_list[table_id];
}
pagenum_t redistribute_internal_pages(int64_t table_id, pagenum_t internal_page_num, pagenum_t neighbor_page_num, int neighbor_index, int k_prime_index, int64_t k_prime) {
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    int i;
    pagenum_t parent_page_num;
    Internal_page *n = (Internal_page *) malloc(sizeof(Internal_page));
    Internal_page *neighbor = (Internal_page *) malloc(sizeof(Internal_page));
    Internal_page *parent = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, internal_page_num, (page_t *) n);
    buffer_read_page(table_id, neighbor_page_num, (page_t *) neighbor);
    parent_page_num = n->parent_page_number;
    buffer_read_page(table_id, parent_page_num, (page_t *) parent);
    /**
     * Case: n has a neighbor to the left.
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     * n 이 오른쪽 neighbor 가 왼쪽
     * neighbor 에서 n 으로 보내
     * 일단 왜 neighbor_index 를 -1 과 -1이 아닌걸로 구분했지? => 0 때문에
     * 바꿔말하면 현재 노드가 0 이 아니라는 거네
     */
    if (neighbor_index != -1) {
        for (i = n->number_of_keys; i > 0; i--) {
            n->entry[i] = n->entry[i - 1];
        }
        n->entry[0].page_number = n->left_most_page_number;
        n->entry[0].key = parent->entry[k_prime_index].key;
        n->left_most_page_number = neighbor->entry[neighbor->number_of_keys - 1].page_number;
        Internal_page *tmp = (Internal_page *) malloc(sizeof(Internal_page));
        buffer_read_page(table_id, n->left_most_page_number, (page_t *) tmp);
        tmp->parent_page_number = internal_page_num;
        buffer_write_page(table_id, n->left_most_page_number, (page_t *) tmp);
        parent->entry[k_prime_index].key = neighbor->entry[neighbor->number_of_keys - 1].key;
        free(tmp);
    }
        /**
         * 원래는 neighbor가 왼쪽 인데 이 경우는 n 이 왼쪽인 케이스
         * => n / neighbor 이런 구조
         * Case: n is the leftmost child.
         * Take a key-pointer pair from the neighbor to the right.
         * Move the neighbor's leftmost key-pointer pair
         * to n's rightmost position.
         */
    else {
        /**
         * 일단 여기서 문제 생긴듯
         */
        n->entry[n->number_of_keys].key = parent->entry[k_prime_index].key;
        n->entry[n->number_of_keys].page_number = neighbor->left_most_page_number;
        Internal_page *tmp = (Internal_page *) malloc(sizeof(Internal_page));
        buffer_read_page(table_id, n->entry[n->number_of_keys].page_number, (page_t *) tmp);
        tmp->parent_page_number = internal_page_num;
        parent->entry[k_prime_index].key = neighbor->entry[0].key;
        neighbor->left_most_page_number = neighbor->entry[0].page_number;
        for (i = 0; i < neighbor->number_of_keys - 1; i++) {
            neighbor->entry[i] = neighbor->entry[i + 1];
        }
        buffer_write_page(table_id, n->entry[n->number_of_keys].page_number, (page_t *) tmp);
        free(tmp);
    }
    /**
     * n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */
    n->number_of_keys++;
    neighbor->number_of_keys--;
    buffer_write_page(table_id, internal_page_num, (page_t *) n);
    buffer_write_page(table_id, neighbor_page_num, (page_t *) neighbor);
    buffer_write_page(table_id, parent_page_num, (page_t *) parent);
    free(n);
    free(neighbor);
    free(parent);
    return root_pagenum_list[table_id];
}
// Find records with a key between the range:  begin_key ≤ key ≤ end_key
int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes) {
    int i, j, index;
    pagenum_t leaf_page_num = find_leaf_page(table_id, begin_key);
    Leaf_page *lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    buffer_read_page(table_id, leaf_page_num, (page_t *) lp);
    // buffer_pin_off(table_id, leaf_page_num);
    Slot_page *sp_list = (Slot_page *) malloc(sizeof(Slot_page) * lp->number_of_keys);
    for (i = 0; i < lp->number_of_keys; i++) {
        memcpy(&sp_list[i], lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
    }
    /**
     * find begin_key index
     */
    for (i = 0; i < lp->number_of_keys; i++) {
        if (sp_list[i].key >= begin_key) {
            index = i;
            break;
        }
    }
    /**
     * if value not found
     */
    if (i == lp->number_of_keys) {
        free(lp);
        free(sp_list);
        return -1;
    }
    /**
     * store values and keys
     */
    int value_index = 0;
    while (1) {
        for (i = index; i < lp->number_of_keys; i++) {
            if (sp_list[i].key > end_key) {
                break;
            }
            keys->push_back(sp_list[i].key);
            val_sizes->push_back(sp_list[i].size);
            values->push_back(nullptr);
            (*values)[value_index] = (char *) malloc(sizeof(char) * (sp_list[i].size));
            memcpy((*values)[value_index++], lp->page_body + sp_list[i].offset - 128, sp_list[i].size);
        }
        if (i != lp->number_of_keys) break;
        else if (lp->right_sibling_page_number == 0) break;
        else {
            index = 0;
            buffer_read_page(table_id, lp->right_sibling_page_number, (page_t *) lp);
            // buffer_pin_off(table_id, lp->right_sibling_page_number);
            free(sp_list);
            sp_list = (Slot_page *) malloc(sizeof(Slot_page) * lp->number_of_keys);
            for (i = 0; i < lp->number_of_keys; i++) {
                memcpy(&sp_list[i], lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
            }
        }
    }
    free(sp_list);
    free(lp);
    return 0;
}
// Initialize the database system.
int init_db(int num_buf) {
    buffer_init(num_buf);
    maximum_fdlists_size = 20;
    root_pagenum_list = (pagenum_t *)malloc(sizeof(pagenum_t) * 30);
    return 0;
}
// Shutdown the database system.
int shutdown_db() {
    buffer_destroy();
    file_close_table_files();
    free(root_pagenum_list);
    if (fd_lists.size() == 0) {
        printf("closed clear!\n");
        return 0;
    }
    else {
        printf("closed not clear!\n");
        return -1;
    }
}
pagenum_t find_leaf_page(int64_t table_id, int64_t key) {
    
    pagenum_t pagenum = root_pagenum_list[table_id];
    Internal_page *ip = (Internal_page *) malloc(sizeof(Internal_page));
    buffer_read_page(table_id, root_pagenum_list[table_id], (page_t *)ip);
    // buffer_pin_off(table_id, root_pagenum_list[table_id]);
    if (ip->is_leaf == 1) {
        free(ip);
        return pagenum;
    }
    while (ip->is_leaf == 0) {
        int i = 0;
        while (i < ip->number_of_keys) {
            if (key >= ip->entry[i].key) i++;
            else break;
        }
        if (i == 0)
            pagenum = ip->left_most_page_number;
        else
            pagenum = ip->entry[i - 1].page_number;
        buffer_read_page(table_id, pagenum, (page_t *)ip);
        // buffer_pin_off(table_id, pagenum);
    }
    free(ip);
    return pagenum;
}
/**
 * make leaf page
 */
pagenum_t make_leaf_page(int64_t table_id) {
    Leaf_page *lp = (Leaf_page *) malloc(sizeof(Leaf_page));
    pagenum_t lp_page_num = buffer_alloc_page(table_id);
    /**
     * set leaf page attributes
     */
    lp->parent_page_number = 0;
    lp->is_leaf = 1;
    lp->number_of_keys = 0;
    lp->amount_of_free_space = 4096 - 128;
    lp->right_sibling_page_number = 0;
    buffer_write_page(table_id, lp_page_num, (page_t *)lp);
    free(lp);
    return lp_page_num;
}
/**
 * make internal page
 */
pagenum_t make_internal_page(int64_t table_id) {
    Internal_page *new_ip = (Internal_page *) malloc(sizeof(Internal_page));
    pagenum_t root_page_num = buffer_alloc_page(table_id);
    /**
     * set new_ip page attribute
     */
    new_ip->parent_page_number = 0;
    new_ip->is_leaf = 0;
    new_ip->number_of_keys = 0;
    new_ip->left_most_page_number = 0;
    /**
     * entry 처리
     */
    buffer_write_page(table_id, root_page_num, (page_t *) new_ip);
    /**
     * free memories
     */
    free(new_ip);
    return root_page_num;
}
/* project5 의 추가코드 */
/*
 * Find a record with the matching key from the given table.
 * If a matching key is found, store its value in ret_val and the corresponding size in 'val_size'.
 * If successful, return 0. Otherwise, return non-zero value
 * trx_db_find 
 */
int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size, int trx_id) {
    const int SHARED = 0;
    const int EXCLUSIVE = 1;
    // read leaf_page info
    Leaf_page* lp = (Leaf_page *)malloc(sizeof(Leaf_page));
    pagenum_t leaf_pagenum = find_leaf_page(table_id, key);
    /* read leaf page */
    buffer_read_leaf_page(table_id, leaf_pagenum, (page_t *)lp);
    // store the current_number_of_keys
    uint32_t current_number_of_keys = lp->number_of_keys;
    Slot_page sp;
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        // read slot page
        memcpy(&sp, lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        /* 이 때가 trx 가 target record 를 발견한 시점 */
        if (key == sp.key) {
            /* dead lock detected */
            lock_t* lock = lock_acquire(table_id, leaf_pagenum, key, trx_id, SHARED);
            if (lock == NULL) {
                /* trx abort */
                printf("trx abort\n");
                std::cout << "db_find" << std::endl;
                std::cout << "trx_id: " << trx_id << std::endl;
                return -1;
            }

            /* value can be changed */
            buffer_read_leaf_page(table_id, leaf_pagenum, (page_t *)lp);

            // store size and val to ret_val and val_size
            memcpy(val_size, &sp.size, 2);
            memcpy(ret_val, lp->page_body + sp.offset - 128, sp.size);
            free(lp);
            return 0;
        }
    }
    // return non-zero value
//    printf("key is not found\n");
    free(lp);
    return -1;
}
/*
 *  1. db_find(mutex lock, unlock 해결) 로 값을 찾기
 *  2. key 찾고 나서 lock_acquire(trx_id 연결)
 *  3. value update
 *  4. buffer_write
*/
int db_update(int64_t table_id, int64_t key, char* value, uint16_t new_val_size, uint16_t* old_val_size, int trx_id) {
    // int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size)
    /* value 로 update, new_val_size = old_val_size */
    const int SHARED = 0;
    const int EXCLUSIVE = 1;
    // read leaf_page info
    Leaf_page* lp = (Leaf_page *)malloc(sizeof(Leaf_page));
    pagenum_t leaf_pagenum = find_leaf_page(table_id, key);
    /* read leaf page */
    buffer_read_page(table_id, leaf_pagenum, (page_t *)lp);
    // store the current_number_of_keys
    uint32_t current_number_of_keys = lp->number_of_keys;
    Slot_page sp;
    for (uint32_t i = 0; i < current_number_of_keys; i++) {
        // read slot page
        memcpy(&sp, lp->page_body + SLOT_PAGE_SIZE*i, SLOT_PAGE_SIZE);
        // if the key exists in Leaf_page
        if (key == sp.key) {
            lock_t* lock = lock_acquire(table_id, leaf_pagenum, key, trx_id, EXCLUSIVE);
            /* dead lock detected */
            if (lock == NULL) {
                /* trx abort */
                printf("trx abort\n");
                std::cout << "db_update" << std::endl;
                std::cout << "trx_id: " << trx_id << std::endl;
                return -1;
            }
            /* value can be changed */
            *old_val_size = new_val_size;

            // store size and val to ret_val and val_size
            buffer_read_leaf_page(table_id, leaf_pagenum, (page_t *)lp);
            store_log(trx_id, key, table_id, sp, *lp, leaf_pagenum);

            memcpy(&sp.size, &new_val_size, 2);
            memcpy(lp->page_body + sp.offset - 128, value, sp.size);
            
            buffer_write_leaf_page(table_id, leaf_pagenum, (page_t *)lp);
            free(lp);
            return 0;
        }
    }
    // return non-zero value
    free(lp);
    return -1;
}