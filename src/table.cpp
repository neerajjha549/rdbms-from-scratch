#include "table.h"
#include "btree.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

Table* db_open(const std::string& filename) {
    Pager* pager = pager_open(filename);
    Table* table = new Table();
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        void* root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
        set_node_root(root_node, true);
    }
    return table;
}

void db_close(Table* table) {
    Pager* pager = table->pager;
    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i] != nullptr) {
            pager_flush(pager, i);
            free(pager->pages[i]);
            pager->pages[i] = nullptr;
        }
    }
    int result = close(pager->file_descriptor);
    if (result == -1) {
        std::cerr << "Error closing db file." << std::endl;
    }
    delete pager;
    delete table;
}

// Finds the position of a key in a leaf node
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->page_num = page_num;

    // Binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);
        if (key == key_at_index) {
            cursor->cell_num = index;
            return cursor;
        }
        if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    cursor->cell_num = min_index;
    return cursor;
}


// Finds the correct leaf node for a key and returns a cursor to it.
// Crucially, this now sets the parent pointer on each node it traverses.
Cursor* table_find(Table* table, uint32_t key) {
    uint32_t root_page_num = table->root_page_num;
    void* root_node = get_page(table->pager, root_page_num);

    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(table, root_page_num, key);
    } 
    
    // It's an internal node, traverse down to the correct leaf
    uint32_t current_page_num = root_page_num;
    void* current_node = root_node;
    while(get_node_type(current_node) == NODE_INTERNAL) {
        uint32_t child_index = internal_node_find_child(current_node, key);
        uint32_t child_page_num = *internal_node_child(current_node, child_index);
        
        // Before we move to the child, set its parent pointer
        void* child_node = get_page(table->pager, child_page_num);
        *node_parent(child_node) = current_page_num;

        current_page_num = child_page_num;
        current_node = get_page(table->pager, current_page_num);
    }
    return leaf_node_find(table, current_page_num, key);
}

Cursor* table_start(Table* table) {
    // A "select" starts a scan from the beginning, which means finding key 0.
    Cursor* cursor = table_find(table, 0); 
    void* node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    cursor->end_of_table = (num_cells == 0);
    return cursor;
}

void* cursor_value(Cursor* cursor) {
    void* page = get_page(cursor->table->pager, cursor->page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        // We have reached the end of this leaf node.
        // For a true B+ tree, we would follow the next_leaf pointer here.
        // For now, we just end the table scan.
        cursor->end_of_table = true;
    }
}
