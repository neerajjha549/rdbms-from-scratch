#include "table.h"
#include "btree.h"

// Static forward declarations for internal helper functions
static Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);
static Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key);


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
        if (pager->pages[i] == nullptr) {
            continue;
        }
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = nullptr;
    }

    int result = close(pager->file_descriptor);
    if (result == -1) {
        std::cerr << "Error closing db file." << std::endl;
        exit(EXIT_FAILURE);
    }
    delete pager;
    delete table;
}

void table_insert(Table* table, Row* row_to_insert) {
    uint32_t key_to_insert = row_to_insert->id;
    Cursor* cursor = table_find(table, key_to_insert);

    void* node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if (key_at_index == key_to_insert) {
            std::cout << "Error: Duplicate key." << std::endl;
            delete cursor;
            return;
        }
    }

    leaf_node_insert(table, cursor->page_num, cursor->cell_num, row_to_insert->id, row_to_insert);
    delete cursor;
    std::cout << "Executed." << std::endl;
}

void table_delete(Table* table, uint32_t key) {
    Cursor* cursor = table_find(table, key);
    void* node = get_page(table->pager, cursor->page_num);

    if (cursor->cell_num < *leaf_node_num_cells(node) && *leaf_node_key(node, cursor->cell_num) == key) {
        btree_delete(table, cursor->page_num, cursor->cell_num, key);
        std::cout << "Executed." << std::endl;
    } else {
        std::cout << "Error: Key " << key << " not found." << std::endl;
    }
    delete cursor;
}


void* cursor_value(Cursor* cursor) {
    void* page = get_page(cursor->table->pager, cursor->page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        uint32_t next_page_num = *leaf_node_next_leaf(node);
        if (next_page_num == 0) {
            cursor->end_of_table = true;
        } else {
            cursor->page_num = next_page_num;
            cursor->cell_num = 0;
        }
    }
}

Cursor* table_find(Table* table, uint32_t key) {
    void* root_node = get_page(table->pager, table->root_page_num);
    
    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(table, table->root_page_num, key);
    } else {
        return internal_node_find(table, table->root_page_num, key);
    }
}

Cursor* table_start(Table* table) {
    Cursor* cursor = table_find(table, 0);
    
    void* node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    cursor->end_of_table = (num_cells == 0);
    
    return cursor;
}

static Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
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

static Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_keys = *internal_node_num_keys(node);
    
    uint32_t child_index = 0;
    // Find the first key that is >= to the search key.
    // The child pointer to the left of that key is the one we want.
    for(child_index = 0; child_index < num_keys; child_index++) {
        if(key <= *internal_node_key(node, child_index)) {
            break;
        }
    }
    
    uint32_t child_num = *internal_node_child(node, child_index);
    void* child = get_page(table->pager, child_num);
    switch (get_node_type(child)) {
        case NODE_LEAF:
            return leaf_node_find(table, child_num, key);
        case NODE_INTERNAL:
            return internal_node_find(table, child_num, key);
        default:
            std::cerr << "Error: Invalid node type found in internal_node_find." << std::endl;
            exit(EXIT_FAILURE);
    }
}
